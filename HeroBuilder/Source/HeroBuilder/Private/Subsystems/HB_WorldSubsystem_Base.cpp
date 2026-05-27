// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_WorldSubsystem_Base.h"


void UHB_WorldSubsystem_Base::OnPlayerLogin(AGameModeBase* GameMode, APlayerController* PlayerController)
{
    // TODO: Implement player login logic
}

void UHB_WorldSubsystem_Base::OnPlayerLogout(AGameModeBase* GameMode, AController* Exiting)
{
    // TODO: Implement player logout logic
}

void UHB_WorldSubsystem_Base::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	NetMode = GetWorld()->GetNetMode();
	if(NetMode != ENetMode::NM_Client)
	{
		FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UHB_WorldSubsystem_Base::OnPlayerLogin);
		FGameModeEvents::GameModeLogoutEvent.AddUObject(this, &UHB_WorldSubsystem_Base::OnPlayerLogout);
	}

}

void UHB_WorldSubsystem_Base::Deinitialize()
{
	Super::Deinitialize();
	if(NetMode != ENetMode::NM_Client)
	{
		FGameModeEvents::GameModePostLoginEvent.RemoveAll(this);
		FGameModeEvents::GameModeLogoutEvent.RemoveAll(this);
    }
}

void UHB_WorldSubsystem_Base::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

TStatId UHB_WorldSubsystem_Base::GetStatId() const
{
	return TStatId();
}
