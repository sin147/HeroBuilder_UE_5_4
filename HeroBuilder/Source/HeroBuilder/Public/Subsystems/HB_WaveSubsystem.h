// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Config/WaveData.h"
#include "HB_WaveSubsystem.generated.h"

class AHB_Enemy_Base;

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_WaveSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
private:
	TObjectPtr<UWaveData> WaveData;

public:

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
	
};
