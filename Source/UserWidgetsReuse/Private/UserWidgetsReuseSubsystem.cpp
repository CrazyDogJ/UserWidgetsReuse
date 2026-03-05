// Fill out your copyright notice in the Description page of Project Settings.


#include "UserWidgetsReuseSubsystem.h"

void UUserWidgetsReuseSubsystem::Deinitialize()
{
	Super::Deinitialize();

	ReleaseReuseWidgets();
	ReleaseAllPools();
}

FUserWidgetPool* UUserWidgetsReuseSubsystem::GetOrAddWidgetPool(const TSubclassOf<UUserWidget>& WidgetClass)
{
	if (const auto Found = WidgetPool.Find(WidgetClass))
	{
		return Found;
	}

	auto* New = &WidgetPool.Add(WidgetClass);
	New->SetWorld(GetWorld());
	New->SetDefaultPlayerController(GetWorld()->GetFirstLocalPlayerFromController()->PlayerController);
	return New;
}

UUserWidget* UUserWidgetsReuseSubsystem::GetOrCreateWidget(TSubclassOf<UUserWidget> InUserWidgetClass, bool bCreate)
{
	if (const auto Found = WidgetMap.Find(InUserWidgetClass))
	{
		return *Found;
	}

	if (bCreate)
	{
		const auto NewWidget = CreateWidget<UUserWidget, APlayerController*>(GetWorld()->GetFirstLocalPlayerFromController()->PlayerController, InUserWidgetClass);
		if (NewWidget)
		{
			WidgetMap.Add(InUserWidgetClass, NewWidget);
		}
	
		return NewWidget;
	}
	
	return nullptr;
}

UUserWidget* UUserWidgetsReuseSubsystem::RequestUserWidget(TSubclassOf<UUserWidget> InWidgetClass)
{
	if (!InWidgetClass)
	{
		return nullptr;
	}

	const auto Pool = GetOrAddWidgetPool(InWidgetClass);
	const auto WidgetInstance = Pool->GetOrCreateInstance(InWidgetClass);

	return WidgetInstance;
}

void UUserWidgetsReuseSubsystem::ReleaseUserWidget(UUserWidget* InUserWidget)
{
	if (InUserWidget)
	{
		const auto WidgetClass = InUserWidget->GetClass();
		if (const auto Pool = GetOrAddWidgetPool(WidgetClass))
		{
			InUserWidget->RemoveFromParent();
			Pool->Release(InUserWidget);
		}
	}
}

void UUserWidgetsReuseSubsystem::ReleaseReuseWidgets()
{
	WidgetMap.Empty();
}

void UUserWidgetsReuseSubsystem::ReleaseAllPools()
{
	for (auto Itr : WidgetPool)
	{
		Itr.Value.ReleaseAll(true);
		Itr.Value.ReleaseAllSlateResources();
		Itr.Value.ResetPool();
	}
	WidgetPool.Empty();
}

void UUserWidgetsReuseSubsystem::ReleasePool(TSubclassOf<UUserWidget> Class)
{
	if (const auto Found = WidgetPool.Find(Class))
	{
		Found->ReleaseAll(true);
		Found->ReleaseAllSlateResources();
		Found->ResetPool();
		WidgetPool.Remove(Class);
	}
}
