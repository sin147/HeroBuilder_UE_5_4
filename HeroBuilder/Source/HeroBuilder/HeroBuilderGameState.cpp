// Fill out your copyright notice in the Description page of Project Settings.

#include "HeroBuilderGameState.h"
#include "Manager/HB_Base_Manager.h"
#include "Net/UnrealNetwork.h"

AHeroBuilderGameState::AHeroBuilderGameState()
{
	bReplicates = true;
}

void AHeroBuilderGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroBuilderGameState, ReplicatedManagers);
}

void AHeroBuilderGameState::RegisterManager(AHB_Base_Manager* Manager)
{
	if (!HasAuthority() || !IsValid(Manager))
	{
		return;
	}
	ReplicatedManagers.AddUnique(Manager);
}

void AHeroBuilderGameState::ClearManagers()
{
	if (!HasAuthority())
	{
		return;
	}
	ReplicatedManagers.Reset();
}
