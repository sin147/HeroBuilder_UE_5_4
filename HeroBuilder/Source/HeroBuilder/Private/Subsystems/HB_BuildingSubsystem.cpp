// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_BuildingSubsystem.h"
#include "Building/HB_Building_Base.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Subsystems/HB_EnemySubsystem.h"

void UHB_BuildingSubsystem::FindAnyVaildTarget(AHB_Building_Base* InBuilding)
{
	NeedFindTargetBuildingsQueue.Enqueue(InBuilding);
}

void UHB_BuildingSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!NeedFindTargetBuildingsQueue.IsEmpty())
	{
		TArray<TObjectPtr<AHB_Enemy_Base>> EnemyArray = GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->GetEnemyArray();
		for (int32 i = 0; i < FindNumByTick; i++)
		{
			TPair<TObjectPtr<AHB_Enemy_Base>, float>TargetDistancePair(nullptr,1000000000);
			TObjectPtr<AHB_Building_Base> OutItem;
			NeedFindTargetBuildingsQueue.Dequeue(OutItem);
			float Distance = 0;
			for (TObjectPtr<AHB_Enemy_Base> Enemy : EnemyArray)
			{
				Distance = FVector::DistSquared(OutItem->GetActorLocation(), Enemy->GetActorLocation());
				if (TargetDistancePair.Value > Distance)
				{
					TargetDistancePair.Key = Enemy;
					TargetDistancePair.Value = Distance;
				}
			}
			OutItem->SetTarget(Cast<AActor>(TargetDistancePair.Key));
		}
	}
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
