// Fill out your copyright notice in the Description page of Project Settings.


#include "DataConfig/WaveConfig.h"

FWaveSetting UWaveConfig::GetWaveConfigByIndex(int32 Index)
{
	if(WaveConfigs.Contains(Index))
	{
		return WaveConfigs[Index];
	}
    return FWaveSetting();
}
