// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_GridSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Subsystems/HB_ResourceSubsystem.h"
#include "Grid/HB_Grid_Base.h"
#include "Manager/HB_GridManager.h"

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
	// 订阅新建筑生成通知
	if (UHB_BuildingSubsystem* Building=GetWorld()->GetSubsystem<UHB_BuildingSubsystem>())
	{
		Building->OnSpawnBuilding.AddUObject(this, &UHB_GridSubsystem::OnSpawnBuilding);
	}
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("Failed to get BuildingManager"));
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

	// 根据 GridData 中配置的 Area Level 列表，依次从中心向外生成区域
	if (IsValid(GridData))
	{
		const TArray<int32> AllLevels = GridData->GetAllAreaLevels();
		for (int32 Level : AllLevels)
		{
			SpawnAreaByLevel(Level);
		}
	}
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("OnWorldBeginPlay: GridData is null, skip spawning areas"));
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
	const int32 Width = GridData->GetGridWidthFragment();
	if (Width == 0)
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("CalulateGridIndexByLocation: GridWidthFragment is 0, cannot divide"));
		return FVector2D::ZeroVector;
	}
	int X = FMath::Floor(Location.X / Width);
	int Y = FMath::Floor(Location.Y / Width);

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
    int32 GridWidth = GridData->GetGridWidthFragment()*100;
	int32 GridLength = GridData->GetGridLengthFragment()*100;

    //获取Grid类
    TSubclassOf<AHB_Grid_Base> GridClass = GridData->GetGridClassByLevel(Level);

	//Level == 0 时只在中心生成一个Grid，避免上下边重叠
	if (Level == 0)
	{
		SpawnGrid(GridClass, FVector::ZeroVector, FRotator::ZeroRotator);
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

	//根据等级生成区域
    //生成X方向的区域（上下两条横边）
    for(int32 i=0; i<GridCount; i++)
    {
		//生成上边的区域
        SpawnGrid(GridClass, FVector(LeftUpPos.X + i * GridWidth, LeftUpPos.Y, 0), FRotator::ZeroRotator);
        //生成下边的区域
        SpawnGrid(GridClass, FVector(LeftDownPos.X + i * GridWidth, LeftDownPos.Y, 0), FRotator::ZeroRotator);
	}
	//生成Y方向的区域（左右两条竖边，去掉四个角避免和横边重复）
    for(int32 i=1; i<GridCount-1; i++)
	{
		//生成左边的区域
		SpawnGrid(GridClass,FVector(LeftDownPos.X, LeftDownPos.Y + i * GridLength, 0), FRotator::ZeroRotator);
		//生成右边的区域
        SpawnGrid(GridClass,FVector(RightDownPos.X, RightDownPos.Y + i * GridLength, 0), FRotator::ZeroRotator);
	}

}
void UHB_GridSubsystem::SpawnGrid(TSubclassOf<AHB_Grid_Base> GridClass, FVector Location, FRotator Rotation)
{
    GetWorld()->SpawnActor<AHB_Grid_Base>(GridClass, Location, Rotation);
    
}
void UHB_GridSubsystem::OnSpawnBuilding(AHB_Building_Base* NewBuilding, FTransform SpawnTransform)
{
	FVector2D GridIndex = CalulateGridIndexByLocation(SpawnTransform.GetLocation());
	GetManager<AHB_GridManager>()->CacheUsedGridInfo(GridIndex.X,GridIndex.Y);
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
