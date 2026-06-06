// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Config/InteractData.h"
#include "Components/HB_DamageComponent.h"
#include "Manager/HB_CharacterManager.h"
#include "HeroBuilderCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UHB_CharacterSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogPlayerCharacter, Log, All);

UCLASS(config=Game)
class AHeroBuilderCharacter : public ACharacter
{
	GENERATED_BODY()
	friend class UHB_CharacterSubsystem;
	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;
	
	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ChangeConstructionModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	UFUNCTION(Server,Reliable)
	void Server_Attack();

public:
	AHeroBuilderCharacter();

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttack() const;
	void SetAttack(float NewAttack);

	//Move对外暴露（Subsystem的TickMoveToTarget会调用）
	void Move(const FInputActionValue& Value);

protected:
	bool CanMove();
	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			
	void ChangeConstructionMode(const FInputActionValue& Value);
	//请求服务端切换建造模式（在 IM_Normal 与 IM_Construction 之间切换）
	UFUNCTION(Server, Reliable)
	void Server_ChangeConstructionMode();
	//通知 Owning Client 切换建造模式（客户端本地反馈/状态刷新）
	UFUNCTION(Client, Reliable)
	void Client_ChangeConstructionMode();
	UFUNCTION(NetMulticast,Reliable)
	void Multicast_ChangeConstructionMode();
	/******************************************************/
	void OnInteractPressed(const FInputActionValue& Value);
	//请求服务端开启一次交互流程（按下交互键时由客户端发起）
	UFUNCTION(Server, Reliable)
	void Server_BeginInteract();
	UFUNCTION(Client, Reliable)
	void Client_BeginInteract();
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BeginInteract();
	//交互键抬起：负责路由到服务端中止交互流程
	void OnInteractReleased(const FInputActionValue& Value);
	//请求服务端中止当前交互流程（抬起交互键时由客户端发起）
	UFUNCTION(Server, Reliable)
	void Server_AbortInteract();
	UFUNCTION(Client, Reliable)
	void Client_AbortInteract();
	UFUNCTION(NetMulticast, Reliable)
    void Multicast_AbortInteract();
	/******************************************************/


protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UHB_CharacterSubsystem* GetCharacterSubsystem() const;
};

