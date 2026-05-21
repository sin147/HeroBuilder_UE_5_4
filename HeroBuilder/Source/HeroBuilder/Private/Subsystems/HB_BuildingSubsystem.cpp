// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_BuildingSubsystem.h"
#include "Building/HB_Building_Base.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Manager/HB_BuildingManager.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Subsystems/HB_NotifySubsystem.h"

DEFINE_LOG_CATEGORY(LogBuildingSystem);

void UHB_BuildingSubsystem::FindAnyVaildTarget(AHB_Building_Base* InBuilding)
{
	if (IsValid(InBuilding))
	{
		UE_LOG(LogBuildingSystem, Log, TEXT("Adding building to find target queue - Building: %s"), *InBuilding->GetName());
		FindTargetBuildingQueue.Enqueue(InBuilding);
	}
	else
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("Attempted to find target for invalid building"));
	}

}

void UHB_BuildingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BuildingData = LoadObject<UBuildingData>(this, TEXT("/Game/Config/DA_BuildingConfig"));

	if (!IsValid(BuildingData))
	{
        UE_LOG(LogBuildingSystem, Error, TEXT("Failed to load BuildingData asset from /Game/Config/DA_BuildingConfig"));
		UE_LOG(LogBuildingSystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
        UE_LOG(LogBuildingSystem, Log, TEXT("Successfully loaded BuildingData asset"));
	}
}

void UHB_BuildingSubsystem::TickFindTarget()
{
    if (NetMode != ENetMode::NM_Client && !FindTargetBuildingQueue.IsEmpty())
	{
		UE_LOG(LogBuildingSystem, Verbose, TEXT("TickFindTarget - Queue not empty, Available enemies: %d"), 
			GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->GetEnemyNum());
		
		TArray<TObjectPtr<AHB_Enemy_Base>> EnemyArray = GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->GetAllAliveEnemies();
		float CurrentlyFindNum = 0;
		//遍历队列,找到最近的敌人
        while(!FindTargetBuildingQueue.IsEmpty() && CurrentlyFindNum < FindNumByTick)
		{
			//取出一个建筑
			TObjectPtr<AHB_Building_Base> FindingBuilding;
            FindTargetBuildingQueue.Dequeue(FindingBuilding);
			
			if (!IsValid(FindingBuilding))
			{
				UE_LOG(LogBuildingSystem, Warning, TEXT("Skipping invalid building in find target queue"));
				continue;
			}
			
			
			//找到最近的敌人
            TPair<TObjectPtr<AHB_Enemy_Base>, float>TargetDistancePair(nullptr, 1000000000);
			float Distance = 0;
			for (TObjectPtr<AHB_Enemy_Base> Enemy : EnemyArray)
			{
				if (!IsValid(Enemy)) continue;
				
				Distance = FVector::Distance(FindingBuilding->GetActorLocation(), Enemy->GetActorLocation());
                if (TargetDistancePair.Value > Distance && FindingBuilding->GetCombatRange() >= Distance)
				{
					TargetDistancePair.Key = Enemy;
					TargetDistancePair.Value = Distance;
				}
			}
			CurrentlyFindNum++;
			//若找到目标，则设置目标，否则重新加入队列
			if (TargetDistancePair.Key)
			{
				UE_LOG(LogBuildingSystem, Log, TEXT("Found target for building %s - Enemy: %s, Distance: %.2f"), 
					*FindingBuilding->GetName(), *TargetDistancePair.Key->GetName(), FMath::Sqrt(TargetDistancePair.Value));
				FindingBuilding->SetTarget(Cast<AActor>(TargetDistancePair.Key));
            }
			else
			{
				FindTargetBuildingQueue.Enqueue(FindingBuilding);
			}
		}
	}
}

void UHB_BuildingSubsystem::TickSpawnBuilding()
{
	if (NetMode != ENetMode::NM_Client && !SpawnBuildingQueue.IsEmpty())
	{
		UE_LOG(LogBuildingSystem, Verbose, TEXT("TickSpawnBuilding - Queue not empty"));
		float CurrentlySpawnNum = 0;
        while(!SpawnBuildingQueue.IsEmpty() && CurrentlySpawnNum < SpawnNumByTick)
		{
			//取出一个建筑
			TPair<TSubclassOf<AHB_Building_Base>, FTransform> OutItem;
			SpawnBuildingQueue.Dequeue(OutItem);
			
			if (OutItem.Key)
			{
				UE_LOG(LogBuildingSystem, Log, TEXT("Spawning building - Class: %s, Location: %s"), 
					*OutItem.Key->GetName(), *OutItem.Value.GetLocation().ToString());
				
				//生成建筑
				TObjectPtr<AHB_Building_Base> DeferredBuilding = GetWorld()->SpawnActorDeferred<AHB_Building_Base>(OutItem.Key, OutItem.Value);
				if (IsValid(DeferredBuilding))
				{
					//初始化建筑数据
					if (IsValid(BuildingData))
					{
						DeferredBuilding->InitialBuilding(BuildingData->GetBuildingInfoByBuildingClass(OutItem.Key));
					}
					else
					{
						UE_LOG(LogBuildingSystem, Warning, TEXT("BuildingData is not valid, using default building config"));
						FBuildingConfig DefaultConfig;
						DeferredBuilding->InitialBuilding(DefaultConfig);
					}
					UE_LOG(LogBuildingSystem, Log, TEXT("Successfully spawned building: %s"), *DeferredBuilding->GetName());
					GetManager<AHB_BuildingManager>()->AddBuilding(DeferredBuilding);
					DeferredBuilding->FinishSpawning(OutItem.Value);

					// 发送新建筑生成通知，让所有敌人重新寻找目标
					if (UHB_NotifySubsystem* NotifySubsystem = GetWorld()->GetSubsystem<UHB_NotifySubsystem>())
					{
						NotifySubsystem->SendToServer(NotifyNames::ON_SPAWN_BUILDING);
					}
				}
				else
				{
					UE_LOG(LogBuildingSystem, Error, TEXT("Failed to spawn building: %s"), *OutItem.Key->GetName());
				}
			}
			else
			{
				UE_LOG(LogBuildingSystem, Warning, TEXT("Skipping spawn due to null building class"));
			}
            CurrentlySpawnNum++;
		}
		UE_LOG(LogBuildingSystem, Verbose, TEXT("TickSpawnBuilding completed - Processed: %.0f buildings"), CurrentlySpawnNum);
	}
}

void UHB_BuildingSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TickFindTarget();
	TickSpawnBuilding();
}

void UHB_BuildingSubsystem::SpawnBuilding(TSubclassOf<AHB_Building_Base> InClass, const FTransform& InTransform)
{
	if (InClass)
	{
		UE_LOG(LogBuildingSystem, Log, TEXT("Adding building to spawn queue - Class: %s, Location: %s"), 
			*InClass->GetName(), *InTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("Attempted to spawn building with null class"));
	}
	SpawnBuildingQueue.Enqueue(TPair<TSubclassOf<AHB_Building_Base>, FTransform>(InClass, InTransform));
}

void UHB_BuildingSubsystem::DestroyBuilding(AHB_Building_Base*InBuilding)
{
	if (IsValid(InBuilding))
	{
		UE_LOG(LogBuildingSystem, Log, TEXT("Adding building to destroy queue - Building: %s, Location: %s"), 
			*InBuilding->GetName(), *InBuilding->GetActorLocation().ToString());
	}
	else
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("Attempted to destroy invalid building"));
	}
	GetManager<AHB_BuildingManager>()->RemoveBuilding(InBuilding);
	InBuilding->Destroy();
}

TArray<TObjectPtr<AHB_Building_Base>> UHB_BuildingSubsystem::GetAllBuilding()
{
	return GetManager<AHB_BuildingManager>()->GetAllBuildings();
}

TArray<TObjectPtr<AHB_Building_Base>> UHB_BuildingSubsystem::GetAllValidBuilding()
{
	
	TArray<TObjectPtr<AHB_Building_Base>> AllBuilding = GetAllBuilding();
	TArray<TObjectPtr<AHB_Building_Base>> AliveBuilding;
	for (TObjectPtr<AHB_Building_Base> Building : AllBuilding)
	{
		if (GetHelper<AHB_BuildingHelper>()->IsValidBuilding(Building))
		{
			AliveBuilding.Add(Building);
		}
	}
	return AliveBuilding;
}

UStaticMesh* UHB_BuildingSubsystem::GetBuildingPreviewMesh(TSubclassOf<AHB_Building_Base> InClass)
{
	UStaticMesh* RetStaticMesh = nullptr;

	if (InClass)
	{
		RetStaticMesh=BuildingData->GetPreviewMeshByBuildingClass(InClass);
    }

	if (!RetStaticMesh)
	{
		UE_LOG(LogBuildingSystem, Error, TEXT("GetBuildingPreviewMesh failed, InClass:%s"), *InClass->GetName());
	}
    return RetStaticMesh;
}

bool UHB_BuildingSubsystem::IsValidBuilding(AHB_Building_Base* InBuilding)
{
	return GetHelper<AHB_BuildingHelper>()->IsValidBuilding(InBuilding);
}
