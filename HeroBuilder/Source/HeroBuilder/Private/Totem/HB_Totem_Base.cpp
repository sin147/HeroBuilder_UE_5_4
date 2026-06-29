// Fill out your copyright notice in the Description page of Project Settings.

#include "Totem/HB_Totem_Base.h"
#include "Subsystems/HB_TotemSubsystem.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Config/TotemData.h"

// Sets default values
AHB_Totem_Base::AHB_Totem_Base()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AHB_Totem_Base::Initialize(const FTotemConfig& InTotemConfig)
{
	// 记录最大波次
	MaxWaveIndex = 0;
	for (const TPair<int32, FWaveConfig>& Pair : InTotemConfig.WaveConfigs)
	{
		if (Pair.Key > MaxWaveIndex)
		{
			MaxWaveIndex = Pair.Key;
		}
	}
	CurrentWaveIndex = 0;
	bIsActive = false;
	ClearWaveCache();
}

void AHB_Totem_Base::Active()
{
	if (bIsActive)
	{
		UE_LOG(LogTemp, Warning, TEXT("Totem %s is already active, ignoring duplicate Active() call."), *GetName());
		return;
	}
	// 从第1波开始
	StartWave(1);
}

void AHB_Totem_Base::SkipPreparatory()
{
	if (bIsActive && CurrentlyWaveDelay > 0)
	{
		CurrentlyWaveDelay = 0;
		UE_LOG(LogTemp, Log, TEXT("Totem %s skipped preparatory for wave %d."), *GetName(), CurrentWaveIndex);
	}
}

void AHB_Totem_Base::StartWave(int32 WaveIndex)
{
	if (WaveIndex <= 0 || WaveIndex > MaxWaveIndex)
	{
		UE_LOG(LogTemp, Warning, TEXT("Totem %s invalid wave index: %d (max: %d)"), *GetName(), WaveIndex, MaxWaveIndex);
		return;
	}

	UHB_TotemSubsystem* TotemSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_TotemSubsystem>() : nullptr;
	if (!TotemSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("TotemSubsystem is null in AHB_Totem_Base::StartWave"));
		return;
	}

	FWaveConfig WaveConfig;
	if (!TotemSubsystem->GetWaveConfig(GetClass(), WaveIndex, WaveConfig))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get wave config for Totem %s, wave %d"), *GetName(), WaveIndex);
		return;
	}

	// 更新波次状态
	CurrentWaveIndex = WaveIndex;
	ClearWaveCache();

	CurrentlyWaveDelay = WaveConfig.WaveDelay;

	for (const FWaveEnemyConfig& WaveEnemyConfig : WaveConfig.EnemyConfigs)
	{
		TSubclassOf<AHB_Enemy_Base> EnemyClass = WaveEnemyConfig.GetEnemyClass();
		int32 EnemyCount = WaveEnemyConfig.GetEnemyCount();
		float SpawnInterval = WaveEnemyConfig.GetSpawnInterval();

		if (!EnemyClass || EnemyCount <= 0)
		{
			continue;
		}

		FEnemySpawnInfo SpawnInfo(EnemyCount, SpawnInterval);
		CacheWaveEnemySpawnInfos.Add(EnemyClass, SpawnInfo);
	}

	bIsActive = true;

	// 广播波次切换
	OnWaveChanged.Broadcast(this, CurrentWaveIndex);

	UE_LOG(LogTemp, Log, TEXT("Totem %s started wave %d, delay: %.2f, enemy types: %d"),
		*GetName(), CurrentWaveIndex, CurrentlyWaveDelay, CacheWaveEnemySpawnInfos.Num());
}

void AHB_Totem_Base::ClearWaveCache()
{
	CacheWaveEnemySpawnInfos.Empty();
	CurrentlyWaveDelay = 0;
}

bool AHB_Totem_Base::IsCurrentWaveComplete() const
{
	if (!bIsActive)
	{
		return false;
	}

	// 还在准备阶段不算完成
	if (CurrentlyWaveDelay > 0)
	{
		return false;
	}

	// 检查所有敌人类型是否都生成完毕
	for (const auto& Elem : CacheWaveEnemySpawnInfos)
	{
		const FEnemySpawnInfo& SpawnInfo = Elem.Value;
		if (SpawnInfo.CurrentlySpawnCount < SpawnInfo.TotalSpawnCount)
		{
			return false;
		}
	}

	return true;
}

bool AHB_Totem_Base::IsAllWavesComplete() const
{
	return bIsActive && CurrentWaveIndex >= MaxWaveIndex && IsCurrentWaveComplete();
}

FVector AHB_Totem_Base::GetRandomSpawnLocation() const
{
	// 在以图腾为中心的圆环范围内随机选择一个点
	const float Angle = FMath::RandRange(0.f, 2.f * PI);
	const float Distance = FMath::RandRange(100.f, SpawnRadius);
	const FVector Offset(
		FMath::Cos(Angle) * Distance,
		FMath::Sin(Angle) * Distance,
		0.f);
	return GetActorLocation() + Offset;
}

void AHB_Totem_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bIsActive)
	{
		return;
	}

	// 检查当前波次是否已完成
	if (IsCurrentWaveComplete())
	{
		// 是否还有下一波？
		if (CurrentWaveIndex < MaxWaveIndex)
		{
			StartWave(CurrentWaveIndex + 1);
		}
		else
		{
			// 全部完成
			bIsActive = false;
			ClearWaveCache();
			OnAllWavesComplete.Broadcast(this);
			UE_LOG(LogTemp, Log, TEXT("Totem %s all waves complete!"), *GetName());
		}
		return;
	}

	// 准备阶段倒计时
	if (CurrentlyWaveDelay > 0)
	{
		CurrentlyWaveDelay -= DeltaTime;
		return;
	}

	// 生成阶段：遍历所有敌人类型
	for (auto& Elem : CacheWaveEnemySpawnInfos)
	{
		TSubclassOf<AHB_Enemy_Base> EnemyClass = Elem.Key;
		FEnemySpawnInfo& SpawnInfo = Elem.Value;

		if (SpawnInfo.CurrentlySpawnCount >= SpawnInfo.TotalSpawnCount)
		{
			continue;
		}

		SpawnInfo.CurrentlySpawnDelay -= DeltaTime;

		if (SpawnInfo.CurrentlySpawnDelay <= 0)
		{
			UHB_EnemySubsystem* EnemySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_EnemySubsystem>() : nullptr;
			if (EnemySubsystem)
			{
				FTransform SpawnTransform;
				SpawnTransform.SetLocation(GetRandomSpawnLocation());
				SpawnTransform.SetRotation(FQuat::Identity);
				SpawnTransform.SetScale3D(FVector::OneVector);

				EnemySubsystem->Spawn(EnemyClass, SpawnTransform);
				SpawnInfo.CurrentlySpawnCount++;
				SpawnInfo.CurrentlySpawnDelay = SpawnInfo.NeedSpawnDelay;
				UE_LOG(LogTemp, Verbose, TEXT("Totem %s queued enemy spawn %s (%d/%d)"),
					*GetName(), *EnemyClass->GetName(),
					SpawnInfo.CurrentlySpawnCount, SpawnInfo.TotalSpawnCount);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("EnemySubsystem is null in AHB_Totem_Base::Tick"));
			}
		}
	}
}

FEnemySpawnInfo::FEnemySpawnInfo()
{
}

FEnemySpawnInfo::FEnemySpawnInfo(int32 InTotalSpawnCount, float InNeedSpawnDelay)
{
	TotalSpawnCount = InTotalSpawnCount;
	NeedSpawnDelay = InNeedSpawnDelay;
	CurrentlySpawnDelay = 0.f; // 第一个敌人立即生成
}