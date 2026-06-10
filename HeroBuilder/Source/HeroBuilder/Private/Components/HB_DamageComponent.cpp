// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/HB_DamageComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogDamageComponent);

UHB_DamageComponent::UHB_DamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHB_DamageComponent::BeginPlay()
{
	Super::BeginPlay();

	// 服务端初始化当前血量
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority())
		{
			CurrentHealth = MaxHealth;
			bIsDead = false;
		}
	}
}

void UHB_DamageComponent::InitHealth(float InMaxHealth)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	MaxHealth = InMaxHealth;
	const float OldHealth = CurrentHealth;
	CurrentHealth = MaxHealth;
	bIsDead = false;
}

void UHB_DamageComponent::ApplyDamage(AActor* Attacker, float Damage)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		// 严格服务端权威：客户端调用直接忽略
		return;
	}
	if (bIsDead)
	{
		return;
	}
	if (Damage <= 0.f)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	const float NewHealth = FMath::Max(0.f, CurrentHealth - Damage);

	// 写入复制属性（先写LastAttacker，确保客户端OnRep时已能拿到Attacker）

	CurrentHealth = NewHealth;

	UE_LOG(LogDamageComponent, Log, TEXT("%s took %.1f damage, health: %.1f -> %.1f"),
		*Owner->GetName(), Damage, OldHealth, CurrentHealth);

	// 服务端等价OnRep派发：本端立即派发血量变化
	OnHealthChanged.Broadcast(OldHealth, NewHealth, MaxHealth, Attacker);

	if (NewHealth <= 0.f)
	{
		bIsDead = true;
		// 服务端等价OnRep派发：本端立即派发死亡
		OnDeath.Broadcast(Attacker);
	}
	HealthChangeInfo.Damage = Damage;
	HealthChangeInfo.Attacker = Attacker;
	HealthChangeInfo.MaxHealth = MaxHealth;
	HealthChangeInfo.OldHealth = OldHealth;
	HealthChangeInfo.NewHealth = NewHealth;
	HealthChangeInfo.bIsDead = bIsDead;
}

void UHB_DamageComponent::OnRep_HealthChange()
{
    if (HealthChangeInfo.bIsDead)
	{
		OnDeath.Broadcast(HealthChangeInfo.Attacker);
	}
	else
	{
		OnHealthChanged.Broadcast(HealthChangeInfo.OldHealth, HealthChangeInfo.NewHealth, HealthChangeInfo.MaxHealth, HealthChangeInfo.Attacker);
	}

}

void UHB_DamageComponent::Heal(float HealAmount)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if (bIsDead || HealAmount <= 0.f)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + HealAmount);
	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		// 服务端等价OnRep派发
		OnHealthChanged.Broadcast(OldHealth, CurrentHealth, MaxHealth, nullptr);
	}
}

void UHB_DamageComponent::Revive(float NewHealth)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	const float ClampedHealth = FMath::Clamp(NewHealth, 0.f, MaxHealth);
	CurrentHealth = ClampedHealth;
	bIsDead = false;

	UE_LOG(LogDamageComponent, Log, TEXT("%s revived, health: %.1f -> %.1f"),
		*Owner->GetName(), OldHealth, CurrentHealth);

	if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
	{
		// 服务端等价OnRep派发
		OnHealthChanged.Broadcast(OldHealth, CurrentHealth, MaxHealth, nullptr);
	}
}

void UHB_DamageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHB_DamageComponent, CurrentHealth);
	DOREPLIFETIME(UHB_DamageComponent, bIsDead);
	DOREPLIFETIME(UHB_DamageComponent, HealthChangeInfo);
	DOREPLIFETIME_CONDITION(UHB_DamageComponent, MaxHealth, COND_InitialOnly);
}