// Fill out your copyright notice in the Description page of Project Settings.

#include "Manager/HB_TotemManager.h"
#include "Net/UnrealNetwork.h"
#include "Totem/HB_Totem_Base.h"

AHB_TotemManager::AHB_TotemManager()
{
}

TArray<TObjectPtr<AHB_Totem_Base>> AHB_TotemManager::GetAllTotems() const
{
	return Totems;
}

TArray<TObjectPtr<AHB_Totem_Base>> AHB_TotemManager::GetAllActiveTotems() const
{
	TArray<TObjectPtr<AHB_Totem_Base>> ActiveTotems;
	for (TObjectPtr<AHB_Totem_Base> Totem : Totems)
	{
		if (IsValid(Totem) && Totem->IsActive())
		{
			ActiveTotems.Add(Totem);
		}
	}
	return ActiveTotems;
}

void AHB_TotemManager::AddTotem(AHB_Totem_Base* InTotem)
{
	if (IsValid(InTotem))
	{
		Totems.AddUnique(InTotem);
	}
}

void AHB_TotemManager::RemoveTotem(AHB_Totem_Base* InTotem)
{
	if (Totems.Contains(InTotem))
	{
		Totems.RemoveSingleSwap(InTotem);
	}
}

void AHB_TotemManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHB_TotemManager, Totems);
}