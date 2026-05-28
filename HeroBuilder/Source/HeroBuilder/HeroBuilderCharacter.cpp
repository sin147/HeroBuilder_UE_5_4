// Copyright Epic Games, Inc. All Rights Reserved.

#include "HeroBuilderCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "Subsystems/HB_InteractSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogPlayerCharacter);

//////////////////////////////////////////////////////////////////////////
// AHeroBuilderCharacter

void AHeroBuilderCharacter::Server_Attack_Implementation()
{

}

void AHeroBuilderCharacter::TickUpdateState(float DeltaTime)
{
	// 交互相关状态不应被移动打断
	if (CurrentlyState == EPCS_PreInteract || CurrentlyState == EPCS_Interact || CurrentlyState == EPCS_PostInteract)
	{
		return;
	}

	if(!GetVelocity().IsNearlyZero())
	{
		SwitchState(EPCS_Move);
	}
	else
	{
		if (CurrentlyState == EPCS_Move)
		{
			SwitchState(EPCS_Idle);
		}
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
	case EPCS_PreInteract:
	{
		CurrentInteractDelay = PreInteractDelay;
		break;
	}
	case EPCS_Interact:
	{
		break;
	}
	case EPCS_PostInteract:
	{
		CurrentInteractDelay = PostInteractDelay;
		break;
	}
	default:
		break;
	}
	CurrentlyState = EnterState;
	const FString StateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)EnterState);
	UE_LOG(LogPlayerCharacter, Log, TEXT("'%s' Enter State '%s'!"), *GetNameSafe(this), *StateName);
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
	{
		GetCharacterMovement()->StopMovementImmediately();
		break;
	}
	case EPCS_PreInteract:
    {
        break;
    }
	case EPCS_Interact:
    {
        break;
    }
	case EPCS_PostInteract:
    {
        break;
    }
	default:
		break;
	}
	CurrentlyState = EPCS_None;
	const FString StateName = StaticEnum<EPlayerCharacterState>()->GetNameStringByValue((int64)LeaveState);
	UE_LOG(LogPlayerCharacter, Log, TEXT("'%s' Leave State '%s'!"), *GetNameSafe(this), *StateName);
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

void AHeroBuilderCharacter::SwitchState(EPlayerCharacterState NewState)
{
	if (CurrentlyState == NewState)
	{
		return;
	}

	OnLeaveState(CurrentlyState);
	OnEnterState(NewState);
}

void AHeroBuilderCharacter::SetPreInteractDelay(float Delay)
{
	PreInteractDelay = Delay;
}

void AHeroBuilderCharacter::SetPostInteractDelay(float Delay)
{
    PostInteractDelay = Delay;
}

float AHeroBuilderCharacter::GetAttack() const
{
	return Attack;
}

void AHeroBuilderCharacter::SetAttack(float NewAttack)
{
	Attack = NewAttack;
}

void AHeroBuilderCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
}

void AHeroBuilderCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if(HasAuthority())
	{
		TickUpdateState(DeltaTime);
		switch (CurrentlyState)
		{
		case EPCS_None:
			break;
		case EPCS_Idle:
			break;
		case EPCS_Move:
			break;
		case EPCS_PreInteract:
		{
			//前摇计时，结束后进入交互帧
			if (CurrentInteractDelay > 0.f)
			{
				CurrentInteractDelay -= DeltaTime;
			}
			else
			{
				SwitchState(EPCS_Interact);
			}
			break;
		}
		case EPCS_Interact:
		{
			//交互帧已在OnEnterState中执行，这里直接进入后摇
			SwitchState(EPCS_PostInteract);
			//到达交互帧：服务端调用交互逻辑
			Server_TryInteract();
			break;
		}
		case EPCS_PostInteract:
		{
			//后摇计时，结束后回到Idle
			if (CurrentInteractDelay > 0.f)
			{
				CurrentInteractDelay -= DeltaTime;
			}
			else
			{
				SwitchState(EPCS_Idle);
			}
			break;
		}
		default:
			break;
		}
	}
}

void AHeroBuilderCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AHeroBuilderCharacter, CurrentlyState);
	DOREPLIFETIME(AHeroBuilderCharacter, InteractTarget);
	DOREPLIFETIME(AHeroBuilderCharacter, CurrentInteractMode);
	DOREPLIFETIME(AHeroBuilderCharacter, Attack);

}

void AHeroBuilderCharacter::OnRep_CurrentInteractMode()
{
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
	//本地端通过Server RPC请求服务器切换交互模式
	UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>();
	if (!InteractSys)
	{
		return;
	}
	const EPlayerCharacterInteractMode CurrentMode = InteractSys->GetInteractMode(this);
	const EPlayerCharacterInteractMode NewMode = (CurrentMode == IM_ConstructionMode) ? IM_Normal : IM_ConstructionMode;
	Server_SwitchInteractMode(static_cast<uint8>(NewMode));
}

void AHeroBuilderCharacter::Interact(const FInputActionValue& Value)
{
	//仅在Idle/Move状态下可以起手交互，避免重复触发
	if (CurrentlyState != EPCS_Idle && CurrentlyState != EPCS_Move)
	{
		return;
	}
	SwitchState(EPCS_PreInteract);
}

void AHeroBuilderCharacter::Server_SwitchInteractMode_Implementation(uint8 NewMode)
{
	if (UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>())
	{
		InteractSys->SwitchInteractMode(this, static_cast<EPlayerCharacterInteractMode>(NewMode));
	}
}

void AHeroBuilderCharacter::Server_TryInteract_Implementation()
{
	if (UHB_InteractSubsystem* InteractSys = GetWorld()->GetSubsystem<UHB_InteractSubsystem>())
	{
		InteractSys->TryInteract(this);
	}
}