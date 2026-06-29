// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "HB_GridManager.generated.h"

class AHB_Building_Base;
class AHB_Resource_Base;
USTRUCT(BlueprintType)
struct FGridInfo
{
	GENERATED_BODY()
public:
	FGridInfo() : X(0), Y(0) {}
	FGridInfo(int32 InX, int32 InY) : X(static_cast<int16>(InX)), Y(static_cast<int16>(InY)) {}
	UPROPERTY()
	int16 X = 0;
	UPROPERTY()
	int16 Y = 0;
	bool operator==(const FGridInfo& Other) const
	{
		return Other.X == X && Other.Y == Y;
	}
};

FORCEINLINE uint32 GetTypeHash(const FGridInfo& GridInfo)
{
	return HashCombine(GetTypeHash(static_cast<int32>(GridInfo.X)), GetTypeHash(static_cast<int32>(GridInfo.Y)));
}

/**
 * 
 */
UCLASS()
class HEROBUILDER_API AHB_GridManager : public AHB_Base_Manager
{
	GENERATED_BODY()
private:
	UPROPERTY(Replicated)
	TArray<FGridInfo> UsedGridInfos;
	// FreeGridInfos 仅在服务端用于资源/敌人生成时选取空闲 Fragment，客户端不需要同步，避免 Bunch 过大
	UPROPERTY()
	TArray<FGridInfo> FreeGridInfos;
public:
	void CacheUsedGridInfo(int InX,int InY);
	void RemoveUsedGridInfo(int InX, int InY);
	/** 将指定坐标注册为可用 Fragment（生成 Grid 时调用），若已被占用则跳过 */
	void RegisterFreeGridInfo(int InX, int InY);
	TArray<FGridInfo> GetUsedGridInfo();
	TArray<FGridInfo> GetFreeGridInfo();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
