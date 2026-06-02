// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/HB_Base_Manager.h"

// Sets default values
AHB_Base_Manager::AHB_Base_Manager()
{
	PrimaryActorTick.bCanEverTick = false;

	// 所有 Manager 默认网络复制 + 始终相关
	// 这样服务端 SpawnActor 出来的 Manager 会被自动复制到所有客户端
	bReplicates = true;
	bAlwaysRelevant = true;
	bNetLoadOnClient = false;     // 由服务端动态生成，不从地图加载
	SetReplicateMovement(false);  // Manager 不需要位置同步
	NetUpdateFrequency = 10.f;    // Manager 同步频率不需要太高，节省带宽
}

// Called when the game starts or when spawned
void AHB_Base_Manager::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHB_Base_Manager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

