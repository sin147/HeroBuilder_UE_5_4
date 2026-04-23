// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnPoint/HB_SpawnPoint_Enemy.h"
#include "Subsystems/HB_WaveSubsystem.h"

void AHB_SpawnPoint_Enemy::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UHB_WaveSubsystem>()->AddSpawnPoint(this);
}
