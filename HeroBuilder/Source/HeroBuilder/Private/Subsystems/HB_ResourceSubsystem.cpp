// Fill out your copyright notice in the Description page of Project Settings.

#include "Subsystems/HB_ResourceSubsystem.h"
#include "Resource/HB_Resource_Base.h"
#include "Manager/HB_ResourceManager.h"
#include "Helper/HB_ResourceHelper.h"
#include "Subsystems/HB_GridSubsystem.h"
#include "Config/ResourceData.h"

DEFINE_LOG_CATEGORY(LogResourceSubsystem);

void UHB_ResourceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResourceData = LoadObject<UResourceData>(this, TEXT("/Game/Config/DA_ResourceConfig"));

	if (!IsValid(ResourceData))
	{
		UE_LOG(LogResourceSubsystem, Error, TEXT("Failed to load ResourceData asset from /Game/Config/DA_ResourceConfig"));
		UE_LOG(LogResourceSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
		UE_LOG(LogResourceSubsystem, Log, TEXT("Successfully loaded ResourceData asset"));
		//初始化每个生成配置的计时器
		SpawnConfigTimers.Init(0.f, ResourceData->GetResourceSpawnConfigs().Num());
	}
}

void UHB_ResourceSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickSpawnResource();
	TickAutoSpawn(DeltaTime);
}

void UHB_ResourceSubsystem::TickAutoSpawn(float DeltaTime)
{
	if (NetMode == ENetMode::NM_Client)
	{
		return;
	}
	if (!IsValid(ResourceData))
	{
		return;
	}

	const TArray<FResourceSpawnConfig>& SpawnConfigs = ResourceData->GetResourceSpawnConfigs();
	if (SpawnConfigs.Num() == 0)
	{
		return;
	}

	//容量补齐，避免配置在运行时被改动越界
	if (SpawnConfigTimers.Num() != SpawnConfigs.Num())
	{
		SpawnConfigTimers.SetNumZeroed(SpawnConfigs.Num());
	}

	for (int32 i = 0; i < SpawnConfigs.Num(); i++)
	{
		const FResourceSpawnConfig& Config = SpawnConfigs[i];
		if (!Config.ResourceClass)
		{
			continue;
		}

		//累计计时
		SpawnConfigTimers[i] += DeltaTime;

		//达到生成间隔
		if (SpawnConfigTimers[i] >= Config.SpawnInterval)
		{
			SpawnConfigTimers[i] = 0.f;

			//检查最大存在数量
			const int32 AliveCount = GetAliveResourceCountByClass(Config.ResourceClass);
			if (AliveCount >= Config.MaxAliveCount)
			{
				UE_LOG(LogResourceSubsystem, Verbose, TEXT("Resource %s reached max alive count: %d/%d, skip spawn"),
					*Config.ResourceClass->GetName(), AliveCount, Config.MaxAliveCount);
				continue;
			}

			//随机生成位置
			const FTransform SpawnTransform = GetRandomSpawnTransform();
			SpawnResource(Config.ResourceClass, SpawnTransform);
		}
	}
}

int32 UHB_ResourceSubsystem::GetAliveResourceCountByClass(TSubclassOf<AHB_Resource_Base> ResourceClass) const
{
	if (!ResourceClass)
	{
		return 0;
	}
	AHB_ResourceManager* Manager = const_cast<UHB_ResourceSubsystem*>(this)->GetManager<AHB_ResourceManager>();
	if (!Manager)
	{
		return 0;
	}
	int32 Count = 0;
	for (TObjectPtr<AHB_Resource_Base> Resource : Manager->GetAllResources())
	{
		if (IsValid(Resource) && !Resource->IsDeath() && Resource->IsA(ResourceClass))
		{
			Count++;
		}
	}
	return Count;
}

FTransform UHB_ResourceSubsystem::GetRandomSpawnTransform() const
{
	FVector Center = FVector::ZeroVector;
	FVector Extent = FVector(2000.f, 2000.f, 0.f);
	if (IsValid(ResourceData))
	{
		Center = ResourceData->GetSpawnAreaCenter();
		Extent = ResourceData->GetSpawnAreaExtent();
	}

	const float RandX = FMath::FRandRange(-Extent.X, Extent.X);
	const float RandY = FMath::FRandRange(-Extent.Y, Extent.Y);
	FVector SpawnLocation = Center + FVector(RandX, RandY, 0);
	FVector2D GridIndex= GetWorld()->GetSubsystem<UHB_GridSubsystem>()->CalulateGridIndexByLocation(SpawnLocation);
	TArray<FGridInfo> UsedGrids=GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetUsedGridIndexs();
	TArray<FGridInfo> FreeGrids = GetWorld()->GetSubsystem<UHB_GridSubsystem>()->GetUsedGridIndexs();
	if(UsedGrids.Contains(FGridInfo(GridIndex.X,GridIndex.Y)))
	{
		if (!FreeGrids.IsEmpty())
		{
			SpawnLocation=FVector(FreeGrids[0].X, FreeGrids[0].Y,0);
		}
		return GetRandomSpawnTransform();
	}

	const FRotator SpawnRotation = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
	return FTransform(SpawnRotation, SpawnLocation);
}

void UHB_ResourceSubsystem::TickSpawnResource()
{
	if (NetMode != ENetMode::NM_Client && !SpawnResourceQueue.IsEmpty())
	{
		float CurrentlySpawnNum = 0;
		while (!SpawnResourceQueue.IsEmpty() && CurrentlySpawnNum < SpawnNumByTick)
		{
			TPair<TSubclassOf<AHB_Resource_Base>, FTransform> OutItem;
			SpawnResourceQueue.Dequeue(OutItem);

			if (OutItem.Key)
			{
				UE_LOG(LogResourceSubsystem, Log, TEXT("Spawning resource - Class: %s, Location: %s"),
					*OutItem.Key->GetName(), *OutItem.Value.GetLocation().ToString());

				TObjectPtr<AHB_Resource_Base> DeferredResource = GetWorld()->SpawnActorDeferred<AHB_Resource_Base>(OutItem.Key, OutItem.Value);
				if (IsValid(DeferredResource))
				{
					if (IsValid(ResourceData))
					{
						DeferredResource->InitialResource(ResourceData->GetResourceInfoByResourceClass(OutItem.Key));
					}
					else
					{
						UE_LOG(LogResourceSubsystem, Warning, TEXT("ResourceData is not valid, using default resource config"));
						FResourceConfig DefaultConfig;
						DeferredResource->InitialResource(DefaultConfig);
					}

					GetManager<AHB_ResourceManager>()->AddResource(DeferredResource);
					DeferredResource->FinishSpawning(OutItem.Value);
					UE_LOG(LogResourceSubsystem, Log, TEXT("Successfully spawned resource: %s"), *DeferredResource->GetName());
				}
				else
				{
					UE_LOG(LogResourceSubsystem, Error, TEXT("Failed to spawn resource: %s"), *OutItem.Key->GetName());
				}
			}
			else
			{
				UE_LOG(LogResourceSubsystem, Warning, TEXT("Skipping spawn due to null resource class"));
			}
			CurrentlySpawnNum++;
		}
	}
}

void UHB_ResourceSubsystem::SpawnResource(TSubclassOf<AHB_Resource_Base> InClass, const FTransform& InTransform)
{
	if (InClass)
	{
		UE_LOG(LogResourceSubsystem, Log, TEXT("Adding resource to spawn queue - Class: %s, Location: %s"),
			*InClass->GetName(), *InTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogResourceSubsystem, Warning, TEXT("Attempted to spawn resource with null class"));
	}
	SpawnResourceQueue.Enqueue(TPair<TSubclassOf<AHB_Resource_Base>, FTransform>(InClass, InTransform));
}

void UHB_ResourceSubsystem::DestroyResource(AHB_Resource_Base* InResource)
{
	if (!IsValid(InResource))
	{
		return;
	}
	GetManager<AHB_ResourceManager>()->RemoveResource(InResource);
	InResource->Destroy();
}

void UHB_ResourceSubsystem::OnResourceDeath(AHB_Resource_Base* InResource)
{
	if (!IsValid(InResource))
	{
		return;
	}
	if (NetMode == ENetMode::NM_Client)
	{
		return;
	}

	const EResourceType Type = InResource->GetResourceType();
	const int32 Amount = InResource->GetResourceAmount();

	UE_LOG(LogResourceSubsystem, Log, TEXT("Resource %s died, granting %d of type %d"),
		*InResource->GetName(), Amount, static_cast<int32>(Type));

	if (AHB_ResourceManager* Manager = GetManager<AHB_ResourceManager>())
	{
		Manager->AddResourceAmount(Type, Amount);
	}
}

TArray<TObjectPtr<AHB_Resource_Base>> UHB_ResourceSubsystem::GetAllResources()
{
	return GetManager<AHB_ResourceManager>()->GetAllResources();
}

TArray<TObjectPtr<AHB_Resource_Base>> UHB_ResourceSubsystem::GetAllAliveResources()
{
	TArray<TObjectPtr<AHB_Resource_Base>> AllResources = GetAllResources();
	TArray<TObjectPtr<AHB_Resource_Base>> AliveResources;
	for (TObjectPtr<AHB_Resource_Base> Resource : AllResources)
	{
		if (GetHelper<AHB_ResourceHelper>()->IsValidResource(Resource))
		{
			AliveResources.Add(Resource);
		}
	}
	return AliveResources;
}

int32 UHB_ResourceSubsystem::GetResourceNum()
{
	return GetManager<AHB_ResourceManager>()->GetAllResources().Num();
}

bool UHB_ResourceSubsystem::IsValidResource(TObjectPtr<AHB_Resource_Base> InResource)
{
	return GetHelper<AHB_ResourceHelper>()->IsValidResource(InResource);
}

int32 UHB_ResourceSubsystem::GetResourceAmount(EResourceType InType)
{
	if (AHB_ResourceManager* Manager = GetManager<AHB_ResourceManager>())
	{
		return Manager->GetResourceAmount(InType);
	}
	return 0;
}

bool UHB_ResourceSubsystem::ConsumeResourceAmount(EResourceType InType, int32 InAmount)
{
	if (AHB_ResourceManager* Manager = GetManager<AHB_ResourceManager>())
	{
		return Manager->ConsumeResourceAmount(InType, InAmount);
	}
	return false;
}
