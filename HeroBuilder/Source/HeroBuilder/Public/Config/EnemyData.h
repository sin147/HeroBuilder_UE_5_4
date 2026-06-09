// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Config/InteractData.h"
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

	//玩家靠近本敌人时应进入哪种交互类型（默认 IT_Attack：敌人作为可被攻击的目标）
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Enemy", SimpleDisplay="交互类型")
	TEnumAsByte<EInteractType> InteractType = IT_Attack;
};

/**
 * 敌人数据资产
 */
UCLASS(BlueprintType)
class HEROBUILDER_API UEnemyData : public UDataAsset
{
	GENERATED_BODY()

private:
	UPROPERTY(EditAnywhere, EditFixedSize, Category = "Enemy", meta = (ReadOnlyKeys))
	TMap<TSubclassOf<AHB_Enemy_Base>, FEnemyConfig> EnemyInfoMap;
	
public:
	FEnemyConfig GetEnemyInfoByEnemyClass(TSubclassOf<AHB_Enemy_Base> EnemyClass);

#if WITH_EDITOR
protected:
	virtual void PostInitProperties() override;
	virtual void PostLoad() override;

private:
	//扫描所有AHB_Enemy_Base子类并同步到EnemyInfoMap
	void RefreshEnemyInfoMap();
#endif
};