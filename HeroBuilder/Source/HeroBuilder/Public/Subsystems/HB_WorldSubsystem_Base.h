// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/World.h"
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

	/**
	 * 服务端/客户端通用：通过 GameState 获取已复制的 Manager。
	 * Manager 的存储已统一迁移到 GameState（ReplicatedManagers）。
	 */
	template <typename T>
	T* GetManager()
	{
		UWorld* World = this->GetWorld();
		if (!World)
		{
			return nullptr;
		}
		if (AHeroBuilderGameState* GS = World->GetGameState<AHeroBuilderGameState>())
		{
			return GS->GetManager<T>();
		}
		return nullptr;
	}

	/**
	 * 仅服务端有效：通过 GameState 获取 Helper。
	 * Helper 内部不需要网络同步，仅在服务端使用；
	 * 客户端调用会拿到 nullptr（GameState 上的 Helper 列表不参与复制）。
	 */
	template <typename T>
	T* GetHelper()
	{
		UWorld* World = this->GetWorld();
		if (!World)
		{
			return nullptr;
		}
		if (AHeroBuilderGameState* GS = World->GetGameState<AHeroBuilderGameState>())
		{
			return GS->GetHelper<T>();
		}
		return nullptr;
	}

};
