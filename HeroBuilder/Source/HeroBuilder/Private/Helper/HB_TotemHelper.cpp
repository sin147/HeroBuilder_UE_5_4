// Fill out your copyright notice in the Description page of Project Settings.

#include "Helper/HB_TotemHelper.h"
#include "Totem/HB_Totem_Base.h"

bool AHB_TotemHelper::IsValidTotem(TObjectPtr<AHB_Totem_Base> InTotem)
{
	return IsValid(InTotem);
}