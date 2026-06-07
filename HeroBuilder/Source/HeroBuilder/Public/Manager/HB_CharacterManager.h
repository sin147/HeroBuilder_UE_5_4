// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "HB_CharacterManager.generated.h"

class ACharacter;
class AActor;

//玩家角色状态枚举（从HeroBuilderCharacter迁移至此，便于Manager/Subsystem访问）
UENUM(BlueprintType)
enum EPlayerCharacterState :uint8
{
	EPCS_None UMETA(DisplayName = "无"),
	EPCS_Idle UMETA(DisplayName = "空闲"),
	EPCS_Move UMETA(DisplayName = "移动"),
	EPCS_MoveToTarget UMETA(DisplayName = "追击目标"),
	EPCS_PreInteract UMETA(DisplayName = "交互前摇"),
	EPCS_Interact UMETA(DisplayName = "交互"),
	EPCS_PostInteract UMETA(DisplayName = "交互后摇"),
};

//单条玩家角色状态数据（参与Replicate）
USTRUCT()
struct FCharacterStateEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
private:
	//前状态
	TEnumAsByte<EPlayerCharacterState> PreviousState = EPCS_Idle;
public:
	//所属角色（作为该条记录的Key）
	UPROPERTY()
	TObjectPtr<ACharacter> Character = nullptr;

	//当前状态
	UPROPERTY()
	TEnumAsByte<EPlayerCharacterState> CurrentlyState = EPCS_Idle;

	//交互前摇时间
	UPROPERTY()
	float PreInteractDelay = 0.3f;

	//交互后摇时间
	UPROPERTY()
	float PostInteractDelay = 0.3f;

	//攻击力
	UPROPERTY()
	float Attack = 10.f;

	//交互/攻击范围
	UPROPERTY()
	float InteractRange = 100.f;

	void PreReplicatedRemove(const FFastArraySerializer& ArraySerializer);
	void PostReplicatedAdd(const FFastArraySerializer& ArraySerializer);
	void PostReplicatedChange(const FFastArraySerializer& ArraySerializer);
};

class AHB_CharacterManager;

USTRUCT()
struct FCharacterStateContainer : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FCharacterStateEntry> CharacterStateEntries;

	//反向指针：仅本地使用，不参与复制；供FastArrayItem回调反查Manager→World→Subsystem
	TWeakObjectPtr<AHB_CharacterManager> OwnerManager;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& Parms)
	{
		return FastArrayDeltaSerialize<FCharacterStateEntry, FCharacterStateContainer>(CharacterStateEntries, Parms, *this);
	}
};
// 关键：告诉 UE 这个结构体走 NetDeltaSerializer
template<>
struct TStructOpsTypeTraits<FCharacterStateContainer> : public TStructOpsTypeTraitsBase2<FCharacterStateContainer>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

/**
 * 角色管理器（单例）
 * 服务端权威：以 Character 为 Key 维护所有玩家的角色状态数据；整张表复制给所有客户端。
 * 仅承担"数据维护 + 遍历"职责，逻辑由 UHB_CharacterSubsystem 负责。
 */
UCLASS()
class HEROBUILDER_API AHB_CharacterManager : public AHB_Base_Manager
{
	GENERATED_BODY()

public:
	AHB_CharacterManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	//—— 表项注册/移除 ——
	void RegisterCharacter(ACharacter* InCharacter);
	void RemoveEntry(ACharacter* InCharacter);

	//—— 同步属性访问（Replicated） ——
	EPlayerCharacterState GetCurrentlyState(ACharacter* InCharacter) const;
	void SetCurrentlyState(ACharacter* InCharacter, EPlayerCharacterState NewState);

	float GetAttack(ACharacter* InCharacter) const;
	void SetAttack(ACharacter* InCharacter, float NewAttack);

	float GetInteractRange(ACharacter* InCharacter) const;
	void SetInteractRange(ACharacter* InCharacter, float NewRange);

	//—— 遍历 ——
	const TArray<FCharacterStateEntry>& GetAllEntries() const { return CharacterStateContainer.CharacterStateEntries; }
	TArray<TObjectPtr<ACharacter>> GetAllCharacters() const;

private:
	//全玩家角色状态表（整张数组复制给所有客户端；UPROPERTY不支持复制TMap，故用TArray）
	UPROPERTY(Replicated)
	FCharacterStateContainer CharacterStateContainer;

	//查表辅助
	const FCharacterStateEntry* FindEntry(ACharacter* InCharacter) const;
	FCharacterStateEntry* FindEntryMutable(ACharacter* InCharacter);
	FCharacterStateEntry& FindOrAddEntry(ACharacter* InCharacter);
};
