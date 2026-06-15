// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/WidgetComponent.h"
#include "Types/HB_Enums.h"
#include "HB_Enemy_Base.generated.h"

class UHB_DamageComponent;
class UHB_InteractComponent;

struct FEnemyConfig;
class AHB_Building_Base;
DECLARE_LOG_CATEGORY_EXTERN(LogEnemy, Log, All);

// 血量变化委托：旧值、新值、最大值、伤害来源
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnEnemyHealthChangedDelegate, float, OldHealth, float, NewHealth, float, MaxHealth, AActor*, Attacker);

class UBoxComponent;


UCLASS(Abstract)
class HEROBUILDER_API AHB_Enemy_Base : public ACharacter
{
	GENERATED_BODY()

public:
	// 血量变化时调用（蓝图可重写）
	UFUNCTION(BlueprintImplementableEvent)
	void OnHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker);

private:
	//是否死亡
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float DeathTime=10;
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float CombatRange=100;
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
	TObjectPtr<AActor> Target;
	UPROPERTY(EditAnywhere, Category = "Attribute")
    TSubclassOf<AActor> TargetClass;
	UPROPERTY()
	TObjectPtr<AAIController> AIController;
	FTimerHandle DeathTimer;
	ENetMode NetMode;
	bool WasFindTarget = false;
	bool IsValidTarget(TObjectPtr<AActor> InBuilding);
protected:
	UPROPERTY(Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TEnumAsByte<EEnemyState> CurrentState;
	bool SwitchState(EEnemyState NewState);
	FString GetStateName(EEnemyState State);
public:
	// Sets default values for this character's properties
	AHB_Enemy_Base();
	bool IsDeath();

	//获取攻击力
    UFUNCTION(Category = "Attribute|Attack",BlueprintPure)
	float GetAttack() { return Attack; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	//在所有组件创建/注册完成、属性首次复制之前调用：用于绑定 DamageComponent 委托，
	//避免在 BeginPlay 时机才绑定导致首包 OnRep 派发被错过；同时对蓝图子类删除/覆盖继承组件做运行时兜底。
	virtual void PostInitializeComponents() override;

	/** 血量显示 Widget 组件，可在蓝图中指定 WidgetClass */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** 伤害组件：维护血量与伤害逻辑 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	TObjectPtr<UHB_DamageComponent> DamageComponent;

	/** 交互组件：玩家靠近本敌人时应进入哪种交互类型（默认 IT_Attack） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interact")
	TObjectPtr<UHB_InteractComponent> InteractComponent;

	//初始血量
	float MaxHealth = 100.f;

	//攻击力

	float Attack = 10.f;

	//攻击延迟
	UPROPERTY(EditAnywhere, Category = "Attribute|Attack")
	float AttackPreDelay = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Attribute|Attack")
	float AttackPostDelay = 1.0f;
	float CurrentAttackDelay = 0.0f;

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

	/** 由DamageComponent委托回调：血量变化 */
	UFUNCTION()
	void HandleHealthChanged(float OldHealth, float NewHealth, float MaxHealthValue, AActor* Attacker);

	/** 由DamageComponent委托回调：死亡 */
	UFUNCTION()
	void HandleDeath();
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//移动到目标Actor
	UFUNCTION(BlueprintCallable)
	void StartMove();

	UFUNCTION(BlueprintCallable)
	void StopMove();

	UFUNCTION(BlueprintCallable)
	void StartAttack();

	UFUNCTION(BlueprintCallable)
	void StopAttack();

	void SetTarget(TObjectPtr<AActor>InTarget);

	//初始化敌人数据
	void InitialEnemy(const FEnemyConfig& InConfig);

};
