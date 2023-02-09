// Copyright Eddie Ataberk 2021 All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SkinnedDecalInstance.h"
#include "Components/ActorComponent.h"
#include "Materials/MaterialLayersFunctions.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "SkinnedDecalSampler.generated.h"


UENUM()
enum ESkinnedDecalAdditionalData
{
	NoAdditionalData,
	SpawnTime,
	DecalBoneID,
};


class USkinnedDecalInstance;
UCLASS(Blueprintable, BlueprintType, hidecategories = (Collision, Object, Physics, SceneComponent, Activation, "Components|Activation", Mobility), ClassGroup = Rendering, meta = (BlueprintSpawnableComponent))
class SKINNEDDECALCOMPONENT_API USkinnedDecalSampler : public UActorComponent
{
	GENERATED_BODY()

public:	
	USkinnedDecalSampler();

	virtual void BeginPlay() override;
	
	UPROPERTY(BlueprintReadOnly, Category = "Meshes")
		USkeletalMeshComponent* Mesh;

	UFUNCTION(BlueprintCallable, Category = "Skinned Decal Component")
		int32 SpawnDecal(FVector Location, const FQuat Rotation, FName BoneName = NAME_None, float Size = 10.f, int32 SubUV = 0, int32 Index = -1);
	
	UFUNCTION(BlueprintCallable, Category = "Skinned Decal Component")
		void RemoveDecal(const int32 Index = -1);

	UFUNCTION(BlueprintCallable, Category = "Skinned Decal Component")
		void SetMeshComponent(USkeletalMeshComponent* MeshComponent, bool Child = false);

	UFUNCTION(BlueprintCallable, Category = "Skinned Decal Component")
		void ClearAllDecals();

	UFUNCTION(BlueprintCallable, Category = "Skinned Decal Component")
		void UpdateAllDecals();
		
	UFUNCTION(BlueprintCallable, Category = "Skinned Decal Component")
		void CloneDecals(USkinnedDecalSampler* Source);

	UPROPERTY(BlueprintReadOnly, Category = "Decals")
		TArray<FVector> DecalLocations;

	UPROPERTY(BlueprintReadOnly, Category = "Decals")
		TArray<int32> EmptyIndexes;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
		int32 LayerIndex = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
		TEnumAsByte<EMaterialParameterAssociation> Association = EMaterialParameterAssociation::GlobalParameter;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Material")
	TEnumAsByte<ESkinnedDecalAdditionalData> AdditionalData = ESkinnedDecalAdditionalData::SpawnTime;
	
	UPROPERTY(BlueprintReadWrite, Category = "Material")
		TArray<UMaterialInstanceDynamic*> Materials;
	
	UPROPERTY(BlueprintReadOnly, Category = "Material")
		int32 LastDecalIndex = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Performance")
		int MaxDecals = 500;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Performance")
		float MinDecalDistance = 10.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Translucent Blend")
		bool TranslucentBlend = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Translucent Blend")
		UMaterialInterface* TranslucentBlendMaterial;
	
	UFUNCTION(BlueprintPure, Category = "Skinned Decal Component")
	UTextureRenderTarget2D* GetDataTarget();

	UFUNCTION()
	void UpdateInstance(USkinnedDecalInstance* Instance);

	UPROPERTY()
	TMap<USkinnedDecalInstance*,int32> InstanceMap;

	UPROPERTY()
	UMaterialInstanceDynamic* TranslucentBlendMaterialDynamic;

	UPROPERTY()
	TArray<USkeletalMeshComponent*> RenderMeshes;

	UFUNCTION(BlueprintCallable, Category = "Skinned Decal Component")
	void SetupMaterials();

	UPROPERTY()
	UTextureRenderTarget2D* DataTarget;
};
