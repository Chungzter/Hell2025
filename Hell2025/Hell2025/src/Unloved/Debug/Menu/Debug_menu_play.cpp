#include "Debug_menu.h"

#include "Hell/File/File.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Session/Session.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Debug::Menu::Play {

    enum struct Action : uint32_t {
        CAMPAIGN,
        DEATH_MATCH,
    };

    PageId g_homepageId = ROOT_PAGE_ID;
    PageId g_campaignPageId = ROOT_PAGE_ID;
    PageId g_deathMatchPageId = ROOT_PAGE_ID;
    std::vector<std::string> g_mapNames;

    void BuildMainMenu();
    void BuildCampaignMenu();
    void BuildDeathmatchMenu();

    void ApplyEdit(uint32_t, const Value&);
    void ApplyCampaignEdit(uint32_t id, const Value&);
    void ApplyDeathmatchEdit(uint32_t id, const Value&);

    void BuildMapList() {
        g_mapNames.clear();
        for (const FileInfo& fileInfo : Hell::File::IterateDirectory("res/maps", { "map" })) g_mapNames.push_back(fileInfo.name);
        std::sort(g_mapNames.begin(), g_mapNames.end());
        for (uint32_t i = 0; i < g_mapNames.size(); i++) AddAction(i, g_mapNames[i]);
    }

    void StartGame(uint32_t mapIndex, GameMode mode) {
        if (mapIndex >= g_mapNames.size()) return;
        Unloved::Session::StartNewGame(mode, g_mapNames[mapIndex]);
        Debug::HideMenu();
    }

    void RegisterMenu() {
        g_homepageId = RegisterRootPage("Play", "PLAY", BuildMainMenu, ApplyEdit);
        g_campaignPageId = RegisterPage("CAMPAIGN", g_homepageId, BuildCampaignMenu, ApplyCampaignEdit);
        g_deathMatchPageId = RegisterPage("DEATHMATCH", g_homepageId, BuildDeathmatchMenu, ApplyDeathmatchEdit);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        AddSubMenu(static_cast<uint32_t>(Action::CAMPAIGN), "Campaign", g_campaignPageId);
        AddSubMenu(static_cast<uint32_t>(Action::DEATH_MATCH), "Deathmatch", g_deathMatchPageId);
    }

    void BuildCampaignMenu() {
        BuildMapList();
    }

    void BuildDeathmatchMenu() {
        BuildMapList();
    }

    void ApplyEdit(uint32_t, const Value&) {
    }

    void ApplyCampaignEdit(uint32_t id, const Value&) {
        StartGame(id, GameMode::CAMPAIGN);
    }

    void ApplyDeathmatchEdit(uint32_t id, const Value&) {
        StartGame(id, GameMode::DEATH_MATCH);
    }

}
