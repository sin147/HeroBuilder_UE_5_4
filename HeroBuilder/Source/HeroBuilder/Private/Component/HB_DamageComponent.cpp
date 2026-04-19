// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/HB_DamageComponent.h"
#include "Subsystems/HB_DamageSubsystem.h"
DEFINE_LOG_CATEGORY(LogDamageComponent);

// Sets default values for this component's properties
UHB_DamageComponent::UHB_DamageComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


void UHB_DamageComponent::NetMulticast_ApplyDamage_Implementation(AActor* Attacker, float Damage)
{
	if(OnApplyDamage_Client.IsBound())
	{
		OnApplyDamage_Client.Execute(Attacker, Damage);
    }
}

// Called when the game starts
void UHB_DamageComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UHB_DamageComponent::Server_ApplyDamage_Implementation(AActor* Attacker, float Damage)
{
	if(OnApplyDamage_Server.IsBound())
    {
        OnApplyDamage_Server.Execute(Attacker,Damage);
    }
	NetMulticast_ApplyDamage(Attacker, Damage);
}

void UHB_DamageComponent::TakeDamage(AActor* Target, float Damage)
{
	Server_TakeDamage(Target, Damage);
}

void UHB_DamageComponent::Server_TakeDamage_Implementation(AActor* Target, float Damage)
{
    GetWorld()->GetSubsystem<UHB_DamageSubsystem>()->TakeDamage(GetOwner(), Damage, Target);
}


// Called every frame
void UHB_DamageComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

