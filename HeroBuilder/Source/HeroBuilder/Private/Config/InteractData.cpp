// Fill out your copyright notice in the Description page of Project Settings.


#include "Config/InteractData.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

void UInteractData::PostInitProperties()
{
	Super::PostInitProperties();
	//新建实例时（包括CDO之外的真实实例）自动补齐所有枚举
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		InitializeInteractMap();
	}
}

void UInteractData::PostLoad()
{
	Super::PostLoad();
	//从磁盘加载后补齐可能新增的枚举值
	InitializeInteractMap();
}

void UInteractData::InitializeInteractMap()
{
	const UEnum* InteractModeEnum = StaticEnum<EPlayerCharacterInteractMode>();
	if (!InteractModeEnum)
	{
		return;
	}

	//枚举的最后一项通常是自动生成的_MAX，需要排除
	const int32 NumEnums = InteractModeEnum->NumEnums() - 1;
	for (int32 i = 0; i < NumEnums; ++i)
	{
		const int64 EnumValue = InteractModeEnum->GetValueByIndex(i);
		const EPlayerCharacterInteractMode Mode = static_cast<EPlayerCharacterInteractMode>(EnumValue);

		//仅在缺失时补一个空配置，避免覆盖用户已有数据
		if (!InteractAnimationsMap.Contains(Mode))
		{
			InteractAnimationsMap.Add(Mode, FInteractInfo());
		}
	}
}

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
