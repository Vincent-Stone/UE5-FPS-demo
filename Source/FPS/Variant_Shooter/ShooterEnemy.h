#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterEnemy.generated.h"

class USphereComponent;
class AShooterCharacter;

UCLASS()
class FPS_API AShooterEnemy : public AActor
{
    GENERATED_BODY()

public:
    AShooterEnemy();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    FTimerHandle RespawnTimerHandle;

    /** 碰撞球（也是感知范围） */
    UPROPERTY(VisibleAnywhere)
    USphereComponent* CollisionSphere;

    /** 可视模型 */
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;

    /** 当前目标玩家（仅服务器使用） */
    UPROPERTY()
    AShooterCharacter* TargetCharacter;

    /** Enemy HP */
    UPROPERTY(ReplicatedUsing = OnRep_HP)
    float CurrentHP;

    UPROPERTY(EditDefaultsOnly)
    float MaxHP = 50.f;

    /** 移动速度 */
    UPROPERTY(EditDefaultsOnly)
    float MoveSpeed = 300.f;

    /** 重生点 */
    FVector SpawnLocation;

    /** 重生延迟 */
    UPROPERTY(EditDefaultsOnly)
    float RespawnDelay = 5.f;

    /** 是否已死亡 */
    bool bIsDead = false;

protected:
    /** 搜索最近玩家 */
    void FindTarget();

    /** 移动逻辑（服务器） */
    void MoveToTarget(float DeltaSeconds);

    /** 碰到玩家 */
    UFUNCTION()
    void OnOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    /** 死亡 */
    void Die(AController* KillerController);

    /** 重生 */
    void Respawn();

    /** HP 同步 */
    UFUNCTION()
    void OnRep_HP();

public:
    /** 被子弹/榴弹伤害 */
    virtual float TakeDamage(
        float Damage,
        struct FDamageEvent const& DamageEvent,
        AController* EventInstigator,
        AActor* DamageCauser
    ) override;

    /** Replication */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
