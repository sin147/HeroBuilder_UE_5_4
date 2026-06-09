// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Config/InteractData.h"
#include "ResourceData.generated.h"

class AHB_Resource_Base;

/**
 * 资源类型
 */
UENUM(BlueprintType)
enum EResourceType : uint8
{
	RT_None UMETA(DisplayName = "None"),
	RT_Wood UMETA(DisplayName = "Wood"),
	RT_Stone UMETA(DisplayName = "Stone"),
	RT_Iron UMETA(DisplayName = "Iron"),
	RT_Gold UMETA(DisplayName = "Gold"),
	RT_Food UMETA(DisplayName = "Food"),
};

/**
 * 资源配置结构体
 */
USTRUCT(BlueprintType)
struct FResourceConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "资源名称")
	FString ResourceName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "资源类型")
	TEnumAsByte<EResourceType> ResourceType = EResourceType::RT_None;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "生命值")
	float Health = 100.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "脱战恢复延迟")
	float RecoverDelay = 5.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "每秒恢复速度")
	float RecoverSpeed = 10.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "受击硬直时间")
	float BeHitDuration = 0.3f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "死亡保留时间")
	float DeathTime = 5.f;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "死亡掉落资源数量")
	int32 ResourceAmount = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource", SimpleDisplay = "交互模式")
	TEnumAsByte<EInteractType> InteractMode = IT_Normal;
};

/**
 * 资源生成配置结构体
 */
USTRUCT(BlueprintType)
struct FResourceSpawnConfig
{
	GENERATED_BODY()
public:
	//资源类
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ResourceSpawn", SimpleDisplay = "资源类")
	TSubclassOf<AHB_Resource_Base> ResourceClass;
	//生成间隔（秒）
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ResourceSpawn", SimpleDisplay = "生成间隔")
	float SpawnInterval = 5.f;
	//最大存在数量
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ResourceSpawn", SimpleDisplay = "最大存在数量")
	int32 MaxAliveCount = 10;
};

/**
 * 资源数据资产
 */
UCLASS(BlueprintType)
class HEROBUILDER_API UResourceData : public UDataAsset
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Resource", meta = (ReadOnlyKeys))
	TMap<TSubclassOf<AHB_Resource_Base>, FResourceConfig> ResourceInfoMap;
    UPROPERTY(EditAnywhere, EditFixedSize, Category = "Resource", meta = (DisplayName = "资源类型纹理映射", ReadOnlyKeys))
    TMap<TEnumAsByte<EResourceType>, UTexture2D*> ResourceTypeTextureMap;

	//资源生成配置列表
	UPROPERTY(EditAnywhere, Category = "ResourceSpawn", meta = (DisplayName = "资源生成配置列表"))
	TArray<FResourceSpawnConfig> ResourceSpawnConfigs;

	//随机生成区域中心点
	UPROPERTY(EditAnywhere, Category = "ResourceSpawn", meta = (DisplayName = "随机生成区域中心点"))
	FVector SpawnAreaCenter = FVector::ZeroVector;

	//随机生成区域范围（X/Y轴半径）
	UPROPERTY(EditAnywhere, Category = "ResourceSpawn", meta = (DisplayName = "随机生成区域范围"))
	FVector SpawnAreaExtent = FVector(2000.f, 2000.f, 0.f);

	//无FreeGrid可用时，以玩家为中心的随机生成半径
	UPROPERTY(EditAnywhere, Category = "ResourceSpawn", meta = (DisplayName = "玩家中心生成半径", ClampMin = "0.0"))
	float SpawnRadiusAroundPlayer = 1500.f;

public:
	FResourceConfig GetResourceInfoByResourceClass(TSubclassOf<AHB_Resource_Base> ResourceClass);
	const TArray<FResourceSpawnConfig>& GetResourceSpawnConfigs() const { return ResourceSpawnConfigs; }
	FVector GetSpawnAreaCenter() const { return SpawnAreaCenter; }
	FVector GetSpawnAreaExtent() const { return SpawnAreaExtent; }
	float GetSpawnRadiusAroundPlayer() const { return SpawnRadiusAroundPlayer; }
	UFUNCTION(BlueprintPure)
	UTexture2D* GetResourceTypeTexture(EResourceType ResourceType) const;

#if WITH_EDITOR
protected:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

private:
	//扫描所有AHB_Resource_Base子类并同步到ResourceInfoMap
	void RefreshResourceInfoMap();
	//扫描EResourceType所有枚举值并同步到ResourceTypeTextureMap
	void RefreshResourceTypeTextureMap();
#endif
};