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
            << "<k>kCEK</k><i>4</i><k>k2</k><s>" << xmlEscape(file.stem().string()) << "</s>"
            << "<k>k4</k><s>" << data << "</s><k>k13</k><t /><k>k21</k><i>2</i><k>k50</k><i>45</i>"
            << "</dict></plist>";
    }
    auto res = gmd::importGmdAsLevel(tmp);
    std::error_code ec; fs::remove(tmp, ec);
    return res;
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
            LocalLevelManager::get()->m_localLevels->insertObject(level, 0);   // new level, top of list
            moveInto(f, dir / "imported");
            count++;
            std::string name = level->m_levelName;
            Notification::create(fmt::format("Level Drop: imported {}", name), NotificationIcon::Success)->show();
            log::info("Level Drop: imported '{}' from {}", name, f);
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
        return LevelBrowserLayer::init(search);
    }
};

class $modify(GDCreatorLayer, CreatorLayer) {
    bool init() {
        scanDropFolder();
        return CreatorLayer::init();
    }
};
