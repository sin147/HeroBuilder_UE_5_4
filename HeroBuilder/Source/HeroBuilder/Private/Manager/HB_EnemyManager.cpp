// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_EnemyManager.h"

AHB_EnemyManager::AHB_EnemyManager()
{

}

TArray<TObjectPtr<AHB_Enemy_Base>> AHB_EnemyManager::GetAllEnemies() const
{
    return Enemies;
}

void AHB_EnemyManager::AddEnemy(AHB_Enemy_Base* Enemy)
{
	Enemies.AddUnique(Enemy);
}

void AHB_EnemyManager::RemoveEnemy(AHB_Enemy_Base* Enemy)
{
	if(Enemies.Contains(Enemy))
	{
		Enemies.RemoveSingleSwap(Enemy);
	}
}

void AHB_EnemyManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AHB_EnemyManager, Enemies);
}
