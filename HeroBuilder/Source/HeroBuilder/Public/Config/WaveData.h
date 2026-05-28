// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WaveData.generated.h"

class AHB_Enemy_Base;

USTRUCT(BlueprintType)
struct FWaveEnemyConfig
{
	GENERATED_BODY()
	public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<AHB_Enemy_Base> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 EnemyCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SpawnInterval;

};

USTRUCT(BlueprintType)
struct FWaveConfig
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    float WaveInterval;
    UPROPERTY(EditAnywhere)
    TArray<FWaveEnemyConfig> EnemyConfigs;

};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UWaveData : public UDataAsset
{
	GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    TMap<int32, FWaveConfig> WaveConfigs;

    bool GetWaveConfigByWaveIndex(int32 WaveIndex, FWaveConfig& OutConfig) const
    {
        const FWaveConfig* Found = WaveConfigs.Find(WaveIndex);
        if (Found)
        {
            OutConfig = *Found;
            return true;
        }
        UE_LOG(LogTemp, Error, TEXT("WaveConfig not found for WaveIndex %d"), WaveIndex);
        return false;
    }

};
