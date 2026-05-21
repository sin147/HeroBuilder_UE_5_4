// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Helper/HB_Base_Helper.h"
#include "HB_EnemyHelper.generated.h"

class AHB_Enemy_Base;

/**
 * 
 */
UCLASS()
class HEROBUILDER_API AHB_EnemyHelper : public AHB_Base_Helper
{
	GENERATED_BODY()
public:
    bool IsValidEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy);
};
