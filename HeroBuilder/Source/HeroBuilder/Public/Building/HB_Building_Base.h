// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Components/WidgetComponent.h"
#include "Types/HB_Enums.h"
#include "HB_Building_Base.generated.h"

class UHB_DamageComponent;
class UHB_InteractComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogBuilding, Log, All);

// 血量变化委托：旧值、新值、最大值、伤害来源
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBuildingHealthChangedDelegate, float, OldHealth, float, NewHealth, float, MaxHealth, AActor*, Attacker);

struct FBuildingConfig;

UCLASS(Abstract)
class HEROBUILDER_API AHB_Building_Base : public AActor
{
	GENERATED_BODY()

public:
	// 血量变化时调用（蓝图可重写）
	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker);

private:
	UPROPERTY(Replicated)
    TObjectPtr<AActor> Target;
	UPROPERTY(Replicated)
    TEnumAsByte<EBuildingState> CurrentState = EBuildingState::BS_Idle;
	bool SwitchState(EBuildingState NewState);
	float Attack = 10.f;
	float DeathTime = 10.f;
	float PreAttackDelay = 1.f;
	float PostAttackDelay = 1.f;
	float CurrentAttackDelay = 0.f;
	float CombatRange=1000;
	bool WasFindTarget = false;
	UPROPERTY(ReplicatedUsing = OnRep_Rotation)
	FRotator RotateMeshRotation;
	UFUNCTION()
	void OnRep_Rotation();

	// Sets default values for this actor's properties
	AHB_Building_Base();
	void TickBuildingState(float DeltaTime);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//在所有组件创建/注册完成、属性首次复制之前调用：用于绑定 DamageComponent 委托，
	//避免在 BeginPlay 时机才绑定导致首包 OnRep 派发被错过；同时对蓝图子类删除/覆盖继承组件做运行时兜底。
	virtual void PostInitializeComponents() override;
	UPROPERTY(BlueprintReadOnly,EditAnywhere, Category = "Attribute")
	TObjectPtr<UStaticMeshComponent> RotateMesh;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<UStaticMeshComponent> BaseMesh;

	UPROPERTY(EditAnywhere, Category = "Attribute")
	TObjectPtr<USceneComponent> Root;

	/** 血量显示 Widget 组件，可在蓝图中指定 WidgetClass */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** 伤害组件：维护血量与伤害逻辑 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	TObjectPtr<UHB_DamageComponent> DamageComponent;

	/** 交互组件：玩家靠近本建筑时应进入哪种交互类型（默认 IT_Attack） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact")
	TObjectPtr<UHB_InteractComponent> InteractComponent;

	//服务端死亡
	void Server_Death();
	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnPreAttack(AActor* InTarget);

	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnPostAttack(AActor* InTarget);
	//攻击表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnAttack(AActor* InTarget);
	//死亡表现
	UFUNCTION(BlueprintImplementableEvent)
	void OnDeath();
	UFUNCTION(BlueprintImplementableEvent)
	void OnRevive();

	bool bIsServer;
	UPROPERTY(Replicated,BlueprintReadOnly, Category = "Attribute", meta = (AllowPrivateAccess = true))
	float RotateSpeed = 100.0f;

	UFUNCTION(BlueprintCallable)
	void StartRotate();
	UFUNCTION(BlueprintCallable)
	void StopRotate();
	UFUNCTION(BlueprintCallable)
    void StartAttack();
    UFUNCTION(BlueprintCallable)
    void StopAttack();
	UPROPERTY(EditAnywhere, Category = "Attribute")
	TSubclassOf<AActor> TargetClass;
    bool IsValidTarget(AActor* InTarget) const;

	/** 由DamageComponent委托回调：血量变化 */
	UFUNCTION()
	void HandleHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker);

	/** 由DamageComponent委托回调：死亡 */
	UFUNCTION()
	void HandleDeath();
	/** 由DamageComponent委托回调：复活 */
	UFUNCTION()
	void HandleRevive();

public:	
	void InitialBuilding(FBuildingConfig InConfig);
	void SetTarget(AActor* InTarget);
	bool IsDeath();
	float GetCombatRange() const;

	//获取攻击力
	UFUNCTION(Category = "Attribute|Attack",BlueprintPure)
	float GetAttack() { return Attack; }

	//是否处于"待修建"状态：当前死亡（血量为 0），可被玩家通过 IM_Normal 交互治疗复活
	UFUNCTION(BlueprintPure, Category = "Construction")
	bool IsAwaitingConstruction() const;

	//作为"被修建"接收治疗：若死亡则先复活 DamageComponent，再治疗指定数值；血量回满后切回 BS_Idle
	void HealAsConstruction(float Amount, AActor* Healer);
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 命名常量
	static constexpr float MIN_ATTACK_ANGLE = 0.98f;

	// 状态名称辅助函数
	FString GetStateName(EBuildingState State);
};
