// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Config/InteractData.h"
#include "HB_InteractManager.generated.h"

class ACharacter;

//EInteractManagerInteractMode 的定义已迁移至 Config/InteractData.h，避免循环依赖

//单条玩家交互数据
USTRUCT()
struct FInteractEntry
{
	GENERATED_BODY()
public:
	//所属角色（作为该条记录的Key）
	UPROPERTY()
	TObjectPtr<ACharacter> Character = nullptr;

	UPROPERTY()
	TEnumAsByte<EInteractType> InteractType = IT_Normal;

	UPROPERTY()
	TEnumAsByte<EInteractManagerInteractMode> InteractMode = IMIM_Normal;
};

/**
 * 交互管理器（单例）
 * 服务端权威：以 Character 为 Key 维护所有玩家的交互类型/交互模式；整张表复制给所有客户端。
 */
UCLASS()
class HEROBUILDER_API AHB_InteractManager : public AHB_Base_Manager
{
	GENERATED_BODY()

public:
	AHB_InteractManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//—— 交互类型（服务端权威） ——
	EInteractType GetCurrentInteractType(ACharacter* InCharacter) const;
	void SetCurrentInteractType(ACharacter* InCharacter, EInteractType NewType);

	//—— 交互模式（建造/正常） ——
	EInteractManagerInteractMode GetCurrentInteractMode(ACharacter* InCharacter) const;
	void SetCurrentInteractMode(ACharacter* InCharacter, EInteractManagerInteractMode NewMode);

	//玩家登出时清理表项（仅服务端调用）
	void RemoveEntry(ACharacter* InCharacter);

private:
	//全玩家交互数据表（整张数组复制给所有客户端；UPROPERTY不支持复制TMap，故用TArray）
	UPROPERTY(ReplicatedUsing = OnRep_CharacterInteractArray)
	TArray<FInteractEntry> CharacterInteractArray;

	//本地缓存：客户端上一次同步收到的“自己那一项”，用于OnRep时对比触发回调
	FInteractEntry LastLocalEntry;
	bool bHasLastLocalEntry = false;

	UFUNCTION()
	void OnRep_CharacterInteractArray();

	//客户端：尝试找到“自己的Character”（自治代理 / 监听服务器上的本地玩家）
	ACharacter* FindLocalCharacter() const;
};