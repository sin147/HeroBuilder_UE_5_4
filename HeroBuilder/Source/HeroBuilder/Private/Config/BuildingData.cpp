// Fill out your copyright notice in the Description page of Project Settings.


#include "Config/BuildingData.h"

FBuildingInfo UBuildingData::GetBuildingInfoByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass)
{
	if (BuildingInfoMap.Contains(BuildingClass))
	{
        return BuildingInfoMap[BuildingClass];
	}
	return FBuildingInfo();
}

UStaticMesh* UBuildingData::GetPreviewMeshByBuildingClass(TSubclassOf<AHB_Building_Base> BuildingClass)
{
	if (BuildingInfoMap.Contains(BuildingClass))
	{
        return BuildingInfoMap[BuildingClass].PreviewMesh;
	}
	return nullptr;
}
