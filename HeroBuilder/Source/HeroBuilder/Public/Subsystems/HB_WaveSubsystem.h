// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "Config/WaveData.h"
#include "SpawnPoint/HB_SpawnPoint_Enemy.h"
#include "Types/HB_Enums.h"
#include "HB_WaveSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWaveSubsystem, Log, All);
class AHB_Enemy_Base;


/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_WaveSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
    friend AHB_SpawnPoint_Enemy;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
protected:
	void WaveTick(float DeltaTime);
	void AddSpawnPoint(AHB_SpawnPoint_Enemy* SpawnPoint);
private:
	UPROPERTY()
	TObjectPtr<UWaveData> WaveData;
	TEnumAsByte<EWaveState> WaveState;
	int32 CurrentlyWaveIndex;
	float CurrentlyFightTime;
	float RemainingPreparatoryTime;
	TArray<FWaveEnemyConfig> CurrentlyWaveEnemyConfigs;
	FWaveConfig CurrentlyWaveConfig;
	UPROPERTY()
	TArray<TObjectPtr<AHB_SpawnPoint_Enemy>> SpawnPoints;
	bool bAutoNextWave;
	AHB_SpawnPoint_Enemy* GetAnRandSpawnPoint();
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	UFUNCTION(BlueprintCallable)
	void ActiveWaveByIndex(int32 Index,bool AutoNextWave);
	UFUNCTION(BlueprintCallable)
	void SkipPreparatory();
};
