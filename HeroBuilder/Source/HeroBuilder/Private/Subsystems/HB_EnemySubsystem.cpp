// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_EnemySubsystem.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Building/HB_Building_Base.h"

void UHB_EnemySubsystem::FindAnyVaildTarget(AHB_Enemy_Base* InEnemy)
{
	NeedFindTargetEnemysQueue.Enqueue(InEnemy);
}

void UHB_EnemySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!NeedFindTargetEnemysQueue.IsEmpty())
	{
		TMap<TObjectPtr<AHB_Building_Base>,FVector> BuildingPostionMap = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>()->GetAllBuildingMap();
		for (int32 i = 0; i < FindNumByTick; i++)
		{
			TPair<TObjectPtr<AHB_Building_Base>, float>TargetDistancePair(nullptr,1000000000);
			TObjectPtr<AHB_Enemy_Base> OutItem;
			NeedFindTargetEnemysQueue.Dequeue(OutItem);
			float Distance = 0;
			for (TPair<TObjectPtr<AHB_Building_Base>, FVector>BuildingPostionPair:BuildingPostionMap)
			{
				Distance = FVector::DistSquared(OutItem->GetActorLocation(), BuildingPostionPair.Value);
				if (TargetDistancePair.Value > Distance)
				{
					TargetDistancePair.Key = BuildingPostionPair.Key;
					TargetDistancePair.Value = Distance;
				}
			}
			OutItem->SetTarget(Cast<AActor>(TargetDistancePair.Key));
		}
	}
}

void UHB_EnemySubsystem::PaddingEnemy(AHB_Enemy_Base*NewEnemy)
{
	EnemyArray.AddUnique(NewEnemy);
}

void UHB_EnemySubsystem::RemoveEnemy(AHB_Enemy_Base* InEnemy)
{
	EnemyArray.RemoveSingle(InEnemy);
}

TArray<TObjectPtr<AHB_Enemy_Base>> UHB_EnemySubsystem::GetEnemyArray()
{

	return EnemyArray;
}

int32 UHB_EnemySubsystem::GetEnemyNum()
{

	return EnemyArray.Num();
}
