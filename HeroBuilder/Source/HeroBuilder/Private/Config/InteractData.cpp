// Fill out your copyright notice in the Description page of Project Settings.

#include "Config/InteractData.h"

const FInteractInfo* UInteractData::FindInfoByType(EInteractType InteractType) const
{
	//Map 形态下查表 O(1)；TEnumAsByte 与原生枚举 Key 等价（UE反射会自动转换）
	return InteractAnimations.Find(InteractType);
}

UAnimSequence* UInteractData::GetInteractAnimation(EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractType))
	{
		return Info->InteractAnimation;
	}
	return nullptr;
}

float UInteractData::GetInteractDistance(EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractType))
	{
		return Info->InteractDistance;
	}
	return 0.0f;
}

float UInteractData::GetPreInteractDelay(EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractType))
	{
		return Info->PreInteractDelay;
	}
	return 9999.0f;
}

float UInteractData::GetPostInteractDelay(EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractType))
	{
		return Info->PostInteractDelay;
	}
	return 9999.0f;
}

#if WITH_EDITOR
void UInteractData::PostInitProperties()
{
	Super::PostInitProperties();
	//新建实例时（包括CDO之外的真实实例）自动补齐所有枚举
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_NeedPostLoad))
	{
		RefreshInteractAnimations();
	}
}

void UInteractData::PostLoad()
{
	Super::PostLoad();
	//从磁盘加载后补齐可能新增的枚举值
	RefreshInteractAnimations();
}

void UInteractData::RefreshInteractAnimations()
{
	const UEnum* InteractTypeEnum = StaticEnum<EInteractType>();
	if (!InteractTypeEnum)
	{
		return;
	}

	//收集所有合法的 Type 枚举值（NumEnums-1 排除自动生成的 _MAX 项）
	TSet<EInteractType> AllInteractTypeSet;
	{
		const int32 NumEnums = InteractTypeEnum->NumEnums() - 1;
		AllInteractTypeSet.Reserve(NumEnums);
		for (int32 i = 0; i < NumEnums; ++i)
		{
			AllInteractTypeSet.Add(static_cast<EInteractType>(InteractTypeEnum->GetValueByIndex(i)));
		}
	}

	bool bChanged = false;

	// 1) 剔除 Map 中非法 Key（枚举值已被删除/不存在）
	//    使用迭代器删除以避免修改容器期间迭代失效
	for (auto It = InteractAnimations.CreateIterator(); It; ++It)
	{
		const EInteractType T = static_cast<EInteractType>(It.Key().GetValue());
		if (!AllInteractTypeSet.Contains(T))
		{
			UE_LOG(LogTemp, Warning, TEXT("InteractData: 剔除非法Type %d"), static_cast<int32>(T));
			It.RemoveCurrent();
			bChanged = true;
		}
	}

	// 2) 补齐缺失的 Key（新增枚举值时自动加空白条目）
	for (const EInteractType T : AllInteractTypeSet)
	{
		if (!InteractAnimations.Contains(T))
		{
			InteractAnimations.Add(T, FInteractInfo());
			bChanged = true;
		}
	}

	//说明：TMap 自身的迭代顺序由内部哈希决定，编辑器面板会按枚举顺序展示 Key，
	//      因此不需要再做"按枚举序排序"。

	if (bChanged)
	{
		MarkPackageDirty();
	}
}
#endif