// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_WaveSubsystem.h"
#include "Enemy/HB_Enemy_Base.h"

DEFINE_LOG_CATEGORY(LogWaveSubsystem);

void UHB_WaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
    UE_LOG(LogWaveSubsystem, Log, TEXT("HB_WaveSubsystem Initialize"));
    WaveData = LoadObject<UWaveData>(this, TEXT("/Game/Config/DA_WaveConfig"));
    
    if (!IsValid(WaveData))
    {
        UE_LOG(LogWaveSubsystem, Error, TEXT("Failed to load WaveData asset from /Game/Config/DA_WaveConfig"));
        UE_LOG(LogWaveSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
    }
    else
    {
        UE_LOG(LogWaveSubsystem, Log, TEXT("Successfully loaded WaveData asset"));
    }
}

void UHB_WaveSubsystem::WaveTick(float DeltaTime)
{
	ENetMode NetMode = GetWorld()->GetNetMode();
	if(NetMode != NM_DedicatedServer&&NetMode != NM_ListenServer&&NetMode != NM_Standalone)
	{
		return;
	}
	switch (WaveState)
	{
	case WS_Preparatory:
	{
		if (RemainingPreparatoryTime<0)
		{
			WaveState = WS_Fight;
			UE_LOG(LogWaveSubsystem, Log, TEXT("Enter FightState"));
		}
		else
		{
			RemainingPreparatoryTime -= DeltaTime;
		}
	break;
	}
    case WS_Fight:
	{
		bool IsSpawnOver=true;
		CurrentlyFightTime += DeltaTime;
        for (int i=0;i< CurrentlyWaveEnemyConfigs.Num();i++  )
		{
			
			FWaveEnemyConfig& EnemyConfig = CurrentlyWaveEnemyConfigs[i];
			if (EnemyConfig.EnemyCount <= 0)
			{
				continue;
			}
			else
			{
				IsSpawnOver = false;
			}
			if (CurrentlyFightTime>=EnemyConfig.SpawnInterval)
			{
	
				if(SpawnPoints.Num() != 0)
				{
					GetWorld()->SpawnActor<AHB_Enemy_Base>(EnemyConfig.EnemyClass, FTransform(FRotator(0, 0, 0), FVector(SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)]), FVector(1, 1, 1)));

				}
				UE_LOG(LogWaveSubsystem, Log, TEXT("Spawn Enemy %s"), *EnemyConfig.EnemyClass->GetName());
				EnemyConfig.EnemyCount -= 1;
				EnemyConfig.SpawnInterval += CurrentlyWaveConfig.EnemyConfigs[i].SpawnInterval;
			}
		}
		if (IsSpawnOver)
		{
				WaveState = WS_End;
				UE_LOG(LogWaveSubsystem, Log, TEXT("Enter EndState"));
		}
		break;
	}
	case WS_End:
		if (bAutoNextWave)
		{
			ActiveWaveByIndex(CurrentlyWaveIndex + 1, bAutoNextWave);
			UE_LOG(LogWaveSubsystem, Log, TEXT("Enter PreparatoryState"));
		}
		else
		{
			WaveState = WS_Stop;
			UE_LOG(LogWaveSubsystem, Log, TEXT("Enter StopState"));
		}
		break;
	case WS_Stop:
		break;
	default:
		break;
	}
	
}

void UHB_WaveSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	WaveTick(DeltaTime);
}

void UHB_WaveSubsystem::ActiveWaveByIndex(int32 InWaveIndex,bool AutoNextWave)
{
	if (!IsValid(WaveData))
	{
		UE_LOG(LogWaveSubsystem, Error, TEXT("WaveData Not Loaded"));
		WaveState = WS_Stop;
		return;
	}
	if (InWaveIndex >= WaveData->WaveConfigs.Num())
	{
		UE_LOG(LogWaveSubsystem, Error, TEXT("InWaveIndex is out of range"));
		return;
	}


	CurrentlyWaveIndex = InWaveIndex;
	CurrentlyWaveConfig = WaveData->GetWaveConfigByWaveIndex(InWaveIndex);
	RemainingPreparatoryTime = CurrentlyWaveConfig.WaveInterval;
	CurrentlyWaveEnemyConfigs = CurrentlyWaveConfig.EnemyConfigs;
	CurrentlyFightTime = 0;
	bAutoNextWave = AutoNextWave;
	WaveState = WS_Preparatory;
}

void UHB_WaveSubsystem::SkipPreparatory()
{
	if (WaveState == WS_Preparatory)
	{
		WaveState = WS_Fight;
	}
	else
	{
		UE_LOG(LogWaveSubsystem, Error, TEXT("WaveState is not WS_Preparatory Cannot SkipPreparatory"));
	}

}
