#include "MapInstance.h"

#include "Unloved/Maps/MapManager.h"

namespace Unloved {

    uint32_t MapInstance::GetChunkCountX() {
        Map* map = MapManager::GetMapByIndex(m_mapIndex);
        if (!map) return 0;
        else return map->GetChunkCountX();
    }

    uint32_t MapInstance::GetChunkCountZ() {
        Map* map = MapManager::GetMapByIndex(m_mapIndex);
        if (!map) return 0;
        else return map->GetChunkCountZ();
    }

} // namespace Unloved
