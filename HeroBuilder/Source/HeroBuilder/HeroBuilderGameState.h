// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HeroBuilderGameState.generated.h"

class AHB_Base_Manager;
class AHB_Base_Helper;

/**
 * 自定义 GameState：负责集中存放服务端创建的 Managers/Helpers。
 *
 * 设计要点：
 * 1. GameState 本身由引擎自动复制到所有客户端，是放置全局 Actor 引用的标准位置。
 * 2. 服务端在 GameMode::StartPlay 里调用 GameState->SpawnAllManagersAndHelpers()，
 *    由 GameState 内部通过反射扫描所有子类，统一在权威端 Spawn。
 * 3. ReplicatedManagers 复制后，客户端能通过 GetManager<T>() 拿到对应 Manager 实例
 *    （前提：Manager 自身也是 bReplicates=true 的 Actor）。
 * 4. Helpers 仅服务端使用（内部无需同步的属性与逻辑），因此：
 *    - AHB_Base_Helper 本身 bReplicates=false
 *    - HelperList 不参与复制，仅服务端本地数组
 *    - 客户端调用 GetHelper<T>() 会拿到 nullptr（设计上客户端不应使用 Helper）
 */
UCLASS()
class HEROBUILDER_API AHeroBuilderGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AHeroBuilderGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 仅服务端调用：通过反射扫描并 Spawn 出所有 Manager / Helper 子类，
	 *  Manager 注册到 ReplicatedManagers 后复制给客户端；
	 *  Helper 注册到 LocalHelpers 仅服务端本地使用。 */
	void SpawnAllManagersAndHelpers();

	/** 仅服务端调用：把刚创建出来的 Manager 注册进来，复制给客户端。 */
	void RegisterManager(AHB_Base_Manager* Manager);

	/** 仅服务端调用：把刚创建出来的 Helper 注册到本地数组（不复制到客户端）。 */
	void RegisterHelper(AHB_Base_Helper* Helper);

	/** 仅服务端调用：清空。 */
	void ClearManagers();
	void ClearHelpers();

	/** 客户端/服务端通用：按类型查找 Manager。 */
	template<typename T>
	T* GetManager() const
	{
		for (const TObjectPtr<AHB_Base_Manager>& Mgr : ReplicatedManagers)
		{
			if (T* Typed = Cast<T>(Mgr))
			{
				return Typed;
			}
		}
		return nullptr;
	}

	/** 仅服务端有效：按类型查找 Helper。
	 *  Helper 不参与网络同步，客户端调用会拿到 nullptr。 */
	template<typename T>
	T* GetHelper() const
	{
		for (const TObjectPtr<AHB_Base_Helper>& Hp : LocalHelpers)
		{
			if (T* Typed = Cast<T>(Hp))
			{
				return Typed;
			}
		}
		return nullptr;
	}

	const TArray<TObjectPtr<AHB_Base_Manager>>& GetAllManagers() const { return ReplicatedManagers; }
	const TArray<TObjectPtr<AHB_Base_Helper>>& GetAllHelpers() const { return LocalHelpers; }
	virtual void HandleBeginPlay() override;
private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AHB_Base_Manager>> ReplicatedManagers;

	/** Helper 没有需要同步的内容，仅服务端本地保留引用 */
	UPROPERTY()
	TArray<TObjectPtr<AHB_Base_Helper>> LocalHelpers;
};
