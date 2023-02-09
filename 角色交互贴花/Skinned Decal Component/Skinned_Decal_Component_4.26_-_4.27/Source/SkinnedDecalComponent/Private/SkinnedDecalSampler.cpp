// Copyright Eddie Ataberk 2021 All Rights Reserved.

#include "SkinnedDecalSampler.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/Canvas.h"
#include "Runtime/Launch/Resources/Version.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "AnimationRuntime.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

#define PRE427 ENGINE_MAJOR_VERSION < 5 && ENGINE_MINOR_VERSION < 27

USkinnedDecalSampler::USkinnedDecalSampler()
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TranslucentBlendMaterialRef(TEXT("/SkinnedDecalComponent/SkinnedDecalTranslucentBlendMat"));
	if (TranslucentBlendMaterialRef.Succeeded())
	{
		TranslucentBlendMaterial = TranslucentBlendMaterialRef.Object;
	}
	PrimaryComponentTick.bCanEverTick = false;
}

void USkinnedDecalSampler::BeginPlay()
{
	Super::BeginPlay();
	
	if (!Mesh)
	{
		UActorComponent* Component = GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		if (Component)
		{
			SetMeshComponent(Cast<USkeletalMeshComponent>(Component),false);
		}
	}
	SetupMaterials();
}


void USkinnedDecalSampler::UpdateAllDecals()
{
	TArray<UActorComponent*> InstanceComponents;
	GetOwner()->GetComponents(USkinnedDecalInstance::StaticClass(), InstanceComponents);

	for (UActorComponent* InstanceComponent : InstanceComponents)
	{
		Cast<USkinnedDecalInstance>(InstanceComponent)->UpdateDecal();
	}
}

void USkinnedDecalSampler::CloneDecals(USkinnedDecalSampler* Source)
{
	if(!Source)
	{
		return;
	}
	
	DecalLocations = Source->DecalLocations;
	MaxDecals = Source->MaxDecals;
	EmptyIndexes = Source->EmptyIndexes;
	LastDecalIndex = Source->LastDecalIndex;
	DataTarget = Source->GetDataTarget(); //Cast<UTextureRenderTarget2D>(StaticDuplicateObject(Target->GetDataTarget(), Target->GetOuter()));
	Materials.Empty();
	SetupMaterials();
}

UTextureRenderTarget2D* USkinnedDecalSampler::GetDataTarget()
{
	if (!DataTarget)
	{
		DataTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, MaxDecals*5, 1, RTF_RGBA16f, FLinearColor::Black, false);
	}

	return DataTarget;	
}

void USkinnedDecalSampler::UpdateInstance(USkinnedDecalInstance* Instance)
{
    int32 DecalID = -1;
	if (InstanceMap.Contains(Instance))
	{
		DecalID = *InstanceMap.Find(Instance);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't find the Instance Component"))
	}
	DecalID = SpawnDecal(Instance->GetComponentLocation(),Instance->GetComponentQuat(),Instance->GetAttachSocketName(),Instance->Size, Instance->SubUV, DecalID);
	InstanceMap.Add(Instance, DecalID);

}

void USkinnedDecalSampler::SetupMaterials()
{
	for (USkeletalMeshComponent* Component : RenderMeshes)
	{
		for(int i=0; i<Component->GetMaterials().Num(); ++i)
		{
			if(!Materials.Contains(Component->GetMaterials()[i]))
			{
				UMaterialInstanceDynamic* DynamicMaterial;
				
				if(TranslucentBlend)
				{
					if(!TranslucentBlendMaterialDynamic)
					{
						TranslucentBlendMaterialDynamic = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, TranslucentBlendMaterial);
						Materials.Add(TranslucentBlendMaterialDynamic);
					}
					DynamicMaterial = TranslucentBlendMaterialDynamic;
					Component->SetMaterial(i, DynamicMaterial);
				}
				else
				{
					DynamicMaterial = Cast<UMaterialInstanceDynamic>(Component->GetMaterial(i));
					if (!DynamicMaterial)
					{
						if(IsValid(Component->GetMaterial(i)))
						{
							DynamicMaterial = UMaterialInstanceDynamic::Create(Component->GetMaterial(i),GetOuter());
							Mesh->SetMaterial(i, DynamicMaterial);
							//Mesh->CreateDynamicMaterialInstance(i, MaterialInterfaces[i], Mesh->GetMaterialSlotNames()[i]);
						}
					}
				}
				if (DynamicMaterial)
				{
					DynamicMaterial->SetScalarParameterValueByInfo(FMaterialParameterInfo("DecalMax", Association, LayerIndex), MaxDecals*5);
					DynamicMaterial->SetTextureParameterValueByInfo(FMaterialParameterInfo("DecalInfo", Association, LayerIndex), GetDataTarget());
					DynamicMaterial->SetScalarParameterValueByInfo(FMaterialParameterInfo("DecalLast", Association, LayerIndex), DecalLocations.Num());
					Materials.Add(DynamicMaterial);
				}
			}
		}
	}
}

void USkinnedDecalSampler::ClearAllDecals()
{
	if (DataTarget)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, DataTarget, FLinearColor::Black);
	}
	DecalLocations.Empty();
	LastDecalIndex = 0;
	for(int16 i=0; i<Materials.Num(); ++i)
	{
		if(IsValid(Materials[i]))
		{
			Materials[i]->SetScalarParameterValueByInfo(FMaterialParameterInfo("DecalLast", Association, LayerIndex), 0.f);
		}
	}
}

int32 USkinnedDecalSampler::SpawnDecal(FVector Location, FQuat Rotation, FName BoneName, float Size, int32 SubUV, int32 Index)
{
	//UE_LOG(LogTemp, Warning, TEXT("StartSpawnDecal"));

	if (!Mesh)
	{
		UActorComponent* Component = GetOwner()->GetComponentByClass(USkeletalMeshComponent::StaticClass());
		if (Component)
		{			
			SetMeshComponent(Cast<USkeletalMeshComponent>(Component),false);
		}
		if (!Mesh)
		{
			return Index;
		}
	}


	if (!Materials.IsValidIndex(0))
	{
		SetMeshComponent(Mesh);
		if (!Materials.IsValidIndex(0))
		{
			return Index;
		}
	}
	
	const FTransform BoneWorldTransform = Mesh->GetSocketTransform(BoneName, RTS_World);

#if PRE427
	const FReferenceSkeleton& RefSkeleton = Mesh->SkeletalMesh->RefSkeleton;
#else
	const FReferenceSkeleton& RefSkeleton = Mesh->SkeletalMesh->GetRefSkeleton();
#endif

	const int32 BoneIndex = Mesh->GetBoneIndex(BoneName);
	const FTransform ReferenceTransform = FAnimationRuntime::GetComponentSpaceTransformRefPose(RefSkeleton, BoneIndex);
	const FVector DecalLocation = ReferenceTransform.TransformPosition(BoneWorldTransform.InverseTransformPosition(Location));
	
	//Check Min Decal Distance
	for (int16 i = 0; i < DecalLocations.Num(); ++i)
	{
		if (i != Index)
		{
			const float DistSquared = (DecalLocation - DecalLocations[i]).SizeSquared();
			if (DistSquared < MinDecalDistance)
			{
				return Index;
			}
		}
	}
	const FQuat DecalRotation = ReferenceTransform.TransformRotation(BoneWorldTransform.InverseTransformRotation(Rotation));

	////////
	// Determine Decal Index
	
	int32 DecalIndex = Index;
	//	UE_LOG(LogTemp, Warning, TEXT("DecalIndexDefault: %i"), DecalIndex);
	if (Index < 0)
	{
		if(EmptyIndexes.IsValidIndex(0))
		{
			DecalIndex = EmptyIndexes[0];
		}
		else
		{
			DecalIndex = (LastDecalIndex + 1) % MaxDecals;
		}
		//	UE_LOG(LogTemp, Warning, TEXT("DecalIndexSetTo: %i"), DecalIndex);
		LastDecalIndex = DecalIndex;
	}

	if (DecalLocations.Num() - 1 < DecalIndex)
	{
		DecalLocations.SetNum(DecalIndex + 1);
	}
	DecalLocations[DecalIndex] = DecalLocation;

	if (!(Index < 0))
	{
		LastDecalIndex = DecalLocations.Num();
	}
	
	
	for(int16 i=0; i<Materials.Num(); ++i)
	{
		if(IsValid(Materials[i]))
		{
			Materials[i]->SetScalarParameterValueByInfo(FMaterialParameterInfo("DecalLast", Association, LayerIndex), DecalLocations.Num());

			//return DecalIndex;
		}
	}
	
	UCanvas* Canvas;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext RenderTargetContext;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, GetDataTarget(), /*out*/ Canvas, /*out*/ CanvasSize, /*out*/ RenderTargetContext);
	if(!::IsValid(Canvas))
	{
		return DecalIndex;
	}

	float DataLocation = DecalIndex * 5 + 1;
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation, 0), FVector2D(DataLocation, 1), 1, DecalLocation);

	FMatrix DecalMatrix = FTransform(DecalRotation).ToMatrixNoScale();
	
	FVector BasisX, BasisY, BasisZ;
	DecalMatrix.GetUnitAxes(BasisX, BasisY, BasisZ);

	float AdditionalDataValue = 0;

	switch (AdditionalData)
	{
	case NoAdditionalData:
		AdditionalDataValue = 0.f;
		break;

	case SpawnTime:
		if(!GetWorld()->IsPreviewWorld())
		{
			AdditionalDataValue = GetWorld()->GetTimeSeconds();
		}
		break;

	case DecalBoneID:
		AdditionalDataValue = BoneIndex;
		break;
	}

	//	UE_LOG(LogTemp, Warning, TEXT("AddData: %f"), AdditionalDataValue);

	
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 1, 0), FVector2D(DataLocation + 1, 1), 1, BasisX);
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 2, 0), FVector2D(DataLocation + 2, 1), 1, BasisY);
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 3, 0), FVector2D(DataLocation + 3, 1), 1, BasisZ);
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 4, 0), FVector2D(DataLocation + 4, 1), 1, FLinearColor(Size, SubUV, AdditionalDataValue, 1));

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, RenderTargetContext);

	//UE_LOG(LogTemp, Warning, TEXT("SpawnDecal: %i"), DecalIndex);
	EmptyIndexes.Remove(DecalIndex);
	return DecalIndex;
}

void USkinnedDecalSampler::RemoveDecal(const int32 Index)
{
	if(Index<0)
	{
		return;
	}
	
	EmptyIndexes.Add(Index);
	
	UCanvas* Canvas;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext RenderTargetContext;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, GetDataTarget(), /*out*/ Canvas, /*out*/ CanvasSize, /*out*/ RenderTargetContext);
	if(!::IsValid(Canvas))
	{
		return;
	}

	const float DataLocation = Index * 5 + 1;
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation, 0), FVector2D(DataLocation, 1), 1, FLinearColor::Black);
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 1, 0), FVector2D(DataLocation + 1, 1), 1, FLinearColor::Black);
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 2, 0), FVector2D(DataLocation + 2, 1), 1, FLinearColor::Black);
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 3, 0), FVector2D(DataLocation + 3, 1), 1, FLinearColor::Black);
	Canvas->UCanvas::K2_DrawLine(FVector2D(DataLocation + 4, 0), FVector2D(DataLocation + 4, 1), 1, FLinearColor::Black);

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, RenderTargetContext);

	//UE_LOG(LogTemp, Warning, TEXT("RemoveDecal: %i"), Index);
}

void USkinnedDecalSampler::SetMeshComponent(USkeletalMeshComponent* MeshComponent, bool Child)
{
	if(!::IsValid(MeshComponent))
	{
		return;
	}

	if (!Child)
	{
		for (UActorComponent* Component : GetOwner()->GetComponentsByTag(USkeletalMeshComponent::StaticClass(), "TranslucentDecalMesh"))
    	{
    		if (Component)
    		{
    			Component->DestroyComponent();
    		}
    	}
		Mesh = MeshComponent;
		Materials.Empty();
	}
	
	if (TranslucentBlend)
	{
		const FName MeshName = MakeUniqueObjectName(GetOuter(),USkeletalMeshComponent::StaticClass(), "DecalMesh");
		USkeletalMeshComponent* TranslucentMesh = NewObject<USkeletalMeshComponent>(GetOwner(),MeshName);
		TranslucentMesh->RegisterComponent();
		TranslucentMesh->SetSkeletalMesh(MeshComponent->SkeletalMesh);
		GetOwner()->AddInstanceComponent(TranslucentMesh);
		TranslucentMesh->AttachToComponent(MeshComponent, FAttachmentTransformRules::SnapToTargetIncludingScale);
		TranslucentMesh->SetMasterPoseComponent(MeshComponent, false);
		TranslucentMesh->BindClothToMasterPoseComponent();
		TranslucentMesh->ComponentTags.Add("TranslucentDecalMesh");
		RenderMeshes.Add(TranslucentMesh);
	}
	else
	{
		RenderMeshes.Add(MeshComponent);
	}

	SetupMaterials();
}

