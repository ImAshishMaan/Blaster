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
		FVector End = Start + (HitTarget - Start) * 2.0f;
		FHitResult FireHit;
		UWorld* World = GetWorld();
		if(World) {
			World->LineTraceSingleByChannel(FireHit, Start, End, ECollisionChannel::ECC_Visibility);
			FVector BeamEnd = End;
			if(FireHit.bBlockingHit) {
				BeamEnd = FireHit.ImpactPoint;
				ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
				if(BlasterCharacter && HasAuthority() && InstigatorController) {
					UGameplayStatics::ApplyDamage(BlasterCharacter, Damage, InstigatorController, this,
					                              UDamageType::StaticClass());
				}
				if(ImpactParticle) {
					UGameplayStatics::SpawnEmitterAtLocation(World, ImpactParticle, FireHit.ImpactPoint,
					                                         FireHit.ImpactNormal.Rotation());
				}
				if(HitSound) {
					UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint);
				}
			}
			if(BeamParticle) {
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(
					World, BeamParticle, ScoketTransform);
				if(Beam) {
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}

		if(MuzzleFlash) {
			UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, ScoketTransform);
		}
		if(FireSound) {
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, ScoketTransform.GetLocation());
		}
	}
}

FVector AHitScanWeapon::TraceEndWithScatter(const FVector& TraceStart, const FVector& HitTarget) {
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	FVector EndLoc = SphereCenter + RandVec;
	FVector ToEndLoc = EndLoc - TraceStart;

	//DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);
	DrawDebugSphere(GetWorld(), EndLoc, 4, 12, FColor::Green, true);
	DrawDebugLine(GetWorld(), TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()), FColor::Cyan, true);
	
	
	return FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size());
	
}
