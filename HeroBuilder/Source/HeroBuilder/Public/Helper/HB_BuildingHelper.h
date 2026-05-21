// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Helper/HB_Base_Helper.h"
#include "HB_BuildingHelper.generated.h"

class AHB_Building_Base;

/**
 * 
 */
UCLASS()
class HEROBUILDER_API AHB_BuildingHelper : public AHB_Base_Helper
{
	GENERATED_BODY()
public:
	bool IsValidBuilding(TObjectPtr<AHB_Building_Base> InBuilding);
	
};
