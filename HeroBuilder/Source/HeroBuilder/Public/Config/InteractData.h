// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InteractData.generated.h"
UENUM(BlueprintType)
enum EInteractType :uint8
{
	IT_None UMETA(DisplayName = "无"),
	IT_Normal UMETA(DisplayName = "正常类型"),
    IT_Construction UMETA(DisplayName = "建造类型"),
    IT_Lumber UMETA(DisplayName = "砍伐类型"),
    IT_Gather UMETA(DisplayName = "采集类型"),
    IT_Mine UMETA(DisplayName = "挖掘类型"),
    IT_Attack UMETA(DisplayName = "攻击类型"),
};

//与角色交互模式（建造模式开关）保持一致的枚举
//放在这里以便InteractData作为基础数据可直接使用，避免循环依赖
UENUM(BlueprintType)
enum EInteractMode :uint8
{
	IM_Normal UMETA(DisplayName = "正常"),
	IM_Construction UMETA(DisplayName = "建筑模式"),
};

USTRUCT(BlueprintType)
struct FInteractInfo
{
	GENERATED_BODY()
public:
	//该条配置对应的交互类型（同一InteractMode下用以区分多种交互行为）
	//作为该条记录的Key：由 RefreshInteractAnimations 自动按枚举补齐，编辑器内只读不可改
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	TEnumAsByte<EInteractType> InteractType = IT_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> InteractAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float InteractDistance=500;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float PreInteractDelay=2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float PostInteractDelay=2;
};

//Wrapper：UE 反射不支持 TMap<..., TArray<...>>，需要用一层 USTRUCT 包一层
USTRUCT(BlueprintType)
struct FInteractInfoArray
{
	GENERATED_BODY()
public:
	//Items 集合由 RefreshInteractAnimations 按 EInteractType 自动补齐/裁剪：
	//用户不能在编辑器中自行增删条目，仅能修改条目内字段（动画/距离/前后摇）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (EditFixedOrder), EditFixedSize)
	TArray<FInteractInfo> Items;
};

/**
 * 
 */
UCLASS(BlueprintType)
class HEROBUILDER_API UInteractData : public UDataAsset
{
	GENERATED_BODY()
private:
	//以 InteractMode 为 Key 的配置表；同一 Mode 下可挂多条 FInteractInfo（按 InteractType 区分）。
	//Map 的 Key 集合由 RefreshInteractAnimations 在编辑器内按枚举自动补齐/裁剪。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true", ReadOnlyKeys), EditFixedSize)
	TMap<TEnumAsByte<EInteractMode>, FInteractInfoArray> InteractAnimations;

public:
	//按 (Mode, Type) 查表获取该条配置的字段
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	UAnimSequence* GetInteractAnimation(EInteractMode InteractMode, EInteractType InteractType);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetInteractDistance(EInteractMode InteractMode, EInteractType InteractType);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetPreInteractDelay(EInteractMode InteractMode, EInteractType InteractType);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetPostInteractDelay(EInteractMode InteractMode, EInteractType InteractType);

#if WITH_EDITOR
protected:
	//新建/构造时初始化 Map
	virtual void PostInitProperties() override;
	//从磁盘加载时补齐新增枚举
	virtual void PostLoad() override;

private:
	//遍历 EInteractManagerInteractMode 所有枚举值，将缺失的 Key 补齐到 InteractAnimations，并剔除非法 Key
	void RefreshInteractAnimations();
#endif

private:
	//查表辅助：在 Mode 数组中按 Type 查找；找不到返回 nullptr
	const FInteractInfo* FindInfoByType(EInteractMode InteractMode, EInteractType InteractType) const;
};