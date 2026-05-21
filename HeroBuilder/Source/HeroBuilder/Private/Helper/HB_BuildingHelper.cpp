// Fill out your copyright notice in the Description page of Project Settings.


#include "Helper/HB_BuildingHelper.h"
#include "Building/HB_Building_Base.h"

bool AHB_BuildingHelper::IsValidBuilding(TObjectPtr<AHB_Building_Base> InBuilding)
{
	return IsValid(InBuilding) && !InBuilding->IsDeath();
}
