// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/HB_Enemy_Base.h"
#include "EnemyData.generated.h"

/**
 * 敌人配置结构体
 */
USTRUCT(BlueprintType)
struct FEnemyConfig
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="敌人名称")
	FString EnemyName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="攻击范围")
	float CombatRange = 200;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="攻击力")
	float Attack = 10;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="移动速度")
	float MoveSpeed = 300;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="预攻击延迟")
	float PreAttackDelay = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="后攻击延迟")
	float PostAttackDelay = 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="生命值")
	float Health = 100;
};

/**
 * 敌人数据资产
 */
UCLASS()
class HEROBUILDER_API UEnemyData : public UDataAsset
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, Category = "Enemy")
	TMap<TSubclassOf<AHB_Enemy_Base>, FEnemyConfig> EnemyInfoMap;
	
public:
	FEnemyConfig GetEnemyInfoByEnemyClass(TSubclassOf<AHB_Enemy_Base> EnemyClass);
};
