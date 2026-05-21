// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_GridManager.h"
#include "Building/HB_Building_Base.h"
#include "Enemy/HB_Enemy_Base.h"

void AHB_GridManager::SetGridInfo(int InX, int InY,AHB_Building_Base* InBuilding)
{
	FGridInfo DefaultGrid;
	FGridInfo& TargetGrid= DefaultGrid;
	if (!GridInfos.Contains(FGridInfo(InX, InY)))
	{
		GridInfos.Add(FGridInfo(InX, InY));
	}
	else
	{
		GridInfos.Find(TargetGrid);
	}
	TargetGrid.SetBuilding(InBuilding);
}

bool AHB_GridManager::GetGridInfo(FGridInfo &OutGridInfo)
{
    for (auto& Pair : GridInfos)
    {
        if (Pair== OutGridInfo)
        {
            OutGridInfo = Pair;
            return true;
        }
    }
    return false;
}

void AHB_GridManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_GridManager, GridInfos);
}

TObjectPtr<AHB_Building_Base> FGridInfo::GetBuilding()
{
	return IsValid(Building) ? Building : nullptr;
}

bool FGridInfo::SetBuilding(TObjectPtr<AHB_Building_Base> InBuilding)
{
	if (IsValid(InBuilding))
	{
		Building = InBuilding;
		return true;
	}
	return false;
}

TArray<TObjectPtr<AHB_Enemy_Base>> FGridInfo::GetEnemies()
{
    return Enemies;
}

bool FGridInfo::AddEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy)
{
    if (IsValid(InEnemy))
    {
        Enemies.Add(InEnemy);
        return true;
    }
	return false;
}

bool FGridInfo::RemoveEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy)
{
    if (IsValid(InEnemy))
    {
        Enemies.Remove(InEnemy);
        return true;
    }
	return false;
}
