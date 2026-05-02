// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_BuildingSubsystem.h"
#include "Building/HB_Building_Base.h"

void UHB_BuildingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{

}

void UHB_BuildingSubsystem::Tick(float DeltaTime)
{

}

void UHB_BuildingSubsystem::PaddingBuilding(AHB_Building_Base*NewBuilding)
{
	BuildingPositionMap.Add(NewBuilding,NewBuilding->GetActorLocation());
}

void UHB_BuildingSubsystem::RemoveBuilding(AHB_Building_Base*InBuilding)
{
	BuildingPositionMap.Remove(InBuilding);
}

TArray<TObjectPtr<AHB_Building_Base>> UHB_BuildingSubsystem::GetBuildingArray()
{
	TArray<TObjectPtr<AHB_Building_Base>> OutKeys;
	BuildingPositionMap.GetKeys(OutKeys);
	return OutKeys;
}

TMap<TObjectPtr<AHB_Building_Base>, FVector> UHB_BuildingSubsystem::GetAllBuildingMap()
{
	return BuildingPositionMap;
}
