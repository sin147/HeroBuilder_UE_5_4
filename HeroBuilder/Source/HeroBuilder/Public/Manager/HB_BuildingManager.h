// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Building/HB_Building_Base.h"
#include "HB_BuildingManager.generated.h"


/**
 * 
 */
UCLASS()
class HEROBUILDER_API AHB_BuildingManager : public AHB_Base_Manager
{
	GENERATED_BODY()
private:
	UPROPERTY(Replicated)
	TArray<TObjectPtr<AHB_Building_Base>> Buildings;
public:

    TArray<TObjectPtr<AHB_Building_Base>> GetAllBuildings() const;
	void AddBuilding(AHB_Building_Base* Building);
	void RemoveBuilding(AHB_Building_Base* Building);
public:
	AHB_BuildingManager();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
