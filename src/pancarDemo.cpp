//============================================================================
// Name        : physics.cpp
// Author      : Enis Bayramoglu
// Version     :
// Copyright   : If you copy this work and become a millionaire out of it and you don't give me a dime, I HATE YOU!
// Description : Hello World in C++, Ansi-style
//============================================================================
#define __SCROLL_IMPL__ // This definition causes the Scroll symbols to be compiled with this unit

#include <thread>
#include <mutex>
#include <fstream>

#include <orx.h>

#include <scroll_ext/InMemoryTexture.h>
#include <scroll_ext/ExtendedObject.h>
#include "orx_utilities.h"
#include <util/config_processors.h>
#include "pancarDemo.h"
#include <scroll_ext/config_utils.hpp>
#include "persistence.h"
#include "remote.h"
#include "analytics.h"
#include "monetization.h"
#include "platform.h"
#include <game_state.h>
#include <test/test.h>
#include <gameObjects/screens/SimpleScreen.h>

#include "util/random.hpp"

using std::min;
using std::max;

template pancarDemo & Scroll<pancarDemo>::GetInstance();

void orderViewports();
void processLevelList();
void ProcessSoundBusSection(const char * section);

std::vector<std::function<void()>> & get_initializers() {
    static std::vector<std::function<void()>> initializers;
    return initializers;
}

void register_initializer(std::function<void()> initializer) {
    get_initializers().push_back(initializer);
}

class SoundDummy: public ExtendedMonomorphic {
public:
    virtual void ExtOnCreate() {
        orxObject_Pause(GetOrxObject(),true);
    }
    SCROLL_BIND_CLASS("SoundDummy")
};

extern InMemoryTexture crow_father_partmap;

std::string liveconfig_name() {
    auto game_ver = ::GetValue<std::string>("BuildInfo", "Version");
    return "lc" + game_ver;
}

class LiveConfigDownloader {
    std::unique_ptr<std::thread> worker;
    struct {
        canceller_t cancel_download{false};
        bool request_completed = false;
        response_t response;
        std::mutex mut;
    } shared_state;
public:
    LiveConfigDownloader() {
        const char * pubkeyhash = GetValue<const char *>("LiveConfig", "PubkeyHash");
        auto game_ver = ::GetValue<std::string>("BuildInfo", "Version");
        worker.reset(new std::thread([=]() {
            auto info = GetPlatform()->getPlatformInfo();
            params_t p = {{"uid", info.user_id}, {"osver", info.os_version}};
            auto url = "https://api.mategames.com/featherytale/" + info.platform_name + "/" + game_ver + "/liveconfig";
            response_t resp = get(url, p, -1, &shared_state.cancel_download, pubkeyhash);
            std::lock_guard<std::mutex> guard(shared_state.mut);
            shared_state.request_completed = true;
            shared_state.response = resp;
        }));
    }
    bool SaveFileIfDownloadCompleted() {
        bool completed; response_t resp;
        {
            std::lock_guard<std::mutex> guard(shared_state.mut);
            completed = shared_state.request_completed;
            resp = shared_state.response;
        }
        if(completed && resp.response_code == 200) {
            auto livepath = orxFile_GetApplicationSaveDirectory(liveconfig_name().c_str());
            std::ofstream liveconfig(livepath);
            liveconfig << resp.response;
            orxConsole_Log("Successfully downloaded live config.");
            return true;
        } else {
            std::stringstream ss;
            ss << "Live config was not downloaded; Completed: " << completed << " Resp: " << resp.response_code;
            std::cerr << ss.str() << std::endl;
            orxConsole_Log(ss.str().c_str());
            return false;
        }
    }
    void StopDownloadIfNotFinished() {
        std::lock_guard<std::mutex> guard(shared_state.mut);
        shared_state.cancel_download = true;
    }
    ~LiveConfigDownloader() {
        StopDownloadIfNotFinished();
        if(worker->joinable()) worker->detach();
    }
};

struct LoadingScreen: SimpleScreen {
    SCROLL_BIND_CLASS("LoadingScreen")
    LiveConfigDownloader downloader;
    LoadingScreen() {
        get_texture("common/images/crow_father_map.png", [&](InMemoryTexture tex) {crow_father_partmap = std::move(tex);});
    }
    virtual void EndScreen() {
        downloader.SaveFileIfDownloadCompleted();
        orxConfig_Load(orxFile_GetApplicationSaveDirectory(liveconfig_name().c_str()));
        applySettings();
        processLevelList();
        ProcessSoundBusSection("PhysicsFXBus");
        start_analytics_session();
        SetLifeTime(0);
    }
    virtual void Update(const orxCLOCK_INFO &_rstInfo) {
        if(_rstInfo.fTime > GetValue<orxFLOAT>("Duration")) {
            SetNextScreen(::GetValue<const char *>("Game", "StartingScreen"));
        }
    }
};

void SetGroup(orxOBJECT * obj, const char * group_name) {
    auto group_id = orxString_GetID(group_name);
    orxObject_SetGroupID(obj, group_id);
}

orxVIEWPORT * GetCursorViewport() {
    ConfigSectionGuard guard("Runtime");
    if(orxConfig_HasValue("CursorViewport")) {
        auto id = orxConfig_GetU64("CursorViewport");
        return (orxVIEWPORT *) orxStructure_Get(id);
    } else {
        auto new_viewport = orxViewport_CreateFromConfig("CursorViewport");
        orxConfig_SetU64("CursorViewport", orxStructure_GetGUID(new_viewport));
        orderViewports();
        return new_viewport;
    }
}

struct Cursor: ExtendedMonomorphic {
    SCROLL_BIND_CLASS("Cursor")
    orxVIEWPORT * viewport;
    Cursor(): viewport(GetCursorViewport()) {
    }
    virtual void Update(const orxCLOCK_INFO &_rstInfo) {
        auto mInWrld = getMouseInWorld(viewport);
        SetGroup(GetOrxObject(), mInWrld ? "Cursor" : "Hidden");
        if(mInWrld) {
            mInWrld->fZ = 0;
            SetPosition(*mInWrld);
            if(orxInput_IsActive("Pick")) SetAnim("HandTouchAnim");
            else SetAnim("HandHoverAnim");
        }
    }
};

using hr_clock = std::chrono::high_resolution_clock;

std::mt19937 gen;
orxFLOAT loading_duration;
orxOBJECT * loading_screen;

void seed_random_sources() {
    orxU32 seed[4] = {0,0,0,0};
    for (orxU32 & i:seed) i = hr_clock::now().time_since_epoch().count();
    orxMath_SetRandomSeeds(seed);
    gen.seed(seed[0]);
}

void orderViewports() {
    if(!orxStructure_GetFirst(orxSTRUCTURE_ID_VIEWPORT)) return;
    bool sorted = false;
    while(!sorted) {
        sorted = true;
        auto cur = orxStructure_GetFirst(orxSTRUCTURE_ID_VIEWPORT);
        while(auto next = orxStructure_GetNext(cur)) {
            orxFLOAT cur_order = GetValue<orxFLOAT>(orxViewport_GetName(orxVIEWPORT(cur)), "Order");
            orxFLOAT next_order = GetValue<orxFLOAT>(orxViewport_GetName(orxVIEWPORT(next)), "Order");
            if(cur_order > next_order) {
                orxLinkList_Remove((orxLINKLIST_NODE *) cur->hStorageNode);
                orxLinkList_AddAfter((orxLINKLIST_NODE *) next->hStorageNode, (orxLINKLIST_NODE *) cur->hStorageNode);
                sorted = false;
            } else {
                cur = next;
            }
        }
    }
}

void updatePersistentObjects(const char * new_level) {
    std::set<std::string> persistentSet;
    {
        ConfigSectionGuard guard(new_level);
        ConfigListRange<const char *> persistentList("PersistentList");
        persistentSet.insert(persistentList.begin(), persistentList.end());
    }
    ConfigSectionGuard guard("PersistenObjects");
    for(auto name: persistentSet) {
        auto obj_id = orxConfig_GetU64(name.c_str());
        if(obj_id && orxStructure_Get(obj_id)) continue;
        else orxConfig_SetU64(name.c_str(), orxStructure_GetGUID(orxObject_CreateFromConfig(name.c_str())));
    }
    auto num_keys = orxConfig_GetKeyCounter();
    for(size_t i=0; i<num_keys; i++) {
        auto name = orxConfig_GetKey(i);
        auto obj_id = orxConfig_GetU64(name);
        auto obj_ptr = orxOBJECT(orxStructure_Get(obj_id));
        if(obj_ptr && persistentSet.find(name) == persistentSet.end()) {
            orxObject_SetLifeTime(obj_ptr, 0);
        }
    }
}

// TODO check if the object really is a level
void newLevel(pancarDemo & game, const char * level_name) {
    if(game.level) game.level->EndScreen();
    updatePersistentObjects(level_name);
    auto levelObj = ExtendedMonomorphic::CreateNamed(level_name);
    game.level = levelObj->GetAspect<game_screen>();
    game.registered_manager = &game.level->GetGestureManager();
    auto output_texture = orxTexture_CreateFromFile(orxTEXTURE_KZ_SCREEN_NAME);
    game.level->SetOutputTexture(output_texture);
    orderViewports();
}

orxSTATUS orxFASTCALL lifetimeHandler(const orxEVENT *_pstEvent) {
    switch(_pstEvent->eID) {
    case orxOBJECT_EVENT_DELETE:
        auto obj = orxOBJECT(_pstEvent->hSender);
        deleteChildrenRecursive(obj);
        break;
    }
    return orxSTATUS_SUCCESS;
}

orxSTATUS orxFASTCALL touchHandler(const orxEVENT *_pstEvent);
orxSTATUS orxFASTCALL slowdownCoeffParamHandler(const orxEVENT *_pstEvent);

void ProcessSoundBusSection(const char * section) {
    auto section_id = orxString_GetID(section);
    ConfigSectionGuard guard(section);
    if(orxConfig_HasValue("Parent")) {
        orxSound_SetBusParent(section_id, orxString_GetID(orxConfig_GetString("Parent")));
    }
    if(orxConfig_HasValue("Volume")) {
        orxSound_SetBusVolume(section_id, orxConfig_GetFloat("Volume"));
    }
    if(orxConfig_HasValue("Pitch")) {
        orxSound_SetBusPitch(section_id, orxConfig_GetFloat("Pitch"));
    }
}


orxSTATUS pancarDemo::Init ()
{
    orxLOG("Starting Game");
    {
        ConfigSectionGuard guard("Screenshot");
        auto screenshot_directory = orxFile_GetApplicationSaveDirectory("screenshots");
        orxFile_MakeDirectory(screenshot_directory);
        orxConfig_SetString("Directory", screenshot_directory);
    }
    init_remote();
    init_persistence();

    setGameSession(getGameSession()+1);

    init_analytics();
    init_monetization();
    for(auto & initializer: get_initializers()) initializer();
    orxViewport_Enable(GetMainViewport(), false);

    if(GetValue<bool>("Mouse", "ShowCustomCursor")) Create<Cursor>();
    createTestCursorIfTestMode();

    seed_random_sources();

    init_in_memory_texture();
    orxEvent_AddHandler ( orxEVENT_TYPE_SYSTEM, touchHandler);
    orxEvent_AddHandler ( orxEVENT_TYPE_OBJECT, lifetimeHandler);
    orxEvent_AddHandler ( orxEVENT_TYPE_SHADER, slowdownCoeffParamHandler);

    applySettings();
    newLevel(*this, GetBindName<LoadingScreen>());
    orxViewport_AddShader(GetMainViewport(),"PassthroughShader");

    return orxSTATUS_SUCCESS;
}

orxSTATUS pancarDemo::Run ()
{
    orxSTATUS eResult = orxSTATUS_SUCCESS;
    update_analytics();
    update_monetization();

    return eResult;
}

// double DESIRED_MIN_FRAME_RATE = 60;
void pancarDemo::Update(const orxCLOCK_INFO &_rstInfo) {
    debugActions();

    if(const char * next_level = level->GetNextScreen()) {
        if(RequestTransition(next_level)) newLevel(*this, next_level);
    }

    if(registered_manager) registered_manager->time_update(orxSystem_GetTime());
}

orxOBJECT * debugBoard = nullptr;

void pancarDemo::debugActions() {
    if(IsPressed("CreateBoard")) {
        if(debugBoard) {
            orxObject_SetLifeTime(debugBoard,0);
            debugBoard = nullptr;
        } else debugBoard = orxObject_CreateFromConfig("Board");
    }
}

/** Exit function
 */
void pancarDemo::Exit()
{
    exit_monetization();
    exit_analytics();
    cleanup_remote();
}

pancarDemo::pancarDemo() {

}

void config_SetStringVector(const char * key, std::vector<const char *> list) {
    if(list.size()>0) orxConfig_SetListString(key, list.data(), list.size());
}

// This function does the following:
// * Generates StoryModePage's with "Screens" keys pointing at levels
// * Generates StoryModePage's Previous & NextPage keys for page navigation
// * Assigns the story mode page that the level should go to when menu pressed (MenuScreen)
// * Assigns the "NextLevel" property for each level.
// * Assigns the "DependsOn" property for each level.
// * Sets LevelNumber for each level.
// * Sets MinHearts for each level.
// * Sets the observers for each level.
// * Sets the tools for each level.
// * Sets the HintString by assembling the HintSets for each level
// * Sets the Challenge1/2 keys to $[ChallengeDescPrefix]Chal1/2
// * Sets CutSceneNumber for each cut scene.
void processLevelList() {
    std::vector<const char *> levels;
    {
        ConfigSectionGuard guard("GameMetaData");
        for(auto level: ConfigListRange<const char *>("Levels")) {levels.push_back(level);}
    }

    auto get_page_name = [](int page_num){
        return Persist(("StoryModePage" + to_string(page_num)).c_str());
    };

    const size_t levels_per_page = 8;
    size_t level_number = 1;
    size_t cutscene_number = 1;
    size_t min_hearts = 0;
    size_t min_heart_increment = 0;
    std::vector<const char *> tools, observers;
    for(size_t i=0; i<levels.size(); i+=levels_per_page) {
        size_t page_num = i/levels_per_page;
        auto cur_page_name = get_page_name(page_num);
        size_t num_of_levels = std::min(levels.size()-i, levels_per_page);

        ConfigSectionGuard page_guard(cur_page_name);
        orxConfig_SetParent(cur_page_name, "LevelSelectScreen");

        bool last_page = i >= levels.size() - levels_per_page;
        if(!last_page) orxConfig_SetString("NextPage", get_page_name(page_num+1));
        if(i>0) orxConfig_SetString("PreviousPage", get_page_name(page_num-1));

        orxConfig_SetListString("Screens", levels.data()+i, num_of_levels);

        for(size_t j=i; j<i+num_of_levels; ++j) {
            const char * previous_level = nullptr;
            // Find previous level
            for(int k=j-1; k>=0; --k) {
                if(GetValue<std::string>(levels[k],"ScreenType") == "Level") {
                    previous_level = levels[k];
                    break;
                }
            }

            ConfigSectionGuard level_guard(levels[j]);
            if(previous_level) orxConfig_SetString("DependsOn", previous_level);
            if(j<levels.size()-1) orxConfig_SetString("NextLevel", levels[j+1]);
            orxConfig_SetString("MenuScreen", cur_page_name);
            if(orxConfig_GetString("ScreenType") == std::string("Level")) {
                orxConfig_SetU32("LevelNumber", level_number);
                level_number++;
                if(orxConfig_HasValue("MinHeartIncrement")) min_heart_increment = orxConfig_GetU32("MinHeartIncrement");
                min_hearts += min_heart_increment;
                orxConfig_SetU32("MinHearts", min_hearts);
                auto process_list_increment = [&](const char * incr_key,
                                                  const char * list_key,
                                                  std::vector<const char *> & list) {
                    if(orxConfig_HasValue(incr_key)) {
                        auto incr = orxConfig_GetString(incr_key);
                        if(incr[0] == '~') {
                            config_SetStringVector(list_key, list);
                            list.push_back(incr+1);
                        } else {
                            list.push_back(incr);
                            config_SetStringVector(list_key, list);
                        }
                    } else config_SetStringVector(list_key, list);
                };
                process_list_increment("NewTool", "Tools", tools);
                process_list_increment("NewObserver", "Observers", observers);
                std::string prefix = orxConfig_GetString("ChallengeDescPrefix");
                orxConfig_SetString("Challenge1", ("$"+prefix+"C1").c_str());
                orxConfig_SetString("Challenge2", ("$"+prefix+"C2").c_str());

                // Set HintString based on HintSets
                std::vector<const char *> hints;
                for(auto hint_set : ConfigListRange<const char *>("HintSets")) {
                    ConfigSectionGuard hint_set_guard("HintSets");
                    for(auto hint: ConfigListRange<const char *>(hint_set)) {
                        hints.push_back(hint);
                    }
                }
                orxConfig_SetListString("HintString", hints.data(), hints.size());
            } else if(orxConfig_GetString("ScreenType") == std::string("CutScene")) {
                orxConfig_SetU32("CutSceneNumber", cutscene_number);
                cutscene_number++;
            }
        }
    }
}

class PCPlatform: public Platform {
    bool adFree = false;
    platform_info getPlatformInfo() {
        platform_info result;
        result.user_id = "PC_TESTER";
        result.platform_name = "ios";
        result.device_name = "PC";
        result.manufacturer = "SomeDudeInc";
        result.os_version = "ios 8.1";
        result.device_language = "tr";
        return result;
    }
    void showAd() {
        std::cout << "Showing ad" << std::endl;
        SetAdRequestResult(true);
    }
    bool isAdFree() {
        return adFree;
    }
    void purchaseAdFree() {
        std::cout << "Going Ad Free!" << std::endl;
        adFree = true;
        adFreePurchased();
    }
    void purchaseConsumable(item_info item) {
        std::cout << "Purchasing item: " << item.item_name << std::endl;
        consumablePurchased(item);
    }
    void queryInventory(std::vector<item_info> items) {
        static const std::map<std::string, std::string> prices = {
                {"a_few_coins", "2.00TL"},
                {"sack_of_coins", "9.00TL"},
                {"chest_of_coins", "16.00TL"},
                {"adfree", "10.00TL"},
        };
        std::vector<item_inventory_info> result;
        for(auto & item: items) {
            item_inventory_info inventory_info = {item.item_name, prices.at(item.item_name)};
            result.push_back(inventory_info);
        }
        reportInventory(result);
    }
    void showStorePage() {
        std::cout << "Showing store page" << std::endl;
    }
    void showShareDialog() {
        std::cout << "Showing share dialog" << std::endl;
        reportFacebookShareResult(true);
    }

    void showShareDialog(share_dialog_info info) {
        std::cout << "Showing share dialog with image " << info.image_path << std::endl;
        std::cout << " And with description: " << info.description << std::endl;
        reportFacebookShareResult(true);
    }
};

const char * pancarDemo::GetEncryptionKey() const {
    return "UJLtiRhipgxERQtaefikQD9PJLccxrC5";
}

int main(int argc, char* argv[])
{
    // TODO add sliders for relevant parameters
    // TODO re-bonding
    // TODO object serialization, loading and saving
    // TODO object editor
    // TODO implement "muscles", bonds, that respond to intelligent input.
    // FIXME Currently, a debug build of the application fails on my HTC Desire S,
    //       probably an assertion is being triggered, try to find out what it is.
    //       Hint: physics object secondary texture is not shown.
    //       Found it: [ASSERT] : <pstShader->s32ParamCounter < sstDisplay.iTextureUnitNumber>
    //       This is caused by the fact that the device can only handle one texture per shader
    //       Design a render system where texture limits are resolved gracefully.

#ifdef PC_BUILD
    SetPlatform(new PCPlatform);
#endif
    try {
        pancarDemo::GetInstance().Execute (argc, argv);
    } catch (...) {
        orxLOG("Something wrong");
    }

    return EXIT_SUCCESS;
}
