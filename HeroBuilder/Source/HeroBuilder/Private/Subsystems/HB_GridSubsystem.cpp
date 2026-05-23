// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_GridSubsystem.h"

DEFINE_LOG_CATEGORY(LogGridSubsystem);

void UHB_GridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GridData = LoadObject<UGridData>(this, TEXT("/Game/Config/DA_GridConfig"));

	if (!IsValid(GridData))
	{
        UE_LOG(LogGridSubsystem, Error, TEXT("Failed to load GridData asset from /Game/Config/DA_GridConfig"));
		UE_LOG(LogGridSubsystem, Error, TEXT("Please check if the asset exists in Content/Config/ folder"));
	}
	else
	{
        UE_LOG(LogGridSubsystem, Log, TEXT("Successfully loaded GridData asset"));
	}
}
int32 UHB_GridSubsystem::GetGridWidth() const
{
    int32 Ret = 0;
	if (GridData)
	{
        Ret = GridData->GetGridWidth();
    }
	else
	{
		UE_LOG(LogGridSubsystem, Error, TEXT("GridData is null"));
	}
    return Ret;
}

int32 UHB_GridSubsystem::GetGridHeight() const
{
    int32 Ret = 0;
    if (GridData)
    {
        Ret = GridData->GetGridHeight();
    }
    else
    {
        UE_LOG(LogGridSubsystem, Error, TEXT("GridData is null"));
    }
    return Ret;
}