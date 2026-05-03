// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HB_ConstructionSubsystem.generated.h"

class AHB_Building_Base;

USTRUCT(BlueprintType)
struct FBuildingSpawnInfo
{
	GENERATED_BODY()
	
public:
	FBuildingSpawnInfo(){}
	FBuildingSpawnInfo(TSubclassOf<AHB_Building_Base> InBuildingClass, FVector InSpawnLocation, FRotator InSpawnRotation)
		: BuildingClass(InBuildingClass), SpawnLocation(InSpawnLocation), SpawnRotation(InSpawnRotation) {}
	TSubclassOf<AHB_Building_Base> BuildingClass;
	FRotator SpawnRotation;
	FVector SpawnLocation;
};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_ConstructionSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
private:
    TQueue<FBuildingSpawnInfo> PreSpawnQueue;
	TArray<TObjectPtr<AHB_Building_Base>> Buildings;
	int32 SpawnNumByTick;
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
    void SpawnBuilding(TSubclassOf<AHB_Building_Base> BuildingClass, FVector SpawnLocation, FRotator SpawnRotation);
	
};
