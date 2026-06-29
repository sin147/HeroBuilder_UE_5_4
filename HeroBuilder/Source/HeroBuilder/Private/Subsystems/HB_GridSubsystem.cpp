// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_GridSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Subsystems/HB_ResourceSubsystem.h"
#include "Subsystems/HB_TotemSubsystem.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Grid/HB_Grid_Base.h"
#include "Manager/HB_GridManager.h"
#include "Totem/HB_Totem_Base.h"
#include "Enemy/HB_Enemy_Base.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavigationSystem.h"
#include "Components/BrushComponent.h"
#include "Engine/CollisionProfile.h"

DEFINE_LOG_CATEGORY(LogGridSubsystem);

void UHB_GridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GridData = LoadObject<UGridData>(this, TEXT("/Game/Config/DA_GridConfig"));

	if (!IsValid(GridData))
	{
        UE_LOG(LogGridSubsystem, Error, TEXT("Failed to load GridData asset from /Game/Config/DA_GridConfig"));
		UE_LOG(LogGridSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
        UE_LOG(LogGridSubsystem, Log, TEXT("Successfully loaded GridData asset"));
	}
}
void UHB_GridSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (NetMode == NM_Client)
	{
		return;
	}

	// 订阅新建筑生成通知
	if (UHB_BuildingSubsystem* Building=GetWorld()->GetSubsystem<UHB_BuildingSubsystem>())
	{
		Building->OnSpawnBuilding.AddUObject(this, &UHB_GridSubsystem::OnSpawnBuilding);
	}
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("Failed to get BuildingSubsystem"));
	}

	// 订阅资源生成/销毁通知
	if (UHB_ResourceSubsystem* Resource = GetWorld()->GetSubsystem<UHB_ResourceSubsystem>())
	{
		Resource->OnSpawnResource.AddUObject(this, &UHB_GridSubsystem::OnSpawnResource);
		Resource->OnDestroyResource.AddUObject(this, &UHB_GridSubsystem::OnDestroyResource);
	}
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("Failed to get ResourceSubsystem"));
	}

	// 订阅图腾生成通知
	if (UHB_TotemSubsystem* Totem = GetWorld()->GetSubsystem<UHB_TotemSubsystem>())
	{
		Totem->OnSpawnTotem.AddUObject(this, &UHB_GridSubsystem::OnSpawnTotem);
	}
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("Failed to get TotemSubsystem"));
	}

	// 订阅敌人生成/销毁通知
	if (UHB_EnemySubsystem* Enemy = GetWorld()->GetSubsystem<UHB_EnemySubsystem>())
	{
		Enemy->OnSpawnEnemy.AddUObject(this, &UHB_GridSubsystem::OnSpawnEnemy);
		Enemy->OnDestroyEnemy.AddUObject(this, &UHB_GridSubsystem::OnDestroyEnemy);
	}
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("Failed to get EnemySubsystem"));
	}


}
int32 UHB_GridSubsystem::GetGridWidthFragment() const
{
    int32 Ret = 0;
	if (IsValid(GridData))
	{
        Ret = GridData->GetGridWidthFragment();
    }
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("GridData is null"));
	}
    return Ret;
}

int32 UHB_GridSubsystem::GetGridLengthFragment() const
{
    int32 Ret = 0;
	if (IsValid(GridData))
	{
        Ret = GridData->GetGridLengthFragment();
    }
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("GridData is null"));
	}
    return Ret;
}

TArray<FGridInfo> UHB_GridSubsystem::GetUsedGridIndexs()
{
	return GetManager<AHB_GridManager>()->GetUsedGridInfo();
}
TArray<FGridInfo> UHB_GridSubsystem::GetFreeGridIndexs()
{
	return GetManager<AHB_GridManager>()->GetFreeGridInfo();
}

bool UHB_GridSubsystem::IsGridUsed(int32 InX, int32 InY) const
{
	AHB_GridManager* GridMgr = const_cast<UHB_GridSubsystem*>(this)->GetManager<AHB_GridManager>();
	if (!GridMgr)
	{
		UE_LOG(LogGridSubsystem, Warning, TEXT("IsGridUsed: GridManager is null"));
		return false;
	}
	//FGridInfo 已重载 operator==，可直接 Contains 查找
	return GridMgr->GetUsedGridInfo().Contains(FGridInfo(InX, InY));
}

void UHB_GridSubsystem::OccupyGrid(int32 InX, int32 InY)
{
	AHB_GridManager* GridMgr = GetManager<AHB_GridManager>();
	if (!GridMgr)
	{
		UE_LOG(LogGridSubsystem, Warning, TEXT("OccupyGrid: GridManager is null"));
		return;
	}
	//直接调用 Manager 的 AddUnique 写入。
	//注：UsedGridInfos 本身是 Replicated，但客户端直接写本地数组是合法的——
	//等服务端下次 Bunch 到达时会被权威值覆盖刷新；这正是"乐观预占位"的预期行为。
	GridMgr->CacheUsedGridInfo(InX, InY);
}

FVector2D UHB_GridSubsystem::CalulateGridIndexByLocation(const FVector& Location) const
{
	if (!IsValid(GridData))
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("CalulateGridIndexByLocation: GridData is null"));
		return FVector2D::ZeroVector;
	}
	const int32 Width = GRID_FRAGMENT_SIZE;
    const int32 Length = GRID_FRAGMENT_SIZE;
	if (Width == 0)
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("CalulateGridIndexByLocation: GridWidthFragment is 0, cannot divide"));
		return FVector2D::ZeroVector;
	}
	if (Length == 0)
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("CalulateGridIndexByLocation: GridLengthFragment is 0, cannot divide"));
		return FVector2D::ZeroVector;
	}
	int X = FMath::Floor(Location.X / Width);
	int Y = FMath::Floor(Location.Y / Length);

	return FVector2D(X,Y);
}

void UHB_GridSubsystem::SpawnAreaByLevel(int32 Level)
{
	FAreaConfig AreaConfig;
	if(!GridData->GetAreaConfigByLevel(Level, AreaConfig))
	{
		UE_LOG(LogGridSubsystem, Warning, TEXT("SpawnAreaByLevel: No AreaConfig for Level %d"), Level);
		return;
	}
    //单个Grid的宽度
    int32 GridWidth = GridData->GetGridWidthFragment() * GRID_FRAGMENT_SIZE;
	int32 GridLength = GridData->GetGridLengthFragment() * GRID_FRAGMENT_SIZE;
	int32 AreaHeight = -GRID_FRAGMENT_SIZE;


    //获取Grid类
    TSubclassOf<AHB_Grid_Base> GridClass = GridData->GetGridClassByLevel(Level);

	//Level == 0 时只在中心生成一个Grid，避免上下边重叠
	if (Level == 0)
	{
        AHB_Grid_Base* NewGrid = SpawnGrid(GridClass, FVector(0, 0, AreaHeight), FRotator::ZeroRotator);
		NewGrid->SetAreaLevel(Level);
		return;
	}

	//单边Grid数量
	int32 GridCount = Level * 2 + 1;
    //左上角位置
	FVector2D LeftUpPos = FVector2D(-GridWidth * Level, GridLength * Level);
    //右上角位置
    FVector2D RightUpPos = FVector2D(GridWidth*Level, GridLength * Level);
	//左下角位置
	FVector2D LeftDownPos = FVector2D(-GridWidth * Level, -GridLength * Level);
	//右下角位置
	FVector2D RightDownPos = FVector2D(GridWidth * Level, -GridLength * Level);

	//根据等级生成区域：分别生成 上、下、左、右 四条边
	TArray<AHB_Grid_Base*> CurrentlyLevelGrids;
	//生成上边的区域（从左上到右上，整行 GridCount 个）
	for (int32 i = 0; i < GridCount; i++)
	{
        AHB_Grid_Base* NewGrid = SpawnGrid(GridClass, FVector(LeftUpPos.X + i * GridWidth, LeftUpPos.Y, AreaHeight), FRotator::ZeroRotator);
		NewGrid->SetAreaLevel(Level);
		CurrentlyLevelGrids.Add(NewGrid);
	}

	//生成下边的区域（从左下到右下，整行 GridCount 个）
	for (int32 i = 0; i < GridCount; i++)
	{
        AHB_Grid_Base* NewGrid = SpawnGrid(GridClass, FVector(LeftDownPos.X + i * GridWidth, LeftDownPos.Y, AreaHeight), FRotator::ZeroRotator);
		NewGrid->SetAreaLevel(Level);
		CurrentlyLevelGrids.Add(NewGrid);
	}

	//生成左边的区域（从左下到左上，去掉两端的角，避免和上下边重复）
	for (int32 i = 1; i < GridCount - 1; i++)
	{
        AHB_Grid_Base* NewGrid = SpawnGrid(GridClass, FVector(LeftDownPos.X, LeftDownPos.Y + i * GridLength, AreaHeight), FRotator::ZeroRotator);
		NewGrid->SetAreaLevel(Level);
		CurrentlyLevelGrids.Add(NewGrid);
	}

	//生成右边的区域（从右下到右上，去掉两端的角，避免和上下边重复）
	for (int32 i = 1; i < GridCount - 1; i++)
	{
        AHB_Grid_Base* NewGrid = SpawnGrid(GridClass, FVector(RightDownPos.X, RightDownPos.Y + i * GridLength, AreaHeight), FRotator::ZeroRotator);
		NewGrid->SetAreaLevel(Level);
		CurrentlyLevelGrids.Add(NewGrid);
	}
	//生成资源
	const TMap<TSubclassOf<AHB_Resource_Base>, int32>& ResourceConfigs = AreaConfig.ResourceConfigs;
	UHB_ResourceSubsystem* ResourceSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_ResourceSubsystem>() : nullptr;
	if (!ResourceSubsystem)
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("SpawnAreaByLevel: Failed to get ResourceSubsystem"));
	}
	else
	{
		for (const auto& ResourceConfig : ResourceConfigs)
		{
			const int32 ResourceCount = ResourceConfig.Value;
			const TSubclassOf<AHB_Resource_Base>& ResourceClass = ResourceConfig.Key;
			for (int32 i = 0; i < ResourceCount; ++i)
			{
				FVector ResourceSpawnPosition;
				bool bSpawned = false;

				// 在当前区域 Grid 中随机尝试，避免某个 Grid 已满时一直命中它
				for (int32 Try = 0; Try < CurrentlyLevelGrids.Num(); ++Try)
				{
					AHB_Grid_Base* RandomGrid = CurrentlyLevelGrids[FMath::RandRange(0, CurrentlyLevelGrids.Num() - 1)];
					if (RandomGrid && RandomGrid->GetRandomPosition(ResourceSpawnPosition))
					{
						ResourceSubsystem->SpawnResource(ResourceClass, FTransform(FRotator::ZeroRotator, ResourceSpawnPosition+FVector(0,0,100), FVector(1, 1, 1)));
						bSpawned = true;
						break;
					}

				}

				if (!bSpawned)
				{
					UE_LOG(LogGridSubsystem, Warning, TEXT("SpawnAreaByLevel Level=%d: 当前区域没有足够空闲 Fragment 生成资源 %s，已生成 %d/%d"),
						Level, *ResourceClass->GetName(), i, ResourceCount);
					break;
				}
			}
		}
	}

	//生成图腾
	UHB_TotemSubsystem* TotemSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_TotemSubsystem>() : nullptr;
	if (!TotemSubsystem)
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("SpawnAreaByLevel: Failed to get TotemSubsystem"));
	}
	else
	{
		const TMap<TSubclassOf<AHB_Totem_Base>, int32>& TotemConfigs = AreaConfig.TotemConfigs;
		for (const auto& TotemConfig : TotemConfigs)
		{
			const int32 TotemCount = TotemConfig.Value;
			const TSubclassOf<AHB_Totem_Base>& TotemClass = TotemConfig.Key;
			for (int32 i = 0; i < TotemCount; ++i)
			{
				FVector TotemSpawnPosition;
				bool bSpawned = false;

				// 在当前区域 Grid 中随机尝试，避免某个 Grid 已满时一直命中它
				for (int32 Try = 0; Try < CurrentlyLevelGrids.Num(); ++Try)
				{
					AHB_Grid_Base* RandomGrid = CurrentlyLevelGrids[FMath::RandRange(0, CurrentlyLevelGrids.Num() - 1)];
					if (RandomGrid && RandomGrid->GetRandomPosition(TotemSpawnPosition))
					{
						TotemSubsystem->SpawnTotem(TotemClass, FTransform(FRotator::ZeroRotator, TotemSpawnPosition+FVector(0,0,200), FVector(1, 1, 1)));
						bSpawned = true;
						break;
					}
				}

				if (!bSpawned)
				{
					UE_LOG(LogGridSubsystem, Warning, TEXT("SpawnAreaByLevel Level=%d: 当前区域没有足够空闲位置生成图腾 %s，已生成 %d/%d"),
						Level, *TotemClass->GetName(), i, TotemCount);
					break;
				}
			}
		}
	}
	//生成敌人
	UHB_EnemySubsystem* EnemySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UHB_EnemySubsystem>() : nullptr;
	if (!EnemySubsystem)
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("SpawnAreaByLevel: Failed to get EnemySubsystem"));
	}
	else
	{
		const TMap<TSubclassOf<AHB_Enemy_Base>, int32>& EnemyConfigs = AreaConfig.EnemyConfigs;
		for (const auto& EnemyConfig : EnemyConfigs)
		{
			const int32 EnemyCount = EnemyConfig.Value;
			const TSubclassOf<AHB_Enemy_Base>& EnemyClass = EnemyConfig.Key;
			for (int32 i = 0; i < EnemyCount; ++i)
			{
				FVector EnemySpawnPosition;
				bool bSpawned = false;

				// 在当前区域 Grid 中随机尝试，避免某个 Grid 已满时一直命中它
				for (int32 Try = 0; Try < CurrentlyLevelGrids.Num(); ++Try)
				{
					AHB_Grid_Base* RandomGrid = CurrentlyLevelGrids[FMath::RandRange(0, CurrentlyLevelGrids.Num() - 1)];
					if (RandomGrid && RandomGrid->GetRandomPosition(EnemySpawnPosition))
					{
						EnemySubsystem->Spawn(EnemyClass, FTransform(FRotator::ZeroRotator, EnemySpawnPosition+FVector(0,0,200), FVector(1, 1, 1)));
						bSpawned = true;
						break;
					}
				}

				if (!bSpawned)
				{
					UE_LOG(LogGridSubsystem, Warning, TEXT("SpawnAreaByLevel Level=%d: 当前区域没有足够空闲位置生成敌人 %s，已生成 %d/%d"),
						Level, *EnemyClass->GetName(), i, EnemyCount);
					break;
				}
			}
		}
	}
}


AHB_Grid_Base* UHB_GridSubsystem::SpawnGrid(TSubclassOf<AHB_Grid_Base> GridClass, FVector Location, FRotator Rotation)
{
	AHB_Grid_Base* NewGrid = GetWorld()->SpawnActor<AHB_Grid_Base>(GridClass, Location, Rotation);
	// GridActor 会在 OnConstruction 中调用 RegisterGridActor，这里无需重复注册
    return NewGrid;
}

void UHB_GridSubsystem::RegisterGridActor(AHB_Grid_Base* InGridActor)
{
	if (!IsValid(InGridActor))
	{
		return;
	}

	const FGridInfo CenterCoord = InGridActor->GetCenterFragmentCoord();
	GridActorMap.Add(CenterCoord, InGridActor);

	AHB_GridManager* GridMgr = GetManager<AHB_GridManager>();
	if (!GridMgr)
	{
		UE_LOG(LogGridSubsystem, Warning, TEXT("RegisterGridActor: GridManager is null"));
		return;
	}

	const TArray<FGridInfo> FragmentCoords = InGridActor->GetAllFragmentCoords();
	for (const FGridInfo& Coord : FragmentCoords)
	{
		GridMgr->RegisterFreeGridInfo(Coord.X, Coord.Y);
	}
}

AHB_Grid_Base* UHB_GridSubsystem::FindGridActorByFragmentCoord(int32 InX, int32 InY) const
{
	// 根据 Fragment 坐标反推 Grid 中心坐标：Grid 中心是 GridWidthFragment/LengthFragment 的整数倍
	const int32 GridWidthFragment = const_cast<UHB_GridSubsystem*>(this)->GetGridWidthFragment();
	const int32 GridLengthFragment = const_cast<UHB_GridSubsystem*>(this)->GetGridLengthFragment();
	if (GridWidthFragment <= 0 || GridLengthFragment <= 0)
	{
		return nullptr;
	}

	const int32 CenterX = FMath::Floor((static_cast<float>(InX) + GridWidthFragment / 2.0f) / GridWidthFragment) * GridWidthFragment;
	const int32 CenterY = FMath::Floor((static_cast<float>(InY) + GridLengthFragment / 2.0f) / GridLengthFragment) * GridLengthFragment;

	if (const TWeakObjectPtr<AHB_Grid_Base>* GridPtr = GridActorMap.Find(FGridInfo(CenterX, CenterY)))
	{
		return GridPtr->Get();
	}
	return nullptr;
}

FVector UHB_GridSubsystem::GetNextNavigationPoint(FVector CurrentLocation, FVector TargetLocation)
{
	// 计算朝向（仅在 XY 平面内，忽略 Z 轴差异，避免高度差导致斜率计算异常）
	FVector Delta = TargetLocation - CurrentLocation;
	Delta.Z = 0.0f;
	const FVector Direction = Delta.GetSafeNormal();
	//预测步长
	float StepX = Direction.X * GRID_FRAGMENT_SIZE;
	float StepY = Direction.Y * GRID_FRAGMENT_SIZE;
	//计算下一个点
	float NextX = CurrentLocation.X + StepX;
	float NextY = CurrentLocation.Y + StepY;

    const FVector NextPoint = FVector(NextX, NextY, CurrentLocation.Z);
	return NextPoint;
}

void UHB_GridSubsystem::OnSpawnBuilding(AHB_Building_Base* NewBuilding, FTransform SpawnTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(SpawnTransform.GetLocation());
	GetManager<AHB_GridManager>()->CacheUsedGridInfo(GridIndex.X, GridIndex.Y);
}

void UHB_GridSubsystem::OnDestroyBuilding(AHB_Building_Base* InBuilding, FTransform InTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(InTransform.GetLocation());
	GetManager<AHB_GridManager>()->RemoveUsedGridInfo(GridIndex.X, GridIndex.Y);
}

void UHB_GridSubsystem::OnSpawnResource(AHB_Resource_Base* InResource, FTransform InTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(InTransform.GetLocation());
	GetManager<AHB_GridManager>()->CacheUsedGridInfo(GridIndex.X, GridIndex.Y);
}

void UHB_GridSubsystem::OnDestroyResource(AHB_Resource_Base* InResource, FTransform InTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(InTransform.GetLocation());
	GetManager<AHB_GridManager>()->RemoveUsedGridInfo(GridIndex.X, GridIndex.Y);
}

void UHB_GridSubsystem::OnSpawnTotem(AHB_Totem_Base* InTotem, FTransform InTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(InTransform.GetLocation());
	GetManager<AHB_GridManager>()->CacheUsedGridInfo(GridIndex.X, GridIndex.Y);
}

void UHB_GridSubsystem::OnSpawnEnemy(AHB_Enemy_Base* InEnemy, FTransform InTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(InTransform.GetLocation());
	GetManager<AHB_GridManager>()->CacheUsedGridInfo(GridIndex.X, GridIndex.Y);
}

void UHB_GridSubsystem::OnDestroyEnemy(AHB_Enemy_Base* InEnemy, FTransform InTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(InTransform.GetLocation());
	GetManager<AHB_GridManager>()->RemoveUsedGridInfo(GridIndex.X, GridIndex.Y);
}
