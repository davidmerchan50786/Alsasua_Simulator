#include "World/Collectibles/HistoricalManager.h"
void UHistoricalManager::UnlockHistoricalItem(FHistoricalCollectible Item) {
    for(const auto& Existing : UnlockedItems) {
        if(Existing.Id == Item.Id) return;
    }
    UnlockedItems.Add(Item);
    OnItemUnlocked.Broadcast(Item);
}