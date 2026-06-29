// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Helper/HB_Base_Helper.h"
#include "HB_TotemHelper.generated.h"

class AHB_Totem_Base;

/**
 * 图腾相关无状态工具方法集合
 * 仅服务端 GameState 会持有该 Helper 实例；客户端调用方应做空保护。
 */
UCLASS()
class HEROBUILDER_API AHB_TotemHelper : public AHB_Base_Helper
{
	GENERATED_BODY()
public:
	bool IsValidTotem(TObjectPtr<AHB_Totem_Base> InTotem);
};
