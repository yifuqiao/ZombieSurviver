// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ZombieSurviver.h"
#include "ZombieSurviverGameMode.h"
#include "ZombieSurviverHUD.h"
#include "ZombieSurviverCharacter.h"

AZombieSurviverGameMode::AZombieSurviverGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AZombieSurviverHUD::StaticClass();
}
