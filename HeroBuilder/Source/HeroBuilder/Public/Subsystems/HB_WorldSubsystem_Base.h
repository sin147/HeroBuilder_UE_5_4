// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "../HeroBuilderGameMode.h"
#include "HB_WorldSubsystem_Base.generated.h"

/**
 * 
 */
UCLASS(Abstract, MinimalAPI)
class UHB_WorldSubsystem_Base : public UTickableWorldSubsystem
{
	GENERATED_BODY()
protected:
	ENetMode NetMode;
	virtual void OnPlayerLogin(AGameModeBase* GameMode, APlayerController* PlayerController);
	virtual void OnPlayerLogout(AGameModeBase* GameMode, AController* Exiting);
public:
	// USubsystem implementation Begin
	 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	 virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	template <typename T>
	T* GetManager()
	{
		AHeroBuilderGameMode* GM = Cast<AHeroBuilderGameMode>(UGameplayStatics::GetGameMode(this->GetWorld()));
		if (!GM)
		{
			return nullptr;
		}
		return GM->GetManager<T>();
	}
	template <typename T>
	T* GetHelper()
	{
		AHeroBuilderGameMode* GM = Cast<AHeroBuilderGameMode>(UGameplayStatics::GetGameMode(this->GetWorld()));
		if (!GM)
		{
			return nullptr;
		}
		return GM->GetHelper<T>();
	}

};
