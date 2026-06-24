// Fill out your copyright notice in the Description page of Project Settings.


#include "Config/GridData.h"
#include "Grid/HB_Grid_Base.h"

int32 UGridData::GetGridWidthFragment() const
{
    return GridWidthFragment;
}

int32 UGridData::GetGridLengthFragment() const
{
    return GridLengthFragment;
}

TSubclassOf<AHB_Grid_Base> UGridData::GetGridClassByLevel(int32 AreaLevel) const
{
    if (AreaLevel >= 0 && AreaConfigs.Contains(AreaLevel))
    {
        return AreaConfigs[AreaLevel].GridClass;
    }
	return nullptr;
}

bool UGridData::GetAreaConfigByLevel(int32 AreaLevel, FAreaConfig& OutConfig) const
{
    if (AreaLevel >= 0 && AreaConfigs.Contains(AreaLevel))
    {
        OutConfig = AreaConfigs[AreaLevel];
        return true;
    }
    return false;
}

TArray<int32> UGridData::GetAllAreaLevels() const
{
    TArray<int32> Levels;
    AreaConfigs.GetKeys(Levels);
    //升序排序：保证从中心(Level=0)向外依次生成
    Levels.Sort();
    return Levels;
}
