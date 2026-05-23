// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Helper/HB_Base_Helper.h"
#include "HB_ResourceHelper.generated.h"

class AHB_Resource_Base;

/**
 * 
 */
UCLASS()
class HEROBUILDER_API AHB_ResourceHelper : public AHB_Base_Helper
{
	GENERATED_BODY()
public:
	bool IsValidResource(TObjectPtr<AHB_Resource_Base> InResource);
};
