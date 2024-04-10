// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectCharacter.h"

#include "BlueprintEditor.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "PropertyPathHelpers.h"
#include "Channels/MovieSceneChannelTraits.h"
#include "Weapons/WeaponBase.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// AMTPSCharacter

AMyProjectCharacter::AMyProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = true;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = true;

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
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
 // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation

	ZoomedFOV = 65.0f;
	ZoomInterpSpeed = 20;

	//Create HealthComponent
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
}

void AMyProjectCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();
	HealthComponent->OnHealthChanged.AddDynamic(this, &AMyProjectCharacter::OnHealthChange);
	DefaultFOV = FollowCamera->FieldOfView;
	bIsSprinting = false;

	//Spawn a default weapon
	if(GetLocalRole() == ROLE_Authority)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(StarerWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
		if(CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "WeaponSocket");
		}
	}
	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AMyProjectCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	float TargetFOV = bWantsToZoom ? ZoomedFOV : DefaultFOV;
	float NewFOV = FMath::FInterpTo(FollowCamera->FieldOfView, TargetFOV, DeltaTime, ZoomInterpSpeed);
	
	FollowCamera->SetFieldOfView(NewFOV);
}

FVector AMyProjectCharacter::GetPawnViewLocation() const
{
	if(FollowCamera)
	{
		return FollowCamera->GetComponentLocation();
	}
	return Super::GetPawnViewLocation();
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Look);

		// Shooting
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Started, this, &AMyProjectCharacter::StartFire);
		EnhancedInputComponent->BindAction(ShootAction, ETriggerEvent::Completed, this, &AMyProjectCharacter::StopFire);
		// Sprinting
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AMyProjectCharacter::StartSprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AMyProjectCharacter::StopSprint);

		// Zooming
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Started, this, &AMyProjectCharacter::BeginZoom);
		EnhancedInputComponent->BindAction(ZoomAction, ETriggerEvent::Completed, this, &AMyProjectCharacter::EndZoom);

		// Reloading
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::Reloading);

		// First Weapon
		EnhancedInputComponent->BindAction(FirstWeaponAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::FirstWeapon);

		// Second Weapon
		EnhancedInputComponent->BindAction(SecondWeaponAction, ETriggerEvent::Triggered, this, &AMyProjectCharacter::SecondWeapon);
		
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void AMyProjectCharacter::OnHealthChange(UHealthComponent* HealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if(Health <= 0.0f && !bDied)
	{
		bDied = true;
		Dying();
	}
}

void AMyProjectCharacter::Move(const FInputActionValue& Value)
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

void AMyProjectCharacter::Look(const FInputActionValue& Value)
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

void AMyProjectCharacter::StartFire()
{
	if(CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AMyProjectCharacter::StopFire()
{
	if(CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AMyProjectCharacter::StartSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = 700.f;
	bIsSprinting = true;
}

void AMyProjectCharacter::StopSprint()
{
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	bIsSprinting = false;
}

void AMyProjectCharacter::BeginZoom()
{
	bWantsToZoom = true;
}

void AMyProjectCharacter::EndZoom()
{
	bWantsToZoom = false;
}

void AMyProjectCharacter::Reloading()
{
	if(CurrentWeapon)
	{
		CurrentWeapon->Reload();
	}
}

void AMyProjectCharacter::Dying()
{
	GetMovementComponent()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetSimulatePhysics(true);
	DetachFromControllerPendingDestroy();
	SetLifeSpan(10.0f);
}

void AMyProjectCharacter::FirstWeapon()
{
	if(CurrentWeapon)
	{
		AWeaponBase* OldWeapon = CurrentWeapon;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponArray[0], FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
		if(CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "WeaponSocket");
			OldWeapon->Destroy();
		}
	}
}

void AMyProjectCharacter::SecondWeapon()
{
	if(CurrentWeapon)
	{
		AWeaponBase* OldWeapon = CurrentWeapon;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(WeaponArray[1], FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
		if(CurrentWeapon)
		{
			CurrentWeapon->SetOwner(this);
			CurrentWeapon->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "WeaponSocket");
			OldWeapon->Destroy();
		}
	}
}

void AMyProjectCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AMyProjectCharacter, CurrentWeapon);
	DOREPLIFETIME(AMyProjectCharacter, bDied);
}

void AMyProjectCharacter::ServerDying_Implementation()
{
	Dying();
}

bool AMyProjectCharacter::ServerDying_Validate()
{
	return true;
}