#include "ShooterEnemy.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "EngineUtils.h"
#include "Engine/DamageEvents.h"
#include "ShooterCharacter.h"
#include "ShooterGameMode.h"

AShooterEnemy::AShooterEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->SetSphereRadius(150.f);
    CollisionSphere->SetCollisionProfileName(TEXT("Pawn"));

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AShooterEnemy::OnOverlap);
}

void AShooterEnemy::BeginPlay()
{
    Super::BeginPlay();

    SpawnLocation = GetActorLocation();
    CurrentHP = MaxHP;
}

void AShooterEnemy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!HasAuthority() || bIsDead)
        return;

    /*FVector NewLocation = GetActorLocation();
    NewLocation.X += 100.f * DeltaSeconds;
	SetActorLocation(NewLocation);*/

    FindTarget();
    MoveToTarget(DeltaSeconds);
}

void AShooterEnemy::FindTarget()
{
   // if (TargetCharacter && !TargetCharacter->IsDead())
     //   return;

    TargetCharacter = nullptr;

    float BestDist = FLT_MAX;

    for (TActorIterator<AShooterCharacter> It(GetWorld()); It; ++It)
    {
        AShooterCharacter* SC = *It;
        if (!SC || SC->bIsDead)
            continue;

        float Dist = FVector::DistSquared(GetActorLocation(), SC->GetActorLocation());
        if (Dist < BestDist)
        {
            BestDist = Dist;
            TargetCharacter = SC;
        }
    }
}

void AShooterEnemy::MoveToTarget(float DeltaSeconds)
{
    if (!TargetCharacter)
        return;

    FVector Dir = (TargetCharacter->GetActorLocation() - GetActorLocation());
    Dir.Normalize();

    FVector NewLocation = GetActorLocation();
    NewLocation += Dir * MoveSpeed * DeltaSeconds;
    SetActorLocation(NewLocation);

    //AddActorWorldOffset(Dir * MoveSpeed * DeltaSeconds, true);
}

void AShooterEnemy::OnOverlap(
    UPrimitiveComponent*,
    AActor* OtherActor,
    UPrimitiveComponent*,
    int32,
    bool,
    const FHitResult&
)
{
    if (!HasAuthority() || bIsDead)
        return;

    AShooterCharacter* SC = Cast<AShooterCharacter>(OtherActor);
    if (!SC)
        return;

    SC->TakeDamage(30.f, FDamageEvent(), nullptr, this);

    Die(nullptr); // 自爆
}

float AShooterEnemy::TakeDamage(
    float Damage,
    FDamageEvent const&,
    AController* EventInstigator,
    AActor*
)
{
    if (!HasAuthority() || bIsDead)
        return 0.f;

    CurrentHP -= Damage;
    OnRep_HP();

    if (CurrentHP <= 0.f)
    {
        Die(EventInstigator);
    }

    return Damage;
}

void AShooterEnemy::Die(AController* KillerController)
{
    if (bIsDead)
        return;

    bIsDead = true;
    SetActorHiddenInGame(true);
    SetActorEnableCollision(false);

    /** 加分：走 GameMode */
    if (AShooterGameMode* GM = GetWorld()->GetAuthGameMode<AShooterGameMode>())
    {
        if (KillerController)
        {
            GM->AddScoreForKill(KillerController, 1);
        }
    }

    GetWorldTimerManager().SetTimer(
        RespawnTimerHandle,
        this,
        &AShooterEnemy::Respawn,
        RespawnDelay,
        false
    );

    /*GetWorldTimerManager().SetTimerForNextTick([this]()
        {
            GetWorldTimerManager().SetTimer(
                RespawnTimerHandle,
                this,
                &AShooterEnemy::Respawn,
                RespawnDelay,
                false
            );
        });*/
}

void AShooterEnemy::Respawn()
{
    bIsDead = false;
    CurrentHP = MaxHP;

    SetActorLocation(SpawnLocation);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
}

void AShooterEnemy::OnRep_HP()
{
    // 以后可加血条表现
}

void AShooterEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AShooterEnemy, CurrentHP);
}
