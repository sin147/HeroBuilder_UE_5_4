// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Systems/SubsystemBase.h"
#include "DamageSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UDamageSubsystem : public USubsystemBase
{
	GENERATED_BODY()
	friend class ADamageProxy;
	friend class IDamageInterface;
protected:
	//服务器应用伤害
	void Server_TakeDamageImp(AActor* DamageTarget, float Damage, AActor* DamageCauser);

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UFUNCTION(BlueprintCallable)
    void TakeDamage(AActor* DamageTarget, float Damage, AActor* DamageCauser);
};
