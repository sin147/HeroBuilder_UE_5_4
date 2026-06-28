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


/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_TotemSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
protected:
	void TotemTick(float DeltaTime);
private:
	UPROPERTY()
	TObjectPtr<UTotemData> TotemData;
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	UFUNCTION(BlueprintCallable)
	void ActiveTotem(AHB_Totem_Base*InTotem);
	UFUNCTION(BlueprintCallable)
	void SkipPreparatory(AHB_Totem_Base* InTotem);
	void SpawnTotem(TSubclassOf<AHB_Totem_Base> InTotem,FTransform SpawnTransform);
	//获取配置
	bool  GetConfig(TSubclassOf<AHB_Totem_Base> TotemClass, FTotemConfig& OutConfig) const;
	//获取波次配置
	bool GetWaveConfig(TSubclassOf<AHB_Totem_Base> TotemClass, int32 WaveIndex, FWaveConfig& OutWaveConfig) const;
};
