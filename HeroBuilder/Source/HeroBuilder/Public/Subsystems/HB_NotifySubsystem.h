// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/HB_WorldSubsystem_Base.h"
#include "Delegates/Delegate.h"
#include "GameFramework/PlayerController.h"
#include "HB_NotifySubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNotifyDelegate,FString,Notify);

// 通知名称常量
namespace NotifyNames
{
	static const FString ON_SPAWN_BUILDING = TEXT("ON_SPAWN_BUILDING");
}

/**
 * 通知子系统，用于处理游戏内的通知消息传递
 */
UCLASS()
class HEROBUILDER_API UHB_NotifySubsystem : public UHB_WorldSubsystem_Base
{
	GENERATED_BODY()
	friend class ANotifyProxy;
private:
	TQueue<FString> NotifyQueue;
protected:
	
	// 存储通知绑定的委托
	TMap<FString, FNotifyDelegate> NotifyDelegates;
	TObjectPtr<ANotifyProxy> NotifyProxy;
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	// 绑定通知处理函数
	void BindNotify(const FString& NotifyName, const FName& FunctionName, UObject* Object);
	
	// 解绑指定通知的特定处理函数
	UFUNCTION(BlueprintCallable, Category = "Notify")
	void UnbindNotify(const FString& NotifyName, const FName& FunctionName, UObject* Object);
	
	// 解绑指定通知的所有处理函数
	UFUNCTION(BlueprintCallable, Category = "Notify")
	void UnbindAllNotify(const FString& NotifyName);
	
	// 解绑所有通知的所有处理函数
	UFUNCTION(BlueprintCallable, Category = "Notify")
	void UnbindAll();
	
	// 发送通知到指定客户端
	UFUNCTION(BlueprintCallable, Category = "Notify")
	void SendToClient(const FString& Notify, APlayerController* Target);
	
	// 发送通知到服务器
	UFUNCTION(BlueprintCallable, Category = "Notify")
	void SendToServer(const FString& InNotify);
	
	// 发送通知到所有客户端
	UFUNCTION(BlueprintCallable, Category = "Notify")
	void SendToAllClient(const FString& Notify);
	
private:
	// 内部处理通知的方法
	void HandleNotify(const FString& Notify);
};

UCLASS()
class HEROBUILDER_API ANotifyProxy : public AActor
{
	GENERATED_BODY()
public:
	// 服务器发送通知到客户端的RPC
	UFUNCTION(Client, Reliable)
	void ClientReceiveNotify(const FString& Notify);
	
	// 客户端发送通知到服务器的RPC
	UFUNCTION(Server, Reliable)
	void ServerReceiveNotify(const FString& Notify);
	
	// 服务器发送通知到所有客户端的RPC
	UFUNCTION(NetMulticast, Reliable)
	void MulticastReceiveNotify(const FString& Notify);

public:
	ANotifyProxy();
};