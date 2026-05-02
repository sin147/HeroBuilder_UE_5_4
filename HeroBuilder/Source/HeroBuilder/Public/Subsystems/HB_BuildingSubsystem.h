// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HB_BuildingSubsystem.generated.h"

class AHB_Building_Base;
/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_BuildingSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
private:
	TMap<TObjectPtr<AHB_Building_Base>,FVector> BuildingPositionMap;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	void PaddingBuilding(AHB_Building_Base*NewBuilding);
	void RemoveBuilding(AHB_Building_Base*InBuilding);
	TArray<TObjectPtr<AHB_Building_Base>> GetBuildingArray();
	virtual TStatId GetStatId() const override { return TStatId(); }
	TMap<TObjectPtr<AHB_Building_Base>, FVector> GetAllBuildingMap();
};
