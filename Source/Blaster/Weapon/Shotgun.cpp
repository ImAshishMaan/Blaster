#include "Shotgun.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget) {
	//Super::Fire(HitTarget);
	AWeapon::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if(OwnerPawn == nullptr) return;
	APlayerController* InstigatorController = Cast<APlayerController>(OwnerPawn->GetController());

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if(MuzzleFlashSocket) {
		FTransform ScoketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = ScoketTransform.GetLocation();

		TMap<ABlasterCharacter*, uint32> HitMap;
		for(uint32 i = 0; i < NumberOfPellets; ++i) {
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if(BlasterCharacter && HasAuthority() && InstigatorController) {
				if(HitMap.Contains(BlasterCharacter)) {
					HitMap[BlasterCharacter]++;
				} else {
					HitMap.Emplace(BlasterCharacter, 1);
				}
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
		for(auto HitPair: HitMap) {
			if(InstigatorController && HitPair.Key && HasAuthority()) {
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass());
			}
		}
	}
}
