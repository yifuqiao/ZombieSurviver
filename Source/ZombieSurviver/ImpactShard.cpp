#include "ZombieSurviver.h"
#include "ImpactShard.h"
#include "Components/StaticMeshComponent.h"

AImpactShard::AImpactShard()
{
	PrimaryActorTick.bCanEverTick = true;
	UniformScale = 0.08f;
	InitialLifeSpan = 1.35f;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShardMesh"));
	RootComponent = Mesh;
	Mesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	Mesh->SetSimulatePhysics(true);
	Mesh->SetEnableGravity(true);
	Mesh->CanCharacterStepUpOn = ECB_No;
	SetCanBeDamaged(false);
}

void AImpactShard::InitializeShard(UStaticMesh* InMesh, UMaterialInterface* InMaterial, const FLinearColor& Tint, const FVector& Velocity, float InScale)
{
	UniformScale = FMath::Max(0.01f, InScale);
	if (InMesh)
	{
		Mesh->SetStaticMesh(InMesh);
	}

	if (InMaterial)
	{
		Mesh->SetMaterial(0, InMaterial);
	}
	else
	{
		UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (Base)
		{
			UMaterialInstanceDynamic* Mid = UMaterialInstanceDynamic::Create(Base, this);
			if (Mid)
			{
				Mid->SetVectorParameterValue(TEXT("Color"), Tint);
				Mid->SetVectorParameterValue(TEXT("BaseColor"), Tint);
				Mesh->SetMaterial(0, Mid);
			}
		}
	}

	Mesh->SetWorldScale3D(FVector(UniformScale));
	Mesh->SetPhysicsLinearVelocity(Velocity);
	Mesh->SetPhysicsAngularVelocity(FMath::VRand() * 720.f);
}

void AImpactShard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float Life = GetLifeSpan();
	if (Life > 0.f && Life < 0.4f)
	{
		Mesh->SetWorldScale3D(FVector(UniformScale * (Life / 0.4f)));
	}
}
