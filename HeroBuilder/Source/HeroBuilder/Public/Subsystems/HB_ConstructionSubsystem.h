// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "../Config/ConstructionData.h"
#include "Engine/StaticMeshActor.h"
#include "HB_ConstructionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConstructionSubsystem,Log,All)

class AHB_Building_Base;
class AHB_ConstructionManager;
class APreBuilding;

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UHB_ConstructionSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
private:
	float GridWidth=400;
	float GridHeight=100;
	UPROPERTY()
	TObjectPtr<UConstructionData> ConstructionData;
	void TickPreviewBuildingPos();

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
    void SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass);
	void ConstructionBegin(ACharacter* InCharacter);
	bool CheckCanConstruction(ACharacter* InCharacter);
	void ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter);
	void CancelConstructionMode(TObjectPtr<ACharacter>InCharacter);

	//获取单例 ConstructionManager：统一走基类 GetManager<T>()，服务端读 GameMode、客户端读已复制的 GameState
	AHB_ConstructionManager* GetConstructionManager();
};
