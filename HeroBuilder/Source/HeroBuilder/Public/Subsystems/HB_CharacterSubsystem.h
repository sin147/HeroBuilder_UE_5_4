// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HB_WorldSubsystem_Base.h"
#include "Manager/HB_CharacterManager.h"
#include "HB_CharacterSubsystem.generated.h"

class ACharacter;
class AActor;
class AHeroBuilderCharacter;

DECLARE_LOG_CATEGORY_EXTERN(LogCharacterSubsystem, Log, All);

/**
 * 角色子系统
 * 负责所有玩家角色的状态机逻辑（SwitchState / OnEnter / OnLeave / Tick 推进 / 追击与交互流程）
 * 数据由 AHB_CharacterManager 维护，整张表服务端权威+Replicate；本Subsystem仅承担逻辑。
 */
UCLASS()
class HEROBUILDER_API UHB_CharacterSubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()

private:

	//已注册参与Tick的角色（仅服务端使用，客户端通过CharacterManager表项遍历）
	UPROPERTY()
	TArray<TObjectPtr<ACharacter>> RegisteredCharacters;
	TMap<TObjectPtr<ACharacter>, float> CharacterInteractTimer;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual void OnPlayerLogin(AGameModeBase* GameMode, APlayerController* PlayerController) override;
	virtual void OnPlayerLogout(AGameModeBase* GameMode, AController* Exiting) override;

public:
	//—— Manager访问 ——
	AHB_CharacterManager* GetCharacterManager();

	//—— 角色注册/反注册（由Character在BeginPlay/EndPlay触发） ——
	void RegisterCharacter(ACharacter* InCharacter);
	void UnregisterCharacter(ACharacter* InCharacter);

	//—— 状态机API（服务端权威）——
	void SwitchState(ACharacter* InCharacter, EPlayerCharacterState NewState);
	void AbortInteract(ACharacter* InCharacter);
	void BeginInteractFlow(ACharacter* InCharacter);

	//—— 同步属性Getter（透传到Manager，便于外部Caller使用） ——
	UFUNCTION(BlueprintPure, Category = "Character")
	EPlayerCharacterState GetCurrentState(ACharacter* InCharacter) const;
	AActor* GetInteractTarget(ACharacter* InCharacter) const;
	void SetInteractTarget(ACharacter* InCharacter, AActor* Target);
	UFUNCTION(BlueprintPure, Category = "Character|Stats")
	float GetAttack(ACharacter* InCharacter) const;
	void SetAttack(ACharacter* InCharacter, float NewAttack);
	float GetInteractRange(ACharacter* InCharacter) const;
	void SetInteractRange(ACharacter* InCharacter, float NewRange);

	UFUNCTION(BlueprintPure, Category = "Character|Move")
	UCameraComponent* GetCharacterFollowCamera(AHeroBuilderCharacter* InCharacter) const;
	UFUNCTION(BlueprintPure, Category = "Character|Move")
	FVector GetCharacterFollowCameraForward(AHeroBuilderCharacter* InCharacter) const;

private:
	//状态进入/离开（仅服务端权威环境调用）
	void OnEnterState(ACharacter* InCharacter, EPlayerCharacterState EnterState);
	void OnLeaveState(ACharacter* InCharacter, EPlayerCharacterState LeaveState);
	bool CanSwitchState(ACharacter* InCharacter, EPlayerCharacterState NewState, EPlayerCharacterState OldState);

	//每角色推进
	void TickCharacter(ACharacter* InCharacter, float DeltaTime);
	void TickUpdateState(ACharacter* InCharacter, float DeltaTime);
	//客户端本地：仅做视觉/预测处理，绝不触发权威逻辑
	void TickCharacterState(ACharacter* InCharacter, float DeltaTime);

	//移动到目标
	void TickAuthorityMoveToTarget(ACharacter* InCharacter, float DeltaTime);
	void TickMoveToTarget(ACharacter* InCharacter, float DeltaTime);
};
