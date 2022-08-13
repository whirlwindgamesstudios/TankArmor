float ATankPawn::GetArmorInPoint(FVector StartLocation,FVector EndLocation,FRotator Rotation)
{
	FHitResult HitResult;
	FVector2D UV=FVector2D(0.f,0.f);
	TArray<TEnumAsByte<EObjectTypeQuery>> Array;
	Array.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Vehicle));
	TArray < AActor *> ActorsToIgnore;
	ActorsToIgnore.Init(this, 1);
	UKismetSystemLibrary ::LineTraceSingleForObjects (GetWorld(), StartLocation,EndLocation, Array, false , ActorsToIgnore, EDrawDebugTrace ::ForDuration , HitResult, true , FLinearColor ::Red , FLinearColor ::Green , 10.f);
	if(HitResult.Actor!=nullptr)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(GetWorld(),RTArmorTexture);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(GetWorld(),RTArmorTexture,RTArmorMap);
		if(FindCollisionUVFromHit(HitResult,UV))
		{
			if(UV.X!=0&&UV.Y!=0)
			{
				FColor ArmorColor = UKismetRenderingLibrary::ReadRenderTargetUV(GetWorld(),RTArmorTexture,UV.X,UV.Y);
				float DOT=UKismetMathLibrary::Dot_VectorVector(HitResult.ImpactNormal,UKismetMathLibrary::GetForwardVector(Rotation));
				float AsinD=UKismetMathLibrary::DegAsin(DOT);
				float abs=UKismetMathLibrary::Abs(AsinD);
				float cosD=UKismetMathLibrary::DegCos(90-abs);
				GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Armor:%hhu,Angle:%f,Total Armor:%f "), ArmorColor.R,90-abs,ArmorColor.R/cosD));
				return ArmorColor.R/cosD;
			}
		}
	}
	return 0.0f;
}
struct FVertexUVPair
{
    FVector2D UVs;
    FVector Position;
};
 
struct FSortVertexByDistance
{
    FSortVertexByDistance(const FVector& InSourceLocation)
        : SourceLocation(InSourceLocation)
    {
 
    }
 
    FVector SourceLocation;
 
    bool operator()(const FVertexUVPair A, const FVertexUVPair B) const
    {
        float DistanceA = FVector::DistSquared(SourceLocation, A.Position);
        float DistanceB = FVector::DistSquared(SourceLocation, B.Position);
 
        return DistanceA < DistanceB;
    }
};
 
 
bool ATankPawn::FindCollisionUVFromHit(const FHitResult& Hit, FVector2D& UV)
{
	FSkeletalMeshRenderData* RenderData=nullptr;
    if (UPrimitiveComponent* HitPrimComp = Hit.Component.Get())
    {
        TArray<FVertexUVPair> AllVertices;
        const FVector LocalHitPos = HitPrimComp->GetComponentToWorld().InverseTransformPosition(Hit.Location);
 
        if (USkeletalMeshComponent* SkelComp = Cast<USkeletalMeshComponent>(HitPrimComp))
        {
        	RenderData = SkelComp->GetSkeletalMeshRenderData();
            if (RenderData!=nullptr)
            {
                if (RenderData->LODRenderData.Num() > 0)
                {
                    for (uint32 i = 0; i < RenderData->LODRenderData[0].StaticVertexBuffers.StaticMeshVertexBuffer.GetNumVertices() ; ++i)
                    {
                        FVertexUVPair NewPair;
                        NewPair.Position = RenderData->LODRenderData[0].StaticVertexBuffers.PositionVertexBuffer.VertexPosition(i);
                        NewPair.UVs = RenderData->LODRenderData[0].StaticVertexBuffers.StaticMeshVertexBuffer.GetVertexUV(i, 0);
                        AllVertices.Emplace(NewPair);
                    }
                }
            }
        }
        else if(UBodySetup* BodySetup = HitPrimComp->GetBodySetup())
        {
            const int32 VertexCount = BodySetup->UVInfo.VertPositions.Num();
            for (int32 Index = 0; Index < VertexCount; Index++)
            {
                FVertexUVPair NewPair;
                NewPair.Position = BodySetup->UVInfo.VertPositions[Index];
                NewPair.UVs = BodySetup->UVInfo.VertUVs[0][Index];
                AllVertices.Emplace(NewPair);
            }
        }
    	
        AllVertices.Sort(FSortVertexByDistance(LocalHitPos));
 
        if (AllVertices.Num() < 7)
        {
            return false;
        }
    	
        FVector Pos0 = AllVertices[0].Position,Pos1,Pos2;
    	FVector2D UV0=AllVertices[0].UVs,UV1,UV2;
    	
    	for (int i=0;i<=AllVertices.Num()-7;i++)
    	{
    		if(AllVertices[i+3].Position!=Pos0)
    		{
    			Pos1=AllVertices[i+3].Position;
    			UV1 = AllVertices[i+3].UVs;
    			break;
    		}
    		if(AllVertices[i+6].Position!=Pos0&&AllVertices[i+6].Position!=Pos1)
    		{
    			Pos2=AllVertices[i+6].Position;
    			UV2 = AllVertices[i+6].UVs;
    			break;
    		}
    	}
    	
        FVector BaryCoords = FMath::ComputeBaryCentric2D(LocalHitPos, Pos0, Pos1, Pos2);

        UV = (BaryCoords.X * UV0) + (BaryCoords.Y * UV1) + (BaryCoords.Z * UV2);
    	if(UV.X==0&&UV.Y==0){return false;}
        return true;
    }
 
    return false;
}