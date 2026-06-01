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

	//角色初始化数值（仅服务端BeginPlay写入Manager表项；运行期数据以Manager为准）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float InitAttack = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	float InitInteractRange = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interact, meta = (AllowPrivateAccess = "true"))
	float InitPreInteractDelay = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interact, meta = (AllowPrivateAccess = "true"))
	float InitPostInteractDelay = 0.3f;

	UFUNCTION(Server,Reliable)
	void Server_Attack();

public:
	AHeroBuilderCharacter();

	//——状态/属性访问壳：内部转调UHB_CharacterSubsystem，保持外部Caller接口不破——
	UFUNCTION(BlueprintPure)
	TEnumAsByte<EPlayerCharacterState> GetCurrentState() const;

	void SetInteractTarget(AActor* Target);
	AActor* GetInteractTarget() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttack() const;
	void SetAttack(float NewAttack);

	void SetPreInteractDelay(float Delay);
	void SetPostInteractDelay(float Delay);

	//Move对外暴露（Subsystem的TickMoveToTarget会调用）
	void Move(const FInputActionValue& Value);

protected:

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			
	void ChangeConstructionMode(const FInputActionValue& Value);

	void Interact(const FInputActionValue& Value);
	//交互键抬起：负责路由到服务端中止交互流程
	void OnInteractReleased(const FInputActionValue& Value);

	//请求服务端切换交互模式
	UFUNCTION(Server, Reliable)
	void Server_SwitchInteractMode(uint8 NewMode);

	//请求服务端切换交互类型
	UFUNCTION(Server, Reliable)
	void Server_SwitchInteractType(uint8 NewMode);
	//请求服务端触发交互
	UFUNCTION(Server, Reliable)
	void Server_TryInteract();
	//请求服务端开启一次交互流程（按下交互键时由客户端发起）
	UFUNCTION(Server, Reliable)
	void Server_BeginInteract();
	//请求服务端中止当前交互流程（抬起交互键时由客户端发起）
	UFUNCTION(Server, Reliable)
	void Server_AbortInteract();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FVector GetFollowCameraForward();

private:
	UHB_CharacterSubsystem* GetCharacterSubsystem() const;
};

