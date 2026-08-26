#include "Systems/Social/SocialMediaSubsystem.h"
#include "ManifestacionSubsystem.h"
#include "EconomiaCriminalSubsystem.h"
#include "Kismet/GameplayStatics.h"

void USocialMediaSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* W = GetWorld())
    {
        if (UManifestacionSubsystem* Manifa = W->GetGameInstance()->GetSubsystem<UManifestacionSubsystem>())
            Manifa->OnEstado.AddDynamic(this, &USocialMediaSubsystem::OnManifestacionStateChange);

        UEconomiaCriminalSubsystem::OnCriminalActivity.AddDynamic(this, &USocialMediaSubsystem::OnCriminalActivity);
    }
}

void USocialMediaSubsystem::OnManifestacionStateChange(EEstadoManifestacion Estado)
{
    if (Estado == EEstadoManifestacion::Marcha)
    {
        FEvidencePost Post;
        Post.Description = TEXT("Manifestacion en marcha - livestream viral");
        Post.ImpactValue = 10.f;
        Post.ViralPotential = 2.0f;
        PostToFeed(Post);
    }
    else if (Estado == EEstadoManifestacion::Dispersando)
    {
        FEvidencePost Post;
        Post.Description = TEXT("Carga policial en manifestacion");
        Post.ImpactValue = 20.f;
        Post.ViralPotential = 3.0f;
        PostToFeed(Post);
    }
}

void USocialMediaSubsystem::OnCriminalActivity(FName ActivityType, int32 Severity)
{
    FEvidencePost Post;
    Post.Description = FString::Printf(TEXT("Actividad criminal detectada: %s"), *ActivityType.ToString());
    Post.ImpactValue = (float)Severity * 0.5f;
    Post.RiskValue = (float)Severity;
    Post.ViralPotential = 1.0f + (float)Severity * 0.05f;
    UploadEvidence(Post);
}

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
