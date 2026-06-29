// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "Config/TotemData.h"
#include "Totem/HB_Totem_Base.h"
#include "Types/HB_Enums.h"
#include "HB_TotemSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTotemSubsystem, Log, All);

class AHB_Enemy_Base;
class AHB_TotemManager;
class AHB_TotemHelper;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSpawnTotem, AHB_Totem_Base* /*InTotem*/, FTransform /*InTransform*/);

/**
 * 图腾子系统
 * 负责图腾数据的加载、图腾实例的生成与激活、波次配置的查询。
 *
 * 架构分层：
 * - Data    : UTotemData 只读配置（波次/敌人）
 * - Manager : AHB_TotemManager 持有图腾实例列表（网络复制）
 * - Helper  : AHB_TotemHelper 提供有效性校验（仅服务端）
 * - Actor   : AHB_Totem_Base 负责单体的波次 Tick 推进
 * - Subsystem（本类）: 协调上述四层，提供外部调用入口
 */
UCLASS()
class HEROBUILDER_API UHB_TotemSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }

	/*************************************外部接口*****************************************/

	/** 生成图腾并初始化（不自动激活） */
	UFUNCTION(BlueprintCallable, Category = "Totem")
	AHB_Totem_Base* SpawnTotem(TSubclassOf<AHB_Totem_Base> InTotemClass, const FTransform& SpawnTransform);

	/** 激活指定图腾（开始第一波） */
	UFUNCTION(BlueprintCallable, Category = "Totem")
	void ActiveTotem(AHB_Totem_Base* InTotem);

	/** 跳过指定图腾的当前准备时间 */
	UFUNCTION(BlueprintCallable, Category = "Totem")
	void SkipPreparatory(AHB_Totem_Base* InTotem);

	/** 获取所有图腾 */
	TArray<TObjectPtr<AHB_Totem_Base>> GetAllTotems() const;

	/** 获取所有已激活的图腾 */
	TArray<TObjectPtr<AHB_Totem_Base>> GetAllActiveTotems() const;

	/** 图腾是否有效 */
	bool IsValidTotem(TObjectPtr<AHB_Totem_Base> InTotem) const;

	/** 获取配置查表接口 */
	bool GetConfig(TSubclassOf<AHB_Totem_Base> TotemClass, FTotemConfig& OutConfig) const;
	bool GetWaveConfig(TSubclassOf<AHB_Totem_Base> TotemClass, int32 WaveIndex, FWaveConfig& OutWaveConfig) const;

	/** 图腾生成事件（供 GridSubsystem 等监听以同步占位） */
	FOnSpawnTotem OnSpawnTotem;

protected:
	/** 图腾波次完成时的统一回调（可在此处做全局奖励/通知） */
	UFUNCTION()
	void OnTotemAllWavesComplete(AHB_Totem_Base* InTotem);

private:
	UPROPERTY()
	TObjectPtr<UTotemData> TotemData;

	/** Manager 访问 */
	AHB_TotemManager* GetTotemManager() const;

	/** Helper 访问 */
	AHB_TotemHelper* GetTotemHelper() const;
};
