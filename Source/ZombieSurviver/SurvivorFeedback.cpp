#include "ZombieSurviver.h"
#include "SurvivorFeedback.h"
#include "ImpactShard.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
	template<typename T>
	T* LoadRooted(const TCHAR* Path)
	{
		T* Asset = LoadObject<T>(nullptr, Path);
		if (Asset && !Asset->IsRooted())
		{
			Asset->AddToRoot();
		}
		return Asset;
	}

	bool CanPlayOnMesh(UAnimSequence* Anim, USkeletalMeshComponent* MeshComp)
	{
		if (!Anim || !MeshComp || !MeshComp->SkeletalMesh || !MeshComp->GetAnimInstance())
		{
			return false;
		}
		USkeleton* Skeleton = Anim->GetSkeleton();
		return Skeleton == nullptr || Skeleton == MeshComp->SkeletalMesh->Skeleton;
	}

	void PlayOnUpperBody(USkeletalMeshComponent* MeshComp, UAnimSequence* Anim, float PlayRate)
	{
		if (!CanPlayOnMesh(Anim, MeshComp))
		{
			return;
		}
		MeshComp->GetAnimInstance()->PlaySlotAnimation(Anim, TEXT("UpperBoddy"), 0.05f, 0.12f, PlayRate, 1);
	}
}

void SurvivorFeedback::PlayAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float Volume, float Pitch)
{
	if (!WorldContext || !Sound)
	{
		return;
	}
	UGameplayStatics::SpawnSoundAtLocation(WorldContext, Sound, Location, FRotator::ZeroRotator, Volume, Pitch);
}

UParticleSystemComponent* SurvivorFeedback::Burst(UWorld* World, const FVector& Location, UParticleSystem* System, float Scale)
{
	if (!World || !System)
	{
		return nullptr;
	}
	UParticleSystemComponent* Component = UGameplayStatics::SpawnEmitterAtLocation(World, System, Location, FRotator::ZeroRotator, true);
	if (Component)
	{
		Component->SetWorldScale3D(FVector(Scale));
	}
	return Component;
}

void SurvivorFeedback::SpawnShards(UWorld* World, const FVector& Origin, const FVector& Normal, int32 Count, UStaticMesh* Mesh, UMaterialInterface* Material, const FLinearColor& Tint, float Scale, float Speed)
{
	if (!World || !Mesh || Count <= 0)
	{
		return;
	}

	const FVector SafeNormal = Normal.IsNearlyZero() ? FVector::UpVector : Normal.GetSafeNormal();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector Dir = (SafeNormal + FMath::VRand() * 0.7f).GetSafeNormal();
		const FRotator Rot(FMath::FRandRange(-180.f, 180.f), FMath::FRandRange(-180.f, 180.f), FMath::FRandRange(-180.f, 180.f));
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AImpactShard* Shard = World->SpawnActor<AImpactShard>(AImpactShard::StaticClass(), Origin + Dir * 6.f, Rot, Params);
		if (!Shard)
		{
			continue;
		}
		const FVector Velocity = Dir * FMath::FRandRange(Speed * 0.45f, Speed) + FVector(0.f, 0.f, FMath::FRandRange(30.f, 140.f));
		Shard->InitializeShard(Mesh, Material, Tint, Velocity, Scale * FMath::FRandRange(0.65f, 1.25f));
	}
}

void SurvivorFeedback::PlayZombieHit(USkeletalMeshComponent* MeshComp, const FVector& FromLocation, bool bHeavy)
{
	UAnimSequence* Anim = nullptr;
	if (MeshComp)
	{
		const FVector ToHit = (FromLocation - MeshComp->GetComponentLocation()).GetSafeNormal();
		const float Forward = FVector::DotProduct(MeshComp->GetForwardVector(), ToHit);
		const float Side = FVector::DotProduct(MeshComp->GetRightVector(), ToHit);
		const TCHAR* Path = TEXT("/Game/Character/LandOfTheDead_Zombie/anim/HitFront.HitFront");
		if (bHeavy)
		{
			Path = TEXT("/Game/Character/LandOfTheDead_Zombie/anim/HitHead.HitHead");
		}
		else if (Forward < -0.35f)
		{
			Path = TEXT("/Game/Character/LandOfTheDead_Zombie/anim/HitBack.HitBack");
		}
		else if (Side > 0.4f)
		{
			Path = TEXT("/Game/Character/LandOfTheDead_Zombie/anim/HitRightSide.HitRightSide");
		}
		else if (Side < -0.4f)
		{
			Path = TEXT("/Game/Character/LandOfTheDead_Zombie/anim/HitLeftSide.HitLeftSide");
		}
		Anim = LoadRooted<UAnimSequence>(Path);
	}
	PlayOnUpperBody(MeshComp, Anim, bHeavy ? 1.25f : 1.1f);
}

void SurvivorFeedback::PlayZombieBite(USkeletalMeshComponent* MeshComp)
{
	UAnimSequence* Anim = LoadRooted<UAnimSequence>(TEXT("/Game/Character/LandOfTheDead_Zombie/anim/AttackBite.AttackBite"));
	PlayOnUpperBody(MeshComp, Anim, FMath::FRandRange(0.9f, 1.2f));
}

UParticleSystem* SurvivorFeedback::Sparks()
{
	return LoadRooted<UParticleSystem>(TEXT("/Game/StarterContent/Particles/P_Sparks.P_Sparks"));
}

UParticleSystem* SurvivorFeedback::Smoke()
{
	return LoadRooted<UParticleSystem>(TEXT("/Game/StarterContent/Particles/P_Smoke.P_Smoke"));
}

UParticleSystem* SurvivorFeedback::Explosion()
{
	return LoadRooted<UParticleSystem>(TEXT("/Game/StarterContent/Particles/P_Explosion.P_Explosion"));
}

UParticleSystem* SurvivorFeedback::Dust()
{
	return LoadRooted<UParticleSystem>(TEXT("/Game/StarterContent/Particles/P_Ambient_Dust.P_Ambient_Dust"));
}

USoundBase* SurvivorFeedback::Collapse()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Collapse_Cue.Collapse_Cue"));
}

USoundBase* SurvivorFeedback::Boom()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Explosion_Cue.Explosion_Cue"));
}

USoundBase* SurvivorFeedback::SparkSound()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Fire_Sparks01_Cue.Fire_Sparks01_Cue"));
}

USoundBase* SurvivorFeedback::Click()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Light01_Cue.Light01_Cue"));
}

USoundBase* SurvivorFeedback::ClickSoft()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Light02_Cue.Light02_Cue"));
}

USoundBase* SurvivorFeedback::Hiss()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Steam01_Cue.Steam01_Cue"));
}

USoundBase* SurvivorFeedback::Wind()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Starter_Wind05.Starter_Wind05"));
}

USoundBase* SurvivorFeedback::Music()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Starter_Music_Cue.Starter_Music_Cue"));
}

USoundBase* SurvivorFeedback::Birds()
{
	return LoadRooted<USoundBase>(TEXT("/Game/StarterContent/Audio/Starter_Birds01.Starter_Birds01"));
}

UStaticMesh* SurvivorFeedback::Cube()
{
	return LoadRooted<UStaticMesh>(TEXT("/Engine/BasicShapes/Cube.Cube"));
}

UStaticMesh* SurvivorFeedback::Sphere()
{
	return LoadRooted<UStaticMesh>(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
}
