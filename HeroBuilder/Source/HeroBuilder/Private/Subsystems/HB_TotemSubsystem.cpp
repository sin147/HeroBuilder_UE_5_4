// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_TotemSubsystem.h"
#include "Enemy/HB_Enemy_Base.h"
#include "Subsystems/HB_EnemySubsystem.h"

DEFINE_LOG_CATEGORY(LogTotemSubsystem);

void UHB_TotemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
    UE_LOG(LogTotemSubsystem, Log, TEXT("HB_TotemSubsystem Initialize"));
    TotemData = LoadObject<UTotemData>(this, TEXT("/Game/Config/DA_TotemConfig"));
    
    if (!IsValid(TotemData))
    {
        UE_LOG(LogTotemSubsystem, Error, TEXT("Failed to load TotemData asset from /Game/Config/DA_TotemConfig"));
        UE_LOG(LogTotemSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
    }
    else
    {
        UE_LOG(LogTotemSubsystem, Log, TEXT("Successfully loaded TotemData asset"));
    }
}

void UHB_TotemSubsystem::TotemTick(float DeltaTime)
{

}

void UHB_TotemSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TotemTick(DeltaTime);
}

void UHB_TotemSubsystem::ActiveTotem(AHB_Totem_Base* InTotem)
{
}

void UHB_TotemSubsystem::SkipPreparatory(AHB_Totem_Base* InTotem)
{


}

void UHB_TotemSubsystem::SpawnTotem(TSubclassOf<AHB_Totem_Base> InTotem, FTransform SpawnTransform)
{
	AHB_Totem_Base* NewTotem=GetWorld()->SpawnActor<AHB_Totem_Base>(InTotem, SpawnTransform);
    if (NewTotem)
    {

    }
    else
    {

    }
}

bool UHB_TotemSubsystem::GetConfig(TSubclassOf<AHB_Totem_Base> TotemClass, FTotemConfig& OutConfig) const
{
	if (TotemData)
	{
		FTotemConfig Config;
		if (TotemData->GetTotemConfig(TotemClass, Config))
		{
			OutConfig = Config;
			return true;
		}
		else
		{
			UE_LOG(LogTotemSubsystem, Warning, TEXT("No config found for Totem class: %s"), *TotemClass->GetName());
		}
	}
	else
	{
		UE_LOG(LogTotemSubsystem, Error, TEXT("TotemData is not valid."));
	}
	return false; // Return a default config if not found
}
bool UHB_TotemSubsystem::GetWaveConfig(TSubclassOf<AHB_Totem_Base> TotemClass, int32 WaveIndex, FWaveConfig& OutWaveConfig) const
{
	if (TotemData)
	{
		FTotemConfig Config;
		if (TotemData->GetTotemConfig(TotemClass, Config))
		{
			if (Config.GetWaveConfig(WaveIndex, OutWaveConfig))
			{
				return true;
			}
		}
	}
	return false;
}

