// Fill out your copyright notice in the Description page of Project Settings.


#include "UserWidgetsReuseSubsystem.h"

FUserWidgetPool* UUserWidgetsReuseSubsystem::GetOrAddWidgetPool(const TSubclassOf<UUserWidget>& WidgetClass)
{
	if (bReleasingWorld || !GetWorld() || !GetWorld()->GetFirstLocalPlayerFromController())
	{
		return nullptr;
	}
	
	if (const auto Found = WidgetPool.Find(WidgetClass))
	{
		return Found;
	}

	auto* New = &WidgetPool.Add(WidgetClass);
	New->SetWorld(GetWorld());
#if ENGINE_MAJOR_VERSION == 4
	
#else
	New->SetDefaultPlayerController(GetWorld()->GetFirstLocalPlayerFromController()->PlayerController);
#endif
	return New;
}

UUserWidget* UUserWidgetsReuseSubsystem::GetOrCreateWidget(TSubclassOf<UUserWidget> InUserWidgetClass, bool bCreate)
{
	if (bReleasingWorld) return nullptr;
	if (const auto Found = WidgetMap.Find(InUserWidgetClass))
	{
		if (IsValid(*Found)) return *Found;
		WidgetMap.Remove(InUserWidgetClass);
	}

	// 修复：专服、地图切换或本地控制器尚未建立时，不能创建界面。
	const auto LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	APlayerController* Controller = LocalPlayer ? LocalPlayer->PlayerController : nullptr;
	if (bCreate && InUserWidgetClass && IsValid(Controller))
	{
		const auto NewWidget = CreateWidget<UUserWidget>(Controller, InUserWidgetClass);
		
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

	if (const auto Pool = GetOrAddWidgetPool(InWidgetClass))
	{
		const auto WidgetInstance = Pool->GetOrCreateInstance(InWidgetClass);
		return WidgetInstance;
	}
	
	return nullptr;
}

void UUserWidgetsReuseSubsystem::ReleaseUserWidget(UUserWidget* InUserWidget)
{
	if (InUserWidget)
	{
		const auto WidgetClass = InUserWidget->GetClass();
		// 修复：释放时只访问现有池，避免在关卡退出时建立新池。
		const auto* Pool = WidgetPool.Find(WidgetClass);
		if (Pool && Pool->GetActiveWidgets().Contains(InUserWidget))
		{
			InUserWidget->RemoveFromParent();
			// RemoveFromParent 可能经蓝图回调销毁原池。
			if (auto* CurrentPool = WidgetPool.Find(WidgetClass)) CurrentPool->Release(InUserWidget);
		}
	}
}

void UUserWidgetsReuseSubsystem::ReleaseReuseWidgets()
{
	// 修复：视口也持有单例控件，清空缓存时同步解除父子关系。
	auto Widgets = MoveTemp(WidgetMap);
	for (const auto& Pair : Widgets)
	{
		if (IsValid(Pair.Value)) Pair.Value->RemoveFromParent();
	}
}

void UUserWidgetsReuseSubsystem::ReleaseAllPools()
{
	// 修复：从注册表移出正在关闭的池，直接释放原池持有的 Slate 引用。
	auto PoolsToRelease = MoveTemp(WidgetPool);
	for (auto& Itr : PoolsToRelease)
	{
		const auto Active = Itr.Value.GetActiveWidgets();
		for (auto* Widget : Active) if (IsValid(Widget)) Widget->RemoveFromParent();
		Itr.Value.ReleaseAll(true);
		Itr.Value.ReleaseAllSlateResources();
		Itr.Value.ResetPool();
	}
}

void UUserWidgetsReuseSubsystem::ReleasePool(TSubclassOf<UUserWidget> Class)
{
	// 修复：先移出注册表，控件卸载回调不能再访问正在释放的池。
	FUserWidgetPool Removed;
	if (WidgetPool.RemoveAndCopyValue(Class, Removed))
	{
		const auto Active = Removed.GetActiveWidgets();
		for (auto* Widget : Active) if (IsValid(Widget)) Widget->RemoveFromParent();
		Removed.ReleaseAll(true);
		Removed.ReleaseAllSlateResources();
		Removed.ResetPool();
	}
}

void UUserWidgetsReuseSubsystem::Deinitialize()
{
	bReleasingWorld = true;
	ReleaseReuseWidgets();
	ReleaseAllPools();
	Super::Deinitialize();
}