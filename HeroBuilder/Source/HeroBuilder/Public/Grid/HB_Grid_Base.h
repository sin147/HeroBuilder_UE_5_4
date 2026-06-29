// Fill out your copyright notice in the Description page of Project Settings.
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Manager/HB_GridManager.h"
#include "Net/UnrealNetwork.h"
#include "HB_Grid_Base.generated.h"


class UInstancedStaticMeshComponent;
struct FFragmentInfo
{
public:
	FFragmentInfo() = default;
	FFragmentInfo(int32 InIndex, const FGridInfo& InGridCoord)
		: Index(InIndex), GridCoord(InGridCoord) {
	}
	//Fragment索引
	int32 Index = 0;
	// 对应的全局 Fragment 坐标
	FGridInfo GridCoord;
};

UCLASS()
class HEROBUILDER_API AHB_Grid_Base : public AActor
{
	GENERATED_BODY()
private:
	TArray<FFragmentInfo> FragmentInfos;
	// 全局 Fragment 坐标 -> FragmentInfos 索引
	TMap<FGridInfo, int32> FragmentCoordToIndexMap;

	/** 当前区域等级，用于客户端按 GridData 还原材质 */
	UPROPERTY(ReplicatedUsing = OnRep_AreaLevel)
	int32 AreaLevel = -1;

	/** 根据 AreaLevel 应用对应材质 */
	void ApplyAreaMaterial();

	UFUNCTION()
	void OnRep_AreaLevel();

public:	
	// 用于批量绘制所有格子的 ISM 组件，一次 DrawCall 即可绘制全部实例
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
	UInstancedStaticMeshComponent* GridISMComponent;
	// Sets default values for this actor's properties
	AHB_Grid_Base();
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// 在Actor构造（包括编辑器中放置/属性修改、以及运行时Spawn）时生成所有Grid实例
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 生成所有 ISM 实例，并在服务端注册到 GridSubsystem */
	void GenerateGridInstances();

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void SetGridMaterial(UMaterialInterface* Material);
	/** 设置区域等级，服务端会复制给客户端并自动应用材质 */
	void SetAreaLevel(int32 InLevel);
	int32 GetAreaLevel() const { return AreaLevel; }
	bool GetRandomPosition(FVector& OutPosition);
	/** 获取本 Grid 的所有 Fragment 全局坐标 */
	TArray<FGridInfo> GetAllFragmentCoords() const;
	/** 获取本 Grid 中心对应的 Fragment 全局坐标 */
	FGridInfo GetCenterFragmentCoord() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
