// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Types/HB_Enums.h"
#include "InteractData.generated.h"

USTRUCT(BlueprintType)
struct FInteractInfo
{
	GENERATED_BODY()
public:
	//说明：本结构体作为 TMap<EInteractType, FInteractInfo> 的 Value，
	//      EInteractType 由 Map 的 Key 表达，因此这里不再冗余存储 InteractType 字段，
	//      避免出现"Map Key 与 Item.InteractType 不一致"的脏数据风险。

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAnimSequence> InteractAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float InteractDistance=500;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float PreInteractDelay=2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true"))
	float PostInteractDelay=2;
};

/**
 * 
 */
UCLASS(BlueprintType)
class HEROBUILDER_API UInteractData : public UDataAsset
{
	GENERATED_BODY()
private:
	//交互配置表：按 EInteractType 作为 Key 一个枚举一条记录，集合由 RefreshInteractAnimations 自动补齐缺失 Key / 副除非法 Key。
	//用户在编辑器中不能自行增删条目，仅能修改条目内字段（动画/距离/前后摇）。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "InteractData", meta = (AllowPrivateAccess = "true",ReadOnlyKeys), EditFixedSize)
	TMap<TEnumAsByte<EInteractType>,FInteractInfo> InteractAnimations;

public:
	//按 InteractType 查表获取该条配置的字段
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	UAnimSequence* GetInteractAnimation(EInteractType InteractType);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetInteractDistance(EInteractType InteractType);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetPreInteractDelay(EInteractType InteractType);
	UFUNCTION(BlueprintCallable, Category = "InteractData")
	float GetPostInteractDelay(EInteractType InteractType);

#if WITH_EDITOR
protected:
	//新建/构造时初始化 Items
	virtual void PostInitProperties() override;
	//从磁盘加载时补齐新增枚举
	virtual void PostLoad() override;

private:
	//遍历 EInteractType 所有枚举值，将缺失的 Type Key 补齐到 InteractAnimations，并剔除枚举中已不存在的非法 Key
	void RefreshInteractAnimations();
#endif

private:
	//查表辅助：按 Type 查找；找不到返回 nullptr
	const FInteractInfo* FindInfoByType(EInteractType InteractType) const;
};