// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HB_Grid_Base.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class HEROBUILDER_API AHB_Grid_Base : public AActor
{
	GENERATED_BODY()
	
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

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	void SetGridMaterial(UMaterialInterface* Material);

};
