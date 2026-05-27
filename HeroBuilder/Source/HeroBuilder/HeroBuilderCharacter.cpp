// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "Subsystems/HB_ConstructionSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogPlayerCharacter);

//////////////////////////////////////////////////////////////////////////
// AHeroBuilderCharacter

void AHeroBuilderCharacter::OnEnterInteractMode_Implementation(EPlayerCharacterInteractMode EnterMode)
{
	switch (EnterMode)
	{
	case IM_None:
		break;
	case IM_Normal:
		break;
	case IM_ConstructionMode:
		GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>()->Server_ActiveConstructionMode(this);
		break;
	default:
		break;
	}
	CurrentlyInteractMode = EnterMode;
	UE_LOG(LogPlayerCharacter, Log, TEXT("'%s' Enter InteractMode '%d'!"), *GetNameSafe(this), EnterMode);
}

void AHeroBuilderCharacter::OnLeaveInteractMode_Implementation(EPlayerCharacterInteractMode LeaveMode)
{

	switch (LeaveMode)
	{
	case IM_None:
		break;
	case IM_Normal:
		break;
	case IM_ConstructionMode:
		GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>()->Server_CancelConstructionMode(this);
		break;
	default:
		break;
	}
	CurrentlyInteractMode = IM_None;
	UE_LOG(LogPlayerCharacter, Log, TEXT("'%s' Leave InteractMode '%d'!"), *GetNameSafe(this), LeaveMode);
}

void AHeroBuilderCharacter::Server_Attack_Implementation()
{

}

void AHeroBuilderCharacter::TickUpdateState(float DeltaTime)
{
	if(!GetVelocity().IsNearlyZero())
	{
		SwitchState(EPCS_Move);
	}

}

void AHeroBuilderCharacter::OnEnterState_Implementation(EPlayerCharacterState EnterState)
{
	switch (EnterState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_PreAttack:
	{
		GetCharacterMovement()->StopMovementImmediately();
		break;
	}
	case EPCS_Attack:
		break;
	case EPCS_PostAttack:
		break;
	default:
		break;
	}
}

void AHeroBuilderCharacter::OnLeaveState_Implementation(EPlayerCharacterState LeaveState)
{
	switch (LeaveState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_PreAttack:
		break;
	case EPCS_Attack:
		break;
	case EPCS_PostAttack:
		break;
	default:
		break;
	}
}

AHeroBuilderCharacter::AHeroBuilderCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)
}

void AHeroBuilderCharacter::SwitchInteractMode(EPlayerCharacterInteractMode NewMode)
{
	if (CurrentlyInteractMode == NewMode)
	{
		return;
	}
	OnLeaveInteractMode(CurrentlyInteractMode);
	OnEnterInteractMode(NewMode);
	CurrentlyInteractMode = NewMode;
}

void AHeroBuilderCharacter::SwitchState(EPlayerCharacterState NewState)
{
	if (CurrentlyState == NewState)
	{
		return;
	}
	OnLeaveState(CurrentlyState);
	OnEnterState(NewState);


}

void AHeroBuilderCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void AHeroBuilderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	

	switch (CurrentlyState)
	{
	case EPCS_None:
		break;
	case EPCS_Idle:
		break;
	case EPCS_Move:
		break;
	case EPCS_PreAttack:
		break;
	case EPCS_Attack:
		break;
	case EPCS_PostAttack:
		break;
	default:
		break;
	}
}

void AHeroBuilderCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroBuilderCharacter, CurrentlyInteractMode);
	DOREPLIFETIME(AHeroBuilderCharacter, CurrentlyState);

}

FVector AHeroBuilderCharacter::GetFollowCameraForward()
{
	return FollowCamera->GetForwardVector();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AHeroBuilderCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHeroBuilderCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHeroBuilderCharacter::Look);

		// Construction Mode
        EnhancedInputComponent->BindAction(ChangeConstructionModeAction, ETriggerEvent::Started, this, &AHeroBuilderCharacter::ChangeConstructionMode);

		// Interact
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AHeroBuilderCharacter::Interact);
	}
	else
	{
		UE_LOG(LogPlayerCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AHeroBuilderCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AHeroBuilderCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AHeroBuilderCharacter::ChangeConstructionMode(const FInputActionValue& Value)
{
	if (CurrentlyInteractMode == IM_ConstructionMode)
	{
		SwitchInteractMode(IM_Normal);
	}
	else
	{
		SwitchInteractMode(IM_ConstructionMode);
	}

}

void AHeroBuilderCharacter::Interact(const FInputActionValue& Value)
{
	switch (CurrentlyInteractMode)
	{
	case IM_None:
	{
		
		break;
	}
	case IM_ConstructionMode:
	{
		Server_ConstructionBegin(this);
		break;
	}
	default:
		break;
	}
}

void AHeroBuilderCharacter::Server_ConstructionBegin_Implementation(ACharacter*InCharacter)
{
	if (IsValid(InCharacter))
	{
		GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>()->ConstructionBegin(InCharacter);
	}

}

void AHeroBuilderCharacter::Server_ConstructionMode_Implementation(bool bEnable)
{
	if (bEnable)
	{
		GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>()->Server_ActiveConstructionMode(this);
	}
	else
	{
		GetWorld()->GetSubsystem<UHB_ConstructionSubsystem>()->Server_CancelConstructionMode(this);
	}
}
