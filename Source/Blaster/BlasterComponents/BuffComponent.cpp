#include "BuffComponent.h"

#include "TimerManager.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

UBuffComponent::UBuffComponent() {
	PrimaryComponentTick.bCanEverTick = true;
}


void UBuffComponent::BeginPlay() {
	Super::BeginPlay();
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed) {
	InitialBaseSpeed = BaseSpeed;
	InitialCrouchSpeed = CrouchSpeed;
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction) {
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	HealRampUp(DeltaTime);
	ShieldRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealthAmount, float HealingTime) {
	bHealing = true;

	HealingRate = HealthAmount / HealingTime;
	AmountToHeal += HealthAmount;
}

void UBuffComponent::ReplinishShield(float ShieldAmount, float ShieldReplenishTime) {
	bReplenishingShield = true;

	ShieldReplenishRate = ShieldAmount / ShieldReplenishTime;
	ShieldReplenishAmount += ShieldAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime) {
	if(!bHealing || Character == nullptr || Character->IsElimmed()) return;
	const float HealThisFrame = HealingRate * DeltaTime;
	Character->SetHealth(FMath::Clamp(Character->GetHealth() + HealThisFrame, 0.f, Character->GetMaxHealth()));
	Character->UpdateHUDHealth();
	AmountToHeal -= HealThisFrame;
	if(AmountToHeal <= 0.f || Character->GetHealth() >= Character->GetMaxHealth()) {
		bHealing = false;
		AmountToHeal = 0.f;
	}
}

void UBuffComponent::ShieldRampUp(float DeltaTime) {
	if(!bReplenishingShield || Character == nullptr || Character->IsElimmed()) return;
	const float ReplenishThisFrame = ShieldReplenishRate * DeltaTime;
	Character->SetShield(FMath::Clamp(Character->GetShield() + ReplenishThisFrame, 0.f, Character->GetMaxShield()));
	Character->UpdateHUDShield();
	ShieldReplenishAmount -= ReplenishThisFrame;
	if(ShieldReplenishAmount <= 0.f || Character->GetShield() >= Character->GetMaxShield()) {
		bReplenishingShield = false;
		ShieldReplenishAmount = 0.f;
	}
}

void UBuffComponent::ResetSpeeds() {
	if(Character == nullptr || Character->GetCharacterMovement() == nullptr) return;

	Character->GetCharacterMovement()->MaxWalkSpeed = InitialBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = InitialCrouchSpeed;
	MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed);
}

void UBuffComponent::BuffSpeed(float BuffBaseSpeed, float BuffCrouchSpeed, float BuffTime) {
	if(Character == nullptr) return;
	Character->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &UBuffComponent::ResetSpeeds, BuffTime);
	if(Character->GetCharacterMovement()) {
		Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
	}
	MulticastSpeedBuff(BuffBaseSpeed, BuffCrouchSpeed);
}

void UBuffComponent::MulticastSpeedBuff_Implementation(float BuffBaseSpeed, float BuffCrouchSpeed) {
	Character->GetCharacterMovement()->MaxWalkSpeed = BuffBaseSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeedCrouched = BuffCrouchSpeed;
}
