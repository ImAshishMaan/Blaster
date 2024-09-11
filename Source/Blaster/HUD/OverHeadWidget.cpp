#include "OverHeadWidget.h"
#include "Components/TextBlock.h"

void UOverHeadWidget::SetDisplayText(FString& TextToDisplay) {
	if(DisplayText) {
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverHeadWidget::ShowPlayerNetRole(APawn* InPawn) {
	if(!InPawn) return;

	ENetRole RemoteRole = InPawn->GetRemoteRole();
	FString Role;
	switch(RemoteRole) {
	case ENetRole::ROLE_Authority:
		Role = FString("Authority");
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString("AutonomousProxy");
		break;
	case ROLE_SimulatedProxy:
		Role = FString("SimulatedProxy");
		break;
	case ROLE_None:
		Role = FString("None");
		break;
	}
	FString RemoteRoleString = FString("Local Role: ") + Role;
	SetDisplayText(RemoteRoleString);
}

void UOverHeadWidget::NativeDestruct() {
	RemoveFromParent();
	Super::NativeDestruct();
}
