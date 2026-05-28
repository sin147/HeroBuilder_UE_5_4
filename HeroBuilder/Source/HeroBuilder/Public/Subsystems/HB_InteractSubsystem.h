// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HB_WorldSubsystem_Base.h"
#include "Config/InteractData.h"
#include "HB_InteractSubsystem.generated.h"

class ACharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogInteractSubsystem, Log, All);

/**
 * 交互子系统
 * 负责管理所有玩家的交互模式与最近交互目标
 */
UCLASS()
class HEROBUILDER_API UHB_InteractSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
private:
	TObjectPtr<UInteractData> InteractData;
	//玩家控制器列表
	TArray<APlayerController*> PlayerControllers;

	//交互检测距离
	float InteractTraceDistance = 500.f;

	//Tick：更新玩家附近的交互目标
	void TickUpdateInteractTarget(float DeltaTime);
protected:
	virtual void Tick(float DeltaTime) override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnPlayerLogin(AGameModeBase* GameMode, APlayerController* PlayerController) override;
	virtual void OnPlayerLogout(AGameModeBase* GameMode, AController* Exiting) override;
public:
	//切换玩家交互模式
	void SwitchInteractMode(ACharacter* InCharacter, EPlayerCharacterInteractMode NewMode);
	//进入交互模式
	void EnterInteractMode(ACharacter* InCharacter, EPlayerCharacterInteractMode EnterMode);
	//离开交互模式
	void LeaveInteractMode(ACharacter* InCharacter, EPlayerCharacterInteractMode LeaveMode);
	//获取玩家当前的交互模式
	UFUNCTION(BlueprintCallable)
	EPlayerCharacterInteractMode GetInteractMode(ACharacter* InCharacter) const;
	//准备交互（在角色进入交互模式时调用）
	void PreInteract(ACharacter* InCharacter);
	//Post交互（在角色离开交互模式时调用）
	void PostInteract(ACharacter* InCharacter);

	//触发交互（在角色按下交互键时调用）
	void TryInteract(ACharacter* InCharacter);
	//获取交互动画
	UFUNCTION(BlueprintCallable)
	UAnimSequence* GetInteractAnim(EPlayerCharacterInteractMode InteractMode) const;
};