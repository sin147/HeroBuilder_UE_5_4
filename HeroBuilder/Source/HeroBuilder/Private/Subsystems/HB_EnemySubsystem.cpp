// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_EnemySubsystem.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Building/HB_Building_Base.h"
#include "Helper/HB_EnemyHelper.h"
#include "Manager/HB_EnemyManager.h"
#include "Config/EnemyData.h"

DEFINE_LOG_CATEGORY(LogEnemySubsystem);

void UHB_EnemySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	EnemyData = LoadObject<UEnemyData>(this, TEXT("/Game/Config/DA_EnemyConfig"));

	if (!IsValid(EnemyData))
	{
		UE_LOG(LogEnemySubsystem, Error, TEXT("Failed to load EnemyData asset from /Game/Config/DA_EnemyConfig"));
		UE_LOG(LogEnemySubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
		UE_LOG(LogEnemySubsystem, Log, TEXT("Successfully loaded EnemyData asset"));
	}


}

void UHB_EnemySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	// 订阅新建筑生成通知
	if (UHB_BuildingSubsystem* BuildingSubsystem = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>())
	{
		BuildingSubsystem->OnSpawnBuilding.AddUObject(this,&UHB_EnemySubsystem::OnSpawnBuilding);
	}
}

void UHB_EnemySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickFindTarget();
	TickSpawnEnemy();
}

void UHB_EnemySubsystem::FindAnyVaildTarget(AHB_Enemy_Base* InEnemy)
{
	FindTargetEnemyQueue.Enqueue(InEnemy);
}

void UHB_EnemySubsystem::TickFindTarget()
{
    if (NetMode != ENetMode::NM_Client && !FindTargetEnemyQueue.IsEmpty())
	{
		UHB_BuildingSubsystem* BuildingSubsystem = GetWorld()->GetSubsystem<UHB_BuildingSubsystem>();
		if (!BuildingSubsystem)
		{
			return;
		}
		TArray<TObjectPtr<AHB_Building_Base>> BuildingArray = BuildingSubsystem->GetAllValidBuilding();
		float CurrentlyFindNum = 0;
		//遍历队列,找到最近的建筑
        while(!FindTargetEnemyQueue.IsEmpty() && CurrentlyFindNum < FindNumByTick)
		{
			//取出一个敌人
			TObjectPtr<AHB_Enemy_Base> OutItem;
            FindTargetEnemyQueue.Dequeue(OutItem);
			if (!IsValid(OutItem) || OutItem->IsDeath())
			{
				CurrentlyFindNum++;
				continue;
			}
			//找到最近的建筑
            TPair<TObjectPtr<AHB_Building_Base>, float>TargetDistancePair(nullptr, 1000000000);
			float Distance = 0;
			for (TObjectPtr<AHB_Building_Base> Building : BuildingArray)
			{
				if (!IsValid(Building)) continue;
				Distance = FVector::DistSquared(OutItem->GetActorLocation(), Building->GetActorLocation());
				if (TargetDistancePair.Value > Distance)
				{
					TargetDistancePair.Key = Building;
					TargetDistancePair.Value = Distance;
				}
			}
			CurrentlyFindNum++;
			//若找到目标，则设置目标，否则重新加入队列
			if (TargetDistancePair.Key)
			{
				OutItem->SetTarget(Cast<AActor>(TargetDistancePair.Key));
            }
			else
			{
				FindTargetEnemyQueue.Enqueue(OutItem);
			}
		}
	}
}

void UHB_EnemySubsystem::TickSpawnEnemy()
{
	if (NetMode != ENetMode::NM_Client && !SpawnEnemyQueue.IsEmpty())
	{
		UE_LOG(LogEnemySubsystem, Verbose, TEXT("TickSpawnEnemy - Queue not empty"));
		float CurrentlySpawnNum = 0;
        while(!SpawnEnemyQueue.IsEmpty() && CurrentlySpawnNum < SpawnNumByTick)
		{
			//取出一个敌人
			TPair<TSubclassOf<AHB_Enemy_Base>, FTransform> OutItem;
			SpawnEnemyQueue.Dequeue(OutItem);
			
			if (OutItem.Key)
			{
				UE_LOG(LogEnemySubsystem, Log, TEXT("Spawning enemy - Class: %s, Location: %s"), 
					*OutItem.Key->GetName(), *OutItem.Value.GetLocation().ToString());
				
				//生成敌人（一步式 Spawn：BeginPlay 在 SpawnActor 内部就已执行完毕）
				//注意：因此 InitialEnemy 会在 BeginPlay 之后才调用，依赖配置参数的初始化要在 InitialEnemy 内部自行处理。
				TObjectPtr<AHB_Enemy_Base> NewEnemy = GetWorld()->SpawnActor<AHB_Enemy_Base>(OutItem.Key, OutItem.Value);
				if (IsValid(NewEnemy))
				{
					//初始化敌人数据
					if (IsValid(EnemyData))
					{
						NewEnemy->InitialEnemy(EnemyData->GetEnemyInfoByEnemyClass(OutItem.Key));
					}
					else
					{
						UE_LOG(LogEnemySubsystem, Warning, TEXT("EnemyData is not valid, using default enemy config"));
						FEnemyConfig DefaultConfig;
						NewEnemy->InitialEnemy(DefaultConfig);
					}

					UE_LOG(LogEnemySubsystem, Log, TEXT("Successfully spawned enemy: %s"), *NewEnemy->GetName());
					GetManager<AHB_EnemyManager>()->AddEnemy(NewEnemy);
				}
				else
				{
					UE_LOG(LogEnemySubsystem, Error, TEXT("Failed to spawn enemy: %s"), *OutItem.Key->GetName());
				}
			}
			else
			{
				UE_LOG(LogEnemySubsystem, Warning, TEXT("Skipping spawn due to null enemy class"));
			}
            CurrentlySpawnNum++;
		}
		UE_LOG(LogEnemySubsystem, Verbose, TEXT("TickSpawnEnemy completed - Processed: %.0f enemies"), CurrentlySpawnNum);
	}
}

void UHB_EnemySubsystem::SpawnEnemy(TSubclassOf<AHB_Enemy_Base> InClass, const FTransform& InTransform)
{
	if (InClass)
	{
		UE_LOG(LogEnemySubsystem, Log, TEXT("Adding enemy to spawn queue - Class: %s, Location: %s"), 
			*InClass->GetName(), *InTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogEnemySubsystem, Warning, TEXT("Attempted to spawn enemy with null class"));
	}
	SpawnEnemyQueue.Enqueue(TPair<TSubclassOf<AHB_Enemy_Base>, FTransform>(InClass, InTransform));
}

void UHB_EnemySubsystem::DestroyEnemy(AHB_Enemy_Base* InEnemy)
{
	if (!IsValid(InEnemy))
	{
		return;
	}
	AHB_EnemyManager* EnemyManager = GetManager<AHB_EnemyManager>();
	if (EnemyManager)
	{
		EnemyManager->RemoveEnemy(InEnemy);
	}
	InEnemy->Destroy();
}

TArray<TObjectPtr<AHB_Enemy_Base>> UHB_EnemySubsystem::GetAllEnemies()
{
    return GetManager<AHB_EnemyManager>()->GetAllEnemies();
}

TArray<TObjectPtr<AHB_Enemy_Base>> UHB_EnemySubsystem::GetAllAliveEnemies()
{
	TArray<TObjectPtr<AHB_Enemy_Base>> AllEnemies = GetAllEnemies();
    TArray<TObjectPtr<AHB_Enemy_Base>> AliveEnemies;
    for (TObjectPtr<AHB_Enemy_Base> Enemy : AllEnemies)
    {
        if (GetHelper<AHB_EnemyHelper>()->IsValidEnemy(Enemy))
        {
            AliveEnemies.Add(Enemy);
        }
    }
	return AliveEnemies;
}

int32 UHB_EnemySubsystem::GetEnemyNum()
{
    return GetManager<AHB_EnemyManager>()->GetAllEnemies().Num();
}

bool UHB_EnemySubsystem::IsValidEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy)
{
	return GetHelper<AHB_EnemyHelper>()->IsValidEnemy(InEnemy);
}

void UHB_EnemySubsystem::AddAllEnemiesToFindTargetQueue()
{
	if (NetMode == NM_Client) return;

	TArray<TObjectPtr<AHB_Enemy_Base>> AllEnemies = GetAllEnemies();
	for (TObjectPtr<AHB_Enemy_Base> Enemy : AllEnemies)
	{
		if (IsValid(Enemy) && !Enemy->IsDeath())
		{
			FindTargetEnemyQueue.Enqueue(Enemy);
		}
	}
	UE_LOG(LogEnemySubsystem, Log, TEXT("Added all %d enemies to find target queue (new building spawned)"), AllEnemies.Num());
}

void UHB_EnemySubsystem::OnSpawnBuilding(AHB_Building_Base* NewBuilding, FTransform SpawnTransform)
{
	AddAllEnemiesToFindTargetQueue();
}
