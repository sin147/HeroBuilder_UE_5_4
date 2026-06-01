// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HB_WorldSubsystem_Base.h"
#include "Config/InteractData.h"
#include "Manager/HB_InteractManager.h"
#include "HB_InteractSubsystem.generated.h"

class ACharacter;
class AHB_InteractManager;

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

	//单例 InteractManager 缓存（懒查找：服务端按需Spawn，客户端通过Replication拿到）
	UPROPERTY()
	mutable TObjectPtr<AHB_InteractManager> CachedInteractManager;

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
	//获取单例 InteractManager；服务端在缺失时按需Spawn一个，客户端只读取已复制的实例
	AHB_InteractManager* GetInteractManager() const;

	//切换玩家交互模式
	void SwitchInteractType(ACharacter* InCharacter, EInteractType NewMode);
	//进入交互模式
	void EnterInteractType(ACharacter* InCharacter, EInteractType EnterMode);
	//离开交互模式
	void LeaveInteractType(ACharacter* InCharacter, EInteractType LeaveMode);
	//获取玩家当前的交互模式
	UFUNCTION(BlueprintCallable)
	EInteractType GetInteractType(ACharacter* InCharacter) const;
	//获取玩家当前的交互模式（建造/正常）
    UFUNCTION(BlueprintCallable)
	EInteractMode GetInteractMode(ACharacter* InCharacter) const;
	//设置玩家当前的交互模式（建造/正常）—— 纯Setter：仅写表，无副作用
	void SetCurrentInteractMode(ACharacter* InCharacter, EInteractMode NewMode);
	//切换玩家交互模式（建造/正常）：负责 Leave + Enter 一条龙
	void SwitchInteractMode(ACharacter* InCharacter, EInteractMode NewMode);
	//进入交互模式（建造/正常）
	void EnterInteractMode(ACharacter* InCharacter, EInteractMode EnterMode);
	//离开交互模式（建造/正常）
	void LeaveInteractMode(ACharacter* InCharacter, EInteractMode LeaveMode);
	//准备交互（在角色进入交互模式时调用）
	void PreInteract(ACharacter* InCharacter);
	//Post交互（在角色离开交互模式时调用）
	void PostInteract(ACharacter* InCharacter);

	//触发交互（在角色按下交互键时调用）
	void TryInteract(ACharacter* InCharacter);
	//获取交互动画
	UFUNCTION(BlueprintCallable)
	UAnimSequence* GetInteractAnim(EInteractMode InteractMode, EInteractType InteractType) const;
};