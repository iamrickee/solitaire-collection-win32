#pragma once
#include <windows.h>
#include <string>
#include <utility>
#include <vector>

// Persistent key/value store backed by %APPDATA%\Solitaire\solitaire.ini.
// Also holds the high-score table (one best score per game/variant).
class Settings {
public:
    static constexpr COLORREF kDefaultBackground = RGB(0x2E, 0x7D, 0x4F);   // felt green
    static constexpr COLORREF kDefaultCardBack = RGB(0xFF, 0x00, 0x00);     // the deck's own red plaid

    Settings();

    int GetInt(const wchar_t* section, const wchar_t* key, int def) const;
    void SetInt(const wchar_t* section, const wchar_t* key, int value);
    std::wstring GetString(const wchar_t* section, const wchar_t* key, const wchar_t* def) const;
    void SetString(const wchar_t* section, const wchar_t* key, const std::wstring& value);

    // App-wide appearance.
    COLORREF Background() const;
    void SetBackground(COLORREF c);
    COLORREF CardBack() const;
    void SetCardBack(COLORREF c);
    bool FourColor() const;
    void SetFourColor(bool on);

    // High scores. `label` is a human-readable game/variant name.
    // Returns true if `score` is a new best for that label.
    bool SubmitScore(const std::wstring& label, int score);
    std::vector<std::pair<std::wstring, int>> HighScores() const;
    void ClearHighScores();

private:
    std::wstring path_;
};
