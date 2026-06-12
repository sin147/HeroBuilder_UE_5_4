// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Helper/HB_Base_Helper.h"
#include "Types/HB_Enums.h"
#include "HB_CharacterHelper.generated.h"

class ACharacter;

/**
 * 角色相关无状态工具方法集合
 * 仅服务端 GameMode 会持有该 Helper 实例；客户端调用方应做空保护或直接退化为等价枚举判断
 */
UCLASS()
class HEROBUILDER_API AHB_CharacterHelper : public AHB_Base_Helper
{
	GENERATED_BODY()
public:
	//仅根据状态枚举判断是否处于交互（前摇/进行中/后摇）：纯工具方法，不依赖任何外部对象
	bool IsInteracting(EPlayerCharacterState State) const;
};
