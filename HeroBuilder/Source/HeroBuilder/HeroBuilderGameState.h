// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "HeroBuilderGameState.generated.h"

class AHB_Base_Manager;

/**
 * 自定义 GameState：负责把服务端创建的 Managers 复制到所有客户端。
 *
 * 设计要点：
 * 1. GameState 本身由引擎自动复制到所有客户端，是放置全局 Actor 引用的标准位置。
 * 2. 服务端在 GameMode::StartPlay 创建完 Manager 后，调用 RegisterManager 注册到 GameState。
 * 3. ReplicatedManagers 数组复制后，客户端就能通过 GetManager<T>() 拿到对应实例。
 *    （前提：Manager 自身也是 bReplicates=true 的 Actor —— 已在 AHB_Base_Manager 默认开启）
 */
UCLASS()
class HEROBUILDER_API AHeroBuilderGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AHeroBuilderGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 仅服务端调用：把刚创建出来的 Manager 注册进来，复制给客户端。 */
	void RegisterManager(AHB_Base_Manager* Manager);

	/** 仅服务端调用：清空。 */
	void ClearManagers();

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

	const TArray<TObjectPtr<AHB_Base_Manager>>& GetAllManagers() const { return ReplicatedManagers; }

private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AHB_Base_Manager>> ReplicatedManagers;
};
