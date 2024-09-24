#include "HitScanWeapon.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"
#include "WeaponTypes.h"

void AHitScanWeapon::Fire(const FVector& HitTarget) {
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if(OwnerPawn == nullptr) return;
	APlayerController* InstigatorController = Cast<APlayerController>(OwnerPawn->GetController());

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if(MuzzleFlashSocket) {
		FTransform ScoketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = ScoketTransform.GetLocation();
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
		if(BlasterCharacter && HasAuthority() && InstigatorController) {
			UGameplayStatics::ApplyDamage(BlasterCharacter, Damage, InstigatorController, this,
			                              UDamageType::StaticClass());
		}
		if(ImpactParticle) {
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, FireHit.ImpactPoint,
			                                         FireHit.ImpactNormal.Rotation());
		}
		if(HitSound) {
			UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
		}
		if(MuzzleFlash) {
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, ScoketTransform);
		}
		if(FireSound) {
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, ScoketTransform.GetLocation());
		}
	}
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit) {
	UWorld* World = GetWorld();
	if(World) {
		FVector End = TraceStart + (HitTarget - TraceStart) * 1.25f;
		World->LineTraceSingleByChannel(OutHit, TraceStart, End, ECollisionChannel::ECC_Visibility);
		FVector BeamEnd = End;
		if(OutHit.bBlockingHit) {
			BeamEnd = OutHit.ImpactPoint;
		}
		if(BeamParticle) {
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
				World, BeamParticle, TraceStart, FRotator::ZeroRotator, true);
			if(Beam) {
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
	}
}


