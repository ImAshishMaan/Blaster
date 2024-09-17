#include "BlasterPlayerController.h"

#include "Blaster/Character/BlasterCharacter.h"
#include "Blaster/HUD/BlasterHUD.h"
#include "Blaster/HUD/CharacterOverlayWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"


void ABlasterPlayerController::BeginPlay() {
	Super::BeginPlay();

	BlasterHUD = Cast<ABlasterHUD>(GetHUD());
}

void ABlasterPlayerController::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	SetHUDTime();

	CheckTimeSync(DeltaTime);
}

void ABlasterPlayerController::CheckTimeSync(float DeltaTime) {
	TimeSyncRunningTime += DeltaTime;
	if(IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency) {
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.0f;
	}
}


void ABlasterPlayerController::SetHUDHealth(float Health, float MaxHealth) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->HealthBar &&
		BlasterHUD->CharacterOverlay->HealthText;
	if(bHUDValid) {
		const float HealthPercent = Health / MaxHealth;
		BlasterHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);

		FString HealthText = FString::Printf(TEXT("%d / %d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		BlasterHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void ABlasterPlayerController::SetHUDScore(float Score) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->ScoreAmount;
	if(bHUDValid) {
		int32 ScoreInt = FMath::CeilToInt(Score);
		BlasterHUD->CharacterOverlay->ScoreAmount->SetText(FText::AsNumber(ScoreInt));
	}
}

void ABlasterPlayerController::SetHUDDefeats(int32 Defeats) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->DefeatsAmount;
	if(bHUDValid) {
		BlasterHUD->CharacterOverlay->DefeatsAmount->SetText(FText::AsNumber(Defeats));
	}
}

void ABlasterPlayerController::SetHUDWeaponAmmo(int32 Ammo) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount;
	
	if(bHUDValid) {
		BlasterHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::AsNumber(Ammo));
	}else {
		UE_LOG(LogTemp, Warning, TEXT("bHUDValid is not true: %i"), bHUDValid);
	}
}

void ABlasterPlayerController::SetHUDCarriedAmmo(int32 CarriedAmmo) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount;
	
	if(bHUDValid) {
		BlasterHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::AsNumber(CarriedAmmo));
	}else {
		UE_LOG(LogTemp, Warning, TEXT("bHUDValid is not true: %i"), bHUDValid);
	}
}

void ABlasterPlayerController::SetHUDMatchCountdown(float CountdownTime) {
	BlasterHUD = BlasterHUD == nullptr ? Cast<ABlasterHUD>(GetHUD()) : BlasterHUD;
	bool bHUDValid = BlasterHUD &&
		BlasterHUD->CharacterOverlay &&
		BlasterHUD->CharacterOverlay->MatchCountdownText;
	
	if(bHUDValid) {
		if(CountdownTime < 0.f) {
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
		} else {
			int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
			int32 Seconds = CountdownTime - Minutes * 60;
			FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
			BlasterHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
		}
	}else {
		UE_LOG(LogTemp, Warning, TEXT("bHUDValid is not true: %i"), bHUDValid);
	}
}

void ABlasterPlayerController::OnPossess(APawn* InPawn) {
	Super::OnPossess(InPawn);
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(InPawn);
	if(BlasterCharacter) {
		SetHUDHealth(BlasterCharacter->GetHealth(), BlasterCharacter->GetMaxHealth());
	}
}

void ABlasterPlayerController::SetHUDTime() {
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());
	if(CountdownInt != SecondsLeft) {
		SetHUDMatchCountdown(MatchTime - GetServerTime());
	}
	CountdownInt = SecondsLeft;
}


void ABlasterPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest) {
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ABlasterPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest,
	float TimeServerReceivedClientRequest) {
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + 0.5f * RoundTripTime;

	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
	
}

float ABlasterPlayerController::GetServerTime() {
	//if(HasAuthority()) return GetWorld()->GetTimeSeconds();
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ABlasterPlayerController::ReceivedPlayer() {
	Super::ReceivedPlayer();
	if(IsLocalController()) {
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}


