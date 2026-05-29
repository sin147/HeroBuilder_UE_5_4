// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_GridSubsystem.h"
#include "Subsystems/HB_BuildingSubsystem.h"
#include "Subsystems/HB_ResourceSubsystem.h"
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
}
int32 UHB_GridSubsystem::GetGridWidth() const
{
    int32 Ret = 0;
	if (GridData)
	{
        Ret = GridData->GetGridWidth();
    }
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("GridData is null"));
	}
    return Ret;
}

int32 UHB_GridSubsystem::GetGridHeight() const
{
    int32 Ret = 0;
    if (GridData)
    {
        Ret = GridData->GetGridHeight();
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
FVector2D UHB_GridSubsystem::CalulateGridIndexByLocation(const FVector& Location) const
{
	if (!IsValid(GridData))
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("CalulateGridIndexByLocation: GridData is null"));
		return FVector2D::ZeroVector;
	}
	const int32 Width = GridData->GetGridWidth();
	if (Width == 0)
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("CalulateGridIndexByLocation: GridWidth is 0, cannot divide"));
		return FVector2D::ZeroVector;
	}
	int X = FMath::Floor(Location.X / Width);
	int Y = FMath::Floor(Location.Y / Width);

	return FVector2D(X,Y);
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
