// Fill out your copyright notice in the Description page of Project Settings.


#include "Helper/HB_Base_Helper.h"

// Sets default values
AHB_Base_Helper::AHB_Base_Helper()
{
	PrimaryActorTick.bCanEverTick = false;

	// Helper 内部没有需要网络同步的属性和逻辑，仅在服务端使用，
	// 关闭复制以避免 "has no root component in AActor::IsNetRelevantFor" 警告并节省网络开销
	bReplicates = false;
	bNetLoadOnClient = false;
}

// Called when the game starts or when spawned
void AHB_Base_Helper::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AHB_Base_Helper::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

