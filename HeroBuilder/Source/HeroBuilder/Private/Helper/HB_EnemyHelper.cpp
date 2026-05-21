// Fill out your copyright notice in the Description page of Project Settings.


#include "Helper/HB_EnemyHelper.h"
#include "Enemy/HB_Enemy_Base.h"

bool AHB_EnemyHelper::IsValidEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy)
{
	return IsValid(InEnemy)&&!InEnemy->IsDeath();
}
