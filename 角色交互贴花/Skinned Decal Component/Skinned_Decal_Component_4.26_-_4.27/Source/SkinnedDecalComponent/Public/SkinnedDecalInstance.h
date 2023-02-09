// Copyright Eddie Ataberk. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SkinnedDecalInstance.generated.h"

//Forward declarations
class FPrimitiveSceneProxy;
class USkinnedDecalSampler;

UCLASS(Blueprintable, BlueprintType, hidecategories = (Collision, Object, Physics, Activation, "Components|Activation", Mobility), ClassGroup = Rendering, meta = (BlueprintSpawnableComponent))
class SKINNEDDECALCOMPONENT_API USkinnedDecalInstance : public USceneComponent
{
	GENERATED_BODY()
public:
	// Sets default values for this component's properties
	USkinnedDecalInstance(const FObjectInitializer& ObjectInitializer);
	
	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void DestroyComponent(bool bPromoteChildren) override;
#if WITH_EDITOR
	virtual void PostEditComponentMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UActorComponent Interface
	
	UFUNCTION(BlueprintCallable, Category = "SkinnedDecal")
	void UpdateDecal();
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn="true"), Category = "SkinnedDecal")
		int32 Index = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn="true"), Category = "SkinnedDecal")
		int32 SubUV = 0;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn="true"), Category = "SkinnedDecal")
		float Size = 10.f;

	UFUNCTION(BlueprintCallable, Category = "SkinnedDecal")
	USkinnedDecalSampler* GetSampler();

};