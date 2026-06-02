// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "../HeroBuilderGameMode.h"
#include "../HeroBuilderGameState.h"
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
		UWorld* World = this->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		// 服务端：走 GameMode 的快路径（保持原行为）
		if (AHeroBuilderGameMode* GM = Cast<AHeroBuilderGameMode>(UGameplayStatics::GetGameMode(World)))
		{
			if (T* Mgr = GM->GetManager<T>())
			{
				return Mgr;
			}
		}

		// 客户端：GameMode 在客户端为 null，从 GameState 读取已复制过来的 Manager 列表
		if (AHeroBuilderGameState* GS = World->GetGameState<AHeroBuilderGameState>())
		{
			return GS->GetManager<T>();
		}

		return nullptr;
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
