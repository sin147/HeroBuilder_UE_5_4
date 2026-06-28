// Fill out your copyright notice in the Description page of Project Settings.


#include "Config/TotemData.h"
#include "Totem/HB_Totem_Base.h"

bool UTotemData::GetTotemConfig(TSubclassOf<AHB_Totem_Base> TotemClass, FTotemConfig& OutConfig) const
{
    const FTotemConfig* Found = TotemConfigs.Find(TotemClass);
    if (Found)
    {
        OutConfig = *Found;
        return true;
    }
    UE_LOG(LogTemp, Error, TEXT("TotemConfig not found for TotemIndex %s"), *TotemClass->ClassConfigName.ToString());
    return false;
}

bool FTotemConfig::GetWaveConfig(int32 WaveIndex, FWaveConfig& OutWaveConfig) const
{
    if (WaveConfigs.Contains(WaveIndex))
    {
        OutWaveConfig = WaveConfigs[WaveIndex];
        return true;
    }
	UE_LOG(LogTemp, Error, TEXT("WaveConfig not found for WaveIndex %d"), WaveIndex);
    return false;
}
