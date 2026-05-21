// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_WorldSubsystem_Base.h"


void UHB_WorldSubsystem_Base::PostInitialize()
{
	Super::PostInitialize();
	NetMode = GetWorld()->GetNetMode();
}

void UHB_WorldSubsystem_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId UHB_WorldSubsystem_Base::GetStatId() const
{
	return TStatId();
}
