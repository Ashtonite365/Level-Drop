// Level Drop: import .gmd / raw level-string .txt files from a drop folder as NEW local levels.
#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>
#include <Geode/modify/CreatorLayer.hpp>
#include <Geode/ui/Notification.hpp>
#include <hjfod.gmd-api/include/GMD.hpp>
#include <chrono>
#include <fstream>
#include <sstream>

using namespace geode::prelude;
namespace fs = std::filesystem;

static fs::path dropDir() {
    auto p = Mod::get()->getSettingValue<fs::path>("drop-folder");
    if (p.empty()) p = dirs::getSaveDir() / "level-drop";
    return p;
}

static std::string stamp() {
    return fmt::format("{}", std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

// move without ever overwriting: name collisions get a timestamp suffix
static void moveInto(fs::path const& file, fs::path const& dir) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    auto target = dir / file.filename();
    if (fs::exists(target, ec))
        target = dir / (file.stem().string() + "-" + stamp() + file.extension().string());
    fs::rename(file, target, ec);
    if (ec) log::warn("Level Drop: could not move {}: {}", file, ec.message());
}

// "Orbit Lullaby v3" -> "Orbit Lullaby" (only used when the name comes from the file name)
static std::string stripVersionSuffix(std::string n) {
    auto i = n.find_last_of(' ');
    if (i != std::string::npos && i + 2 < n.size() + 1 && i + 1 < n.size()
        && (n[i + 1] == 'v' || n[i + 1] == 'V') && i + 2 < n.size()) {
        bool digits = true;
        for (size_t k = i + 2; k < n.size(); k++) if (!std::isdigit((unsigned char)n[k])) digits = false;
        if (digits) {
            n.erase(i);
            while (!n.empty() && n.back() == ' ') n.pop_back();
        }
    }
    return n;
}

static std::string xmlEscape(std::string const& s) {
    std::string o;
    for (char c : s) {
        switch (c) {
            case '&': o += "&amp;"; break;
            case '<': o += "&lt;"; break;
            case '>': o += "&gt;"; break;
            default: o += c;
        }
    }
    return o;
}

// .txt = raw level string. Wrap it in a minimal GDShare-style .gmd plist and import that.
static Result<GJGameLevel*> importTxt(fs::path const& file) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return Err("cannot open file");
    std::stringstream ss; ss << in.rdbuf();
    std::string data = ss.str();
    while (!data.empty() && (data.back() == '\n' || data.back() == '\r' || data.back() == ' ')) data.pop_back();
    if (data.empty()) return Err("empty level string");
    if (!data.starts_with("H4sI")) {
        if (data.find(';') == std::string::npos) return Err("not a level string");
        data = ZipUtils::compressString(data, false, 0);   // GD stores k4 gzip+base64
    }
    auto tmp = file.parent_path() / (".leveldrop-" + stamp() + ".gmd");
    {
        std::ofstream out(tmp, std::ios::binary);
        out << "<?xml version=\"1.0\"?><plist version=\"1.0\" gjver=\"2.0\"><dict>"
            << "<k>kCEK</k><i>4</i><k>k2</k><s>" << xmlEscape(stripVersionSuffix(file.stem().string())) << "</s>"
            << "<k>k4</k><s>" << data << "</s><k>k13</k><t /><k>k21</k><i>2</i><k>k50</k><i>45</i>"
            << "</dict></plist>";
    }
    auto res = gmd::importGmdAsLevel(tmp);
    std::error_code ec; fs::remove(tmp, ec);
    return res;
}

// Force GD to write its save files now (CCLocalLevels.dat via LocalLevelManager,
// CCGameManager.dat via GameManager), the same path GD uses on exit.
static bool saveNow(bool notify) {
    bool ok = true;
    try {
        if (auto llm = LocalLevelManager::get()) llm->save(); else ok = false;
        if (auto gm = GameManager::get()) gm->save(); else ok = false;
    } catch (...) { ok = false; }
    if (notify || !ok)
        Notification::create(ok ? "Level Drop: game saved" : "Level Drop: save failed",
            ok ? NotificationIcon::Success : NotificationIcon::Error)->show();
    if (ok) log::info("Level Drop: saved"); else log::error("Level Drop: save failed");
    return ok;
}

// Delete every existing local level with exactly this name, via GD's own delete.
static int removeSameName(std::string const& name) {
    auto arr = LocalLevelManager::get()->m_localLevels;
    std::vector<Ref<GJGameLevel>> victims;
    for (auto lvl : CCArrayExt<GJGameLevel*>(arr))
        if (lvl && std::string(lvl->m_levelName) == name) victims.push_back(lvl);
    for (auto& lvl : victims) {
        GameLevelManager::get()->deleteLevel(lvl);
        if (arr->containsObject(lvl)) arr->removeObject(lvl);   // belt and braces
    }
    return (int)victims.size();
}

static bool g_scanning = false;

// returns number of levels imported
static int scanDropFolder() {
    if (g_scanning) return 0;
    if (LevelEditorLayer::get()) return 0;          // never import while the editor is open
    g_scanning = true;
    int count = 0;
    auto dir = dropDir();
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) { log::warn("Level Drop: cannot create {}: {}", dir, ec.message()); g_scanning = false; return 0; }

    std::vector<fs::path> files;
    for (auto const& e : fs::directory_iterator(dir, ec)) {
        if (!e.is_regular_file()) continue;
        auto ext = e.path().extension().string();
        for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
        if (ext == ".gmd" || ext == ".txt") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end());

    for (auto const& f : files) {
        auto ext = f.extension().string();
        for (auto& c : ext) c = (char)std::tolower((unsigned char)c);
        Result<GJGameLevel*> res = ext == ".txt" ? importTxt(f) : gmd::importGmdAsLevel(f);
        if (res && *res) {
            auto level = *res;
            // .gmd without a name: fall back to the file name (minus a trailing " vN")
            if (std::string(level->m_levelName).empty())
                level->m_levelName = stripVersionSuffix(f.stem().string());
            std::string name = level->m_levelName;
            int removed = 0;
            if (Mod::get()->getSettingValue<bool>("replace-same-name")) removed = removeSameName(name);
            LocalLevelManager::get()->m_localLevels->insertObject(level, 0);   // new level, top of list
            moveInto(f, dir / "imported");
            count++;
            auto msg = removed > 0
                ? fmt::format("Level Drop: replaced {} (removed {} old cop{})", name, removed, removed == 1 ? "y" : "ies")
                : fmt::format("Level Drop: imported {}", name);
            Notification::create(msg, NotificationIcon::Success)->show();
            log::info("Level Drop: imported '{}' from {} (replaced {})", name, f, removed);
        } else {
            std::string err = res ? std::string("null level") : res.unwrapErr();
            moveInto(f, dir / "failed");
            std::ofstream log(dir / "failed" / (f.filename().string() + ".log"));
            log << "Level Drop import failed for " << f.filename().string() << ": " << err << "\n";
            Notification::create(fmt::format("Level Drop: failed {}", f.filename().string()), NotificationIcon::Error)->show();
            log::warn("Level Drop: failed {}: {}", f, err);
        }
    }
    g_scanning = false;
    if (count > 0 && Mod::get()->getSettingValue<bool>("auto-save")) saveNow(false);
    return count;
}

class $modify(GDMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        static bool once = false;
        if (!once) { once = true; scanDropFolder(); }
        return true;
    }
};

class $modify(GDLevelBrowserLayer, LevelBrowserLayer) {
    bool init(GJSearchObject* search) {
        // scan BEFORE the list is built so new levels show up immediately
        if (search && search->m_searchType == SearchType::MyLevels) scanDropFolder();
        if (!LevelBrowserLayer::init(search)) return false;
        if (search && search->m_searchType == SearchType::MyLevels) addSaveButton();
        return true;
    }

    void addSaveButton() {
        auto searchBtn = this->getChildByIDRecursive("search-button");
        auto menu = searchBtn ? searchBtn->getParent() : nullptr;
        if (!menu) { log::warn("Level Drop: search-button not found, Save button not added"); return; }
        auto spr = ButtonSprite::create("Save", "bigFont.fnt", "GJ_button_01.png", .8f);
        spr->setScale(.5f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, menu_selector(GDLevelBrowserLayer::onLevelDropSave));
        btn->setID("save-button"_spr);
        menu->addChild(btn);
        if (menu->getLayout()) {
            // keep it directly after the search button in the menu's layout
            int z = searchBtn->getZOrder();
            btn->setZOrder(z);
            btn->setOrderOfArrival(searchBtn->getOrderOfArrival() + 1);
            menu->updateLayout();
        } else {
            btn->setPosition(searchBtn->getPosition() - CCPoint(0.f,
                searchBtn->getScaledContentSize().height / 2 + btn->getScaledContentSize().height / 2 + 4.f));
        }
    }

    void onLevelDropSave(CCObject*) { saveNow(true); }
};

class $modify(GDCreatorLayer, CreatorLayer) {
    bool init() {
        scanDropFolder();
        return CreatorLayer::init();
    }
};
