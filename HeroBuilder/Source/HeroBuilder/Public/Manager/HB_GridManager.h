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
	FGridInfo(int32 InX,int32 InY):X(InX),Y(InY){}
	UPROPERTY()
	int32 X = 0;
	UPROPERTY()
    int32 Y = 0;
	bool operator==(const FGridInfo& Other) const
	{
		return Other.X == X && Other.Y == Y;
	}
};

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
	UPROPERTY(Replicated)
	TArray<FGridInfo> FreeGridInfos;
public:
	void CacheUsedGridInfo(int InX,int InY);
	void RemoveUsedGridInfo(int InX, int InY);
	TArray<FGridInfo> GetUsedGridInfo();
	TArray<FGridInfo> GetFreeGridInfo();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
