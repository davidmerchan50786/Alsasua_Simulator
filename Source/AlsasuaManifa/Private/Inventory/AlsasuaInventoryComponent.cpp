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
    // Lógica de equipamiento con delay de 0.75s (simulado para BP/GAS)
    UE_LOG(LogAlsasua, Log, TEXT("Iniciando uso de item: %s... (0.75s equip time)"), *ItemID.ToString());
    // Se conectaría con un Montage de equipamiento
}

void UAlsasuaInventoryComponent::RecalculateWeight()
{
    CurrentWeight = 0.0f;
    for (const auto& Item : Items) CurrentWeight += (Item.Weight * Item.Quantity);
}
