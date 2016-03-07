

#include "ZombieSurviver.h"
#include "AIPathFollowingComponent.h"
#include "AI/Navigation/RecastNavMesh.h"



void UAIPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
	Super::FollowPathSegment(DeltaTime);
}

void UAIPathFollowingComponent::SetMoveSegment(int32 SegmentStartIndex)
{
	Super::SetMoveSegment(SegmentStartIndex);
}

void UAIPathFollowingComponent::UpdatePathSegment()
{
	Super::UpdatePathSegment();
}

bool UAIPathFollowingComponent::NavPoly_GetAllPolys(TArray<NavNodeRef>& Polys)
{
	//Rama's UE4 Nav code to get all the nav polys!
	if (!MovementComp) return false;
	//~~~~~~~~~~~~~~~~~~

	//Get Nav Data
	const ANavigationData* NavData = JoyGetNavData();

	const ARecastNavMesh* NavMesh = Cast<ARecastNavMesh>(NavData);
	if (!NavMesh)
	{
		return false;
	}

	TArray<FNavPoly> EachPolys;
	for (int32 v = 0; v < NavMesh->GetNavMeshTilesCount(); v++)
	{

		//CHECK IS VALID FIRST OR WILL CRASH!!! 
		//     256 entries but only few are valid!
		// use continue in case the valid polys are not stored sequentially
		if (!TileIsValid(NavMesh, v))
		{
			continue;
		}

		NavMesh->GetPolysInTile(v, EachPolys);
	}


	//Add them all!
	for (int32 v = 0; v < EachPolys.Num(); v++)
	{
		Polys.Add(EachPolys[v].Ref);
	}
	return true;
}

ANavigationData* UAIPathFollowingComponent::GetMainNavData(FNavigationSystem::ECreateIfEmpty CreateNewIfNoneFound)
{
	UNavigationSystem* NavSys = GetWorld()->GetNavigationSystem();
	if (!NavSys) return NULL;
	return NavSys->GetMainNavData(CreateNewIfNoneFound);
}

ANavigationData* UAIPathFollowingComponent::JoyGetNavData()
{
	FNavAgentProperties& AgentProperties = MovementComp->GetNavAgentPropertiesRef();
	ANavigationData* NavData = GetWorld()->GetNavigationSystem()->GetNavDataForProps(AgentProperties);
	if (NavData == NULL)
	{
	//	VSCREENMSG("ERROR USING MAIN NAV DATA");
		NavData = GetMainNavData((FNavigationSystem::ECreateIfEmpty)1);
	}

	return NavData;
}

bool UAIPathFollowingComponent::TileIsValid(const ARecastNavMesh* NavMesh, int32 TileIndex)
{
	if (!NavMesh) return false;
	//~~~~~~~~~~~~~~
	const FBox TileBounds = NavMesh->GetNavMeshTileBounds(TileIndex);

	return TileBounds.IsValid != 0;
}
