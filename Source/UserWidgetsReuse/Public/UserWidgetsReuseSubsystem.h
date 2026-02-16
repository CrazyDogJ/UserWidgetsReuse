// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/UserWidgetPool.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UserWidgetsReuseSubsystem.generated.h"

UCLASS()
class USERWIDGETSREUSE_API UUserWidgetsReuseSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
protected:
	UPROPERTY()
	TMap<TSubclassOf<UUserWidget>, UUserWidget*> WidgetMap;

	UPROPERTY()
	TMap<TSubclassOf<UUserWidget>, FUserWidgetPool> WidgetPool;
	
	virtual void Deinitialize() override;
	
private:
	FUserWidgetPool* GetOrAddWidgetPool(const TSubclassOf<UUserWidget>& WidgetClass);
	
public:
	/** Get or create a reuse user widget. */
	UFUNCTION(BlueprintCallable, meta=(DeterminesOutputType="InUserWidgetClass"))
	UUserWidget* GetOrCreateWidget(TSubclassOf<UUserWidget> InUserWidgetClass, bool bCreate = true);

	/** Requet user widget from pool. */
	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "InWidgetClass"))
	UUserWidget* RequestUserWidget(TSubclassOf<UUserWidget> InWidgetClass);
	
	UFUNCTION(BlueprintCallable)
	void ReleaseUserWidget(UUserWidget* InUserWidget);
	
	UFUNCTION(BlueprintCallable)
	void ReleasePool(TSubclassOf<UUserWidget> Class);
};
