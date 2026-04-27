// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/HB_DamageComponent.h"
#include "Subsystems/HB_DamageSubsystem.h"
DEFINE_LOG_CATEGORY(LogDamageComponent);

// Sets default values for this component's properties
UHB_DamageComponent::UHB_DamageComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


void UHB_DamageComponent::NetMulticast_ApplyDamage_Implementation(AActor* Attacker, float Damage)
{
	if(OnApplyDamage_Client.IsBound())
	{
		OnApplyDamage_Client.ExecuteIfBound(Attacker, Damage);
    }
}

// Called when the game starts
void UHB_DamageComponent::BeginPlay()
{
	Super::BeginPlay();
	if(GetWorld()->GetNetMode() == NM_Client)
	{
		SetComponentTickEnabled(false);
	}
	bIsServer = GetWorld()->GetNetMode() == NM_DedicatedServer || GetWorld()->GetNetMode() == NM_Standalone || GetWorld()->GetNetMode() == NM_ListenServer;
	// ...
	
}

void UHB_DamageComponent::ApplyDamage(AActor* Attacker, float Damage)
{
	if (bIsServer)
	{
		PreDamage += Damage;
		NetMulticast_ApplyDamage(Attacker, Damage);
		UE_LOG(LogTemp, Log, TEXT("PreDamage %lf"), Damage);
	}

}

void UHB_DamageComponent::TakeDamage(AActor* Target, float Damage)
{
	if (bIsServer)
	{
		GetWorld()->GetSubsystem<UHB_DamageSubsystem>()->TakeDamage(GetOwner(), Damage, Target);
		UE_LOG(LogTemp, Log, TEXT("TakeDamage %lf"), Damage);
	}

}


// Called every frame
void UHB_DamageComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bIsServer&&!bIsDeath && !FMath::IsNearlyZero(PreDamage, KINDA_SMALL_NUMBER))
	{
		Health = FMath::Clamp(Health - PreDamage, 0, MaxHealth);
		UE_LOG(LogTemp, Log, TEXT("CurrentlyHealth %lf"), Health);
		if (Health <= 0)
		{
			bIsDeath = true;
			OnDeath_Server.ExecuteIfBound();
			UE_LOG(LogTemp, Log, TEXT("Death"));
		}
		PreDamage = 0;
	}
	// ...
}

void UHB_DamageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHB_DamageComponent, Health);
	DOREPLIFETIME(UHB_DamageComponent, MaxHealth);
	DOREPLIFETIME(UHB_DamageComponent, Attack);
	DOREPLIFETIME(UHB_DamageComponent, bIsDeath);
}

