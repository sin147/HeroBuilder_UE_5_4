// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
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
struct FCharacterStateEntry
{
	GENERATED_BODY()
public:
	//所属角色（作为该条记录的Key）
	UPROPERTY()
	TObjectPtr<ACharacter> Character = nullptr;

	//当前状态
	UPROPERTY()
	TEnumAsByte<EPlayerCharacterState> CurrentlyState = EPCS_Idle;

	//当前交互目标
	UPROPERTY()
	TObjectPtr<AActor> InteractTarget = nullptr;

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

	//—— 表项注册/移除 ——
	void RegisterCharacter(ACharacter* InCharacter);
	void RemoveEntry(ACharacter* InCharacter);

	//—— 同步属性访问（Replicated） ——
	EPlayerCharacterState GetCurrentlyState(ACharacter* InCharacter) const;
	void SetCurrentlyState(ACharacter* InCharacter, EPlayerCharacterState NewState);

	AActor* GetInteractTarget(ACharacter* InCharacter) const;
	void SetInteractTarget(ACharacter* InCharacter, AActor* Target);

	float GetPreInteractDelay(ACharacter* InCharacter) const;
	void SetPreInteractDelay(ACharacter* InCharacter, float Delay);

	float GetPostInteractDelay(ACharacter* InCharacter) const;
	void SetPostInteractDelay(ACharacter* InCharacter, float Delay);

	float GetAttack(ACharacter* InCharacter) const;
	void SetAttack(ACharacter* InCharacter, float NewAttack);

	float GetInteractRange(ACharacter* InCharacter) const;
	void SetInteractRange(ACharacter* InCharacter, float NewRange);

	//—— 非同步运行期数据（仅服务端/客户端本地使用） ——
	float GetCurrentInteractDelay(ACharacter* InCharacter) const;
	void SetCurrentInteractDelay(ACharacter* InCharacter, float NewDelay);

	bool GetInternalDrivenMove(ACharacter* InCharacter) const;
	void SetInternalDrivenMove(ACharacter* InCharacter, bool bDriven);

	//—— 遍历 ——
	const TArray<FCharacterStateEntry>& GetAllEntries() const { return CharacterStateArray; }

private:
	//全玩家角色状态表（整张数组复制给所有客户端；UPROPERTY不支持复制TMap，故用TArray）
	UPROPERTY(Replicated)
	TArray<FCharacterStateEntry> CharacterStateArray;

	//—— 非同步的每角色运行期数据 ——
	//交互前/后摇倒计时（仅服务端使用）
	TMap<TWeakObjectPtr<ACharacter>, float> CurrentInteractDelayMap;
	//内部追击驱动标记（仅自治代理客户端使用）
	TMap<TWeakObjectPtr<ACharacter>, bool> InternalDrivenMoveMap;

	//查表辅助
	const FCharacterStateEntry* FindEntry(ACharacter* InCharacter) const;
	FCharacterStateEntry* FindEntryMutable(ACharacter* InCharacter);
	FCharacterStateEntry& FindOrAddEntry(ACharacter* InCharacter);
};
