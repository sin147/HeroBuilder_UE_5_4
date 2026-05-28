// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/WidgetComponent.h"
#include "HB_Enemy_Base.generated.h"

class UHB_DamageComponent;

struct FEnemyConfig;
class AHB_Building_Base;
DECLARE_LOG_CATEGORY_EXTERN(LogEnemy, Log, All);

UENUM(BlueprintType)
enum EEnemyState : uint8
{
	ES_Idle UMETA(DisplayName = "Idle"),
	ES_Move UMETA(DisplayName = "Move"),
	ES_PreAttack UMETA(DisplayName = "PreAttack"),
	ES_Attack UMETA(DisplayName = "Attack"),
	ES_PostAttack UMETA(DisplayName = "PostAttack"),
	ES_Death UMETA(DisplayName = "Death")
};


UCLASS(Abstract)
class HEROBUILDER_API AHB_Enemy_Base : public ACharacter
{
	GENERATED_BODY()
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

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** 血量显示 Widget 组件，可在蓝图中指定 WidgetClass */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	/** 伤害组件：维护血量与伤害逻辑 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Damage")
	TObjectPtr<UHB_DamageComponent> DamageComponent;

	//初始血量
	UPROPERTY(EditAnywhere, Category = "Attribute")
	float MaxHealth = 100.f;

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
	void HandleDeath(AActor* Attacker);
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
