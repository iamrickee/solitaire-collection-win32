#include "Settings.h"
#include <shlobj.h>
#include <climits>

namespace {
constexpr wchar_t kAppSection[] = L"App";
constexpr wchar_t kScoreSection[] = L"HighScores";
}

Settings::Settings() {
    PWSTR appData = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &appData))) {
        std::wstring dir = std::wstring(appData) + L"\\Solitaire";
        CoTaskMemFree(appData);
        CreateDirectoryW(dir.c_str(), nullptr);
        path_ = dir + L"\\solitaire.ini";
    } else {
        path_ = L".\\solitaire.ini";
    }
}

int Settings::GetInt(const wchar_t* section, const wchar_t* key, int def) const {
    // GetPrivateProfileInt can't represent negatives reliably; go through the string.
    std::wstring s = GetString(section, key, L"");
    if (s.empty()) return def;
    try { return std::stoi(s); } catch (...) { return def; }
}

void Settings::SetInt(const wchar_t* section, const wchar_t* key, int value) {
    SetString(section, key, std::to_wstring(value));
}

std::wstring Settings::GetString(const wchar_t* section, const wchar_t* key, const wchar_t* def) const {
    wchar_t buf[512];
    GetPrivateProfileStringW(section, key, def, buf, ARRAYSIZE(buf), path_.c_str());
    return buf;
}

void Settings::SetString(const wchar_t* section, const wchar_t* key, const std::wstring& value) {
    WritePrivateProfileStringW(section, key, value.c_str(), path_.c_str());
}

COLORREF Settings::Background() const { return static_cast<COLORREF>(GetInt(kAppSection, L"Background", kDefaultBackground)); }
void Settings::SetBackground(COLORREF c) { SetInt(kAppSection, L"Background", static_cast<int>(c)); }
COLORREF Settings::CardBack() const { return static_cast<COLORREF>(GetInt(kAppSection, L"CardBack", kDefaultCardBack)); }
void Settings::SetCardBack(COLORREF c) { SetInt(kAppSection, L"CardBack", static_cast<int>(c)); }
bool Settings::FourColor() const { return GetInt(kAppSection, L"FourColor", 0) != 0; }
void Settings::SetFourColor(bool on) { SetInt(kAppSection, L"FourColor", on ? 1 : 0); }

bool Settings::SubmitScore(const std::wstring& label, int score) {
    const int kNone = INT_MIN;
    int best = GetInt(kScoreSection, label.c_str(), kNone);
    if (best != kNone && score <= best) return false;
    SetInt(kScoreSection, label.c_str(), score);
    return true;
}

std::vector<std::pair<std::wstring, int>> Settings::HighScores() const {
    std::vector<std::pair<std::wstring, int>> out;
    std::vector<wchar_t> buf(32768);
    DWORD n = GetPrivateProfileSectionW(kScoreSection, buf.data(), static_cast<DWORD>(buf.size()), path_.c_str());
    // Format: "key=value\0key=value\0\0"
    for (const wchar_t* p = buf.data(); p < buf.data() + n && *p; p += wcslen(p) + 1) {
        std::wstring entry(p);
        size_t eq = entry.find(L'=');
        if (eq == std::wstring::npos) continue;
        try { out.emplace_back(entry.substr(0, eq), std::stoi(entry.substr(eq + 1))); } catch (...) {}
    }
    return out;
}

void Settings::ClearHighScores() {
    WritePrivateProfileStringW(kScoreSection, nullptr, nullptr, path_.c_str());
}
