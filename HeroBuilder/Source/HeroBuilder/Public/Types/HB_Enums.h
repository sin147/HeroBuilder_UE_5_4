// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HB_Enums.generated.h"

/**
 * HB_Enums.h
 * 项目内所有 UENUM 集中定义文件。
 * 
 * 使用约定：
 *  - 任何需要使用项目自定义枚举的文件统一 #include "Types/HB_Enums.h"
 *  - 新增枚举请直接添加到本文件，避免分散到各业务头中
 *  - UENUM 不能放进 namespace（UHT 不支持），故全部按平铺方式声明
 */

//==================== 玩家角色 ====================

//玩家角色状态
UENUM(BlueprintType)
enum EPlayerCharacterState :uint8
{
	EPCS_None UMETA(DisplayName = "无"),
	EPCS_Idle UMETA(DisplayName = "空闲"),
	EPCS_Move UMETA(DisplayName = "移动"),
	EPCS_MoveToTarget UMETA(DisplayName = "追击目标"),
	EPCS_PreInteract UMETA(DisplayName = "交互前摇"),
	EPCS_Interact UMETA(DisplayName = "交互"),
	EPCS_PostInteract UMETA(DisplayName = "交互后摇"),
};

//==================== 交互 ====================

//交互类型
UENUM(BlueprintType)
enum EInteractType :uint8
{
	IT_None UMETA(DisplayName = "无"),
	IT_Normal UMETA(DisplayName = "正常类型"),
	IT_Construction UMETA(DisplayName = "建造类型"),
	IT_Lumber UMETA(DisplayName = "砍伐类型"),
	IT_Gather UMETA(DisplayName = "采集类型"),
	IT_Mine UMETA(DisplayName = "挖掘类型"),
	IT_Attack UMETA(DisplayName = "攻击类型"),
};

//与角色交互模式（建造模式开关）保持一致的枚举
UENUM(BlueprintType)
enum EInteractMode :uint8
{
	IM_Normal UMETA(DisplayName = "正常"),
	IM_Construction UMETA(DisplayName = "建筑模式"),
};

//==================== 资源 ====================

//资源类型
UENUM(BlueprintType)
enum EResourceType : uint8
{
	RT_None UMETA(DisplayName = "None"),
	RT_Wood UMETA(DisplayName = "Wood"),
	RT_Stone UMETA(DisplayName = "Stone"),
	RT_Iron UMETA(DisplayName = "Iron"),
	RT_Gold UMETA(DisplayName = "Gold"),
	RT_Food UMETA(DisplayName = "Food"),
};

//资源状态
UENUM(BlueprintType)
enum EResourceState : uint8
{
	RS_Idle UMETA(DisplayName = "Idle"),
	RS_BeHit UMETA(DisplayName = "BeHit"),
	RS_Recover UMETA(DisplayName = "Recover"),
	RS_Death UMETA(DisplayName = "Death"),
};

//==================== 建筑 ====================

//建筑状态
UENUM(BlueprintType)
enum EBuildingState : uint8
{
	BS_Idle UMETA(DisplayName = "Idle"),
	BS_Rotate UMETA(DisplayName = "Rotate"),
	BS_PreAttack UMETA(DisplayName = "PreAttack"),
	BS_Attack UMETA(DisplayName = "Attack"),
	BS_PostAttack UMETA(DisplayName = "PostAttack"),
	BS_Death UMETA(DisplayName = "Death"),
};

//==================== 敌人 ====================

//敌人状态
UENUM(BlueprintType)
enum EEnemyState : uint8
{
	ES_Idle UMETA(DisplayName = "Idle"),
	ES_Move UMETA(DisplayName = "Move"),
	ES_PreAttack UMETA(DisplayName = "PreAttack"),
	ES_Attack UMETA(DisplayName = "Attack"),
	ES_PostAttack UMETA(DisplayName = "PostAttack"),
	ES_Death UMETA(DisplayName = "Death")
};

//==================== 伤害 ====================

//伤害目标类型
UENUM(BlueprintType)
enum ETargetType : int8
{
	Player UMETA(DisplayName = "玩家"),
	Enemy UMETA(DisplayName = "敌人"),
	Resource UMETA(DisplayName = "环境"),
	Building UMETA(DisplayName = "建筑"),
};

//==================== 波次 ====================

//波次状态
UENUM(BlueprintType)
enum ETotemState :int8
{
	WS_Preparatory UMETA(DisplayName = "准备阶段"),
	WS_Fight UMETA(DisplayName = "战斗阶段"),
	WS_Stop UMETA(DisplayName = "停止阶段"),
	WS_End UMETA(DisplayName = "结束阶段"),
};
