// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Systems/SubsystemBase.h"
#include "DataConfig/WaveConfig.h"
#include "WaveSubsystem.generated.h"

UENUM(BlueprintType)
enum class EWaveState : uint8
{
	None UMETA(DisplayName = "None"),
	Reset UMETA(DisplayName = "休整"),
	Combat UMETA(DisplayName = "战斗"),
};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UWaveSubsystem : public USubsystemBase
{
	GENERATED_BODY()
private:
    TObjectPtr<UWaveConfig> WaveConfig;
	TMap<TSubclassOf<AEnemyBase>,int32> EnemyCountMap;
	TMap<TSubclassOf<AEnemyBase>, float> EnemySpawnIntervalMap;
	TMap<TSubclassOf<AEnemyBase>, float> EnemyNeedSpawnIntervalMap;
	TMap<TSubclassOf<AEnemyBase>, int32> EnemyNeedSpawnCountMap;
	EWaveState CurrentWaveType=EWaveState::None;
	float CurrentResetTime;
	float CurrentCombatTime;
	float CurrentNeedResetTime;
	float CurrentlyWaveIndex;
public:
	void ApplyWaveByIndex(int32 Index);
	virtual void Tick(float DeltaTime) override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UFUNCTION(BlueprintCallable)
	void ActiveWave();
};
