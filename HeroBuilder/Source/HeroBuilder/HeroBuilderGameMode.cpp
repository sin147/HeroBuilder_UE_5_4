// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderGameMode.h"
#include "HeroBuilderCharacter.h"
#include "HeroBuilderGameState.h"
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

	// 指定使用自定义 GameState：把 Manager 数组复制到所有客户端
	GameStateClass = AHeroBuilderGameState::StaticClass();
}

void AHeroBuilderGameMode::StartPlay()
{
	Super::StartPlay();

	AHeroBuilderGameState* HBGS = GetGameState<AHeroBuilderGameState>();

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* CurrentClass = *It;

		// 过滤无效类：抽象类、废弃类、编辑器中间类
		if (!CurrentClass || CurrentClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}
		const FString ClassName = CurrentClass->GetName();
		if (ClassName.StartsWith(TEXT("SKEL_")) || ClassName.StartsWith(TEXT("REINST_")) || ClassName.StartsWith(TEXT("TRASHCLASS_")))
		{
			continue;
		}

		if (CurrentClass->IsChildOf(AHB_Base_Manager::StaticClass()) && CurrentClass != AHB_Base_Manager::StaticClass())
		{
			AHB_Base_Manager* Manager = GetWorld()->SpawnActor<AHB_Base_Manager>(CurrentClass);
			if (IsValid(Manager))
			{
				Managers.Add(Manager);
				// 注册到 GameState，使 Manager 引用复制到所有客户端
				if (HBGS)
				{
					HBGS->RegisterManager(Manager);
				}
				UE_LOG(LogHeroBuilderGameMode, Log, TEXT("Create manager %s"), *Manager->GetName());
			}
		}

		if (CurrentClass->IsChildOf(AHB_Base_Helper::StaticClass()) && CurrentClass != AHB_Base_Helper::StaticClass())
		{
			AHB_Base_Helper* Helper = GetWorld()->SpawnActor<AHB_Base_Helper>(CurrentClass);
			if (IsValid(Helper))
			{
				Helpers.Add(Helper);
				UE_LOG(LogHeroBuilderGameMode, Log, TEXT("Create helper %s"), *Helper->GetName());
			}
		}
	}
}
