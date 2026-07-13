#include "DDGIManager.h"

#include "Unloved/Editor/ObjectNames.h"
#include "Unloved/ObjectId.h"

namespace Unloved::DDGIManager {
    Hell::SlotMap<DDGIVolume> g_volumes;
    bool g_probeResetRequested = false;

    Hell::SlotMap<DDGIVolume>& GetVolumes() {
        return g_volumes;
    }

    DDGIVolume* GetVolumeByObjectId(uint64_t objectId) {
        return g_volumes.get(objectId);
    }

    uint64_t AddVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset) {
        Editor::AssignEditorName(createInfo, g_volumes);

        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DDGI_VOLUME);
        g_volumes.emplace_with_id(id, id, createInfo, spawnOffset);
        return id;
    }

    bool RemoveVolume(uint64_t objectId) {
        DDGIVolume* volume = g_volumes.get(objectId);
        if (!volume) return false;

        volume->CleanUp();
        g_volumes.erase(objectId);
        return true;
    }

    void Update() {
        for (DDGIVolume& volume : g_volumes) {
            volume.Update();
        }
    }

    void CleanUp() {
        for (DDGIVolume& volume : g_volumes) {
            volume.CleanUp();
        }

        g_volumes.clear();
        g_probeResetRequested = false;
    }

    void ResetProbes() {
        g_probeResetRequested = true;
    }

    bool ConsumeProbeResetRequest() {
        const bool requested = g_probeResetRequested;
        g_probeResetRequested = false;
        return requested;
    }
}
