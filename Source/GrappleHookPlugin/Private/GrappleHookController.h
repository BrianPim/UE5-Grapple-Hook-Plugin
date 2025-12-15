// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrappleHookController.generated.h"


class UInputAction;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GRAPPLEHOOKPLUGIN_API UGrappleHookController : public UActorComponent
{
	GENERATED_BODY()

public:

	//Input Action to map to grapple hook
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Grapple Hook")
	TObjectPtr<UInputAction> ActionGrappleHook = nullptr;
	
	// Sets default values for this component's properties
	UGrappleHookController();
	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	void SetupGrappleHookInput();
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	void HandleUseGrappleHook();

private:

	//Default
	static constexpr float BaseMaxGrappleRange = 1000.0f;

	//Grapple Range value, can be modified via BP
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grapple Hook", meta = (AllowPrivateAccess = "true"))
	float MaxGrappleRange = BaseMaxGrappleRange;

	//Used to store a reference to the InputComponent cast to an EnhancedInputComponent
	UPROPERTY()
	TObjectPtr<UEnhancedInputComponent> EnhancedInputComponent = nullptr;

	//Used to store a reference to the pawn we are controlling
	UPROPERTY()
	TObjectPtr<APawn> PlayerCharacter = nullptr;

	FVector GrapplePoint = FVector(0.f, 0.f, 0.f);
};
