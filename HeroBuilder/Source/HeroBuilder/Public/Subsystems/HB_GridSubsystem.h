// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HB_WorldSubsystem_Base.h"
#include "Manager/HB_GridManager.h"
#include "Config/GridData.h"
#include "HB_GridSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGridSubsystem, Log, All);
class AHB_Building_Base;
class AHB_Resource_Base;
/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_GridSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
private:
	UPROPERTY()
	TObjectPtr<UGridData> GridData;
	void OnSpawnBuilding(AHB_Building_Base* InBuilding, FTransform InTransform);
	void OnDestroyBuilding(AHB_Building_Base* InBuilding, FTransform InTransform);
	void OnSpawnResource(AHB_Resource_Base* InResource, FTransform InTransform);
	void OnDestroyResource(AHB_Resource_Base* InResource, FTransform InTransform);
	int WitdhSize;
	int HeightSize;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    int32 GetGridWidth() const;	
	int32 GetGridHeight() const;
	TArray<FGridInfo> GetUsedGridIndexs();
	TArray<FGridInfo> GetFreeGridIndexs();
	FVector2D CalulateGridIndexByLocation(const FVector& Location) const;
};
