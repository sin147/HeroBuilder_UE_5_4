// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HB_WorldSubsystem_Base.h"
#include "Manager/HB_GridManager.h"
#include "Config/GridData.h"
#include "HB_GridSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGridSubsystem, Log, All);
class AHB_Building_Base;
class AHB_Resource_Base;
class AHB_Grid_Base;

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_GridSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
private:
	UPROPERTY()
	TObjectPtr<UGridData> GridData;
    TObjectPtr<AHB_GridManager> GridManager;
	void OnSpawnBuilding(AHB_Building_Base* InBuilding, FTransform InTransform);
	void OnDestroyBuilding(AHB_Building_Base* InBuilding, FTransform InTransform);
	void OnSpawnResource(AHB_Resource_Base* InResource, FTransform InTransform);
	void OnDestroyResource(AHB_Resource_Base* InResource, FTransform InTransform);
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    int32 GetGridWidthFragment() const;	
	int32 GetGridLengthFragment() const;
	TArray<FGridInfo> GetUsedGridIndexs();
	TArray<FGridInfo> GetFreeGridIndexs();
	//查询指定 Grid 是否已被占用（被建筑/资源占用则为 true）
	bool IsGridUsed(int32 InX, int32 InY) const;
	//直接对指定 Grid 进行占位标记。
	//典型用途：客户端"乐观预占位"——在等待服务端 SpawnBuilding 同步回来之前，先把目标 Grid 标为已占，
	//防止同一格在网络延迟窗口内被玩家重复触发建造。等服务端权威 Replicate 回来时会被覆盖刷新。
	void OccupyGrid(int32 InX, int32 InY);
	FVector2D CalulateGridIndexByLocation(const FVector& Location) const;
	UFUNCTION(BlueprintCallable)
	void SpawnAreaByLevel(int32 Level);
    AHB_Grid_Base* SpawnGrid(TSubclassOf<AHB_Grid_Base> GridClass, FVector Location, FRotator Rotation);

	FVector GetNextNavigationPoint(FVector CurrentLocation, FVector TargetLocation);
};
