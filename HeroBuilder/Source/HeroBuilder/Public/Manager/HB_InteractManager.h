// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Config/InteractData.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "HB_InteractManager.generated.h"

class ACharacter;
class AActor;

//EInteractManagerInteractMode 的定义已迁移至 Config/InteractData.h，避免循环依赖

//单条玩家交互数据（参与FastArray差量复制）
USTRUCT()
struct FInteractEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
private:
	//本地快照：仅本地使用、不参与复制；客户端PostReplicatedAdd/Change中维护，便于差量比较
	TEnumAsByte<EInteractType> PreviousInteractType = IT_Normal;
	TEnumAsByte<EInteractMode> PreviousInteractMode = IM_Normal;
	TWeakObjectPtr<AActor>     PreviousInteractTarget = nullptr;
public:
	//所属角色（作为该条记录的Key）
	UPROPERTY()
	TObjectPtr<ACharacter> Character = nullptr;

	UPROPERTY()
	TEnumAsByte<EInteractType> InteractType = IT_Normal;

	UPROPERTY()
	TEnumAsByte<EInteractMode> InteractMode = IM_Normal;

	//当前最近的交互目标（由InteractSubsystem周期性更新）
	UPROPERTY()
	TObjectPtr<AActor> InteractTarget = nullptr;

	//—— FastArray 客户端回调 ——
	void PreReplicatedRemove(const FFastArraySerializer& ArraySerializer);
	void PostReplicatedAdd(const FFastArraySerializer& ArraySerializer);
	void PostReplicatedChange(const FFastArraySerializer& ArraySerializer);
};

class AHB_InteractManager;

USTRUCT()
struct FInteractContainer : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FInteractEntry> InteractEntries;

	//反向指针：仅本地使用，不参与复制；供FastArrayItem回调反查Manager→World→Subsystem
	TWeakObjectPtr<AHB_InteractManager> OwnerManager;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& Parms)
	{
		return FastArrayDeltaSerialize<FInteractEntry, FInteractContainer>(InteractEntries, Parms, *this);
	}
};
//关键：告诉 UE 这个结构体走 NetDeltaSerializer
template<>
struct TStructOpsTypeTraits<FInteractContainer> : public TStructOpsTypeTraitsBase2<FInteractContainer>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

/**
 * 交互管理器（单例）
 * 服务端权威：以 Character 为 Key 维护所有玩家的交互类型/交互模式；整张表通过FastArray差量复制给所有客户端。
 */
UCLASS()
class HEROBUILDER_API AHB_InteractManager : public AHB_Base_Manager
{
	GENERATED_BODY()

public:
	AHB_InteractManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	//—— 交互类型（服务端权威） ——
	EInteractType GetCurrentInteractType(ACharacter* InCharacter) const;
	void SetCurrentInteractType(ACharacter* InCharacter, EInteractType NewType);

	//—— 交互模式（建造/正常） ——
	EInteractMode GetCurrentInteractMode(ACharacter* InCharacter) const;
	void SetCurrentInteractMode(ACharacter* InCharacter, EInteractMode NewMode);

	//—— 交互目标（服务端权威） ——
	AActor* GetInteractTarget(ACharacter* InCharacter) const;
	void SetInteractTarget(ACharacter* InCharacter, AActor* Target);

	//玩家登出时清理表项（仅服务端调用）
	void RemoveEntry(ACharacter* InCharacter);

private:
	//全玩家交互数据表（FastArray差量复制给所有客户端）
	UPROPERTY(Replicated)
	FInteractContainer CharacterInteractContainer;

	//查表辅助
	const FInteractEntry* FindEntry(ACharacter* InCharacter) const;
	FInteractEntry* FindEntryMutable(ACharacter* InCharacter);
	FInteractEntry& FindOrAddEntry(ACharacter* InCharacter);
};