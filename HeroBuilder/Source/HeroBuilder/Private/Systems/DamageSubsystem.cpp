// Fill out your copyright notice in the Description page of Project Settings.


#include "Systems/DamageSubsystem.h"
#include "NetworkProxy/DamageProxy.h"

void UDamageSubsystem::Server_TakeDamageImp(AActor* DamageTarget, float Damage, AActor* DamageCauser)
{

}

void UDamageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	NetworkProxyClass = ADamageProxy::StaticClass();
}

void UDamageSubsystem::TakeDamage(AActor* DamageTarget, float Damage, AActor* DamageCauser)
{
	ADamageProxy* DamageProxy = GetNetworkProxy<ADamageProxy>();
	if(DamageProxy)
	{
		DamageProxy->Server_TakeDamage(DamageTarget, Damage, DamageCauser);
	}
}
