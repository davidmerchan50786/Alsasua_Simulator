#include "Press/AlsasuaPhotoComponent.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/Character.h"

UAlsasuaPhotoComponent::UAlsasuaPhotoComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaPhotoComponent::TakeSocialPhoto()
{
    TArray<FPhotoSubject> DetectedSubjects;
    AnalyzeFrame(DetectedSubjects);

    if (DetectedSubjects.Num() > 0)
    {
        UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();
        if (Sentiment)
        {
            float TotalImpact = 0.f;
            for (const FPhotoSubject& Subj : DetectedSubjects)
            {
                TotalImpact += Subj.Importance;
            }

            // Publicar la foto "virtualmente" genera apoyo
            Sentiment->PopularSupport = FMath::Clamp(Sentiment->PopularSupport + (TotalImpact * 2.f), 0.f, 100.f);
            UE_LOG(LogTemp, Log, TEXT("PHOTO: Capturados %d sujetos. Impacto social generado."), DetectedSubjects.Num());
        }
    }

    // Disparar efecto de flash y sonido (se haría en BP o con delegados)
}

void UAlsasuaPhotoComponent::AnalyzeFrame(TArray<FPhotoSubject>& OutSubjects)
{
    // Lógica simplificada: Buscamos actores en un cono frente al jugador
    ACharacter* Owner = Cast<ACharacter>(GetOwner());
    if (!Owner) return;

    FVector Start = Owner->GetActorLocation();
    FVector End = Start + (Owner->GetActorForwardVector() * 2000.f);

    TArray<AActor*> OverlappingActors;
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));

    UKismetSystemLibrary::SphereOverlapActors(GetWorld(), End, 500.f, ObjectTypes, nullptr, TArray<AActor*>(), OverlappingActors);

    for (AActor* A : OverlappingActors)
    {
        // Si el actor es importante (ej. un guardia o un líder), lo añadimos
        FPhotoSubject S;
        S.Actor = A;
        S.Importance = 5.0f; // Importancia base
        OutSubjects.Add(S);
    }
}
