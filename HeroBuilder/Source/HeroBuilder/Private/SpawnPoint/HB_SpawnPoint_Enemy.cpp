// Fill out your copyright notice in the Description page of Project Settings.


#include "SpawnPoint/HB_SpawnPoint_Enemy.h"
#include "Subsystems/HB_WaveSubsystem.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Enemy/HB_Enemy_Base.h"

void AHB_SpawnPoint_Enemy::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UHB_WaveSubsystem>()->AddSpawnPoint(this);
}

void AHB_SpawnPoint_Enemy::Spawn(TSubclassOf<AActor> InActorClass)
{
	GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->SpawnEnemy(InActorClass.Get(), FTransform(this->GetActorRotation(), this->GetActorLocation()));
}
