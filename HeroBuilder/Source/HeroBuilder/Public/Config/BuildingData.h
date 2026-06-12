// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Building/HB_Building_Base.h"
#include "Config/InteractData.h"
#include "Types/HB_Enums.h"
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
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="最大血量")
	float MaxHealth=100;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="旋转速度")
	float RotateSpeed=60;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="预攻击延迟")
    float PreAttackDelay=1;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building",SimpleDisplay="后攻击延迟")
    float PostAttackDelay=1;

	//玩家靠近本建筑时应进入哪种交互类型（默认 IT_Attack：建筑作为可被攻击的目标）
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building", SimpleDisplay = "交互类型")
	TEnumAsByte<EInteractType> InteractType = IT_Construction;

	//建造该建筑所需消耗的资源（Key=资源类型, Value=消耗数量）
	//说明：仅记录"资源类型→数量"的需求清单；具体扣除/校验逻辑由 ResourceManager / BuildingSubsystem 在建造流程中调用
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Building", SimpleDisplay = "建造消耗资源")
	TMap<TEnumAsByte<EResourceType>, int32> BuildCostMap;

};


UCLASS(BlueprintType)
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
