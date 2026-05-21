// Fill out your copyright notice in the Description page of Project Settings.

#include "Config/EnemyData.h"

FEnemyConfig UEnemyData::GetEnemyInfoByEnemyClass(TSubclassOf<AHB_Enemy_Base> EnemyClass)
{
	if (EnemyInfoMap.Contains(EnemyClass))
	{
		return EnemyInfoMap[EnemyClass];
	}
	
	// 返回默认配置
	FEnemyConfig DefaultConfig;
	return DefaultConfig;
}

