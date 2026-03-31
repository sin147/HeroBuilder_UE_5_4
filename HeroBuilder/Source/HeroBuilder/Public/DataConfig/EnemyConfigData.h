// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/EnemyBase.h"
#include "EnemyConfigData.generated.h"

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UEnemyConfigData : public UDataAsset
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	 TMap<TSubclassOf<AEnemyBase>, FEnemyConfig> EnemyConfigs;
public:
	UFUNCTION(BlueprintCallable)
    FEnemyConfig GetEnemyConfig(TSubclassOf<AEnemyBase> EnemyClass);
};
