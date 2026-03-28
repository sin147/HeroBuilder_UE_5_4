// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HeroBuilderGameMode.generated.h"

UCLASS(minimalapi)
class AHeroBuilderGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHeroBuilderGameMode();
	virtual void PostLogin(APlayerController* NewPlayer) override;
};



