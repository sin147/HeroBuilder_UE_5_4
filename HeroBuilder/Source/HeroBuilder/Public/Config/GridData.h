// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Resource/HB_Resource_Base.h"
#include "GridData.generated.h"

#define GRID_FRAGMENT_SIZE 100

class AHB_Grid_Base;

USTRUCT(BlueprintType)
struct FAreaConfig
{
	GENERATED_BODY()
public:
	//网格类
	UPROPERTY(EditAnywhere,SimpleDisplay="网格类")
	TSubclassOf<AHB_Grid_Base> GridClass;
    //敌人配置
	UPROPERTY(EditAnywhere,SimpleDisplay="怪物配置")
    TMap<TSubclassOf<AHB_Enemy_Base>,int32> MonsterConfigs;
	//资源配置
	UPROPERTY(EditAnywhere,SimpleDisplay="资源配置")
    TMap<TSubclassOf<AHB_Resource_Base>,int32> ResourceConfigs;
	//地块材质
	UPROPERTY(EditAnywhere,SimpleDisplay="地块材质")
    TObjectPtr<UMaterialInterface> GridMaterial;
	////图腾配置
	//UPROPERTY(EditAnywhere,SimpleDisplay="图腾配置")
	//TArray<FAreaConfig> TotemConfigs;
};

/**
 * 
 */
UCLASS(BlueprintType)
class HEROBUILDER_API UGridData : public UDataAsset
{
	GENERATED_BODY()
private:

	// 单个Grid所占块数，每个块为100*100*20的正方形
    UPROPERTY(EditAnywhere,SimpleDisplay="网格宽度（100cm/块）")
	int32 GridWidthFragment;
	//长度
    UPROPERTY(EditAnywhere, SimpleDisplay = "网格长度（100cm/块）")
    int32 GridLengthFragment;
	//网格配置
	UPROPERTY(EditAnywhere,SimpleDisplay="网格配置")
	TMap<int32, FAreaConfig> AreaConfigs;

public:
    int32 GetGridWidthFragment() const;
    int32 GetGridLengthFragment() const;
	TSubclassOf<AHB_Grid_Base> GetGridClassByLevel(int32 AreaLevel) const;
    bool GetAreaConfigByLevel(int32 AreaLevel, FAreaConfig& OutConfig) const;
    //获取所有已配置的Area Level列表（升序排序，便于从中心向外依次生成）
    TArray<int32> GetAllAreaLevels() const;
};
