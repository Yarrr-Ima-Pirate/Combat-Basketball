// Fill out your copyright notice in the Description page of Project Settings.


#include "ArenaCharAnimInstance.h"
#include "ArenaBallCharacter.h"
//#include "SAdvancedRotationInputBox.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "MenuSystem/Weapon/Weapon.h"

void UArenaCharAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	ArenaBallCharacter = Cast<AArenaBallCharacter>(TryGetPawnOwner());
}

void UArenaCharAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (ArenaBallCharacter == nullptr)
	{
		ArenaBallCharacter = Cast<AArenaBallCharacter>(TryGetPawnOwner()); 
	}
	if (ArenaBallCharacter == nullptr) return;

	FVector Velocity = ArenaBallCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = ArenaBallCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = ArenaBallCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = ArenaBallCharacter->IsWeaponEquipped();
	EquippedWeapon = ArenaBallCharacter->GetEquippedWeapon();
	bIsCrouched = ArenaBallCharacter->bIsCrouched;
	bAiming = ArenaBallCharacter->IsAiming();
	TurningInPlace = ArenaBallCharacter->GetTurningInPlace();
	bElimmed = ArenaBallCharacter->IsElimmed();

	//Yaw offset for strafing
	FRotator AimRotation = ArenaBallCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(ArenaBallCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime,15.f);
	YawOffset = DeltaRotation.Yaw;

	AO_Yaw = ArenaBallCharacter->GetAO_Yaw();
	AO_Pitch = ArenaBallCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && ArenaBallCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		ArenaBallCharacter->GetMesh()->TransformToBoneSpace(FName("RightHand"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));
		
		if (ArenaBallCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("RightHand"), RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - ArenaBallCharacter->GetHitTarget()));
            
			LookAtRotation.Roll += ArenaBallCharacter->RightHandRotationRoll;
			LookAtRotation.Yaw += ArenaBallCharacter->RightHandRotationYaw;
			LookAtRotation.Pitch += ArenaBallCharacter->RightHandRotationPitch;
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f); 
		}
	}

	bUseFABRIK = ArenaBallCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	bUseAimOffsets = ArenaBallCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !ArenaBallCharacter->GetDisableGameplay();
	bTransformRightHand = ArenaBallCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !ArenaBallCharacter->GetDisableGameplay(); 
}
