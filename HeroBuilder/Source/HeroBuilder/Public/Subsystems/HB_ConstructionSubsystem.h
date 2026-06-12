// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_WorldSubsystem_Base.h"
#include "../Config/ConstructionData.h"
#include "Manager/HB_ConstructionManager.h"
#include "Engine/StaticMeshActor.h"
#include "HB_ConstructionSubsystem.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogConstructionSubsystem,Log,All)

class AHB_Building_Base;
class AHB_ConstructionManager;
class APreBuilding;

//—— 公开委托（蓝图可绑定）：客户端 FastArray 回调与服务端权威路径都收敛到这里 ——
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPreBuildingClassChanged, ACharacter*, Character, TSubclassOf<AHB_Building_Base>, NewClass, TSubclassOf<AHB_Building_Base>, OldClass);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPreBuildingActorChanged, ACharacter*, Character, APreBuilding*, NewActor, APreBuilding*, OldActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPreBuildingActiveChanged, ACharacter*, Character, bool, bIsActive);

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
	UFUNCTION()
	void OnCharacterEnterState(ACharacter* InCharacter, EPlayerCharacterState InState);
	UFUNCTION()
	void OnCharacterLeaveState(ACharacter* InCharacter, EPlayerCharacterState InState);

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void PostInitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { return TStatId(); }
    void SwitchBuilding(ACharacter* InOwnerCharacter, TSubclassOf<AHB_Building_Base> InBuildingClass);
	void PutBuilding(ACharacter* InCharacter);
	void SetEnablePreviewBuildingPos(ACharacter* InCharacter, bool bEnable);
	bool CheckCanConstruction(ACharacter* InCharacter);
	void ActiveConstructionMode(TObjectPtr<ACharacter>InCharacter);
	void CancelConstructionMode(TObjectPtr<ACharacter>InCharacter);

	//获取单例 ConstructionManager：统一走基类 GetManager<T>()，服务端读 GameMode、客户端读已复制的 GameState
	AHB_ConstructionManager* GetConstructionManager();

	//—— 公开委托：所有派发口收敛到 Manager.BroadcastXxx → 这里 Broadcast ——
	UPROPERTY(BlueprintAssignable)
	FOnPreBuildingClassChanged OnPreBuildingClassChanged;
	UPROPERTY(BlueprintAssignable)
	FOnPreBuildingActorChanged OnPreBuildingActorChanged;
	UPROPERTY(BlueprintAssignable)
	FOnPreBuildingActiveChanged OnPreBuildingActiveChanged;
};
