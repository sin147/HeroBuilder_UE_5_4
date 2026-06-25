// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderGameMode.h"
#include "HeroBuilderCharacter.h"
#include "HeroBuilderGameState.h"
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

	// 指定使用自定义 GameState：把 Manager/Helper 数组复制到所有客户端
	GameStateClass = AHeroBuilderGameState::StaticClass();
}

void AHeroBuilderGameMode::StartPlay()
{
	Super::StartPlay();

	// Manager / Helper 的扫描与 Spawn 已统一移交给 GameState 完成
	if (AHeroBuilderGameState* HBGS = GetGameState<AHeroBuilderGameState>())
	{
		HBGS->SpawnAllManagersAndHelpers();
	}
	else
	{
		UE_LOG(LogHeroBuilderGameMode, Error, TEXT("StartPlay: HeroBuilderGameState is null, fail to spawn managers/helpers"));
	}
}