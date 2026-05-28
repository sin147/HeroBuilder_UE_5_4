// Fill out your copyright notice in the Description page of Project Settings.


#include "Config/InteractData.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

UAnimSequence* UInteractData::GetInteractAnimation(EPlayerCharacterInteractMode InteractMode)
{
    if (!InteractAnimationsMap.Contains(InteractMode))
	{
		return nullptr;
	}
	return InteractAnimationsMap[InteractMode].InteractAnimation;
}

float UInteractData::GetInteractDistance(EPlayerCharacterInteractMode InteractMode)
{
	if (!InteractAnimationsMap.Contains(InteractMode))
	{
		return 0.0f;
	}
    return InteractAnimationsMap[InteractMode].InteractDistance;
}

float UInteractData::GetPreInteractDelay(EPlayerCharacterInteractMode InteractMode)
{
	if (!InteractAnimationsMap.Contains(InteractMode))
	{
		return 9999.0f;
	}
    return InteractAnimationsMap[InteractMode].PreInteractDelay;
}

float UInteractData::GetPostInteractDelay(EPlayerCharacterInteractMode InteractMode)
{
	if (!InteractAnimationsMap.Contains(InteractMode))
	{
		return 9999.0f;
	}
    return InteractAnimationsMap[InteractMode].PostInteractDelay;
}

#if WITH_EDITOR
void UInteractData::PostInitProperties()
{
	Super::PostInitProperties();
	//新建实例时（包括CDO之外的真实实例）自动补齐所有枚举
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_NeedPostLoad))
	{
		RefreshInteractAnimationsMap();
	}
}

void UInteractData::PostLoad()
{
	Super::PostLoad();
	//从磁盘加载后补齐可能新增的枚举值
	RefreshInteractAnimationsMap();
}

void UInteractData::RefreshInteractAnimationsMap()
{
	const UEnum* InteractModeEnum = StaticEnum<EPlayerCharacterInteractMode>();
	if (!InteractModeEnum)
	{
		return;
	}

	// 收集所有合法的枚举值（排除最后一项自动生成的_MAX，并排除IM_None）
	TSet<EPlayerCharacterInteractMode> AllInteractModes;
	const int32 NumEnums = InteractModeEnum->NumEnums() - 1;
	for (int32 i = 0; i < NumEnums; ++i)
	{
		const int64 EnumValue = InteractModeEnum->GetValueByIndex(i);
		const EPlayerCharacterInteractMode Mode = static_cast<EPlayerCharacterInteractMode>(EnumValue);
		// 跳过IM_None：它仅作为"无交互"的占位状态，不需要进入配置Map
		if (Mode == IM_None)
		{
			continue;
		}
		AllInteractModes.Add(Mode);
	}

	bool bChanged = false;

	// 主动剔除无效/重复/异常Key
	// 1) 不在AllInteractModes中的Key（例如已被删除的旧枚举值、_MAX占位等）
	// 2) 同一枚举值的重复Key（保险）
	TSet<EPlayerCharacterInteractMode> SeenModes;
	for (auto It = InteractAnimationsMap.CreateIterator(); It; ++It)
	{
		const EPlayerCharacterInteractMode KeyMode = static_cast<EPlayerCharacterInteractMode>(It.Key().GetValue());

		// 异常/不再合法的Key
		if (!AllInteractModes.Contains(KeyMode))
		{
			UE_LOG(LogTemp, Warning, TEXT("InteractData: 剔除非法Key %d"), static_cast<int32>(KeyMode));
			It.RemoveCurrent();
			bChanged = true;
			continue;
		}

		// 重复Key（保险逻辑）
		bool bAlreadySeen = false;
		SeenModes.Add(KeyMode, &bAlreadySeen);
		if (bAlreadySeen)
		{
			UE_LOG(LogTemp, Warning, TEXT("InteractData: 剔除重复Key %d"), static_cast<int32>(KeyMode));
			It.RemoveCurrent();
			bChanged = true;
		}
	}

	// 添加缺失的枚举值（已存在的保留原配置）
	for (const EPlayerCharacterInteractMode& Mode : AllInteractModes)
	{
		if (!InteractAnimationsMap.Contains(Mode))
		{
			InteractAnimationsMap.Add(Mode, FInteractInfo());
			bChanged = true;
		}
	}

	if (bChanged)
	{
		MarkPackageDirty();
	}
}
#endif