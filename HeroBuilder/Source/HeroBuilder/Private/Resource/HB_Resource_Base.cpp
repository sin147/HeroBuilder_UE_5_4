// Fill out your copyright notice in the Description page of Project Settings.


#include "Resource/HB_Resource_Base.h"
#include "Net/UnrealNetwork.h"
#include "Subsystems/HB_ResourceSubsystem.h"
#include "Config/ResourceData.h"

DEFINE_LOG_CATEGORY(LogResource)

// Sets default values
AHB_Resource_Base::AHB_Resource_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

FString AHB_Resource_Base::GetStateName(EResourceState State)
{
	switch (State)
	{
	case EResourceState::RS_Idle: return TEXT("Idle");
	case EResourceState::RS_BeHit: return TEXT("BeHit");
	case EResourceState::RS_Recover: return TEXT("Recover");
	case EResourceState::RS_Death: return TEXT("Death");
	default: return TEXT("Unknown");
	}
}

bool AHB_Resource_Base::SwitchState(EResourceState NewState)
{
	if (CurrentState == RS_Death)
	{
		return false; // 死亡锁定
	}
	if (CurrentState == NewState)
	{
		return false;
	}
	UE_LOG(LogResource, Log, TEXT("SwitchState %s"), *GetStateName(NewState));
	CurrentState = NewState;
	return true;
}

bool AHB_Resource_Base::IsDeath()
{
	return CurrentState == RS_Death;
}

// Called when the game starts or when spawned
void AHB_Resource_Base::BeginPlay()
{
	Super::BeginPlay();
	NetMode = GetWorld()->GetNetMode();
	if (HasAuthority())
	{
		CurrentHealth = MaxHealth;
		CurrentState = RS_Idle;
	}
}

void AHB_Resource_Base::ApplyDamage(AActor* Attacker, float Damage)
{
	if (!HasAuthority())
	{
		return;
	}
	if (IsDeath())
	{
		return;
	}

	CurrentHealth -= Damage;
	UE_LOG(LogResource, Log, TEXT("Resource %s took %.1f damage, current health: %.1f"), *GetName(), Damage, CurrentHealth);

	if (CurrentHealth <= 0.f)
	{
		CurrentHealth = 0.f;
		SwitchState(RS_Death);
		OnDeath();

		// 通知子系统处理资源掉落与销毁
		if (UHB_ResourceSubsystem* ResourceSubsystem = GetWorld()->GetSubsystem<UHB_ResourceSubsystem>())
		{
			ResourceSubsystem->OnResourceDeath(this);
		}
	}
	else
	{
		// 进入受击状态
		CurrentBeHitDuration = BeHitDuration;
		CurrentRecoverDelay = RecoverDelay;
		SwitchState(RS_BeHit);
		OnBeHit(Attacker);
	}
}

// Called every frame
void AHB_Resource_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	switch (CurrentState)
	{
	case RS_Idle:
	{
		// 满血则保持 Idle，未满血则等待脱战延迟过后切到 Recover
		if (CurrentHealth < MaxHealth)
		{
			CurrentRecoverDelay -= DeltaTime;
			if (CurrentRecoverDelay <= 0.f)
			{
				SwitchState(RS_Recover);
				OnRecover();
			}
		}
		break;
	}
	case RS_BeHit:
	{
		CurrentBeHitDuration -= DeltaTime;
		if (CurrentBeHitDuration <= 0.f)
		{
			SwitchState(RS_Idle);
		}
		break;
	}
	case RS_Recover:
	{
		CurrentHealth += RecoverSpeed * DeltaTime;
		if (CurrentHealth >= MaxHealth)
		{
			CurrentHealth = MaxHealth;
			SwitchState(RS_Idle);
		}
		break;
	}
	case RS_Death:
	{
		if (DeathTime > 0.f)
		{
			DeathTime -= DeltaTime;
		}
		else
		{
			SetActorTickEnabled(false);
			if (UHB_ResourceSubsystem* ResourceSubsystem = GetWorld()->GetSubsystem<UHB_ResourceSubsystem>())
			{
				ResourceSubsystem->DestroyResource(this);
			}
		}
		break;
	}
	default:
		break;
	}
}

void AHB_Resource_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_Resource_Base, CurrentState);
	DOREPLIFETIME(AHB_Resource_Base, CurrentHealth);
}

void AHB_Resource_Base::InitialResource(const FResourceConfig& InConfig)
{
	UE_LOG(LogResource, Log, TEXT("Initializing resource - Name: %s, Type: %d, Health: %.1f, Amount: %d"),
		*InConfig.ResourceName, static_cast<int32>(InConfig.ResourceType), InConfig.Health, InConfig.ResourceAmount);

	MaxHealth = InConfig.Health;
	CurrentHealth = MaxHealth;
	RecoverDelay = InConfig.RecoverDelay;
	RecoverSpeed = InConfig.RecoverSpeed;
	BeHitDuration = InConfig.BeHitDuration;
	DeathTime = InConfig.DeathTime;
	ResourceType = InConfig.ResourceType;
	ResourceAmount = InConfig.ResourceAmount;
}

