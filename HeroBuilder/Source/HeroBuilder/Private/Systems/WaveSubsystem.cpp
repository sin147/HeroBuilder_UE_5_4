// Fill out your copyright notice in the Description page of Project Settings.


#include "Systems/WaveSubsystem.h"

void UWaveSubsystem::ApplyWaveByIndex(int32 Index)
{
	CurrentlyWaveIndex = Index;
	FWaveSetting WaveSetting = WaveConfig->GetWaveConfigByIndex(Index);
    //当前休整时间
	CurrentNeedResetTime = WaveSetting.ResetInterval;
	CurrentResetTime = 0;
	EnemyNeedSpawnCountMap.Empty();
	EnemyCountMap.Empty();
	EnemySpawnIntervalMap.Empty();
	for (const auto& EnemySetting : WaveSetting.EnemySettings)
	{
		EnemyNeedSpawnCountMap.Add(EnemySetting.Key, EnemySetting.Value.EnemyCount);
		EnemyNeedSpawnIntervalMap.Add(EnemySetting.Key, EnemySetting.Value.SpawnInterval);
		EnemyCountMap.Add(EnemySetting.Key, 0);
		EnemySpawnIntervalMap.Add(EnemySetting.Key, 0);
	}
	UKismetSystemLibrary::PrintString(GetWorld(), FString::Printf(TEXT("Apply Wave %d"), Index),true,true,FLinearColor::Red,10);
}

void UWaveSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	switch (CurrentWaveType)
	{
	case EWaveState::None:
		break;
	case EWaveState::Reset:
	{	
		CurrentResetTime += DeltaTime;
		UE_LOG(LogTemp, Log, TEXT("ResetTime %lf"), CurrentResetTime);
		if (CurrentResetTime >= CurrentNeedResetTime)
		{
			CurrentWaveType = EWaveState::Combat;
			CurrentResetTime = 0;
		}
		break;
	}
	case EWaveState::Combat:
	{
		CurrentCombatTime += DeltaTime;
		UE_LOG(LogTemp, Log, TEXT("CombattTime %lf"), CurrentCombatTime);
		int32 SpawnOverEnemy = 0;
		for (TPair<TSubclassOf<AActor>, float> EnemySpawnIntervalPair : EnemySpawnIntervalMap)
		{

			TSubclassOf<AActor> EnemyClass = EnemySpawnIntervalPair.Key;
			if (EnemyNeedSpawnCountMap[EnemyClass] > 0)
			{
				EnemySpawnIntervalMap[EnemyClass] += DeltaTime;
				if (EnemySpawnIntervalMap[EnemyClass] >= EnemyNeedSpawnIntervalMap[EnemyClass])
				{
					EnemySpawnIntervalMap[EnemyClass] = 0;
					EnemyNeedSpawnCountMap[EnemyClass]--;
					EnemyCountMap[EnemyClass]++;
					//接敌人系统的生成接口TODO
					UE_LOG(LogTemp, Log, TEXT("Spawn************************************"));
				}
			}
			else
			{
				SpawnOverEnemy++;
			}

		}
		if (SpawnOverEnemy == EnemyNeedSpawnCountMap.Num())
		{
			CurrentWaveType = EWaveState::Reset;
			CurrentCombatTime = 0;
			ApplyWaveByIndex(++CurrentlyWaveIndex);
		}
		break;
	}
	default:
		break;
	}
}

void UWaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WaveConfig = LoadObject<UWaveConfig>(nullptr, TEXT("/Game/Data/WaveConfig.WaveConfig"));
	if(!WaveConfig)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::FromString("Failed to load WaveConfig"),
			nullptr
		);
	}
}

void UWaveSubsystem::ActiveWave()
{
	CurrentWaveType = EWaveState::Reset;
	ApplyWaveByIndex(1);
}
