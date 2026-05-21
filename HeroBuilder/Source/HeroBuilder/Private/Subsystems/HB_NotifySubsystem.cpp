// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/HB_NotifySubsystem.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

void UHB_NotifySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UHB_NotifySubsystem::Deinitialize()
{
	// 销毁NotifyProxy
	if (NotifyProxy && GetWorld())
	{
		GetWorld()->DestroyActor(NotifyProxy);
		NotifyProxy = nullptr;
	}
	
	Super::Deinitialize();
}

void UHB_NotifySubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void UHB_NotifySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	// 创建NotifyProxy实例
	if (GetWorld())
	{
		NotifyProxy = GetWorld()->SpawnActor<ANotifyProxy>(ANotifyProxy::StaticClass());
		if (NotifyProxy)
		{
			NotifyProxy->SetReplicates(true);
		}
	}
}

void UHB_NotifySubsystem::BindNotify(const FString& NotifyName, const FName& FunctionName, UObject* Object)
{
	// 先检查 Object 是否有效，避免空指针崩溃
	if (!IsValid(Object) || !FunctionName.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("BindNotify: Object or FunctionName is invalid!"));
		return;
	}

	// 找到或创建对应的事件委托
	FNotifyDelegate& ExistingDelegate = NotifyDelegates.FindOrAdd(NotifyName);
	FScriptDelegate ScriptDelegate;
	ScriptDelegate.BindUFunction(Object, FunctionName);
    ExistingDelegate.AddUnique(ScriptDelegate);
}

void UHB_NotifySubsystem::UnbindNotify(const FString& NotifyName, const FName& FunctionName, UObject* Object)
{
	// 检查参数有效性
	if (!IsValid(Object) || !FunctionName.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UnbindNotify: Object or FunctionName is invalid!"));
		return;
	}

	// 查找对应的委托
	if (FNotifyDelegate* ExistingDelegate = NotifyDelegates.Find(NotifyName))
	{
		// 创建要移除的脚本委托
		FScriptDelegate ScriptDelegateToRemove;
		ScriptDelegateToRemove.BindUFunction(Object, FunctionName);
		
		// 从委托中移除指定的绑定
		ExistingDelegate->Remove(ScriptDelegateToRemove);
		
		// 如果委托没有绑定了，从映射中移除
		if (!ExistingDelegate->IsBound())
		{
			NotifyDelegates.Remove(NotifyName);
		}
	}
}

void UHB_NotifySubsystem::UnbindAllNotify(const FString& NotifyName)
{
	// 直接移除指定通知的所有绑定
	NotifyDelegates.Remove(NotifyName);
}

void UHB_NotifySubsystem::UnbindAll()
{
	// 清空所有通知的绑定
	NotifyDelegates.Empty();
}

void UHB_NotifySubsystem::SendToClient(const FString& Notify, APlayerController* Target)
{
	if (!Target || !NotifyProxy)
	{
		return;
	}
	
	// 通过NotifyProxy发送RPC到指定客户端
	NotifyProxy->ClientReceiveNotify(Notify);
}

void UHB_NotifySubsystem::SendToServer(const FString& InNotify)
{
	if (!NotifyProxy)
	{
		return;
	}
	
	// 通过NotifyProxy发送RPC到服务器
	NotifyProxy->ServerReceiveNotify(InNotify);
}

void UHB_NotifySubsystem::SendToAllClient(const FString& Notify)
{
	if (!NotifyProxy)
	{
		return;
	}
	
	// 通过NotifyProxy发送RPC到所有客户端
	NotifyProxy->MulticastReceiveNotify(Notify);
}

void UHB_NotifySubsystem::HandleNotify(const FString& Notify)
{
	// 解析通知名称和内容
	FString NotifyName;
	FString NotifyContent;

	if (FNotifyDelegate* Delegate = NotifyDelegates.Find(Notify))
	{
		Delegate->Broadcast(Notify);
	}
}

// ANotifyProxy的实现
void ANotifyProxy::ClientReceiveNotify_Implementation(const FString& Notify)
{
	// 客户端接收到通知，交给子系统处理
	if (UHB_NotifySubsystem* NotifySubsystem = GetWorld()->GetSubsystem<UHB_NotifySubsystem>())
	{
		NotifySubsystem->HandleNotify(Notify);
	}
}

void ANotifyProxy::ServerReceiveNotify_Implementation(const FString& Notify)
{
	// 服务器接收到通知，交给子系统处理
	if (UHB_NotifySubsystem* NotifySubsystem = GetWorld()->GetSubsystem<UHB_NotifySubsystem>())
	{
		NotifySubsystem->HandleNotify(Notify);
	}
}

void ANotifyProxy::MulticastReceiveNotify_Implementation(const FString& Notify)
{
	// 所有客户端接收到通知，交给子系统处理
	if (UHB_NotifySubsystem* NotifySubsystem = GetWorld()->GetSubsystem<UHB_NotifySubsystem>())
	{
		NotifySubsystem->HandleNotify(Notify);
	}
}

ANotifyProxy::ANotifyProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
}
