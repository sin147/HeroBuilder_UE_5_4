// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Manager/HB_Base_Manager.h"
#include "Building/HB_Building_Base.h"
#include "Engine/StaticMeshActor.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "HB_ConstructionManager.generated.h"

class AHB_Building_Base;
class AHB_ConstructionManager;
class APreBuilding;
class ACharacter;

//单条玩家建造预览数据（参与FastArray差量复制）
USTRUCT()
struct FPreBuildingInfo : public FFastArraySerializerItem
{
	GENERATED_BODY()
private:
	//本地快照：仅本地使用、不参与复制；客户端PostReplicatedAdd/Change中维护，便于差量比较
	TSubclassOf<AHB_Building_Base>      PreviousBuildingClass = nullptr;
	TWeakObjectPtr<APreBuilding>        PreviousPreBuildingMeshActor = nullptr;
	bool                                bPreviousIsActive = false;
public:
	//所属角色（作为该条记录的Key）
	UPROPERTY()
	TObjectPtr<ACharacter> Character = nullptr;

	UPROPERTY()
	TSubclassOf<AHB_Building_Base> BuildingClass;

	UPROPERTY()
	TObjectPtr<APreBuilding> PreBuildingMeshActor;

	//是否处于激活状态：决定预览体是否可见、Tick 是否跟随许可状态是否生效
	UPROPERTY()
	bool bIsActive = false;

	UPROPERTY()
	bool bActiveTickPos = true;

	//—— FastArray 客户端回调 ——
	void PreReplicatedRemove(const FFastArraySerializer& ArraySerializer);
	void PostReplicatedAdd(const FFastArraySerializer& ArraySerializer);
	void PostReplicatedChange(const FFastArraySerializer& ArraySerializer);
};

USTRUCT()
struct FPreBuildingContainer : public FFastArraySerializer
{
	GENERATED_BODY()
	UPROPERTY()
	TArray<FPreBuildingInfo> PreBuildingEntries;

	//反向指针：仅本地使用，不参与复制；供FastArrayItem回调反查Manager→World→Subsystem
	TWeakObjectPtr<AHB_ConstructionManager> OwnerManager;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& Parms)
	{
		return FastArrayDeltaSerialize<FPreBuildingInfo, FPreBuildingContainer>(PreBuildingEntries, Parms, *this);
	}
};
//关键：告诉 UE 这个结构体走 NetDeltaSerializer
template<>
struct TStructOpsTypeTraits<FPreBuildingContainer> : public TStructOpsTypeTraitsBase2<FPreBuildingContainer>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};

/**
 * 建造管理器（单例）
 * 服务端权威：以 Character 为 Key 维护所有玩家的建造预览数据；整张表通过FastArray差量复制给所有客户端。
 */
UCLASS()
class HEROBUILDER_API AHB_ConstructionManager : public AHB_Base_Manager
{
	GENERATED_BODY()

public:
	AHB_ConstructionManager();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;

	//—— 建造类（服务端权威） ——
	TSubclassOf<AHB_Building_Base> GetBuildingClass(ACharacter* InCharacter) const;
	void SetBuildingClass(ACharacter* InCharacter, TSubclassOf<AHB_Building_Base> NewClass);

	//—— 预览体 Actor ——
	APreBuilding* GetPreBuildingMeshActor(ACharacter* InCharacter) const;
	void SetPreBuildingMeshActor(ACharacter* InCharacter, APreBuilding* NewActor);

	//—— 激活状态 ——
	bool GetIsActive(ACharacter* InCharacter) const;
	void SetIsActive(ACharacter* InCharacter, bool bInActive);
	bool GetActiveTickPos(ACharacter* InCharacter) const;
	void SetActiveTickPos(ACharacter* InCharacter, bool bInActive);

	//是否存在该角色的Entry
	bool HasEntry(ACharacter* InCharacter) const;

	//一次性新增 Entry：原子写入 BuildingClass / PreBuildingMeshActor / bIsActive，并只标脏一次
	//用于首次进入 ConstructionMode 等场景，避免分多次 Setter 调用导致客户端先收到 BuildingClass=nullptr 的中间态
	//返回值：true=新增成功；false=Entry 已存在（不会修改任何字段，调用者应改用 SetXxx 进行更新）
	bool AddEntry(ACharacter* InCharacter,
	              TSubclassOf<AHB_Building_Base> InBuildingClass,
	              APreBuilding* InPreBuildingMeshActor,
	              bool bInActive);

	//玩家登出时清理表项（仅服务端调用）
	void RemoveEntry(ACharacter* InCharacter);

	//遍历所有Entry（用于Tick等场景）
	const TArray<FPreBuildingInfo>& GetAllEntries() const { return PreBuildingContainer.PreBuildingEntries; }

	//—— 统一对外派发预览数据变化（服务端权威路径与客户端 FastArray 路径都收敛到这里） ——
	void BroadcastPreBuildingClassChanged(ACharacter* InCharacter, TSubclassOf<AHB_Building_Base> OldClass, TSubclassOf<AHB_Building_Base> NewClass);
	void BroadcastPreBuildingActorChanged(ACharacter* InCharacter, APreBuilding* OldActor, APreBuilding* NewActor);
	void BroadcastPreBuildingActiveChanged(ACharacter* InCharacter, bool bIsActive);

private:
	//全玩家建造预览数据表（FastArray差量复制给所有客户端）
	UPROPERTY(Replicated)
	FPreBuildingContainer PreBuildingContainer;

	//查表辅助
	const FPreBuildingInfo* FindEntry(ACharacter* InCharacter) const;
	FPreBuildingInfo* FindEntryMutable(ACharacter* InCharacter);
	FPreBuildingInfo& FindOrAddEntry(ACharacter* InCharacter);
};

UCLASS()
class HEROBUILDER_API APreBuilding : public AStaticMeshActor
{
	GENERATED_BODY()
private:
	UPROPERTY(Replicated, ReplicatedUsing = On_Rep_StateMesh)
	UStaticMesh* StaticMesh;
	UFUNCTION()
	void On_Rep_StateMesh();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
	APreBuilding();
	void SetStaticMesh(TObjectPtr<UStaticMesh> InMesh);

};