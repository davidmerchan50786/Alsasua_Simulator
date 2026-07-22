#include "Systems/Social/SocialMediaSubsystem.h"
#include "Kismet/GameplayStatics.h"

void USocialMediaSubsystem::PostToFeed(FEvidencePost Photo) {
    int32 NewFollowers = FMath::RoundToInt(Photo.ViralPotential * 5.f);
    TotalFollowers += NewFollowers;
    GlobalFollowers += NewFollowers;
    FString Message = FString::Printf(TEXT("¡Post viral! +%d seguidores"), NewFollowers);
    OnViralPost.Broadcast(FText::FromString(Message));
}

void USocialMediaSubsystem::AddFollowers(int32 Amount) {
    TotalFollowers += Amount;
    GlobalFollowers += Amount;
}

void USocialMediaSubsystem::UploadEvidence(const FEvidencePost& Post) {
    int32 NewFollowers = FMath::RoundToInt(FMath::Max(0.f, Post.ImpactValue * 0.5f));
    TotalFollowers += NewFollowers;
    GlobalFollowers += NewFollowers;

    FString Message = FString::Printf(TEXT("Evidence uploaded: %s"), *Post.Description);
    OnViralPost.Broadcast(FText::FromString(Message));
}
