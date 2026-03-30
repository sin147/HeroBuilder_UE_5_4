// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkProxy/DamageProxy.h"
#include "Systems/DamageSubsystem.h"

void ADamageProxy::MountToSubsystem()
{
	GetWorld()->GetSubsystem<UDamageSubsystem>()->SetNetworkProxy(this);
}

void ADamageProxy::Server_TakeDamage_Implementation(AActor* DamageTarget, float Damage, AActor* DamageCauser)
{
	GetWorld()->GetSubsystem<UDamageSubsystem>()->Server_TakeDamageImp(DamageTarget, Damage, DamageCauser);
}
