// Fill out your copyright notice in the Description page of Project Settings.


#include "ArenaBallCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "MenuSystem/ArenaComponents/CombatComponent.h"
#include "MenuSystem/ArenaComponents/BuffComponent.h"
#include "MenuSystem/Weapon/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "MenuSystem/MenuSystem.h"
#include "MenuSystem/PlayerController/ArenaPlayerController.h"
#include "MenuSystem/GameMode/ArenaGameMode.h"
#include "Particles/ParticleSystemComponent.h"
#include "MenuSystem/Weapon/WeaponTypes.h"
#include "MenuSystem/ArenaTypes/CombatState.h"


AArenaBallCharacter::AArenaBallCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>(TEXT("BuffComponent"));
	Buff->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.f;
	MinNetUpdateFrequency = 33.f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimelineComponent"));

	AttachedBall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Attached Ball"));
	AttachedBall->SetupAttachment(GetMesh(), FName("BallSocket"));
}

void AArenaBallCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AArenaBallCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(AArenaBallCharacter, Health);
	DOREPLIFETIME(AArenaBallCharacter, bDisableGameplay);
}

void AArenaBallCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ArenaMappingContext, 0);
		}
	}
}

void AArenaBallCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHudHealth();
	if (HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AArenaBallCharacter::ReceiveDamage);
	}
	
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(ArenaMappingContext, 0);
		}
	}
	if (AttachedBall)
	{
		AttachedBall->SetVisibility(false);
	}
}
void AArenaBallCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	AimOffset(DeltaTime);
	HideCameraIfCharacterClose();
	PollInit();
}

void AArenaBallCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
	if (Buff)
	{
		Buff->Character = this;
		Buff->SetInitialSpeeds(
			GetCharacterMovement()->MaxWalkSpeed,
			GetCharacterMovement()->MaxWalkSpeedCrouched);
	}
}

void AArenaBallCharacter::PlayFireMontage(bool bAiming)
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && FireWeaponMontage)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName;
		SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AArenaBallCharacter::PlayReloadMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (Combat->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_CatLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Rifle");
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("Rifle");
			break;
		}
		
		AnimInstance->Montage_JumpToSection(SectionName);
	}	
}

void AArenaBallCharacter::PlayElimMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ElimMontage)
	{
		AnimInstance->Montage_Play(ElimMontage);
	}	
}

void AArenaBallCharacter::PlayThowGrenadeMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ThrowGrenadeMontage)
	{
		AnimInstance->Montage_Play(ThrowGrenadeMontage);
	}		
}

void AArenaBallCharacter::PlayShootBallMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ShootBallMontage)
	{
		AnimInstance->Montage_Play(ShootBallMontage);
	}	
}

void AArenaBallCharacter::Elim()
{
	if (Combat && Combat->EquippedWeapon)
	{
		Combat->EquippedWeapon->Dropped();
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(
		ElimTimer,
		this,
		&AArenaBallCharacter::ElimTimerFinished,
		ElimDelay
	);
}

void AArenaBallCharacter::MulticastElim_Implementation()
{
	if (ArenaPlayerController)
	{
		ArenaPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();

	//Starting dissolve effect
	if (DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.56f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();

	//Disable character movement
	bDisableGameplay = true;
	GetCharacterMovement()->DisableMovement();
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
	// Disablke collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Spawn elim bot
	if (ElimBotEffect)
	{
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ElimBotEffect,
			ElimBotSpawnPoint,
			GetActorRotation()
			);
	}
	if (ElimBotSound)
	{
		UGameplayStatics::SpawnSoundAtLocation(
			this,
			ElimBotSound,
			GetActorLocation()
		);
	}
}

void AArenaBallCharacter::ElimTimerFinished()
{
	AArenaGameMode* ArenaGameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
	if (ArenaGameMode)
	{
		ArenaGameMode->RequestRespawn(this, Controller);
	}
}

void AArenaBallCharacter::Destroyed()
{
	Super::Destroyed();
	if (ElimBotComponent)
	{
		ElimBotComponent->DestroyComponent();
	}

	AArenaGameMode* ArenaGameMode = Cast<AArenaGameMode>(UGameplayStatics::GetGameMode(this));
	bool bMatchNotInProgress = ArenaGameMode && ArenaGameMode->GetMatchState() != MatchState::InProgress;
	
	if (Combat && Combat->EquippedWeapon && bMatchNotInProgress)
	{
		Combat->EquippedWeapon->Destroy();
	}
}

void AArenaBallCharacter::PlayHitReactMontage()
{
	if (Combat == nullptr || Combat->EquippedWeapon == nullptr)return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void AArenaBallCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHudHealth();
	PlayHitReactMontage();

	if (Health == 0.f)
	{
		AArenaGameMode* ArenaGameMode = GetWorld()->GetAuthGameMode<AArenaGameMode>();
        	if (ArenaGameMode)
        	{
        		ArenaPlayerController = ArenaPlayerController == nullptr ? Cast<AArenaPlayerController>(Controller) : ArenaPlayerController;	
                AArenaPlayerController* AttackerController = Cast<AArenaPlayerController>(InstigatorController);
        		ArenaGameMode->PlayerEliminated(this, ArenaPlayerController, AttackerController);
        	}
	}
}

void AArenaBallCharacter::Move(const FInputActionValue& Value)
{
	if (bDisableGameplay) return;
	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, MovementVector.Y);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void AArenaBallCharacter::Look(const FInputActionValue& Value)
{
	const FVector2d LookAxisValue = Value.Get<FVector2d>();
	if (GetController())
	{
		AddControllerYawInput(LookAxisValue.X);
		AddControllerPitchInput(LookAxisValue.Y);
	}
}

void AArenaBallCharacter::EKeyPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		if (HasAuthority())
		{
			Combat->EquipWeapon(OverlappingWeapon);	
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void AArenaBallCharacter::ItemAttack()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->ThrowGrenade();
	}
}

void AArenaBallCharacter::ShootBall()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->ShootBasketball();
	}
}

void AArenaBallCharacter::Dodge()
{
	if (bDisableGameplay) return;
}

void AArenaBallCharacter::CrouchButtonPressed()
{
	if (bDisableGameplay) return;
	if (!bIsCrouched)
	{
		Crouch();
	}
	else
	{
		UnCrouch();	
	}
}

void AArenaBallCharacter::ReloadButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->Reload();
	}
}

void AArenaBallCharacter::AimButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void AArenaBallCharacter::AimButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

void AArenaBallCharacter::AimOffset(float DeltaTime)
{
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Speed = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	if (Speed == 0.f && !bIsInAir) // standing still, not jumping
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Pitch,0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Pitch = DeltaAimRotation.Pitch;
		if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
		{
			InterpAO_Pitch = AO_Pitch;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaTime);
		
	}
	if (Speed > 0.f || bIsInAir) //running or jumping
	{
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Pitch,0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}
	
	AO_Pitch = GetBaseAimRotation().Pitch;
	if (AO_Pitch > 90.f && !IsLocallyControlled())
	{
		// map pitch from [270, 360) to [-90, 0)
		FVector2d InRange(270.f, 360.f);
		FVector2d OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AArenaBallCharacter::FireButtonPressed()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->FireButtonPressed(true);
	}
}

void AArenaBallCharacter::FireButtonReleased()
{
	if (bDisableGameplay) return;
	if (Combat)
	{
		Combat->FireButtonPressed(false);
	}
}

void AArenaBallCharacter::TurnInPlace(float DeltaTime)
{
	if (AO_Pitch > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Pitch < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
	{
		InterpAO_Pitch = FMath::FInterpTo(InterpAO_Pitch, 0.f, DeltaTime, 10.f);
		AO_Pitch = InterpAO_Pitch;
		if (FMath::Abs(AO_Pitch) < 15.f)
		{
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Pitch,0.f);
		}
	}
	/*if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	*/
}

void AArenaBallCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}	
}

void AArenaBallCharacter::HideCameraIfCharacterClose()
{
	if (!IsLocallyControlled()) return;
	if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
	{
		GetMesh()->SetVisibility(false);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	}
	else
	{
		GetMesh()->SetVisibility(true);
		if (Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh())
		{
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void AArenaBallCharacter::OnRep_Health(float LastHealth)
{
	UpdateHudHealth();
	if (Health < LastHealth)
	{
		PlayHitReactMontage();
	}
}

void AArenaBallCharacter::UpdateHudHealth()
{
	ArenaPlayerController = ArenaPlayerController == nullptr ? Cast<AArenaPlayerController>(Controller) : ArenaPlayerController;
	if (ArenaPlayerController)
	{
		ArenaPlayerController->SetHUDHealth(Health, MaxHealth);
	}	
}

void AArenaBallCharacter::PollInit()
{
	if (ArenaPlayerState == nullptr)
	{
		ArenaPlayerState = GetPlayerState<AArenaPlayerState>();
		if (ArenaPlayerState)
		{
			ArenaPlayerState->AddToScore(0.f);
			ArenaPlayerState->AddToDefeats(0);
		}
	}
}

void AArenaBallCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if (DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void AArenaBallCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AArenaBallCharacter::UpdateDissolveMaterial);
	if (DissolveCurve && DissolveTimeline)
	{
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void AArenaBallCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}	
	OverlappingWeapon = Weapon;
	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

void AArenaBallCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}
	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

// Called to bind functionality to input
void AArenaBallCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::Move);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::Look);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::Jump);
		EnhancedInputComponent->BindAction(EKeyAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::EKeyPressed);
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::Dodge);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::CrouchButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::AimButtonPressed);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AArenaBallCharacter::AimButtonReleased);
		EnhancedInputComponent->BindAction(ItemAttackAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::ItemAttack);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::FireButtonPressed);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AArenaBallCharacter::FireButtonReleased);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::ReloadButtonPressed);
		EnhancedInputComponent->BindAction(ShootBallAction, ETriggerEvent::Triggered, this, &AArenaBallCharacter::ShootBall);
	}
}
bool AArenaBallCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool AArenaBallCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}

AWeapon* AArenaBallCharacter::GetEquippedWeapon()
{
	if (Combat == nullptr) return nullptr;
	return Combat->EquippedWeapon;
}

FVector AArenaBallCharacter::GetHitTarget() const
{
	if (Combat == nullptr) return FVector();
	return Combat->HitTarget;
}

ECombatState AArenaBallCharacter::GetCombatState() const
{
	if (Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;	
}