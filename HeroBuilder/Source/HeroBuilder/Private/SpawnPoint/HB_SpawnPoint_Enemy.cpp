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

void AHB_SpawnPoint_Enemy::OnSpawn(AActor* SpawnActor)
{
	AHB_Enemy_Base* NewEnemy = Cast<AHB_Enemy_Base>(SpawnActor);
	if (NewEnemy)
	{
		GetWorld()->GetSubsystem<UHB_EnemySubsystem>()->PaddingEnemy(NewEnemy);
	}
}
