// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HB_DamageInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UHB_DamageInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class HEROBUILDER_API IHB_DamageInterface
{
	GENERATED_BODY()
	friend class UHB_DamageSubsystem;
	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
protected:
    virtual void ApplyDamage(AActor* Attacker, float Damage)=0;
};
