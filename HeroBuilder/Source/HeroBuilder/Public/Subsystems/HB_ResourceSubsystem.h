// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "../Config/ResourceData.h"
#include "HB_ResourceSubsystem.generated.h"

class AHB_Resource_Base;
DECLARE_LOG_CATEGORY_EXTERN(LogResourceSubsystem, Log, All);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSpawnResource, AHB_Resource_Base* /*Resource*/, FTransform /*Transform*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDestroyResource, AHB_Resource_Base* /*Resource*/, FTransform /*Transform*/);
/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_ResourceSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
private:
	//资源数据
	TObjectPtr<UResourceData> ResourceData;

	//待生成的资源
	TQueue<TPair<TSubclassOf<AHB_Resource_Base>, FTransform>> SpawnResourceQueue;
	int32 SpawnNumByTick = 20;
	void TickSpawnResource();

	//每个生成配置当前累积的时间
	TArray<float> SpawnConfigTimers;
	//自动生成Tick
	void TickAutoSpawn(float DeltaTime);
	//获取指定类型当前存活的资源数量
	int32 GetAliveResourceCountByClass(TSubclassOf<AHB_Resource_Base> ResourceClass) const;
	//随机生成位置
	FTransform GetRandomSpawnTransform() const;

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/*************************************外部接口*****************************************/
	//生成资源
	void SpawnResource(TSubclassOf<AHB_Resource_Base> InClass, const FTransform& InTransform);
	//销毁资源
	void DestroyResource(AHB_Resource_Base* InResource);
	//资源死亡时回调（用于结算资源数量）
	void OnResourceDeath(AHB_Resource_Base* InResource);
	//获取所有资源
	TArray<TObjectPtr<AHB_Resource_Base>> GetAllResources();
	//获取所有有效（未死亡）资源
	TArray<TObjectPtr<AHB_Resource_Base>> GetAllAliveResources();
	//获取资源总数量
	int32 GetResourceNum();
	//是否有效
	bool IsValidResource(TObjectPtr<AHB_Resource_Base> InResource);
	//获取指定类型的资源数量
	UFUNCTION(BlueprintCallable)
	int32 GetResourceAmount(EResourceType InType);
	//消耗资源数量
	UFUNCTION(BlueprintCallable)
	bool ConsumeResourceAmount(EResourceType InType, int32 InAmount);
	FOnSpawnResource OnSpawnResource;
	FOnDestroyResource OnDestroyResource;

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
};
