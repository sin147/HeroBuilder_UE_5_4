// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "HB_GridManager.generated.h"

class AHB_Building_Base;
class AHB_Enemy_Base;
USTRUCT(BlueprintType)
struct FGridInfo
{
	GENERATED_BODY()
private:
	TObjectPtr<AHB_Building_Base> Building;
	TArray<TObjectPtr<AHB_Enemy_Base>> Enemies;
public:
	FGridInfo() {}
	FGridInfo(int32 InX,int32 InY):X(InX),Y(InY){}
	int32 X;
    int32 Y;
	bool operator==(const FGridInfo& Other) const
	{
		return Other.X == X && Other.Y == Y;
	}
	TObjectPtr<AHB_Building_Base> GetBuilding();
	bool SetBuilding(TObjectPtr<AHB_Building_Base> InBuilding);
	TArray<TObjectPtr<AHB_Enemy_Base>> GetEnemies();
	bool AddEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy);
	bool RemoveEnemy(TObjectPtr<AHB_Enemy_Base> InEnemy);
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
	TArray<FGridInfo> GridInfos;
public:
	void SetGridInfo(int InX,int InY,AHB_Building_Base*InBuilding=nullptr);
    bool GetGridInfo(FGridInfo& OutGridInfo);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
