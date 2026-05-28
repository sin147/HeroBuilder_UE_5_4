// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Building/HB_Building_Base.h"
#include "BuildingData.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct FBuildingConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="建筑名称")
	FString BuildingName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="预览模型")
    TObjectPtr<UStaticMesh> PreviewMesh;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="攻击范围")
    float CombatRange=200;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="攻击力")
	float Attack=10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="旋转速度")
	float RotateSpeed=60;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="预攻击延迟")
    float PreAttackDelay=1;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="后攻击延迟")
    float PostAttackDelay=1;

};


UCLASS()
class HEROBUILDER_API UBuildingData : public UDataAsset
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Building", meta = (ReadOnlyKeys))
	TMap<TSubclassOf<AHB_Building_Base>,FBuildingConfig> BuildingInfoMap;
public:
    FBuildingConfig GetBuildingInfoByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass);
	UStaticMesh* GetPreviewMeshByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass);

#if WITH_EDITOR
protected:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

private:
	//扫描所有AHB_Building_Base子类并同步到BuildingInfoMap
	void RefreshBuildingInfoMap();
#endif
};
