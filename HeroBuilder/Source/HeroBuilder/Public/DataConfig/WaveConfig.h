// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WaveConfig.generated.h"
class AEnemyBase;


USTRUCT(BlueprintType)
struct FEnemySetting
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 EnemyCount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float SpawnInterval;

};
USTRUCT(BlueprintType)
struct FWaveSetting
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<TSubclassOf<AEnemyBase>, FEnemySetting> EnemySettings;

	//休整时间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	float ResetInterval;


};



/**
 * 
 */
UCLASS()
class HEROBUILDER_API UWaveConfig : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<int32, FWaveSetting> WaveConfigs;
public:
	FWaveSetting GetWaveConfigByIndex(int32 Index);
};
