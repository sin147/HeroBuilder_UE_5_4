// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderGameMode.h"
#include "HeroBuilderCharacter.h"
#include "Systems/SubsystemBase.h"
#include "NetworkProxy/NetworkProxyBase.h"
#include "UObject/ConstructorHelpers.h"

AHeroBuilderGameMode::AHeroBuilderGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AHeroBuilderGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
    //生成网络代理
   TArray<USubsystemBase*> Subsystems = GetWorld()->GetSubsystemArray<USubsystemBase>();
   for (USubsystemBase* Subsystem : Subsystems)
   {
     Subsystem->CreateNetworkProxy(NewPlayer);
   }

}
