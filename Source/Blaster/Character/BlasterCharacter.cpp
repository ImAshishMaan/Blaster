#include "BlasterCharacter.h"

#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Blaster/Blaster.h"
#include "Blaster/BlasterComponents/BuffComponent.h"
#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/GameMode/BlasterGameMode.h"
#include "Blaster/PlayerController/BlasterPlayerController.h"
#include "Blaster/PlayerState/BlasterPlayerState.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

ABlasterCharacter::ABlasterCharacter() {
	PrimaryActorTick.bCanEverTick = true;
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetMesh());
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	Combat = CreateDefaultSubobject<UCombatComponent>("CombatComponent");
	Combat->SetIsReplicated(true);

	Buff = CreateDefaultSubobject<UBuffComponent>("BuffComponent");
	Buff->SetIsReplicated(true);

	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 850.0f, 0.0f);

	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	NetUpdateFrequency = 66.0f;
	MinNetUpdateFrequency = 33.0f;

	DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>("DissolveTimeline");
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
	DOREPLIFETIME(ABlasterCharacter, Health);
	DOREPLIFETIME(ABlasterCharacter, Shield);
	DOREPLIFETIME(ABlasterCharacter, bDisableGameplay);
}

void ABlasterCharacter::PostInitializeComponents() {
	Super::PostInitializeComponents();

	if(Combat) {
		Combat->Character = this;
	}
	if(Buff) {
		Buff->Character = this;
		Buff->SetInitialSpeeds(GetCharacterMovement()->MaxWalkSpeed, GetCharacterMovement()->MaxWalkSpeedCrouched);
	}
}

void ABlasterCharacter::PlayFireMontage(bool bAiming) {
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage) {
		AnimInstance->Montage_Play(FireWeaponMontage);
		FName SectionName = bAiming ? FName("RifleAim") : FName("RifleHip");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::PlayReloadMontage() {
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ReloadMontage) {
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch(Combat->EquippedWeapon->GetWeaponType()) {
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Pistol");
			break;
		case EWeaponType::EWT_SubmachineGun:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_Shotgun:
			SectionName = FName("Shotgun");
			break;
		case EWeaponType::EWT_SniperRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_GrenadeLauncher:
			SectionName = FName("Rifle");
			break;
		default: ;
		}

		AnimInstance->Montage_JumpToSection(SectionName);
	}
}


void ABlasterCharacter::PlayElimMontage() {
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && ElimMontage) {
		AnimInstance->Montage_Play(ElimMontage);
	}
}

void ABlasterCharacter::Destroyed() {
	Super::Destroyed();

	if(ElimBotComponent) {
		ElimBotComponent->DestroyComponent();
	}
}

void ABlasterCharacter::PlayHitReactMontage() {
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HitReactMontage) {
		AnimInstance->Montage_Play(HitReactMontage);
		FName SectionName = FName("FromFront");
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) {
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ABlasterCharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);

	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ABlasterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ABlasterCharacter::FireButtonReleased);
	PlayerInputComponent->BindAction("Reload", IE_Pressed, this, &ABlasterCharacter::ReloadButtonPressed);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);
}


void ABlasterCharacter::OnRep_ReplicatedMovement() {
	Super::OnRep_ReplicatedMovement();

	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f;
}


void ABlasterCharacter::BeginPlay() {
	Super::BeginPlay();

	SpawnDefaultWeapon();
	UpdateHUDAmmo();
	UpdateHUDHealth();
	UpdateHUDShield();
	if(HasAuthority()) {
		OnTakeAnyDamage.AddDynamic(this, &ABlasterCharacter::ReceiveDamage);
	}
}

void ABlasterCharacter::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);

	RotateInPlace(DeltaTime);

	HideCameraIfCharacterClose();
	PollInit();
}

void ABlasterCharacter::RotateInPlace(float DeltaTime) {
	if(bDisableGameplay) {
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}
	if(GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled()) {
		AimOffset(DeltaTime);
	} else {
		TimeSinceLastMovementReplication += DeltaTime;
		if(TimeSinceLastMovementReplication > 0.25f) {
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}


void ABlasterCharacter::MoveForward(float Value) {
	if(bDisableGameplay) return;
	if(Controller != nullptr && Value != 0.0f) {
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value) {
	if(bDisableGameplay) return;
	if(Controller != nullptr && Value != 0.0f) {
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value) {
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value) {
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed() {
	if(bDisableGameplay) return;
	if(Combat) {
		ServerEquipButtonPressed();
	}
}

void ABlasterCharacter::CrouchButtonPressed() {
	if(bDisableGameplay) return;
	if(bIsCrouched) {
		UnCrouch();
	} else {
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed() {
	if(bDisableGameplay) return;
	if(Combat) {
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased() {
	if(bDisableGameplay) return;
	if(Combat) {
		Combat->SetAiming(false);
	}
}


void ABlasterCharacter::CalculateSpeed(float& Speed) {
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.0f;
	Speed = Velocity.Size();
}

void ABlasterCharacter::AimOffset(float DeltaTime) {
	if(Combat && Combat->EquippedWeapon == nullptr) return;

	float Speed;
	CalculateSpeed(Speed);
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	if(Speed == 0 && !bIsInAir) {
		bRotateRootBone = true;
		FRotator CurrentAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		// Store the current rotation of the character()
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if(TurningInPlace == ETurningInPlace::ETIP_NotTurning) {
			InterpAO_Yaw = AO_Yaw; // If the character is not turning
		}
		bUseControllerRotationYaw = true;

		TurnInPlace(DeltaTime);
	}

	if(Speed > 0 || bIsInAir) {
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		// Store the starting rotation of the character()
		AO_Yaw = 0.0f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
	}

	CalculateAO_Pitch();
}

void ABlasterCharacter::CalculateAO_Pitch() {
	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch > 90.0f && !IsLocallyControlled()) {
		// Map pitch from range 270 - 360 to range -90 - 0
		FVector2D InRange = FVector2D(270.0f, 360.0f);
		FVector2D OutRange = FVector2D(-90.0f, 0.0f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::SimProxiesTurn() {
	if(Combat == nullptr || Combat->EquippedWeapon == nullptr) return;
	bRotateRootBone = false;

	float Speed;
	CalculateSpeed(Speed);

	if(Speed > 0.0f) {
		TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		return;
	}

	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	//UE_LOG(LogTemp, Warning, TEXT("Proxy Rotation: %f"), ProxyYaw);

	if(FMath::Abs(ProxyYaw) > TurnThreshold) {
		if(ProxyYaw > TurnThreshold) {
			TurningInPlace = ETurningInPlace::ETIP_Right;
		} else if(ProxyYaw < -TurnThreshold) {
			TurningInPlace = ETurningInPlace::ETIP_Left;
		} else {
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::Jump() {
	if(bIsCrouched) {
		UnCrouch();
	} else {
		Super::Jump();
	}
}

void ABlasterCharacter::FireButtonPressed() {
	if(bDisableGameplay) return;
	if(Combat) {
		Combat->FireButtonPressed(true);
	}
}

void ABlasterCharacter::FireButtonReleased() {
	if(bDisableGameplay) return;
	if(Combat) {
		Combat->FireButtonPressed(false);
	}
}

void ABlasterCharacter::ReloadButtonPressed() {
	if(bDisableGameplay) return;
	if(Combat) {
		Combat->Reload();
	}
}


void ABlasterCharacter::TurnInPlace(float DeltaTime) {
	//UE_LOG(LogTemp, Warning, TEXT("AO_Yaw: %f"), AO_Yaw);
	if(AO_Yaw > 90.0f) {
		TurningInPlace = ETurningInPlace::ETIP_Right;
	} else if(AO_Yaw < -90.0f) {
		TurningInPlace = ETurningInPlace::ETIP_Left;
	}
	if(TurningInPlace != ETurningInPlace::ETIP_NotTurning) {
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.0f, DeltaTime, 4.0f);
		AO_Yaw = InterpAO_Yaw;
		if(FMath::Abs(AO_Yaw) < 15.0f) {
			TurningInPlace = ETurningInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.0f, GetBaseAimRotation().Yaw, 0.0f);
		}
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation() {
	if(Combat) {
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon) {
	if(OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if(LastWeapon) {
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::HideCameraIfCharacterClose() {
	if(!IsLocallyControlled()) return;

	if((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold) {
		GetMesh()->SetVisibility(false);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) {
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = true;
		}
	} else {
		GetMesh()->SetVisibility(true);
		if(Combat && Combat->EquippedWeapon && Combat->EquippedWeapon->GetWeaponMesh()) {
			Combat->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = false;
		}
	}
}

void ABlasterCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
                                      AController* InstigatedBy, AActor* DamageCauser) {
	if(bElimmed) return;

	float DamageToHealth = Damage;
	if(Shield > 0) {
		if(Shield >= Damage) {
			Shield = Shield - Damage;
			DamageToHealth = 0.0f;
		} else {
			DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.0f, Damage);
			Shield = 0.0f;
		}
	}

	Health = FMath::Clamp(Health - DamageToHealth, 0.0f, MaxHealth);
	PlayHitReactMontage();
	UpdateHUDHealth();
	UpdateHUDShield();

	if(Health == 0.0f) {
		ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
		if(BlasterGameMode) {
			BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) :
				                          BlasterPlayerController;
			ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatedBy);
			BlasterGameMode->PlayerEliminated(this, BlasterPlayerController, AttackerController);
		}
	}
}

void ABlasterCharacter::PollInit() {
	if(BlasterPlayerState == nullptr) {
		BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
		if(BlasterPlayerState) {
			BlasterPlayerState->AddToScore(0.0f);
			BlasterPlayerState->AddToDefeats(0);
		}
	}
}


void ABlasterCharacter::OnRep_Health(float OldHealth) {
	if(Health < OldHealth) {
		PlayHitReactMontage();
	}
	UpdateHUDHealth();
}

void ABlasterCharacter::OnRep_Shield(float OldShield) {
	if(Shield < OldShield) {
		PlayHitReactMontage();
	}
	UpdateHUDShield();
}

void ABlasterCharacter::UpdateHUDHealth() {
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) :
		                          BlasterPlayerController;
	if(BlasterPlayerController) {
		BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
	}
}

void ABlasterCharacter::UpdateHUDShield() {
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) :
		                          BlasterPlayerController;
	if(BlasterPlayerController) {
		BlasterPlayerController->SetHUDShield(Shield, MaxShield);
	}
}

void ABlasterCharacter::UpdateHUDAmmo() {
	BlasterPlayerController = BlasterPlayerController == nullptr ? Cast<ABlasterPlayerController>(Controller) :
		                          BlasterPlayerController;
	if(BlasterPlayerController && Combat && Combat->EquippedWeapon) {
		BlasterPlayerController->SetHUDWeaponAmmo(Combat->EquippedWeapon->GetAmmo());
		BlasterPlayerController->SetHUDCarriedAmmo(Combat->CarriedAmmo);
	}
}

void ABlasterCharacter::SpawnDefaultWeapon() {
	ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
	UWorld* World = GetWorld();
	if(BlasterGameMode && World && !bElimmed && DefaultWeaponClass && Combat) {
		AWeapon* StartingWeapon = World->SpawnActor<AWeapon>(DefaultWeaponClass);
		StartingWeapon->bDestroyWeapon = true;
		Combat->EquipWeapon(StartingWeapon);
	}
}

void ABlasterCharacter::Elim() {
	if(Combat && Combat->EquippedWeapon) {
		if(Combat->EquippedWeapon->bDestroyWeapon) {
			Combat->EquippedWeapon->Destroy();
		}else {
			Combat->EquippedWeapon->Dropped();
		}
	}
	MulticastElim();
	GetWorldTimerManager().SetTimer(ElimTimer, this, &ABlasterCharacter::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::MulticastElim_Implementation() {
	if(BlasterPlayerController) {
		BlasterPlayerController->SetHUDWeaponAmmo(0);
	}
	bElimmed = true;
	PlayElimMontage();

	// Start Dissolve Effect
	if(DisolveMaterialInstance) {
		DynamicDisolveMaterialInstance = UMaterialInstanceDynamic::Create(DisolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDisolveMaterialInstance);
		DynamicDisolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDisolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.0f);
	}
	StartDissolve();

	// Disable Character Movement
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	bDisableGameplay = true;
	if(Combat) {
		Combat->FireButtonPressed(false);
	}
	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Spawn Elim Bot
	if(ElimBotEffect) {
		FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.0f);
		ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnPoint,
		                                                            GetActorLocation().Rotation());

		if(ElimBotSound) {
			UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
		}
	}

	if(IsLocallyControlled() && Combat && Combat->bAiming && Combat->EquippedWeapon && Combat->EquippedWeapon->
		GetWeaponType() == EWeaponType::EWT_SniperRifle) {
		ShowSniperScopeWidget(false);
	}
}

void ABlasterCharacter::ElimTimerFinished() {
	ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
	if(BlasterGameMode) {
		BlasterGameMode->RequestRespawn(this, Controller);
	}
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue) {
	if(DynamicDisolveMaterialInstance) {
		DynamicDisolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ABlasterCharacter::StartDissolve() {
	DissolveTrack.BindDynamic(this, &ABlasterCharacter::UpdateDissolveMaterial);
	if(DissolveCurve && DissolveTimeline) {
		DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeline->Play();
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon) {
	if(OverlappingWeapon) {
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	if(IsLocallyControlled()) {
		if(OverlappingWeapon) {
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped() {
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming() {
	return (Combat && Combat->bAiming);
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const {
	if(Combat == nullptr) return nullptr;

	return Combat->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const {
	if(Combat == nullptr) return FVector();

	return Combat->HitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const {
	if(Combat == nullptr) return ECombatState::ECS_MAX;
	return Combat->CombatState;
}
