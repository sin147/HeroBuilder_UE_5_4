// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_WaveSubsystem.h"

void UHB_WaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("HB_WaveSubsystem Initialize"));
	WaveData = LoadObject<UWaveData>(this, TEXT("/Game/Config/DA_WaveConfig.DA_WaveConfig"));
}

void UHB_WaveSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
