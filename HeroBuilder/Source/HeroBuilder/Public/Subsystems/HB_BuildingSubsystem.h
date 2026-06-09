// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "../Config/BuildingData.h"
#include "Helper/HB_BuildingHelper.h"
#include "HB_BuildingSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBuildingSystem,Log,All)
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnSpawnBuilding, AHB_Building_Base* /*Building*/, FTransform /*Enemy*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnDestroyBuilding, AHB_Building_Base* /*Building*/, FTransform /*Enemy*/);

class AHB_Building_Base;
class AHB_Enemy_Base;
/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_BuildingSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
private:
	//建筑数据
	UPROPERTY()
	TObjectPtr<UBuildingData> BuildingData;

	//待寻找目标的建筑
	TQueue<TObjectPtr<AHB_Building_Base>> FindTargetBuildingQueue;
	int32 FindNumByTick = 20;
	void TickFindTarget();
	//待销毁的建筑
	TQueue<TObjectPtr<AHB_Building_Base>> DestroyBuildingQueue;
	int32 DestroyNumByTick = 20;
public:
	/*************************************外部接口*****************************************/
	//生成建筑
	void SpawnBuilding(TSubclassOf<AHB_Building_Base> InClass, const FTransform& InTransform);
	//以Grid坐标(X,Y)生成建筑：内部根据GridSubsystem格宽，把(X,Y)还原为格子中心的世界坐标后调用SpawnBuilding
	void SpawnBuildingAtGrid(TSubclassOf<AHB_Building_Base> InClass, int32 InX, int32 InY, const FRotator& InRotation = FRotator::ZeroRotator, const FVector& InScale = FVector(1.f, 1.f, 1.f));
	//销毁建筑
	void DestroyBuilding(AHB_Building_Base* InBuilding);
	//寻找目标
	void FindAnyVaildTarget(AHB_Building_Base* InBuilding);
	//获取建筑数组
	TArray<TObjectPtr<AHB_Building_Base>> GetAllBuilding();
	//获取有效的建筑
	TArray<TObjectPtr<AHB_Building_Base>> GetAllValidBuilding();

    //获取建筑的预览网格
    UStaticMesh* GetBuildingPreviewMesh(TSubclassOf<AHB_Building_Base> InClass);
	//是否有效
	bool IsValidBuilding(AHB_Building_Base* InBuilding);
	FOnSpawnBuilding OnSpawnBuilding;
	FOnDestroyBuilding OnDestroyBuilding;
public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }

};
