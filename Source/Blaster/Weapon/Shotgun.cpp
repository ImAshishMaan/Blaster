#include "Shotgun.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::FireShotgun(const TArray<FVector_NetQuantize>& HitTargets) {
	AWeapon::Fire(FVector_NetQuantize());
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if(OwnerPawn == nullptr) return;
	APlayerController* InstigatorController = Cast<APlayerController>(OwnerPawn->GetController());
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if(MuzzleFlashSocket) {
		const FTransform ScoketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		const FVector Start = ScoketTransform.GetLocation();

		// maps hit character to number of hits
		TMap<ABlasterCharacter*, uint32> HitMap;

		for(FVector_NetQuantize HitTarget: HitTargets) {
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if(BlasterCharacter) {
				if(HitMap.Contains(BlasterCharacter)) {
					HitMap[BlasterCharacter]++;
				} else {
					HitMap.Emplace(BlasterCharacter, 1);
				}
				if(ImpactParticle) {
					UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, FireHit.ImpactPoint,
															 FireHit.ImpactNormal.Rotation());
				}
				if(HitSound) {
					UGameplayStatics::PlaySoundAtLocation(this, HitSound, FireHit.ImpactPoint, .5f,
														  FMath::FRandRange(-5.f, 5.f));
				}
			}
		}
		for(auto HitPair: HitMap) {
			if(InstigatorController && HitPair.Key && HasAuthority()) {
				UGameplayStatics::ApplyDamage(
					HitPair.Key,  // character that was hit
					Damage * HitPair.Value, 
					InstigatorController,
					this,
					UDamageType::StaticClass());
			}
		}
	}
}

void AShotgun::ShotgunTraceEndWithScatter(const FVector_NetQuantize& HitTarget, TArray<FVector_NetQuantize>& HitTargets) {
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if(MuzzleFlashSocket == nullptr) return;
	
	const FTransform ScoketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = ScoketTransform.GetLocation();
	
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	
	for(uint32 i = 0; i < NumberOfPellets; ++i) {
		const FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		const FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;
		ToEndLoc = TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size();
		
		HitTargets.Add(ToEndLoc);
	}
}
