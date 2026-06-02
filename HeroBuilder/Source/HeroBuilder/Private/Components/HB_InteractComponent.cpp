// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/HB_InteractComponent.h"

DEFINE_LOG_CATEGORY(LogInteractComponent);

UHB_InteractComponent::UHB_InteractComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// InteractType 仅是只读元数据，无需复制（客户端通过自身相同 CDO + 所属Actor状态推断）；
	// 如未来需要在运行时被服务端切换，再开启 SetIsReplicatedByDefault(true) 与 GetLifetimeReplicatedProps。
}
