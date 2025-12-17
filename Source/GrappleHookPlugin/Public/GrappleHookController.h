// Copyright (c) 2025 Brian Pimentel

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleHookController.generated.h"


class UCharacterMovementComponent;
class UInputMappingContext;
class UInputAction;


//Delegate for weapon being fired.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGrappleEvent);


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GRAPPLEHOOKPLUGIN_API UGrappleHookController : public UActorComponent
{
	GENERATED_BODY()

public:
	
	// Sets default values for this component's properties
	UGrappleHookController();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	//Input Action to map to grapple hook
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple Hook")
	TObjectPtr<UInputAction> ActionGrappleHook = nullptr;
	
	//Input Mapping Context to use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple Hook")
	TObjectPtr<UInputMappingContext> InputMappingContext = nullptr;

	UFUNCTION(BlueprintPure, Category = "Grapple Hook", meta = (ToolTip = "Whether or not the Player is aiming at a valid target."))
	bool HasValidGrappleTarget() const;

	UFUNCTION(BlueprintPure, Category = "Grapple Hook", meta = (ToolTip = "Whether or not the Player is currently using the Grapple Hook."))
	bool IsGrappling() const;

	UFUNCTION(BlueprintPure, Category = "Grapple Hook", meta = (ToolTip = "Returns the point that the Player is grappling towards."))
	AActor* GetGrappleEndPointActor() const;
	
	void SetupGrappleHookInput();

	//Delegates
	UPROPERTY(BlueprintAssignable, Category = "Grapple Hook", meta = (ToolTip = "Hook up additional functionality to this."))
	FGrappleEvent OnGrappleStart;

	UPROPERTY(BlueprintAssignable, Category = "Grapple Hook", meta = (ToolTip = "Hook up additional functionality to this."))
	FGrappleEvent OnGrappleEnd;
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void HandleUseGrappleHook();

	UFUNCTION(BlueprintCallable, Category = "Grapple Hook")
	void CancelGrapple();

	void SetupGrapplePointActor(FVector ImpactPoint, USceneComponent* HitComponent);

	TOptional<FHitResult> GrappleHookLineTrace() const;

private:

	//Defaults
	static constexpr float BaseMaxGrappleRange = 10000.0f;
	static constexpr float BaseMaxSpeed = 2000.0f;
	static constexpr float BaseInitialSpeed = 500.0f;
	static constexpr float BaseSpeedLerpDuration = 1.0f;
	static constexpr float BaseReleaseRange = 100.0f;

	//Grapple Range value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float MaxGrappleRange = BaseMaxGrappleRange;

	//Grapple Speed value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float MaxSpeed = BaseMaxSpeed;

	//Grapple Speed value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float InitialSpeed = BaseInitialSpeed;

	//Grapple Speed value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float SpeedLerpDuration = BaseSpeedLerpDuration;

	//Grapple Release Range value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float ReleaseRange = BaseReleaseRange;

	//Used to store a reference to the InputComponent cast to an EnhancedInputComponent
	UPROPERTY()
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

	//Used to store a reference to the Player's PlayerController
	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController = nullptr;
	
	//Used to store a reference to the Pawn we are controlling
	UPROPERTY()
	TObjectPtr<APawn> PlayerPawn = nullptr;

	//Used to store a reference to the Character we are controlling
	UPROPERTY()
	TObjectPtr<ACharacter> PlayerCharacter = nullptr;
	
	//Used to store a reference to the pawn we are controlling
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> MovementComponent = nullptr;

	//Used to store a reference to where the Grapple hit. An actor because we want to dynamically track the destination if we've grappled to a moving object
	UPROPERTY()
	TObjectPtr<AActor> GrapplePoint = nullptr;


	float CurrentSpeed = InitialSpeed;
	float SpeedLerpElapsed = 0.0f;
	
	float PreviousGravityScale = 0.0f;
	bool PreviousYawBool = false;
};
