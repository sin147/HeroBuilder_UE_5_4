// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_BuildingSubsystem.h"
#include "Building/HB_Building_Base.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Manager/HB_BuildingManager.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Subsystems/HB_GridSubsystem.h"
#include "Subsystems/HB_ResourceSubsystem.h"

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

void UHB_BuildingSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	TickFindTarget();
}

bool UHB_BuildingSubsystem::CanSpawnBuilding(TSubclassOf<AHB_Building_Base> InClass) const
{
	//空类直接拒绝：上层拿不到合法类时也不应该让 UI 点亮"可建造"
	if (!InClass)
	{
		return false;
	}

	//BuildingData 未就绪：默认放行（与原 "未配置=免费" 语义一致，避免初始化顺序问题倒致全局不能建造）
	if (!IsValid(BuildingData))
	{
		return true;
	}

	//该建筑未配资源消耗：免费建筑，直接放行
	const FBuildingConfig BuildingCfg = const_cast<UBuildingData*>(BuildingData.Get())->GetBuildingInfoByBuildingClass(InClass);
	if (BuildingCfg.BuildCostMap.Num() == 0)
	{
		return true;
	}

	//需依赖 ResourceSubsystem 查询玩家现有资源；Subsystem 未就绪路径安全起见返回 false
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	UHB_ResourceSubsystem* ResourceSub = World->GetSubsystem<UHB_ResourceSubsystem>();
	if (!ResourceSub)
	{
		return false;
	}

	//逐项校验：任一资源不足即返回 false； RT_None / 非正数视为无效项跳过
	for (const TPair<TEnumAsByte<EResourceType>, int32>& CostPair : BuildingCfg.BuildCostMap)
	{
		const EResourceType CostType = CostPair.Key.GetValue();
		const int32 CostAmount = CostPair.Value;
		if (CostType == EResourceType::RT_None || CostAmount <= 0)
		{
			continue;
		}
		if (ResourceSub->GetResourceAmount(CostType) < CostAmount)
		{
			return false;
		}
	}
	return true;
}

void UHB_BuildingSubsystem::SpawnBuilding(TSubclassOf<AHB_Building_Base> InClass, const FTransform& InTransform)
{
	//仅服务端生成建筑
	if (NetMode == ENetMode::NM_Client)
	{
		return;
	}

	if (!InClass)
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("Attempted to spawn building with null class"));
		return;
	}

	//建造前置校验：资源不足直接拒绝（与 UI/PreBuilding 复用同一套 CanSpawnBuilding 逻辑，结论一致）
	if (!CanSpawnBuilding(InClass))
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("SpawnBuilding aborted by CanSpawnBuilding: %s"), *InClass->GetName());
		return;
	}

	UE_LOG(LogBuildingSystem, Log, TEXT("Spawning building - Class: %s, Location: %s"),
		*InClass->GetName(), *InTransform.GetLocation().ToString());

	//校验通过后执行实际扣除（必须在真正生成 Actor 之前完成，避免失败时残留脏数据）
	//说明：
	//- BuildingData 未就绪 / BuildCostMap 为空 → CanSpawnBuilding 已放行，这里同样会自然跳过
	//- 通过 ResourceSubsystem 统一对接，避免 BuildingSubsystem 直接耦合 ResourceManager
	if (IsValid(BuildingData))
	{
		const FBuildingConfig BuildingCfg = BuildingData->GetBuildingInfoByBuildingClass(InClass);
		if (BuildingCfg.BuildCostMap.Num() > 0)
		{
			if (UHB_ResourceSubsystem* ResourceSub = GetWorld() ? GetWorld()->GetSubsystem<UHB_ResourceSubsystem>() : nullptr)
			{
				for (const TPair<TEnumAsByte<EResourceType>, int32>& CostPair : BuildingCfg.BuildCostMap)
				{
					const EResourceType CostType = CostPair.Key.GetValue();
					const int32 CostAmount = CostPair.Value;
					if (CostType == EResourceType::RT_None || CostAmount <= 0)
					{
						continue;
					}
					ResourceSub->ConsumeResourceAmount(CostType, CostAmount);
				}
			}
		}
	}

	//生成建筑（一步式 Spawn：BeginPlay 在 SpawnActor 内部就已执行完毕）
	//注意：因此 InitialBuilding 会在 BeginPlay 之后才调用，依赖配置参数的初始化要在 InitialBuilding 内部自行处理。
	TObjectPtr<AHB_Building_Base> NewBuilding = GetWorld()->SpawnActor<AHB_Building_Base>(InClass, InTransform);
	if (!IsValid(NewBuilding))
	{
		UE_LOG(LogBuildingSystem, Error, TEXT("Failed to spawn building: %s"), *InClass->GetName());
		return;
	}

	//初始化建筑数据
	if (IsValid(BuildingData))
	{
		NewBuilding->InitialBuilding(BuildingData->GetBuildingInfoByBuildingClass(InClass));
	}
	else
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("BuildingData is not valid, using default building config"));
		FBuildingConfig DefaultConfig;
		NewBuilding->InitialBuilding(DefaultConfig);
	}

	UE_LOG(LogBuildingSystem, Log, TEXT("Successfully spawned building: %s"), *NewBuilding->GetName());
	GetManager<AHB_BuildingManager>()->AddBuilding(NewBuilding);

	OnSpawnBuilding.Broadcast(NewBuilding, InTransform);
}

void UHB_BuildingSubsystem::SpawnBuildingAtGrid(TSubclassOf<AHB_Building_Base> InClass, int32 InX, int32 InY, const FRotator& InRotation, const FVector& InScale)
{
	//仅服务端生成建筑（与 SpawnBuilding 行为一致；提前拦截可省去无谓的坐标计算）
	if (NetMode == ENetMode::NM_Client)
	{
		return;
	}

	if (!InClass)
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("SpawnBuildingAtGrid: null class"));
		return;
	}

	UHB_GridSubsystem* GridSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_GridSubsystem>() : nullptr;
	if (!GridSubsystem)
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("SpawnBuildingAtGrid: GridSubsystem not ready"));
		return;
	}

	//Grid(X,Y) → 世界坐标（取格子中心，Z 默认 0，与现有 TickPreviewBuildingPos 一致）
	const float GridWidth = static_cast<float>(GridSubsystem->GetGridWidthFragment());
	const FVector SpawnLocation(
		GridWidth * InX + GridWidth * 0.5f,
		GridWidth * InY + GridWidth * 0.5f,
		0.f);

	const FTransform SpawnTransform(InRotation, SpawnLocation, InScale);
	SpawnBuilding(InClass, SpawnTransform);
}

void UHB_BuildingSubsystem::DestroyBuilding(AHB_Building_Base*InBuilding)
{
	if (!IsValid(InBuilding))
	{
		UE_LOG(LogBuildingSystem, Warning, TEXT("Attempted to destroy invalid building"));
		return;
	}
	UE_LOG(LogBuildingSystem, Log, TEXT("Destroying building - Building: %s, Location: %s"),
		*InBuilding->GetName(), *InBuilding->GetActorLocation().ToString());
	OnDestroyBuilding.Broadcast(InBuilding, InBuilding->GetActorTransform());
	AHB_BuildingManager* BuildingManager = GetManager<AHB_BuildingManager>();
	if (BuildingManager)
	{
		BuildingManager->RemoveBuilding(InBuilding);
	}
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
