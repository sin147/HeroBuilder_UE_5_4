// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "HeroBuilderCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogPlayerCharacter, Log, All);

UENUM(BlueprintType)
enum EPlayerCharacterInteractMode:uint8
{
	IM_None ,
	IM_Normal,
    IM_ConstructionMode,
};
UENUM(BlueprintType)
enum EPlayerCharacterState :uint8
{
	EPCS_None,
	EPCS_Idle,
	EPCS_Move,
	EPCS_PreAttack,
	EPCS_Attack,
    EPCS_PostAttack,
};

UCLASS(config=Game)
class AHeroBuilderCharacter : public ACharacter
{
	GENERATED_BODY()
	UPROPERTY(Replicated)
	TEnumAsByte<EPlayerCharacterInteractMode> CurrentlyInteractMode= IM_Normal;
	UPROPERTY(Replicated)
	TEnumAsByte<EPlayerCharacterState> CurrentlyState = EPCS_Idle;

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
	UFUNCTION(Server, Reliable)
	void OnEnterInteractMode(EPlayerCharacterInteractMode EnterMode);
	UFUNCTION(Server, Reliable)
	void OnLeaveInteractMode(EPlayerCharacterInteractMode LeaveMode);
	UFUNCTION(Server,Reliable)
	void Server_Attack();
	void TickUpdateState(float DeltaTime);
	UFUNCTION(Server, Reliable)
	void OnEnterState(EPlayerCharacterState EnterState);
	UFUNCTION(Server, Reliable)
	void OnLeaveState(EPlayerCharacterState LeaveState);
public:
	AHeroBuilderCharacter();
	void SwitchInteractMode(EPlayerCharacterInteractMode NewMode);
	void SwitchState(EPlayerCharacterState NewState);

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			
	void ChangeConstructionMode(const FInputActionValue& Value);

	void Interact(const FInputActionValue& Value);

	UFUNCTION(Server,Reliable)
	void Server_ConstructionMode(bool bEnable);
	UFUNCTION(Server, Reliable)
	void Server_ConstructionBegin(ACharacter* InCharacter);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	// To add mapping context
	virtual void BeginPlay();
	virtual void Tick(float DeltaTime) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	FVector GetFollowCameraForward();
};

