// Fill out your copyright notice in the Description page of Project Settings.

#include "Helper/HB_ResourceHelper.h"
#include "Resource/HB_Resource_Base.h"

bool AHB_ResourceHelper::IsValidResource(TObjectPtr<AHB_Resource_Base> InResource)
{
	return IsValid(InResource) && !InResource->IsDeath();
}
