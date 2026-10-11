// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ZombieSurviver.h"
#include "ZombieSurviverCharacter.h"
#include "ZombieSurviverProjectile.h"
#include "ZombieSurviverGameMode.h"
#include "HealthComponent.h"
#include "SurvivorFeedback.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AZombieSurviverCharacter

AZombieSurviverCharacter::AZombieSurviverCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->AttachParent = GetCapsuleComponent();
	FirstPersonCameraComponent->RelativeLocation = FVector(0, 0, 64.f); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->AttachParent = FirstPersonCameraComponent;
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->AttachTo(Mesh1P, TEXT("GripPoint"), EAttachLocation::SnapToTargetIncludingScale, true);


	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 30.0f, 10.0f);

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->AttachParent = FirstPersonCameraComponent;
	Flashlight->RelativeLocation = FVector(10.f, 8.f, -4.f);
	Flashlight->SetMobility(EComponentMobility::Movable);
	Flashlight->SetIntensity(0.f);
	Flashlight->SetLightColor(FLinearColor(1.f, 0.86f, 0.62f));
	Flashlight->SetInnerConeAngle(12.f);
	Flashlight->SetOuterConeAngle(28.f);
	Flashlight->SetAttenuationRadius(2600.f);
	Flashlight->CastShadows = false;
	Flashlight->SetVisibility(false);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->AttachParent = FirstPersonCameraComponent;
	MuzzleLight->RelativeLocation = GunOffset;
	MuzzleLight->SetMobility(EComponentMobility::Movable);
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.72f, 0.32f));
	MuzzleLight->SetAttenuationRadius(480.f);
	MuzzleLight->CastShadows = false;
	MuzzleLight->SetVisibility(false);

	MaxHealth = 100;
	Health = 100;
	FireInterval = 0.16f;
	Stamina = 100.f;
	StaminaRegenDelay = 0.f;
	FireCooldown = 0.f;
	MeleeCooldown = 0.f;
	DamageFlash = 0.f;
	HitConfirm = 0.f;
	Trauma = 0.f;
	RecoilPitch = 0.f;
	AppliedRoll = 0.f;
	BobTime = 0.f;
	FootstepClock = 0.f;
	ThreatClock = 0.2f;
	HealBank = 0.f;
	TimeSinceDamage = 10.f;
	VulnerableTime = 0.f;
	WalkSpeed = 600.f;
	SprintSpeed = 960.f;
	MuzzleIntensity = 0.f;
	bWantsSprint = false;
	bDead = false;
	bFlashlightOn = false;
	bViewCached = false;

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P are set in the
	// derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

void AZombieSurviverCharacter::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		WalkSpeed = FMath::Max(200.f, Move->MaxWalkSpeed);
		SprintSpeed = WalkSpeed * 1.62f;
		Move->MaxWalkSpeed = WalkSpeed;
	}

	CameraBaseRelLocation = FirstPersonCameraComponent->RelativeLocation;
	ArmsBaseRelLocation = Mesh1P->GetRelativeTransform().GetLocation();
	MuzzleLight->SetRelativeLocation(GunOffset);
	bViewCached = true;

	FirstPersonCameraComponent->PostProcessBlendWeight = 1.f;
	FPostProcessSettings& Post = FirstPersonCameraComponent->PostProcessSettings;
	Post.bOverride_VignetteIntensity = true;
	Post.bOverride_SceneFringeIntensity = true;
	Post.bOverride_GrainIntensity = true;
	Post.bOverride_FilmSaturation = true;
	Post.bOverride_FilmContrast = true;
	Post.bOverride_FilmWhitePoint = true;
	Post.bOverride_BloomIntensity = true;
	Post.VignetteIntensity = 0.34f;
	Post.SceneFringeIntensity = 0.6f;
	Post.GrainIntensity = 0.18f;
	Post.FilmSaturation = 0.92f;
	Post.FilmContrast = 1.05f;
	Post.FilmWhitePoint = FLinearColor::White;
	Post.BloomIntensity = 0.7f;
}

void AZombieSurviverCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFeel(DeltaSeconds);
	UpdatePostProcess(DeltaSeconds);
	if (!bDead)
	{
		UpdateThreat(DeltaSeconds);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AZombieSurviverCharacter::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	// set up gameplay key bindings
	check(InputComponent);

	InputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	InputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	
	//InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AZombieSurviverCharacter::TouchStarted);
	if( EnableTouchscreenMovement(InputComponent) == false )
	{
		InputComponent->BindAction("Fire", IE_Pressed, this, &AZombieSurviverCharacter::OnFire);
	}

	InputComponent->BindAction("Sprint", IE_Pressed, this, &AZombieSurviverCharacter::StartSprint);
	InputComponent->BindAction("Sprint", IE_Released, this, &AZombieSurviverCharacter::StopSprint);
	InputComponent->BindAction("Melee", IE_Pressed, this, &AZombieSurviverCharacter::OnMelee);
	InputComponent->BindAction("Flashlight", IE_Pressed, this, &AZombieSurviverCharacter::ToggleFlashlight);
	
	InputComponent->BindAxis("MoveForward", this, &AZombieSurviverCharacter::MoveForward);
	InputComponent->BindAxis("MoveRight", this, &AZombieSurviverCharacter::MoveRight);
	
	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	InputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	InputComponent->BindAxis("TurnRate", this, &AZombieSurviverCharacter::TurnAtRate);
	InputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	InputComponent->BindAxis("LookUpRate", this, &AZombieSurviverCharacter::LookUpAtRate);
}

void AZombieSurviverCharacter::OnFire()
{ 
	if (bDead || FireCooldown > 0.f)
	{
		return;
	}
	FireCooldown = FireInterval;

	// try and fire a projectile
	if (ProjectileClass != NULL)
	{
		const FRotator SpawnRotation = GetControlRotation();
		// MuzzleOffset is in camera space, so transform it to world space before offsetting from the character location to find the final muzzle position
		const FVector SpawnLocation = GetActorLocation() + SpawnRotation.RotateVector(GunOffset);

		UWorld* const World = GetWorld();
		if (World != NULL)
		{
			// spawn the projectile at the muzzle
			World->SpawnActor<AZombieSurviverProjectile>(ProjectileClass, SpawnLocation, SpawnRotation);
			SurvivorFeedback::Burst(World, SpawnLocation, SurvivorFeedback::Sparks(), 0.22f);
			const FVector Right = SpawnRotation.RotateVector(FVector(0.f, 1.f, 0.35f));
			SurvivorFeedback::SpawnShards(World, SpawnLocation, Right, 1, SurvivorFeedback::Cube(), nullptr, FLinearColor(0.86f, 0.68f, 0.22f), 0.03f, 220.f);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}
	else
	{
		SurvivorFeedback::PlayAt(this, SurvivorFeedback::SparkSound(), GetActorLocation(), 0.7f, FMath::FRandRange(0.95f, 1.08f));
	}

	// try and play a firing animation if specified
	if(FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if(AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}

	const float Kick = FMath::FRandRange(0.85f, 1.25f);
	AddControllerPitchInput(Kick);
	RecoilPitch += Kick;
	AddCameraTrauma(0.28f);
	MuzzleIntensity = 22000.f;
	MuzzleLight->SetVisibility(true);
}

void AZombieSurviverCharacter::OnMelee()
{
	if (bDead || MeleeCooldown > 0.f)
	{
		return;
	}
	MeleeCooldown = 0.48f;
	AddCameraTrauma(0.4f);
	AddControllerPitchInput(-0.6f);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Start = FirstPersonCameraComponent->GetComponentLocation();
	const FVector Forward = FirstPersonCameraComponent->GetForwardVector();
	const FVector End = Start + Forward * 175.f;
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::SparkSound(), Start, 0.45f, 1.45f);
	SurvivorFeedback::Burst(World, Start + Forward * 80.f, SurvivorFeedback::Sparks(), 0.35f);

	FCollisionQueryParams Params(TEXT("Melee"), false, this);
	TArray<FHitResult> Hits;
	World->SweepMultiByChannel(Hits, Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(48.f), Params);

	TArray<AActor*> HitActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* Other = Hit.GetActor();
		if (!Other || HitActors.Contains(Other))
		{
			continue;
		}
		HitActors.Add(Other);
		if (UHealthComponent* HealthComp = Other->FindComponentByClass<UHealthComponent>())
		{
			HealthComp->NotifyHitFrom(Start);
			HealthComp->TakeDamage(40);
		}
		else if (UPrimitiveComponent* Comp = Hit.GetComponent())
		{
			if (Comp->IsSimulatingPhysics())
			{
				Comp->AddImpulseAtLocation(Forward * 1200.f, Hit.ImpactPoint);
			}
		}
	}
}

void AZombieSurviverCharacter::StartSprint()
{
	bWantsSprint = true;
}

void AZombieSurviverCharacter::StopSprint()
{
	bWantsSprint = false;
}

void AZombieSurviverCharacter::ToggleFlashlight()
{
	if (bDead)
	{
		return;
	}
	bFlashlightOn = !bFlashlightOn;
	Flashlight->SetVisibility(bFlashlightOn);
	if (!bFlashlightOn)
	{
		Flashlight->SetIntensity(0.f);
	}
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Click(), GetActorLocation(), 0.7f, bFlashlightOn ? 1.15f : 0.85f);
}

void AZombieSurviverCharacter::BeginTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if( TouchItem.bIsPressed == true )
	{
		return;
	}
	TouchItem.bIsPressed = true;
	TouchItem.FingerIndex = FingerIndex;
	TouchItem.Location = Location;
	TouchItem.bMoved = false;
}

void AZombieSurviverCharacter::EndTouch(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if (TouchItem.bIsPressed == false)
	{
		return;
	}
	if( ( FingerIndex == TouchItem.FingerIndex ) && (TouchItem.bMoved == false) )
	{
		OnFire();
	}
	TouchItem.bIsPressed = false;
}

void AZombieSurviverCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	if ((TouchItem.bIsPressed == true) && ( TouchItem.FingerIndex==FingerIndex))
	{
		if (TouchItem.bIsPressed)
		{
			if (GetWorld() != nullptr)
			{
				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
				if (ViewportClient != nullptr)
				{
					FVector MoveDelta = Location - TouchItem.Location;
					FVector2D ScreenSize;
					ViewportClient->GetViewportSize(ScreenSize);
					FVector2D ScaledDelta = FVector2D( MoveDelta.X, MoveDelta.Y) / ScreenSize;									
					if (ScaledDelta.X != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.X * BaseTurnRate;
						AddControllerYawInput(Value);
					}
					if (ScaledDelta.Y != 0.0f)
					{
						TouchItem.bMoved = true;
						float Value = ScaledDelta.Y* BaseTurnRate;
						AddControllerPitchInput(Value);
					}
					TouchItem.Location = Location;
				}
				TouchItem.Location = Location;
			}
		}
	}
}

void AZombieSurviverCharacter::MoveForward(float Value)
{
	if (bDead || Value == 0.0f)
	{
		return;
	}
	AddMovementInput(GetActorForwardVector(), Value);
}

void AZombieSurviverCharacter::MoveRight(float Value)
{
	if (bDead || Value == 0.0f)
	{
		return;
	}
	AddMovementInput(GetActorRightVector(), Value);
}

void AZombieSurviverCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AZombieSurviverCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

bool AZombieSurviverCharacter::EnableTouchscreenMovement(class UInputComponent* InputComponent)
{
	bool bResult = false;
	if(FPlatformMisc::GetUseVirtualJoysticks() || GetDefault<UInputSettings>()->bUseMouseForTouch )
	{
		bResult = true;
		InputComponent->BindTouch(EInputEvent::IE_Pressed, this, &AZombieSurviverCharacter::BeginTouch);
		InputComponent->BindTouch(EInputEvent::IE_Released, this, &AZombieSurviverCharacter::EndTouch);
		InputComponent->BindTouch(EInputEvent::IE_Repeat, this, &AZombieSurviverCharacter::TouchUpdate);
	}
	return bResult;
}

void AZombieSurviverCharacter::ApplySurvivorDamage(int32 Amount, FVector FromLocation)
{
	if (bDead || Amount <= 0 || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < VulnerableTime)
	{
		return;
	}
	VulnerableTime = Now + 0.4f;
	TimeSinceDamage = 0.f;
	Health = FMath::Max(0, Health - Amount);
	DamageFlash = 1.f;
	AddCameraTrauma(0.85f);
	AddControllerPitchInput(FMath::FRandRange(-2.2f, 1.4f));

	const FVector Away = (GetActorLocation() - FromLocation).GetSafeNormal();
	if (!Away.IsNearlyZero())
	{
		LaunchCharacter(Away * 160.f + FVector(0.f, 0.f, 30.f), true, false);
	}
	SurvivorFeedback::PlayAt(this, SurvivorFeedback::Collapse(), GetActorLocation(), 0.7f, FMath::FRandRange(1.2f, 1.5f));

	if (Health <= 0)
	{
		HandleDeath();
	}
}

void AZombieSurviverCharacter::AddCameraTrauma(float Amount)
{
	Trauma = FMath::Clamp(Trauma + Amount, 0.f, 1.6f);
}

void AZombieSurviverCharacter::ConfirmHit()
{
	HitConfirm = 1.f;
}

float AZombieSurviverCharacter::GetHealthPercent() const
{
	return MaxHealth > 0 ? FMath::Clamp(float(Health) / float(MaxHealth), 0.f, 1.f) : 0.f;
}

float AZombieSurviverCharacter::GetStaminaPercent() const
{
	return FMath::Clamp(Stamina / 100.f, 0.f, 1.f);
}

float AZombieSurviverCharacter::GetCrosshairSpread() const
{
	const float Speed = GetVelocity().Size2D();
	const float MoveSpread = FMath::Clamp(Speed / 80.f, 0.f, 10.f);
	return MoveSpread + Trauma * 8.f + (FireCooldown > 0.f ? 6.f : 0.f);
}

void AZombieSurviverCharacter::HandleDeath()
{
	if (bDead)
	{
		return;
	}
	bDead = true;
	Health = 0;
	bWantsSprint = false;
	DamageFlash = 1.f;
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		DisableInput(PC);
	}
	if (UWorld* World = GetWorld())
	{
		if (AZombieSurviverGameMode* Mode = Cast<AZombieSurviverGameMode>(World->GetAuthGameMode()))
		{
			Mode->NotifySurvivorDown();
		}
	}
}

void AZombieSurviverCharacter::UpdateFeel(float DeltaSeconds)
{
	FireCooldown = FMath::Max(0.f, FireCooldown - DeltaSeconds);
	MeleeCooldown = FMath::Max(0.f, MeleeCooldown - DeltaSeconds);
	DamageFlash = FMath::Max(0.f, DamageFlash - DeltaSeconds * 1.7f);
	HitConfirm = FMath::Max(0.f, HitConfirm - DeltaSeconds * 6.f);
	Trauma = FMath::Max(0.f, Trauma - DeltaSeconds * 1.8f);
	TimeSinceDamage += DeltaSeconds;

	const float NewRecoil = FMath::FInterpTo(RecoilPitch, 0.f, DeltaSeconds, 9.f);
	AddControllerPitchInput(NewRecoil - RecoilPitch);
	RecoilPitch = NewRecoil;

	MuzzleIntensity = FMath::FInterpTo(MuzzleIntensity, 0.f, DeltaSeconds, 26.f);
	MuzzleLight->SetIntensity(MuzzleIntensity);
	MuzzleLight->SetVisibility(MuzzleIntensity > 30.f);

	if (bFlashlightOn)
	{
		const float Flicker = (FMath::FRand() < 0.07f) ? FMath::FRandRange(0.45f, 0.75f) : FMath::FRandRange(0.9f, 1.f);
		Flashlight->SetIntensity(48000.f * Flicker);
	}

	if (bDead)
	{
		AddControllerPitchInput(-18.f * DeltaSeconds);
		return;
	}

	UCharacterMovementComponent* Move = GetCharacterMovement();
	const bool bOnGround = Move && Move->IsMovingOnGround();
	const bool bMoving = GetVelocity().SizeSquared2D() > 3600.f;
	const bool bSprintActive = bWantsSprint && bOnGround && bMoving && Stamina > 1.f;
	if (Move)
	{
		Move->MaxWalkSpeed = bSprintActive ? SprintSpeed : WalkSpeed;
	}

	if (bSprintActive)
	{
		Stamina = FMath::Max(0.f, Stamina - 30.f * DeltaSeconds);
		StaminaRegenDelay = 0.7f;
	}
	else if (StaminaRegenDelay > 0.f)
	{
		StaminaRegenDelay -= DeltaSeconds;
	}
	else
	{
		Stamina = FMath::Min(100.f, Stamina + 22.f * DeltaSeconds);
	}

	if (TimeSinceDamage > 4.5f && Health < MaxHealth && Health > 0)
	{
		HealBank += DeltaSeconds * 6.f;
		if (HealBank >= 1.f)
		{
			Health = FMath::Min(MaxHealth, Health + 1);
			HealBank -= 1.f;
		}
	}

	if (!bViewCached)
	{
		return;
	}

	float BobZ = FMath::Sin(GetWorld()->GetTimeSeconds() * 1.35f) * 0.35f;
	float BobY = 0.f;
	if (bMoving && bOnGround)
	{
		const float Frequency = bSprintActive ? 12.f : 8.2f;
		BobTime += DeltaSeconds * Frequency;
		const float Amplitude = bSprintActive ? 1.7f : 0.85f;
		BobZ += FMath::Sin(BobTime * 2.f) * Amplitude;
		BobY = FMath::Cos(BobTime) * Amplitude * 0.4f;
		FootstepClock -= DeltaSeconds;
		if (FootstepClock <= 0.f)
		{
			FootstepClock = bSprintActive ? 0.32f : 0.48f;
			SurvivorFeedback::PlayAt(this, SurvivorFeedback::ClickSoft(), GetActorLocation() - FVector(0.f, 0.f, 80.f), 0.28f, FMath::FRandRange(0.85f, 1.25f));
		}
	}

	const float Shake = FMath::Sin(GetWorld()->GetTimeSeconds() * 47.f) * Trauma * 1.4f;
	FirstPersonCameraComponent->SetRelativeLocation(CameraBaseRelLocation + FVector(0.f, BobY + Shake, BobZ));
	Mesh1P->SetRelativeLocation(ArmsBaseRelLocation + FVector(0.f, BobY * 0.65f, BobZ * 0.45f));

	const float TargetFOV = bSprintActive ? 101.f : 90.f;
	FirstPersonCameraComponent->SetFieldOfView(FMath::FInterpTo(FirstPersonCameraComponent->FieldOfView, TargetFOV, DeltaSeconds, 7.f));

	const float Strafe = FVector::DotProduct(GetVelocity(), GetActorRightVector());
	const float DesiredRoll = FMath::Clamp(Strafe / 700.f, -1.f, 1.f) * -1.6f + FMath::Sin(GetWorld()->GetTimeSeconds() * 33.f) * Trauma * 1.2f;
	AddControllerRollInput(DesiredRoll - AppliedRoll);
	AppliedRoll = DesiredRoll;
}

void AZombieSurviverCharacter::UpdateThreat(float DeltaSeconds)
{
	ThreatClock -= DeltaSeconds;
	if (ThreatClock > 0.f || !GetWorld())
	{
		return;
	}
	ThreatClock = 0.12f;

	if (BiteTimes.Num() > 24)
	{
		for (auto It = BiteTimes.CreateIterator(); It; ++It)
		{
			if (It.Key() == nullptr || It.Key()->IsPendingKill())
			{
				It.RemoveCurrent();
			}
		}
	}

	const FVector Origin = GetActorLocation();
	FCollisionQueryParams Params(TEXT("Bite"), false, this);
	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByChannel(Overlaps, Origin, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(150.f), Params);

	const float Now = GetWorld()->GetTimeSeconds();
	TArray<AActor*> Considered;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Other = Overlap.GetActor();
		if (!Other || Other == this || Considered.Contains(Other))
		{
			continue;
		}
		Considered.Add(Other);

		UHealthComponent* HealthComp = Other->FindComponentByClass<UHealthComponent>();
		if (!HealthComp || HealthComp->IsDead())
		{
			continue;
		}

		FHitResult Block;
		FCollisionQueryParams Sight(TEXT("BiteSight"), false, this);
		Sight.AddIgnoredActor(Other);
		const FVector Target = Other->GetActorLocation() + FVector(0.f, 0.f, 50.f);
		if (GetWorld()->LineTraceSingleByChannel(Block, GetActorLocation() + FVector(0.f, 0.f, 60.f), Target, ECC_Visibility, Sight))
		{
			continue;
		}

		float* NextBite = BiteTimes.Find(Other);
		if (NextBite && *NextBite > Now)
		{
			continue;
		}
		BiteTimes.Add(Other, Now + FMath::FRandRange(0.85f, 1.15f));

		if (ACharacter* Zombie = Cast<ACharacter>(Other))
		{
			SurvivorFeedback::PlayZombieBite(Zombie->GetMesh());
		}
		ApplySurvivorDamage(8, Other->GetActorLocation());
	}
}

void AZombieSurviverCharacter::UpdatePostProcess(float DeltaSeconds)
{
	(void)DeltaSeconds;
	FPostProcessSettings& Post = FirstPersonCameraComponent->PostProcessSettings;
	const float HealthPct = GetHealthPercent();
	const float Hurt = 1.f - HealthPct;
	const float Time = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	float Pulse = 0.f;
	if (HealthPct < 0.35f && !bDead)
	{
		const float Rate = HealthPct < 0.18f ? 8.5f : 5.2f;
		Pulse = (FMath::Sin(Time * Rate) * 0.5f + 0.5f) * 0.22f;
	}

	Post.VignetteIntensity = 0.32f + Hurt * 0.5f + DamageFlash * 0.35f + Pulse + (bDead ? 0.7f : 0.f);
	Post.SceneFringeIntensity = 0.45f + Hurt * 2.2f + Trauma * 1.4f;
	Post.GrainIntensity = 0.12f + Hurt * 0.45f + Trauma * 0.15f;
	Post.FilmSaturation = FMath::Lerp(0.95f, 0.25f, bDead ? 1.f : Hurt);
	Post.FilmContrast = 1.04f + Hurt * 0.15f;
	Post.FilmWhitePoint = FLinearColor(1.f, 1.f - Hurt * 0.45f - DamageFlash * 0.2f, 1.f - Hurt * 0.55f - DamageFlash * 0.35f);
	Post.BloomIntensity = 0.55f + MuzzleIntensity / 40000.f;
}
