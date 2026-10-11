// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ZombieSurviver.h"
#include "ZombieSurviverHUD.h"
#include "ZombieSurviverCharacter.h"
#include "ZombieSurviverGameMode.h"
#include "WoodWall.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "TextureResource.h"
#include "CanvasItem.h"

AZombieSurviverHUD::AZombieSurviverHUD()
{
	// Set the crosshair texture
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshiarTexObj(TEXT("/Game/FirstPerson/Textures/FirstPersonCrosshair"));
	CrosshairTex = CrosshiarTexObj.Object;
}


void AZombieSurviverHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	APawn* OwnedPawn = PlayerOwner ? PlayerOwner->GetPawn() : nullptr;
	AZombieSurviverCharacter* Survivor = Cast<AZombieSurviverCharacter>(OwnedPawn);
	AZombieSurviverGameMode* Mode = GetWorld() ? Cast<AZombieSurviverGameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
	UFont* Small = GEngine ? GEngine->GetSmallFont() : nullptr;
	UFont* Medium = GEngine ? GEngine->GetMediumFont() : nullptr;
	UFont* Large = GEngine ? GEngine->GetLargeFont() : nullptr;

	const float Width = Canvas->ClipX;
	const float Height = Canvas->ClipY;
	const FVector2D Center(Width * 0.5f, Height * 0.5f);

	if (Survivor)
	{
		const float Flash = Survivor->GetDamageFlash();
		const float Hurt = 1.f - Survivor->GetHealthPercent();
		float Pulse = 0.f;
		if (Hurt > 0.65f && !Survivor->IsSurvivorDead())
		{
			Pulse = (FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f) * 0.5f + 0.5f) * 0.12f;
		}
		const float Overlay = FMath::Clamp(Flash * 0.38f + Hurt * 0.08f + Pulse + (Survivor->IsSurvivorDead() ? 0.45f : 0.f), 0.f, 0.72f);
		if (Overlay > 0.01f)
		{
			FCanvasTileItem Wash(FVector2D::ZeroVector, FVector2D(Width, Height), FLinearColor(0.55f, 0.02f, 0.02f, Overlay));
			Wash.BlendMode = SE_BLEND_Translucent;
			Canvas->DrawItem(Wash);
		}
	}

	const float Margin = Width * 0.035f;
	const float BarWidth = FMath::Clamp(Width * 0.22f, 180.f, 360.f);
	const float BarHeight = FMath::Clamp(Height * 0.016f, 10.f, 18.f);
	if (Survivor)
	{
		DrawBar(Margin, Height - Margin - BarHeight * 3.2f, BarWidth, BarHeight, Survivor->GetHealthPercent(), FLinearColor(0.75f, 0.12f, 0.1f, 0.95f), TEXT("HEALTH"));
		DrawBar(Margin, Height - Margin - BarHeight * 1.3f, BarWidth, BarHeight * 0.65f, Survivor->GetStaminaPercent(), FLinearColor(0.85f, 0.72f, 0.28f, 0.9f), TEXT("SPRINT"));
	}

	if (Mode && Small)
	{
		const FString AliveText = FString::Printf(TEXT("ALIVE  %s"), *Mode->GetSurvivalText());
		const FString Wave = FString::Printf(TEXT("WAVE  %d"), Mode->GetWave());
		const FString Score = FString::Printf(TEXT("SCORE  %d"), Mode->GetScore());
		const FString Horde = FString::Printf(TEXT("HORDE  %d"), Mode->GetAlive());
		const FString Lines[] = { AliveText, Wave, Score, Horde };
		for (int32 Index = 0; Index < 4; ++Index)
		{
			FCanvasTextItem Line(FVector2D(Width - Margin - 180.f, Margin + Index * 22.f), FText::FromString(Lines[Index]), Small, FLinearColor(0.95f, 0.9f, 0.75f, 0.92f));
			Line.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(Line);
		}

		float DoorY = Margin + 100.f;
		const float DoorWidth = FMath::Clamp(Width * 0.16f, 140.f, 260.f);
		const float DoorHeight = FMath::Clamp(Height * 0.012f, 8.f, 14.f);
		const float DoorX = Width - Margin - DoorWidth;
		for (AWoodWall* Entrance : Mode->GetEntrances())
		{
			if (!Entrance || Entrance->IsPendingKill())
			{
				continue;
			}

			const float Integrity = Entrance->GetIntegrity();
			const bool bOpen = Entrance->IsBreached();
			FLinearColor DoorColor = FMath::Lerp(FLinearColor(0.28f, 0.58f, 0.26f, 0.95f), FLinearColor(0.55f, 0.34f, 0.14f, 0.95f), Entrance->GetWearAlpha());
			if (Integrity < 0.66f)
			{
				DoorColor = FLinearColor(0.82f, 0.58f, 0.16f, 0.95f);
			}
			if (Integrity < 0.34f || bOpen)
			{
				DoorColor = FLinearColor(0.78f, 0.16f, 0.1f, 0.95f);
			}
			if (bOpen)
			{
				DoorColor.A = 0.55f + 0.45f * (0.5f + 0.5f * FMath::Sin(GetWorld()->GetTimeSeconds() * 6.f));
			}

			const FString DoorLabel = bOpen
				? FString::Printf(TEXT("%s  OPEN"), *Entrance->GetEntranceLabel())
				: FString::Printf(TEXT("%s  %d/%d"), *Entrance->GetEntranceLabel(), Entrance->GetIntactPlanks(), Entrance->GetPlankCount());
			DrawBar(DoorX, DoorY, DoorWidth, DoorHeight, bOpen ? 0.f : Integrity, DoorColor, DoorLabel);
			DoorY += DoorHeight + 22.f;
		}

		if (Mode->GetBannerAlpha() > 0.02f && Medium)
		{
			UFont* BannerFont = Large ? Large : Medium;
			float TextWidth = 0.f;
			float TextHeight = 0.f;
			Canvas->TextSize(BannerFont, Mode->GetBanner(), TextWidth, TextHeight, 1.35f, 1.35f);
			const FLinearColor BannerColor(1.f, 0.78f, 0.35f, Mode->GetBannerAlpha());
			FCanvasTextItem Banner(FVector2D(Center.X - TextWidth * 0.5f, Height * 0.18f), FText::FromString(Mode->GetBanner()), BannerFont, BannerColor);
			Banner.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Mode->GetBannerAlpha()));
			Banner.Scale = FVector2D(1.35f, 1.35f);
			Canvas->DrawItem(Banner);
		}

		if (PlayerOwner)
		{
			for (const FCombatFloater& Floater : Mode->GetFloaters())
			{
				FVector2D Screen;
				const FVector WorldPoint = Floater.Location + FVector(0.f, 0.f, Floater.Age * 46.f);
				if (!PlayerOwner->ProjectWorldLocationToScreen(WorldPoint, Screen))
				{
					continue;
				}
				FLinearColor Color = Floater.Color;
				Color.A = 1.f - (Floater.Age / 0.9f);
				float TextWidth = 0.f;
				float TextHeight = 0.f;
				Canvas->TextSize(Small, Floater.Text, TextWidth, TextHeight, Floater.Scale, Floater.Scale);
				FCanvasTextItem Pop(FVector2D(Screen.X - TextWidth * 0.5f, Screen.Y), FText::FromString(Floater.Text), Small, Color);
				Pop.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Color.A));
				Pop.Scale = FVector2D(Floater.Scale, Floater.Scale);
				Canvas->DrawItem(Pop);
			}
		}
	}

	if (Mode && Mode->IsGameOver() && Large)
	{
		const FString DeadText(TEXT("YOU DIED"));
		float TextWidth = 0.f;
		float TextHeight = 0.f;
		Canvas->TextSize(Large, DeadText, TextWidth, TextHeight, 1.8f, 1.8f);
		FCanvasTextItem Dead(FVector2D(Center.X - TextWidth * 0.5f, Center.Y - TextHeight * 0.5f), FText::FromString(DeadText), Large, FLinearColor(0.85f, 0.08f, 0.06f, 1.f));
		Dead.EnableShadow(FLinearColor::Black);
		Dead.Scale = FVector2D(1.8f, 1.8f);
		Canvas->DrawItem(Dead);
		if (Small)
		{
			const FString HintText = FString::Printf(TEXT("SURVIVED  %s"), *Mode->GetSurvivalText());
			Canvas->TextSize(Small, HintText, TextWidth, TextHeight);
			FCanvasTextItem Hint(FVector2D(Center.X - TextWidth * 0.5f, Center.Y + 42.f), FText::FromString(HintText), Small, FLinearColor(0.9f, 0.85f, 0.75f, 0.85f));
			Hint.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(Hint);
		}
	}

	if (Mode && !Mode->IsGameOver() && Small)
	{
		FString Prompt;
		if (Survivor && Survivor->WantsHammerPrompt())
		{
			if (AWoodWall* Door = Survivor->GetFocusedEntrance())
			{
				const int32 Percent = FMath::RoundToInt(Door->GetIntegrity() * 100.f);
				Prompt = FString::Printf(TEXT("HOLD RMB / E   HAMMER  %s  %d%%"), *Door->GetEntranceLabel(), Percent);
			}
		}
		else if (Mode->GetSurvivalTime() < 20.f)
		{
			Prompt = TEXT("HOLD RMB OR E AT A DOOR TO HAMMER THE BOARDS");
		}

		if (!Prompt.IsEmpty())
		{
			float TextWidth = 0.f;
			float TextHeight = 0.f;
			Canvas->TextSize(Small, Prompt, TextWidth, TextHeight);
			FCanvasTextItem Hint(FVector2D(Center.X - TextWidth * 0.5f, Height * 0.68f), FText::FromString(Prompt), Small, FLinearColor(0.93f, 0.72f, 0.38f, 0.95f));
			Hint.EnableShadow(FLinearColor::Black);
			Canvas->DrawItem(Hint);
		}
	}

	const float Spread = Survivor ? Survivor->GetCrosshairSpread() : 4.f;
	const float Confirm = Survivor ? Survivor->GetHitConfirm() : 0.f;
	const bool bHammer = Survivor && Survivor->WantsHammerPrompt();
	const FLinearColor IdleColor = bHammer ? FLinearColor(0.93f, 0.68f, 0.32f, 0.95f) : FLinearColor(1.f, 1.f, 1.f, 0.9f);
	const FLinearColor CrossColor = FMath::Lerp(IdleColor, FLinearColor(1.f, 0.25f, 0.15f, 1.f), Confirm);
	DrawCrosshair(Center, Spread, CrossColor);

	if (CrosshairTex)
	{
		const FVector2D CrosshairDrawPosition((Center.X - (CrosshairTex->GetSurfaceWidth() * 0.5f)),
			(Center.Y - (CrosshairTex->GetSurfaceHeight() * 0.5f)));
		FCanvasTileItem TileItem(CrosshairDrawPosition, CrosshairTex->Resource, CrossColor);
		TileItem.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(TileItem);
	}
}

void AZombieSurviverHUD::DrawBar(float X, float Y, float Width, float Height, float Percent, const FLinearColor& Fill, const FString& Label)
{
	FCanvasTileItem Back(FVector2D(X, Y), FVector2D(Width, Height), FLinearColor(0.02f, 0.02f, 0.02f, 0.72f));
	Back.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Back);

	const float Filled = Width * FMath::Clamp(Percent, 0.f, 1.f);
	if (Filled > 1.f)
	{
		FCanvasTileItem Front(FVector2D(X, Y), FVector2D(Filled, Height), Fill);
		Front.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Front);
	}

	if (UFont* Small = GEngine ? GEngine->GetSmallFont() : nullptr)
	{
		FCanvasTextItem Text(FVector2D(X, Y - 16.f), FText::FromString(Label), Small, FLinearColor(0.92f, 0.9f, 0.82f, 0.85f));
		Text.EnableShadow(FLinearColor::Black);
		Canvas->DrawItem(Text);
	}
}

void AZombieSurviverHUD::DrawCrosshair(const FVector2D& Center, float Spread, const FLinearColor& Color)
{
	const float Gap = 5.f + Spread;
	const float Length = 8.f;
	const float Thickness = 2.f;
	const FVector2D Arms[4] = {
		FVector2D(Center.X - Gap - Length, Center.Y - Thickness * 0.5f),
		FVector2D(Center.X + Gap, Center.Y - Thickness * 0.5f),
		FVector2D(Center.X - Thickness * 0.5f, Center.Y - Gap - Length),
		FVector2D(Center.X - Thickness * 0.5f, Center.Y + Gap)
	};
	const FVector2D Sizes[4] = {
		FVector2D(Length, Thickness),
		FVector2D(Length, Thickness),
		FVector2D(Thickness, Length),
		FVector2D(Thickness, Length)
	};
	for (int32 Index = 0; Index < 4; ++Index)
	{
		FCanvasTileItem Arm(Arms[Index], Sizes[Index], Color);
		Arm.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(Arm);
	}
}
