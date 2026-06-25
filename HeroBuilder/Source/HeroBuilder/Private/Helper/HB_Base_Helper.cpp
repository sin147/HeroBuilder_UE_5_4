// Fill out your copyright notice in the Description page of Project Settings.


#include "Helper/HB_Base_Helper.h"

// Sets default values
AHB_Base_Helper::AHB_Base_Helper()
{
	PrimaryActorTick.bCanEverTick = false;

	// Helper 引用通过 GameState 复制到客户端，本身需要被网络识别
	bReplicates = true;
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

