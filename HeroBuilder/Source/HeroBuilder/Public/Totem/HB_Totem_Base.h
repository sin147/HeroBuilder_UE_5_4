// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HB_Totem_Base.generated.h"

class AHB_Enemy_Base;
class UTotemData;

/** 波次切换委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWaveChanged, AHB_Totem_Base*, Totem, int32, NewWaveIndex);

/** 全部完成委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllWavesComplete, AHB_Totem_Base*, Totem);

/** 敌人生成委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemySpawned, AHB_Totem_Base*, Totem, AHB_Enemy_Base*, SpawnedEnemy);

struct FTotemConfig;
struct FWaveConfig;

/**
 * 单种敌人的生成计数器
 */
struct FEnemySpawnInfo
{
public:
	FEnemySpawnInfo();
	FEnemySpawnInfo(int32 InTotalSpawnCount, float InNeedSpawnDelay);

	int32 TotalSpawnCount = 0;
	int32 CurrentlySpawnCount = 0;
	float NeedSpawnDelay = 0;
	float CurrentlySpawnDelay = 0;
};

/**
 * 图腾基类：波次生成器
 *
 * 职责范围：
 * - 接收 TotemSubsystem 指令（Initialize / Active / StartWave / SkipPreparatory）
 * - 维护当前波次状态并 Tick 推进（倒计时 + 生成敌人）
 * - 波次结束后自动切换到下一波；全部完成后广播 OnAllWavesComplete
 * - 不直接访问 Data，配置统一由 TotemSubsystem 查好后传入
 */
UCLASS()
class HEROBUILDER_API AHB_Totem_Base : public AActor
{
	GENERATED_BODY()

private:
	/** 是否已激活 */
	bool bIsActive = false;

	/** 当前波次编号（从1开始） */
	int32 CurrentWaveIndex = 0;

	/** 最大波次数（由 Initialize 时写入） */
	int32 MaxWaveIndex = 0;

	/** 当前波次准备倒计时 */
	float CurrentlyWaveDelay = 0;

	/** 本波次敌人生成缓存 */
	TMap<TSubclassOf<AHB_Enemy_Base>, FEnemySpawnInfo> CacheWaveEnemySpawnInfos;

	/** 生成位置随机范围半径 */
	UPROPERTY(EditDefaultsOnly, Category = "Totem", meta = (AllowPrivateAccess = true))
	float SpawnRadius = 500.f;

public:
	AHB_Totem_Base();

	/** 初始化：由 TotemSubsystem 在 Spawn 后调用，传入查好的配置 */
	void Initialize(const FTotemConfig& InTotemConfig);

	/** 激活：开始第一波 */
	UFUNCTION(BlueprintCallable, Category = "Totem")
	void Active();

	/** 跳过当前波次准备时间 */
	UFUNCTION(BlueprintCallable, Category = "Totem")
	void SkipPreparatory();

	/** 是否已激活 */
	UFUNCTION(BlueprintPure, Category = "Totem")
	bool IsActive() const { return bIsActive; }

	/** 当前波次 */
	UFUNCTION(BlueprintPure, Category = "Totem")
	int32 GetCurrentWaveIndex() const { return CurrentWaveIndex; }

	/** 总波次数 */
	UFUNCTION(BlueprintPure, Category = "Totem")
	int32 GetMaxWaveIndex() const { return MaxWaveIndex; }

	/** 当前波次是否已完成生成 */
	UFUNCTION(BlueprintPure, Category = "Totem")
	bool IsCurrentWaveComplete() const;

	/** 所有波次是否全部完成 */
	UFUNCTION(BlueprintPure, Category = "Totem")
	bool IsAllWavesComplete() const;

	/** 波次全部完成时广播（供 TotemSubsystem / UI 监听） */
	UPROPERTY(BlueprintAssignable, Category = "Totem")
	FOnAllWavesComplete OnAllWavesComplete;

	/** 波次切换时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Totem")
	FOnWaveChanged OnWaveChanged;

	/** 敌人生成时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Totem")
	FOnEnemySpawned OnEnemySpawned;

protected:
	/** 启动指定波次（内部调用） */
	void StartWave(int32 WaveIndex);

	/** 清理当前波次缓存 */
	void ClearWaveCache();

	/** 获取图腾周围一个随机生成位置 */
	FVector GetRandomSpawnLocation() const;

	virtual void Tick(float DeltaTime) override;
};
