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
	MaxHealth = InMaxHealth;
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority())
		{
			const float OldHealth = CurrentHealth;
			CurrentHealth = MaxHealth;
			bIsDead = false;
			if (!FMath::IsNearlyEqual(OldHealth, CurrentHealth))
			{
				OnHealthChanged.Broadcast(OldHealth, CurrentHealth, MaxHealth, nullptr);
			}
		}
	}
}

void UHB_DamageComponent::ApplyDamage(AActor* Attacker, float Damage)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if (bIsDead)
	{
		return;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);
	LastAttacker = Attacker;

	UE_LOG(LogDamageComponent, Log, TEXT("%s took %.1f damage, health: %.1f -> %.1f"),
		*Owner->GetName(), Damage, OldHealth, CurrentHealth);

	// 服务端立即广播一次（OnRep只在客户端触发）
	OnHealthChanged.Broadcast(OldHealth, CurrentHealth, MaxHealth, Attacker);

	if (CurrentHealth <= 0.f && !bIsDead)
	{
		bIsDead = true;
		OnDeath.Broadcast(Attacker);
	}
}

void UHB_DamageComponent::OnRep_CurrentHealth(float OldHealth)
{
	// 客户端通过OnRep通知所有者刷新UI等
	OnHealthChanged.Broadcast(OldHealth, CurrentHealth, MaxHealth, LastAttacker);
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
		OnHealthChanged.Broadcast(OldHealth, CurrentHealth, MaxHealth, nullptr);
	}
}

void UHB_DamageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHB_DamageComponent, CurrentHealth);
}
