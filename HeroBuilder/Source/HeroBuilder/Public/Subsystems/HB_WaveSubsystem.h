// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Config/WaveData.h"
#include "HB_WaveSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWaveSubsystem, Log, All);
class AHB_Enemy_Base;

UENUM(BlueprintType)
enum EWaveState:int8
{
	WS_Preparatory UMETA(DisplayName = "准备阶段"),
	WS_Fight UMETA(DisplayName = "战斗阶段"),
	WS_Stop UMETA(DisplayName = "停止阶段"),
	WS_End UMETA(DisplayName = "结束阶段"),
};


/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_WaveSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
protected:
	void WaveTick(float DeltaTime);

private:
	TObjectPtr<UWaveData> WaveData;
	TEnumAsByte<EWaveState> WaveState;
	int32 CurrentlyWaveIndex;
	float CurrentlyFightTime;
	float RemainingPreparatoryTime;
	TArray<FWaveEnemyConfig> CurrentlyWaveEnemyConfigs;
	FWaveConfig CurrentlyWaveConfig;
	TArray<FVector> SpawnPoints;
	bool bAutoNextWave;
public:

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	UFUNCTION(BlueprintCallable)
	void ActiveWaveByIndex(int32 Index,bool AutoNextWave);
	UFUNCTION(BlueprintCallable)
	void SkipPreparatory();
};
