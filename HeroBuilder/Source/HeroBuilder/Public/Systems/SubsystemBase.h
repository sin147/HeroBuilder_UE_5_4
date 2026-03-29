// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "SubsystemBase.generated.h"

/**
 * 
 */
UCLASS()
class HEROBUILDER_API USubsystemBase : public UTickableWorldSubsystem
{
	GENERATED_BODY()
	friend class ANetworkProxyBase;
private:
	ANetworkProxyBase* NetworkProxy;
protected:
	TSubclassOf<ANetworkProxyBase> NetworkProxyClass;

	//获取网络代理
	template<typename T>
	T* GetNetworkProxy()
	{
		if(!NetworkProxy)
		{
			return nullptr;
		}
		return Cast<T>(NetworkProxy);
	}
	//设置网络代理
	void SetNetworkProxy(ANetworkProxyBase* InNetworkProxy)
	{
		NetworkProxy = InNetworkProxy;
	}

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual ANetworkProxyBase* CreateNetworkProxy(APlayerController* InOwner);
    virtual TStatId GetStatId() const override final;
};
