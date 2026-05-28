// Fill out your copyright notice in the Description page of Project Settings.

#include "Config/ResourceData.h"
#include "Resource/HB_Resource_Base.h"
#if WITH_EDITOR
#include "UObject/UObjectIterator.h"
#endif

FResourceConfig UResourceData::GetResourceInfoByResourceClass(TSubclassOf<AHB_Resource_Base> ResourceClass)
{
	if (ResourceInfoMap.Contains(ResourceClass))
	{
		return ResourceInfoMap[ResourceClass];
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ResourceData: GetResourceInfoByResourceClass: ResourceClass not found"));
	}

	// 返回默认配置
	FResourceConfig DefaultConfig;
	return DefaultConfig;
}

#if WITH_EDITOR
void UResourceData::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_NeedPostLoad))
	{
		RefreshResourceInfoMap();
	}
}

void UResourceData::PostLoad()
{
	Super::PostLoad();
	RefreshResourceInfoMap();
}

namespace
{
	// 判定一个UClass是否是“合法的可作为Map Key的游戏类”
	// 用于过滤掉蓝图骨架类(SKEL_)、重新实例化中间类(REINST_)、
	// 垃圾类(TRASHCLASS_)、热重载残留(HOTRELOADED_)、占位类(PLACEHOLDER-CLASS_)、
	// 死亡类(DEADCLASS_)、抽象类、废弃类、旧版本类等
	static bool IsValidGameplayClass(UClass* Class, UClass* BaseClass)
	{
		if (!Class || !BaseClass)
		{
			return false;
		}
		if (!Class->IsChildOf(BaseClass))
		{
			return false;
		}

		// 过滤抽象类、废弃类、存在更新版本的类
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			return false;
		}

		// 过滤瞬态对象、销毁中、旧版本占位等RF标志
		if (Class->HasAnyFlags(RF_NewerVersionExists | RF_BeginDestroyed | RF_FinishDestroyed))
		{
			return false;
		}

		// 过滤蓝图骨架类与各种中间/占位类
		const FString ClassName = Class->GetName();
		if (ClassName.StartsWith(TEXT("SKEL_"))
			|| ClassName.StartsWith(TEXT("REINST_"))
			|| ClassName.StartsWith(TEXT("TRASHCLASS_"))
			|| ClassName.StartsWith(TEXT("HOTRELOADED_"))
			|| ClassName.StartsWith(TEXT("PLACEHOLDER-CLASS_"))
			|| ClassName.StartsWith(TEXT("DEADCLASS_")))
		{
			return false;
		}

		// 只接受权威类，避免中间类指针残留
		if (Class->GetAuthoritativeClass() != Class)
		{
			return false;
		}

		return true;
	}
}

void UResourceData::RefreshResourceInfoMap()
{
	// 收集所有AHB_Resource_Base的子类（仅蓝图实例化用的真实子类）
	TSet<TSubclassOf<AHB_Resource_Base>> AllResourceClasses;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class || !Class->IsChildOf(AHB_Resource_Base::StaticClass()))
		{
			continue;
		}
		// 跳过基类自身（基类不直接实例化，仅作为子类继承根）
		if (Class == AHB_Resource_Base::StaticClass())
		{
			continue;
		}
		// 跳过抽象类、废弃类、被新版本替换的占位类、隐藏类、不可放置类
		if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Hidden | CLASS_HideDropDown))
		{
			continue;
		}
		// 跳过SKEL_/REINST_/TRASHCLASS_/HOTRELOADED_等编译期/热重载占位类
		const FString ClassName = Class->GetName();
		if (ClassName.StartsWith(TEXT("SKEL_")) ||
			ClassName.StartsWith(TEXT("REINST_")) ||
			ClassName.StartsWith(TEXT("TRASHCLASS_")) ||
			ClassName.StartsWith(TEXT("HOTRELOADED_")) ||
			ClassName.StartsWith(TEXT("PLACEHOLDER-CLASS_")))
		{
			continue;
		}
		// 跳过瞬态/正在被GC的类
		if (Class->HasAnyFlags(RF_ClassDefaultObject | RF_NewerVersionExists | RF_BeginDestroyed))
		{
			continue;
		}
		AllResourceClasses.Add(Class);
	}

	bool bChanged = false;

	// 主动剔除无效/重复/异常Key
	// 1) nullptr Key
	// 2) 不在AllResourceClasses中的Key（包括抽象类、废弃类、REINST_占位类等）
	// 3) 同一UClass*的重复Key（TMap本身不允许重复，这里用TSet二次保险，
	//    防止因热重载/反序列化异常导致的多份相同Key残留）
	TSet<UClass*> SeenClasses;
	for (auto It = ResourceInfoMap.CreateIterator(); It; ++It)
	{
		UClass* KeyClass = It.Key();

		// 无效Key
		if (!KeyClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除无效Key(nullptr)"));
			It.RemoveCurrent();
			bChanged = true;
			continue;
		}

		// 异常/不再合法的Key
		if (!AllResourceClasses.Contains(KeyClass))
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除非法Key %s"), *KeyClass->GetName());
			It.RemoveCurrent();
			bChanged = true;
			continue;
		}

		// 重复Key（保险逻辑）
		bool bAlreadySeen = false;
		SeenClasses.Add(KeyClass, &bAlreadySeen);
		if (bAlreadySeen)
		{
			UE_LOG(LogTemp, Warning, TEXT("ResourceData: 剔除重复Key %s"), *KeyClass->GetName());
			It.RemoveCurrent();
			bChanged = true;
		}
	}

	// 添加新出现的子类（已存在的保留原配置）
	for (const TSubclassOf<AHB_Resource_Base>& ResourceClass : AllResourceClasses)
	{
		if (!ResourceInfoMap.Contains(ResourceClass))
		{
			ResourceInfoMap.Add(ResourceClass, FResourceConfig());
			bChanged = true;
		}
	}

	if (bChanged)
	{
		MarkPackageDirty();
	}
}
#endif