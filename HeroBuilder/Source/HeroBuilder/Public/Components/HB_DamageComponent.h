// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HB_DamageComponent.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogDamageComponent, Log, All);

// 血量变化委托：旧值、新值、最大值、伤害来源
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnHealthChangedDelegate, float, OldHealth, float, NewHealth, float, MaxHealth, AActor*, Attacker);

// 死亡委托：伤害来源
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageDeathDelegate, AActor*, Attacker);


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class HEROBUILDER_API UHB_DamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHB_DamageComponent();

	// 由Subsystem在服务端调用：施加伤害（仅在Authority生效）
	void ApplyDamage(AActor* Attacker, float Damage);

	// 治疗：增加当前血量（不超过MaxHealth），仅在Authority生效
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void Heal(float HealAmount);

	// 设置最大血量（同时把当前血量重置为该值）
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void InitHealth(float InMaxHealth);

	// 复活：将 bIsDead 重置为 false 并把当前血量设为 NewHealth（不超过 MaxHealth）。仅在 Authority 生效。
	// 用于"死亡-治疗-复活"场景：让此前因 ApplyDamage 击杀而锁死的 Heal/ApplyDamage 重新生效。
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void Revive(float NewHealth);

	UFUNCTION(BlueprintCallable, Category = "Damage")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Damage")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Damage")
	bool IsDead() const { return bIsDead; }

	// 血量变化时通知所有者（包括客户端OnRep）
	UPROPERTY(BlueprintAssignable, Category = "Damage")
	FOnHealthChangedDelegate OnHealthChanged;

	// 死亡时通知所有者（仅服务端触发）
	UPROPERTY(BlueprintAssignable, Category = "Damage")
	FOnDamageDeathDelegate OnDeath;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Damage")
	float CurrentHealth = 100.f;

	UFUNCTION()
	void OnRep_CurrentHealth(float OldHealth);

private:
	bool bIsDead = false;

	// 客户端记录上一次最近的Attacker（仅做日志/UI参考），由服务端ApplyDamage时使用
	UPROPERTY()
	TObjectPtr<AActor> LastAttacker = nullptr;
};
