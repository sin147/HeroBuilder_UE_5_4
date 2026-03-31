// Fill out your copyright notice in the Description page of Project Settings.


#include "DataConfig/EnemyConfigData.h"

FEnemyConfig UEnemyConfigData::GetEnemyConfig(TSubclassOf<AEnemyBase> EnemyClass)
{
	if (EnemyConfigs.Contains(EnemyClass))
	{
		return EnemyConfigs[EnemyClass];
	}
    return FEnemyConfig();
}
