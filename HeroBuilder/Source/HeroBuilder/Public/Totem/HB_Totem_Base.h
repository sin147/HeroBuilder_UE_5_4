// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HB_Totem_Base.generated.h"

struct FTotemConfig;
class AHB_Enemy_Base;

struct FEnemySpawnInfo
{
public:
	FEnemySpawnInfo();
	FEnemySpawnInfo(int32 InTotalSpawnCount, float InNeedSpawnDelay);
	int32 TotalSpawnCount = 0;
	int32 CurrentlySpawnCount=0;
	float NeedSpawnDelay = 0;
	float CurrentlySpawnDelay=0;
};

UCLASS()
class HEROBUILDER_API AHB_Totem_Base : public AActor
{
	GENERATED_BODY()
private:
	bool bIsActive = false;
	float CurrentlyWaveDelay = 0;
	TMap<TSubclassOf<AHB_Enemy_Base>, FEnemySpawnInfo> CacheWaveEnemySpawnInfos;
public:	
	// Sets default values for this actor's properties
	AHB_Totem_Base();
	// Spawn an actor
	void Inital(FTotemConfig InTotemConfig);
	void Active();
	void StartWave(int32 WaveIndex);

	void Tick(float DeltaTime) override;

};
