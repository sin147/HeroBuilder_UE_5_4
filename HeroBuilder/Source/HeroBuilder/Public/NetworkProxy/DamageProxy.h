// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NetworkProxy/NetworkProxyBase.h"
#include "DamageProxy.generated.h"

/**
 * 
 */
UCLASS()
class HEROBUILDER_API ADamageProxy : public ANetworkProxyBase
{
	GENERATED_BODY()
	friend class UDamageSubsystem;
protected:
	virtual void MountToSubsystem() override;
	UFUNCTION(Server, Reliable)
	void Server_TakeDamage(AActor* DamageTarget, float Damage, AActor* DamageCauser);
};
