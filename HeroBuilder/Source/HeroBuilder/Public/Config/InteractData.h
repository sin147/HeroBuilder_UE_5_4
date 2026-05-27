// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InteractData.generated.h"
UENUM(BlueprintType)
enum EPlayerCharacterInteractMode :uint8
{
	IM_None,
	IM_Normal,
	IM_ConstructionMode,
	IM_LumberMode,   //砍伐模式
	IM_GatherMode,   //采集模式
	IM_MineMode,     //挖掘模式
	IM_AttackMode,   //攻击模式
};

USTRUCT(BlueprintType)
struct FInteractInfo
{
	GENERATED_BODY()
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> InteractAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float InteractDistance;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float PreInteractDelay;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float PostInteractDelay;
};

/**
 * 
 */
UCLASS()
class HEROBUILDER_API UInteractData : public UDataAsset
{
	GENERATED_BODY()
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"),EditFixedSize)
	TMap<TEnumAsByte<EPlayerCharacterInteractMode>, FInteractInfo> InteractAnimationsMap;

	//遍历EPlayerCharacterInteractMode所有枚举值，将缺失的key补齐到InteractAnimationsMap
	void InitializeInteractMap();
public:
	//新建/构造时初始化Map
	virtual void PostInitProperties() override;
	//从磁盘加载时补齐新增枚举
	virtual void PostLoad() override;

	UFUNCTION(BlueprintCallable, Category = "InteractData")
	UAnimSequence* GetInteractAnimation(EPlayerCharacterInteractMode InteractMode);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetInteractDistance(EPlayerCharacterInteractMode InteractMode);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetPreInteractDelay(EPlayerCharacterInteractMode InteractMode);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetPostInteractDelay(EPlayerCharacterInteractMode InteractMode);
	
};
