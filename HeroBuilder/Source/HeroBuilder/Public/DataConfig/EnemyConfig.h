// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/EnemyBase.h"
#include "EnemyConfig.generated.h"

USTRUCT(BlueprintType)
struct FAttributeSetting
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float HP=100;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Attack=10;
};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UEnemyConfig : public UDataAsset
{
	GENERATED_BODY()
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	 TMap<TSubclassOf<AEnemyBase>, FAttributeSetting> EnemyConfig;
};
