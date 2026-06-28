// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TotemData.generated.h"

class AHB_Totem_Base;
class AHB_Enemy_Base;
USTRUCT(BlueprintType)
struct FWaveEnemyConfig
{
    GENERATED_BODY()
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess=true))
    TSubclassOf<AHB_Enemy_Base> EnemyClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 EnemyCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float SpawnInterval;
public:
    TSubclassOf<AHB_Enemy_Base> GetEnemyClass() const { return EnemyClass; }
    int32 GetEnemyCount() const { return EnemyCount; }
	float GetSpawnInterval() const { return SpawnInterval; }

};

USTRUCT(BlueprintType)
struct FWaveConfig
{
	GENERATED_BODY()
	public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WaveDelay;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FWaveEnemyConfig> EnemyConfigs; 
};

USTRUCT(BlueprintType)
struct FTotemConfig
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    TMap<int32,FWaveConfig>WaveConfigs;
	bool GetWaveConfig(int32 WaveIndex, FWaveConfig& OutWaveConfig) const;
};

/**
 * 
 */
UCLASS(BlueprintType)
class HEROBUILDER_API UTotemData : public UDataAsset
{
	GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere)
    TMap<TSubclassOf<AHB_Totem_Base>, FTotemConfig> TotemConfigs;
    bool GetTotemConfig(TSubclassOf<AHB_Totem_Base> TotemClass, FTotemConfig& OutConfig) const;

};
