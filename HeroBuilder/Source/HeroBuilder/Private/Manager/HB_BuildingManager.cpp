// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_BuildingManager.h"

AHB_BuildingManager::AHB_BuildingManager()
{

}

TArray<TObjectPtr<AHB_Building_Base>> AHB_BuildingManager::GetAllBuildings() const
{
    return Buildings;
}

void AHB_BuildingManager::AddBuilding(AHB_Building_Base* Building)
{
	Buildings.AddUnique(Building);
}

void AHB_BuildingManager::RemoveBuilding(AHB_Building_Base* Building)
{
	if(Buildings.Contains(Building))
	{
		Buildings.RemoveSingleSwap(Building);
	}
}

void AHB_BuildingManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHB_BuildingManager, Buildings);
}
