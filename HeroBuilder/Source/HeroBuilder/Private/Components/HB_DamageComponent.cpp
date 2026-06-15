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

	// 不再在此处主动写 CurrentHealth：
	// Owner（Building / Enemy / Resource）会在自身 BeginPlay 之后通过 InitHealth(配置值) 完成权威初始化。
	// 若过早在此写一次默认 MaxHealth(=100) 会导致客户端先看到 100 再看到配置值的跳变。
	if (AActor* Owner = GetOwner())
	{
		if (Owner->HasAuthority())
		{
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
		OnDeath.Broadcast();
	}
	HealthChangeInfo.Damage = Damage;
	HealthChangeInfo.Attacker = Attacker;
	HealthChangeInfo.MaxHealth = MaxHealth;
	HealthChangeInfo.OldHealth = OldHealth;
	HealthChangeInfo.NewHealth = NewHealth;

}

void UHB_DamageComponent::OnRep_HealthChange()
{
	UE_LOG(LogDamageComponent, Log, TEXT("Client %s health changed, health: %.1f -> %.1f"),
		*GetOwner()->GetName(), HealthChangeInfo.OldHealth, HealthChangeInfo.NewHealth);
	OnHealthChanged.Broadcast(HealthChangeInfo.OldHealth, HealthChangeInfo.NewHealth, HealthChangeInfo.MaxHealth, HealthChangeInfo.Attacker);

}

void UHB_DamageComponent::OnRep_DeathChange()
{
	if(bIsDead)
	{
		OnDeath.Broadcast();
	}
	else
	{
		OnRevive.Broadcast();
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
	HealthChangeInfo.MaxHealth = MaxHealth;
	HealthChangeInfo.OldHealth = OldHealth;
    HealthChangeInfo.NewHealth = CurrentHealth;
}

void UHB_DamageComponent::Revive(float NewHealth)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return;
	}
	if(NewHealth < 0.f)
	{
		NewHealth = MaxHealth;
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
		OnRevive.Broadcast();
	}
	HealthChangeInfo.MaxHealth = MaxHealth;
	HealthChangeInfo.OldHealth = OldHealth;
	HealthChangeInfo.NewHealth = CurrentHealth;
}

void UHB_DamageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHB_DamageComponent, CurrentHealth);
	DOREPLIFETIME(UHB_DamageComponent, bIsDead);
	DOREPLIFETIME(UHB_DamageComponent, HealthChangeInfo);
	DOREPLIFETIME_CONDITION(UHB_DamageComponent, MaxHealth, COND_InitialOnly);
}