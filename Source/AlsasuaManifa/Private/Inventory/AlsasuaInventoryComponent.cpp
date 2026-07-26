#include "Inventory/AlsasuaInventoryComponent.h"

UAlsasuaInventoryComponent::UAlsasuaInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UAlsasuaInventoryComponent::AddItem(FAlsasuaItemData NewItem)
{
    if (CurrentWeight + NewItem.Weight > MaxWeight) return false;

    for (FAlsasuaItemData& Item : Items)
    {
        if (Item.ItemID == NewItem.ItemID && Item.Quantity < Item.MaxStack)
        {
            Item.Quantity += NewItem.Quantity;
            RecalculateWeight();
            OnInventoryChanged.Broadcast();
            return true;
        }
    }

    Items.Add(NewItem);
    RecalculateWeight();
    OnInventoryChanged.Broadcast();
    return true;
}

bool UAlsasuaInventoryComponent::RemoveItem(FName ItemID, int32 Amount)
{
    for (int32 i = 0; i < Items.Num(); i++)
    {
        if (Items[i].ItemID == ItemID)
        {
            Items[i].Quantity -= Amount;
            if (Items[i].Quantity <= 0) Items.RemoveAt(i);
            RecalculateWeight();
            OnInventoryChanged.Broadcast();
            return true;
        }
    }
    return false;
}

void UAlsasuaInventoryComponent::UseItem(FName ItemID)
{
    // Find the item in inventory.
    FAlsasuaItemData* FoundItem = nullptr;
    for (FAlsasuaItemData& Item : Items)
    {
        if (Item.ItemID == ItemID)
        {
            FoundItem = &Item;
            break;
        }
    }
    if (!FoundItem)
    {
        UE_LOG(LogAlsasua, Warning, TEXT("UseItem: Item %s no encontrado en inventario."), *ItemID.ToString());
        return;
    }

    AActor* Owner = GetOwner();
    if (!Owner) return;

    // Try to use via IAlsasuaItemUsable interface.
    if (Owner->GetClass()->ImplementsInterface(UAlsasuaItemUsable::StaticClass()))
    {
        bool bSuccess = IAlsasuaItemUsable::Execute_OnUse(Owner, Owner);
        if (bSuccess)
        {
            RemoveItem(ItemID, 1);
            UE_LOG(LogAlsasua, Log, TEXT("Item %s usado correctamente."), *ItemID.ToString());
        }
        else
        {
            UE_LOG(LogAlsasua, Warning, TEXT("UseItem: OnUse falló para %s."), *ItemID.ToString());
        }
    }
    else
    {
        // No Usable interface — just consume the item.
        RemoveItem(ItemID, 1);
        UE_LOG(LogAlsasua, Log, TEXT("Item %s consumido (sin interfaz IAlsasuaItemUsable)."), *ItemID.ToString());
    }
}

void UAlsasuaInventoryComponent::RecalculateWeight()
{
    CurrentWeight = 0.0f;
    for (const auto& Item : Items) CurrentWeight += (Item.Weight * Item.Quantity);
}
