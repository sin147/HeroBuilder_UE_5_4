// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_ConstructionSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Building/HB_Building_Base.h"

void UHB_ConstructionSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!PreSpawnQueue.IsEmpty())
	{
		int32 SpawnCount = 0;
		while (SpawnCount < SpawnNumByTick && !PreSpawnQueue.IsEmpty())
		{
			FBuildingSpawnInfo SpawnInfo;
			if (PreSpawnQueue.Dequeue(SpawnInfo))
			{
				// 安全检查：确保建筑类有效
				if (SpawnInfo.BuildingClass && SpawnInfo.BuildingClass.Get())
				{
					// 生成建筑actor
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
					
					AHB_Building_Base* Building = GetWorld()->SpawnActor<AHB_Building_Base>(
						SpawnInfo.BuildingClass, 
						SpawnInfo.SpawnLocation, 
						SpawnInfo.SpawnRotation,
						SpawnParams
					);
					
					if (Building)
					{
						// 添加到建筑子系统
						if (UHB_BuildingSubsystem* BuildingSubsystem = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>())
						{
							BuildingSubsystem->PaddingBuilding(Building);
							Buildings.Add(Building);
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("Failed to spawn building from ConstructionSubsystem"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("Invalid building class in ConstructionSubsystem spawn queue"));
				}
				
				SpawnCount++;
			}
		}
	}
}

void UHB_ConstructionSubsystem::SpawnBuilding(TSubclassOf<AHB_Building_Base> BuildingClass, FVector SpawnLocation, FRotator SpawnRotation)
{
	if (BuildingClass)
	{
		PreSpawnQueue.Enqueue(FBuildingSpawnInfo(BuildingClass, SpawnLocation, SpawnRotation));
	}
}
