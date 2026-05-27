// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Component/HB_InteractComponent.h"
#include "HeroBuilderCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogPlayerCharacter, Log, All);


UENUM(BlueprintType)
enum EPlayerCharacterState :uint8
{
	EPCS_None,
	EPCS_Idle,
	EPCS_Move,
	EPCS_PreInteract,
	EPCS_Interact,
    EPCS_PostInteract,
};

UCLASS(config=Game)
class AHeroBuilderCharacter : public ACharacter
{
	GENERATED_BODY()
	UPROPERTY(Replicated)
	TEnumAsByte<EPlayerCharacterState> CurrentlyState = EPCS_Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHB_InteractComponent> InteractComponent;

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

	//交互前摇时间
	UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = Interact, meta = (AllowPrivateAccess = "true"))
	float PreInteractDelay = 0.3f;
	//交互后摇时间
    UPROPERTY(Replicated,EditAnywhere, BlueprintReadWrite, Category = Interact, meta = (AllowPrivateAccess = "true"))
	float PostInteractDelay = 0.3f;
	//内部用于前/后摇倒计时
	float CurrentInteractDelay = 0.f;

	UFUNCTION(Server,Reliable)
	void Server_Attack();
	void TickUpdateState(float DeltaTime);
	UFUNCTION(Server, Reliable)
	void OnEnterState(EPlayerCharacterState EnterState);
	UFUNCTION(Server, Reliable)
	void OnLeaveState(EPlayerCharacterState LeaveState);
public:
	AHeroBuilderCharacter();
	void SwitchState(EPlayerCharacterState NewState);
    void SetPreInteractDelay(float Delay);
    void SetPostInteractDelay(float Delay);
	UFUNCTION(BlueprintPure)
	TEnumAsByte<EPlayerCharacterState> GetCurrentState() const { return CurrentlyState; }

protected:

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
			
	void ChangeConstructionMode(const FInputActionValue& Value);

	void Interact(const FInputActionValue& Value);

	//请求服务端切换交互模式
	UFUNCTION(Server, Reliable)
	void Server_SwitchInteractMode(uint8 NewMode);
	//请求服务端触发交互
	UFUNCTION(Server, Reliable)
	void Server_TryInteract();

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

