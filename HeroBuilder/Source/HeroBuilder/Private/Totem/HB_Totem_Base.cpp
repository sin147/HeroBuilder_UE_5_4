// Fill out your copyright notice in the Description page of Project Settings.


#include "Totem/HB_Totem_Base.h"
#include "Subsystems/HB_TotemSubsystem.h"
#include "Subsystems/HB_EnemySubsystem.h"
#include "Enemy/HB_Enemy_Base.h"

// Sets default values
AHB_Totem_Base::AHB_Totem_Base()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

void AHB_Totem_Base::Active()
{
	//开启第一波
	StartWave(1);
}

void AHB_Totem_Base::StartWave(int32 WaveIndex)
{
	UHB_TotemSubsystem* TotemSubsystem = GetWorld()->GetSubsystem<UHB_TotemSubsystem>();
	if(TotemSubsystem)
	{
		FTotemConfig Config;
		if(TotemSubsystem->GetConfig(GetClass(), Config))
		{
			//获取当前波次配置
			FWaveConfig WaveConfig;
			if(Config.GetWaveConfig(WaveIndex, WaveConfig))
			{
				CurrentlyWaveDelay = WaveConfig.WaveDelay;
				TArray<FWaveEnemyConfig> EnemyConfigs = WaveConfig.EnemyConfigs;
				for (FWaveEnemyConfig WaveEnemyConfig : EnemyConfigs)
				{
					TSubclassOf<AHB_Enemy_Base> EnemyClass = WaveEnemyConfig.GetEnemyClass();
					int32 EnemyCount = WaveEnemyConfig.GetEnemyCount();
					float SpawnInterval = WaveEnemyConfig.GetSpawnInterval();
					FEnemySpawnInfo SpawnInfo(EnemyCount, SpawnInterval);
					CacheWaveEnemySpawnInfos.Add(EnemyClass, SpawnInfo);
				}
				//打开开关
				bIsActive = true;
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("TotemSubsystem is null in AHB_Totem_Base::StartWave"));
	}

}

void AHB_Totem_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(bIsActive)
	{
		//波次倒计时结束后，开始生成敌人
		if(CurrentlyWaveDelay <= 0)
		{

			//开始生成敌人
			for(auto& Elem : CacheWaveEnemySpawnInfos)
			{
				TSubclassOf<AHB_Enemy_Base> EnemyClass = Elem.Key;
				FEnemySpawnInfo& SpawnInfo = Elem.Value;

				if(SpawnInfo.CurrentlySpawnDelay <= 0 && SpawnInfo.CurrentlySpawnCount < SpawnInfo.TotalSpawnCount)
				{
					//生成敌人
					UHB_EnemySubsystem* EnemySubsystem = GetWorld()->GetSubsystem<UHB_EnemySubsystem>();
					if (EnemySubsystem)
					{
						EnemySubsystem->Spawn(EnemyClass, GetActorTransform());
						SpawnInfo.CurrentlySpawnCount++;
						SpawnInfo.CurrentlySpawnDelay = SpawnInfo.NeedSpawnDelay;
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("EnemySubsystem is null in AHB_Totem_Base::Tick"));
					}
				}
				else
				{
					SpawnInfo.CurrentlySpawnDelay -= DeltaTime;
				}
			}
		}
		else
		{
			//开始波次倒计时
			CurrentlyWaveDelay -= DeltaTime;
		}
		
	}
}

FEnemySpawnInfo::FEnemySpawnInfo()
{
}

FEnemySpawnInfo::FEnemySpawnInfo(int32 InTotalSpawnCount, float InNeedSpawnDelay)
{
	TotalSpawnCount = InTotalSpawnCount;
	NeedSpawnDelay = InNeedSpawnDelay;
}
