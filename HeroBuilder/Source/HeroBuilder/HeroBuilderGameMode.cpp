// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderGameMode.h"
#include "HeroBuilderCharacter.h"
#include "Manager/HB_Base_Manager.h"
#include "Helper/HB_Base_Helper.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogHeroBuilderGameMode)

AHeroBuilderGameMode::AHeroBuilderGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AHeroBuilderGameMode::StartPlay()
{
	Super::StartPlay();
	for (TObjectIterator<UClass> It; It; ++It)
	{
        UClass*CurrentClass = *It;
        if (CurrentClass->IsChildOf(AHB_Base_Manager::StaticClass()))
        {
            // TODO: Create manager instance
            AHB_Base_Manager* Manager = GetWorld()->SpawnActor<AHB_Base_Manager>(CurrentClass);
			if (IsValid(Manager))
			{
				Managers.Add(Manager);
				UE_LOG(LogHeroBuilderGameMode, Log, TEXT("Create manager %s"), *Manager->GetName());
			}
        }

        if (CurrentClass->IsChildOf(AHB_Base_Helper::StaticClass()))
        {
            // TODO: Create manager instance
            AHB_Base_Helper* Helper = GetWorld()->SpawnActor<AHB_Base_Helper>(CurrentClass);
            if (IsValid(Helper))
			{
                Helpers.Add(Helper);
                UE_LOG(LogHeroBuilderGameMode, Log, TEXT("Create helper %s"), *Helper->GetName());
			}
		}
	}
}
