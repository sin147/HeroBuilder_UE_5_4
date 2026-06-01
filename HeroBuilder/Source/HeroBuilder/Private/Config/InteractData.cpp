// Fill out your copyright notice in the Description page of Project Settings.

#include "Config/InteractData.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

const FInteractInfo* UInteractData::FindInfoByType(EInteractMode InteractMode, EInteractType InteractType) const
{
	if (const FInteractInfoArray* Arr = InteractAnimations.Find(InteractMode))
	{
		for (const FInteractInfo& Info : Arr->Items)
		{
			if (Info.InteractType == InteractType)
			{
				return &Info;
			}
		}
	}
	return nullptr;
}

UAnimSequence* UInteractData::GetInteractAnimation(EInteractMode InteractMode, EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractMode, InteractType))
	{
		return Info->InteractAnimation;
	}
	return nullptr;
}

float UInteractData::GetInteractDistance(EInteractMode InteractMode, EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractMode, InteractType))
	{
		return Info->InteractDistance;
	}
	return 0.0f;
}

float UInteractData::GetPreInteractDelay(EInteractMode InteractMode, EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractMode, InteractType))
	{
		return Info->PreInteractDelay;
	}
	return 9999.0f;
}

float UInteractData::GetPostInteractDelay(EInteractMode InteractMode, EInteractType InteractType)
{
	if (const FInteractInfo* Info = FindInfoByType(InteractMode, InteractType))
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
	const UEnum* InteractModeEnum = StaticEnum<EInteractMode>();
	const UEnum* InteractTypeEnum = StaticEnum<EInteractType>();
	if (!InteractModeEnum || !InteractTypeEnum)
	{
		return;
	}

	// 收集所有合法的 Mode 枚举值（排除最后一项自动生成的_MAX）
	TSet<EInteractMode> AllInteractModes;
	{
		const int32 NumEnums = InteractModeEnum->NumEnums() - 1;
		for (int32 i = 0; i < NumEnums; ++i)
		{
			const int64 EnumValue = InteractModeEnum->GetValueByIndex(i);
			AllInteractModes.Add(static_cast<EInteractMode>(EnumValue));
		}
	}

	// 收集所有合法的 Type 枚举值（保留枚举顺序，便于排序）
	TArray<EInteractType> AllInteractTypes;
	TSet<EInteractType>   AllInteractTypeSet;
	{
		const int32 NumEnums = InteractTypeEnum->NumEnums() - 1;
		AllInteractTypes.Reserve(NumEnums);
		for (int32 i = 0; i < NumEnums; ++i)
		{
			const EInteractType TypeValue = static_cast<EInteractType>(InteractTypeEnum->GetValueByIndex(i));
			AllInteractTypes.Add(TypeValue);
			AllInteractTypeSet.Add(TypeValue);
		}
	}

	bool bChanged = false;

	// 主动剔除无效 Mode Key
	for (auto It = InteractAnimations.CreateIterator(); It; ++It)
	{
		const EInteractMode KeyMode = static_cast<EInteractMode>(It.Key().GetValue());
		if (!AllInteractModes.Contains(KeyMode))
		{
			UE_LOG(LogTemp, Warning, TEXT("InteractData: 剔除非法Mode Key %d"), static_cast<int32>(KeyMode));
			It.RemoveCurrent();
			bChanged = true;
		}
	}

	// 添加缺失的 Mode Key
	for (const EInteractMode& Mode : AllInteractModes)
	{
		if (!InteractAnimations.Contains(Mode))
		{
			InteractAnimations.Add(Mode, FInteractInfoArray());
			bChanged = true;
		}
	}

	// 对每个 Mode 的 Items 按 EInteractType 补齐/裁剪/排序
	for (auto& Pair : InteractAnimations)
	{
		FInteractInfoArray& Arr = Pair.Value;

		// 1) 剔除非法 Type 与重复 Type，保留每个 Type 的首个出现
		TSet<EInteractType> SeenTypes;
		for (int32 i = Arr.Items.Num() - 1; i >= 0; --i)
		{
			const EInteractType T = static_cast<EInteractType>(Arr.Items[i].InteractType.GetValue());
			if (!AllInteractTypeSet.Contains(T))
			{
				UE_LOG(LogTemp, Warning, TEXT("InteractData: 剔除非法Type %d"), static_cast<int32>(T));
				Arr.Items.RemoveAt(i);
				bChanged = true;
				continue;
			}
		}
		// 第二遍正向扫描去重（保留靠前者）
		for (int32 i = 0; i < Arr.Items.Num();)
		{
			const EInteractType T = static_cast<EInteractType>(Arr.Items[i].InteractType.GetValue());
			if (SeenTypes.Contains(T))
			{
				UE_LOG(LogTemp, Warning, TEXT("InteractData: 剔除重复Type %d"), static_cast<int32>(T));
				Arr.Items.RemoveAt(i);
				bChanged = true;
				continue;
			}
			SeenTypes.Add(T);
			++i;
		}

		// 2) 补齐缺失的 Type
		for (const EInteractType T : AllInteractTypes)
		{
			if (!SeenTypes.Contains(T))
			{
				FInteractInfo NewInfo;
				NewInfo.InteractType = T;
				Arr.Items.Add(NewInfo);
				bChanged = true;
			}
		}

		// 3) 按 EInteractType 枚举顺序排序，编辑器内观感稳定
		Arr.Items.Sort([](const FInteractInfo& A, const FInteractInfo& B)
		{
			return static_cast<uint8>(A.InteractType.GetValue()) < static_cast<uint8>(B.InteractType.GetValue());
		});
	}

	if (bChanged)
	{
		MarkPackageDirty();
	}
}
#endif