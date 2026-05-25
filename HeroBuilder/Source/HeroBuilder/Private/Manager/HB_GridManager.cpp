// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_GridManager.h"
#include "Building/HB_Building_Base.h"
#include "Enemy/HB_Enemy_Base.h"

void AHB_GridManager::CacheUsedGridInfo(int InX, int InY)
{
	UsedGridInfos.AddUnique(FGridInfo(InX, InY));
	FreeGridInfos.RemoveSingle(FGridInfo(InX, InY));
}

void AHB_GridManager::RemoveUsedGridInfo(int InX, int InY)
{
	UsedGridInfos.RemoveSingle(FGridInfo(InX, InY));
	FreeGridInfos.AddUnique(FGridInfo(InX, InY));										
}

TArray<FGridInfo> AHB_GridManager::GetUsedGridInfo()
{
    return UsedGridInfos;
}
TArray<FGridInfo> AHB_GridManager::GetFreeGridInfo()
{
	return FreeGridInfos;
}

void AHB_GridManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_GridManager, UsedGridInfos);
}