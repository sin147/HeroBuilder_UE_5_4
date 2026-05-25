// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "../Config/EnemyData.h"
#include "HB_EnemySubsystem.generated.h"

class AHB_Enemy_Base;
DECLARE_LOG_CATEGORY_EXTERN(LogEnemySubsystem, Log, All);




/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_EnemySubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
private:
	//敌人数据
	TObjectPtr<UEnemyData> EnemyData;
	
	//待寻找目标的敌人
	TQueue<TObjectPtr<AHB_Enemy_Base>> FindTargetEnemyQueue;
	int32 FindNumByTick = 20;
	void TickFindTarget();
	//待生成的敌人
	TQueue<TPair<TSubclassOf<AHB_Enemy_Base>, FTransform>> SpawnEnemyQueue;
	int32 SpawnNumByTick = 20;
	void TickSpawnEnemy();
	//待销毁的敌人
	TQueue<TObjectPtr<AHB_Enemy_Base>> DestroyEnemyQueue;
	int32 DestroyNumByTick = 20;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    
public:
	/*************************************外部接口*****************************************/
	//生成敌人
	void SpawnEnemy(TSubclassOf<AHB_Enemy_Base> InClass, const FTransform& InTransform);
	//销毁敌人
	void DestroyEnemy(AHB_Enemy_Base* InEnemy);
	//寻找目标
	void FindAnyVaildTarget(AHB_Enemy_Base* InEnemy);
	//获取敌人数组
	TArray<TObjectPtr<AHB_Enemy_Base>> GetAllEnemies();
	//获取活着的敌人数组
    TArray<TObjectPtr<AHB_Enemy_Base>> GetAllAliveEnemies();

	int32 GetEnemyNum();
	//是否有效
	bool IsValidEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy);
	// 将所有敌人加入寻找目标队列（用于新建筑生成时触发）
	void AddAllEnemiesToFindTargetQueue();
	// 新建筑生成通知回调
	UFUNCTION()
	void OnSpawnBuilding(AHB_Building_Base* NewBuilding,FTransform SpawnTransform);
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
};
