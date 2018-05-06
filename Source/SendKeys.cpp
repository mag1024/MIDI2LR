// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
==============================================================================

SendKeys.cpp

This file is part of MIDI2LR. Copyright 2015 by Rory Jaffe.

MIDI2LR is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

MIDI2LR is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
MIDI2LR.  If not, see <http://www.gnu.org/licenses/>.
==============================================================================
*/
#include "SendKeys.h"
#include <algorithm>
#include <cctype>
#include <exception>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <unicode/unistr.h>
#include <gsl/gsl>
#ifdef _WIN32
#include "Windows.h"
#else
#import <CoreFoundation/CoreFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <Carbon/Carbon.h>
#include <libproc.h> //proc_ functions in GetPid
#endif
#include "../JuceLibraryCode/JuceHeader.h" //creates ambiguous reference to Point if included before Mac headers

namespace {
    //using transform as specified in http://en.cppreference.com/w/cpp/string/byte/tolower
    std::string ToLower(const std::string& in) noexcept
    {
        auto s = in;
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) noexcept { return std::tolower(c); });
        return s;
    }
}

#ifdef _WIN32

class WindowsFunctionError: public std::exception {
    static_assert(std::is_same<std::remove_pointer<LPSTR>::type, char>(),
        "LPSTR doesn't point to 8-bit char. Problem for windows_function_error.");
public:
    WindowsFunctionError(DWORD n = GetLastError()) noexcept : number_(n)
    {
        LPSTR new_what;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, number_, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            LPSTR(&new_what), 0, nullptr); //-V206
        what_ = {new_what, [](LPSTR w) noexcept {HeapFree(GetProcessHeap(), 0, w); }};
    }
    ~WindowsFunctionError() noexcept = default;
    WindowsFunctionError(const WindowsFunctionError& other) noexcept = default;
    WindowsFunctionError(WindowsFunctionError&& other) noexcept = default;
    WindowsFunctionError& operator=(const WindowsFunctionError& other) noexcept = default;
    WindowsFunctionError& operator=(WindowsFunctionError&& other) noexcept = default;

    const char* what() const noexcept override
    {
        return what_.get();
    }
    DWORD number() const noexcept
    {
        return number_;
    }
private:
    std::shared_ptr<std::remove_pointer<LPSTR>::type> what_;
    DWORD number_;
};

wchar_t MBtoWChar(const std::string& key)
{
    wchar_t full_character;
    const auto return_value = MultiByteToWideChar(CP_UTF8, 0, key.data(),
        gsl::narrow_cast<int>(key.size()), &full_character, 1);
    if (return_value == 0) {
        throw WindowsFunctionError();
    }
    return full_character;
}

HKL GetLanguage(const std::string& program_name) noexcept
{
    const auto h_lr_wnd = FindWindow(nullptr, program_name.c_str());
    if (h_lr_wnd) {
        // get language that LR is using (if hLrWnd is found)
        const auto thread_id = GetWindowThreadProcessId(h_lr_wnd, nullptr);
        return GetKeyboardLayout(thread_id);
    }
    // use keyboard of MIDI2LR application
    return GetKeyboardLayout(0);
}

#else

bool EndsWith(const std::string &main_str, const std::string &to_match)//note: C++20 will have ends_with
{
    return main_str.size() >= to_match.size() &&
        main_str.compare(main_str.size() - to_match.size(), to_match.size(), to_match) == 0;
}

//from https://stackoverflow.com/questions/44296468/using-stdunique-ptr-to-manage-corefoundation-cftype-resources
template <typename CoreFoundationType>
struct cfreleaser {
    constexpr cfreleaser() noexcept = default;
    template <typename U> cfreleaser(cfreleaser<U> const&) noexcept
    {};
    void operator()(CoreFoundationType __attribute__((cf_consumed)) cfp)
    {
        if (cfp) {
            CFRelease(cfp); cfp = nullptr;
        }
    }
};

template <typename CoreFoundationType>
using cf_unique_ptr = std::unique_ptr<
    typename std::decay_t<std::remove_pointer_t<CoreFoundationType>>,
    cfreleaser<CoreFoundationType>>;

pid_t GetPid()
{
    static const std::string kLr{"Adobe Lightroom.app/Contents/MacOS/Adobe Lightroom"};
    static const std::string kLrc{"Adobe Lightroom Classic CC.app/Contents/MacOS/Adobe Lightroom Classic"};
    const int number_processes{proc_listpids(PROC_ALL_PIDS, 0, NULL, 0) + 20};
    std::vector<pid_t> pids(number_processes);//add a few in case more processes show up
    proc_listpids(PROC_ALL_PIDS, 0, pids.data(), sizeof(pid_t)*(number_processes));
    char path_buffer[PROC_PIDPATHINFO_MAXSIZE];
    for (const auto pid : pids) {
        if (pid == 0)
            continue;
        memset(path_buffer, 0, sizeof(path_buffer));
        proc_pidpath(pid, path_buffer, sizeof(path_buffer));
        if (strlen(path_buffer) > 0 && (EndsWith(path_buffer, kLr) || EndsWith(path_buffer, kLrc)))
            return pid;
    }
    juce::NativeMessageBox::showMessageBox(juce::AlertWindow::WarningIcon, "Error",
        "Lightroom PID not found.");
    return 0;
}

/* From: https://stackoverflow.com/questions/1918841/how-to-convert-ascii-character-to-cgkeycode/1971027#1971027
*
* Returns string representation of key, if it is printable.
* Ownership follows the Create Rule; that is, it is the caller's
* responsibility to release the returned object. */
UChar CreateStringForKey(CGKeyCode key_code)
{
    cf_unique_ptr<TISInputSourceRef> current_keyboard{TISCopyCurrentKeyboardInputSource()};
    CFDataRef layout_data = (CFDataRef)TISGetInputSourceProperty(current_keyboard.get(), kTISPropertyUnicodeKeyLayoutData);
    const UCKeyboardLayout *keyboard_layout =
        (const UCKeyboardLayout *)CFDataGetBytePtr(layout_data);
    UInt32 keys_down = 0;
    UniChar chars[4];
    UniCharCount real_length;
    UCKeyTranslate(keyboard_layout, key_code, kUCKeyActionDown, 0,
        LMGetKbdType(), kUCKeyTranslateNoDeadKeysBit, &keys_down,
        sizeof(chars) / sizeof(chars[0]), &real_length, chars);
    if (real_length > 1)
        juce::NativeMessageBox::showMessageBox(juce::AlertWindow::WarningIcon, "Error",
            juce::String("For key code ") + juce::String(key_code)
            + juce::String(", Unicode character is ") + juce::String(real_length)
            + juce::String(" long. It is ") + juce::String((wchar_t*)chars, real_length)
            + juce::String("."));
    return chars[0];
}

/* From: https://stackoverflow.com/questions/1918841/how-to-convert-ascii-character-to-cgkeycode/1971027#1971027
*
* Returns key code for given character via the above function. Throws std::out_of_range on error. */
CGKeyCode KeyCodeForChar(UChar c)
{
    static std::once_flag flag;
    static std::unordered_map<UChar, size_t> char_code_map;
    std::call_once(flag, []() {/* Generate table of keycodes and characters. */
        /* Loop through every key-code (0 - 127) to find its current mapping. */
        for (size_t i = 0; i < 128; ++i) {
            auto uc = CreateStringForKey((CGKeyCode)i);
            if (uc) {
                char_code_map[uc] = i;
            }
        }
    });
    return char_code_map.at(c);
}

#endif

const std::unordered_map<std::string, unsigned char> kKeyMap = {
#ifdef _WIN32
    {"backspace", VK_BACK},
    {"cursor down", VK_DOWN},
    {"cursor left", VK_LEFT},
    {"cursor right", VK_RIGHT},
    {"cursor up", VK_UP},
    {"delete", VK_DELETE},
    {"end", VK_END},
    {"escape", VK_ESCAPE},
    {"home", VK_HOME},
    {"page down", VK_NEXT},
    {"page up", VK_PRIOR},
    {"return", VK_RETURN},
    {"space", VK_SPACE},
    {"tab", VK_TAB},
    {"f1", VK_F1},
    {"f2", VK_F2},
    {"f3", VK_F3},
    {"f4", VK_F4},
    {"f5", VK_F5},
    {"f6", VK_F6},
    {"f7", VK_F7},
    {"f8", VK_F8},
    {"f9", VK_F9},
    {"f10", VK_F10},
    {"f11", VK_F11},
    {"f12", VK_F12},
    {"f13", VK_F13},
    {"f14", VK_F14},
    {"f15", VK_F15},
    {"f16", VK_F16},
    {"f17", VK_F17},
    {"f18", VK_F18},
    {"f19", VK_F19},
    {"f20", VK_F20},
    {"numpad 0", VK_NUMPAD0},
    {"numpad 1", VK_NUMPAD1},
    {"numpad 2", VK_NUMPAD2},
    {"numpad 3", VK_NUMPAD3},
    {"numpad 4", VK_NUMPAD4},
    {"numpad 5", VK_NUMPAD5},
    {"numpad 6", VK_NUMPAD6},
    {"numpad 7", VK_NUMPAD7},
    {"numpad 8", VK_NUMPAD8},
    {"numpad 9", VK_NUMPAD9},
    {"numpad add", VK_ADD},
    {"numpad subtract", VK_SUBTRACT},
    {"numpad multiply", VK_MULTIPLY},
    {"numpad divide", VK_DIVIDE},
    {"numpad decimal", VK_DECIMAL}
#else
{
    "backspace", kVK_Delete
},
    {"cursor down", kVK_DownArrow},
    {"cursor left", kVK_LeftArrow},
    {"cursor right", kVK_RightArrow},
    {"cursor up", kVK_UpArrow},
    {"delete", kVK_ForwardDelete},
    {"end", kVK_End},
    {"escape", kVK_Escape},
    {"home", kVK_Home},
    {"page down", kVK_PageDown},
    {"page up", kVK_PageUp},
    {"return", kVK_Return},
    {"space", kVK_Space},
    {"tab", kVK_Tab},
    {"f1", kVK_F1},
    {"f2", kVK_F2},
    {"f3", kVK_F3},
    {"f4", kVK_F4},
    {"f5", kVK_F5},
    {"f6", kVK_F6},
    {"f7", kVK_F7},
    {"f8", kVK_F8},
    {"f9", kVK_F9},
    {"f10", kVK_F10},
    {"f11", kVK_F11},
    {"f12", kVK_F12},
    {"f13", kVK_F13},
    {"f14", kVK_F14},
    {"f15", kVK_F15},
    {"f16", kVK_F16},
    {"f17", kVK_F17},
    {"f18", kVK_F18},
    {"f19", kVK_F19},
    {"f20", kVK_F20},
        //using ANSI layout codes for keypad, may cause problems in some languages
    {"numpad 0", kVK_ANSI_Keypad0},
    {"numpad 1", kVK_ANSI_Keypad1},
    {"numpad 2", kVK_ANSI_Keypad2},
    {"numpad 3", kVK_ANSI_Keypad3},
    {"numpad 4", kVK_ANSI_Keypad4},
    {"numpad 5", kVK_ANSI_Keypad5},
    {"numpad 6", kVK_ANSI_Keypad6},
    {"numpad 7", kVK_ANSI_Keypad7},
    {"numpad 8", kVK_ANSI_Keypad8},
    {"numpad 9", kVK_ANSI_Keypad9},
    {"numpad add", kVK_ANSI_KeypadPlus},
    {"numpad subtract", kVK_ANSI_KeypadMinus},
    {"numpad multiply", kVK_ANSI_KeypadMultiply},
    {"numpad divide", kVK_ANSI_KeypadDivide},
    {"numpad decimal", kVK_ANSI_KeypadDecimal}
#endif
};

void rsj::SendKeyDownUp(const std::string& key, int modifiers) noexcept
{
    const bool alt_opt{gsl::narrow_cast<bool>(modifiers & 0x1)};
    const bool command{gsl::narrow_cast<bool>(modifiers & 0x8)};
    const bool control{gsl::narrow_cast<bool>(modifiers & 0x2)};
    const bool shift{gsl::narrow_cast<bool>(modifiers & 0x4)};

    try {
        static std::mutex mutex_sending{};
        const auto mapped_key = kKeyMap.find(ToLower(key));
        const auto in_keymap = mapped_key != kKeyMap.end();

#ifdef _WIN32
        static_assert(sizeof(WCHAR) == sizeof(UChar),
            "For Unicode handling, assuming windows wide char is same as ICU 16-bit Unicode char.");
        BYTE vk = 0;
        BYTE vk_modifiers = 0;
        if (in_keymap)
            vk = mapped_key->second;
        else {// Translate key code to keyboard-dependent scan code, may be UTF-8
            const auto uc{icu::UnicodeString::fromUTF8(key)[0]};
            static const auto language_id = GetLanguage("Lightroom");
            static_assert(LOBYTE(0xffff) == 0xff && HIBYTE(0xffff) == 0xff, "Assuming VkKeyScanEx returns 0xffff on error");
            const auto vk_code_and_shift = VkKeyScanExW(uc, language_id);
            if (vk_code_and_shift == 0xffff) //-V547
                throw WindowsFunctionError();
            vk = LOBYTE(vk_code_and_shift);
            vk_modifiers = HIBYTE(vk_code_and_shift);
        }

        //construct virtual keystroke sequence
        std::vector<WORD> strokes{vk}; // start with actual key, then mods
        if (shift || vk_modifiers & 0x1) {
            strokes.push_back(VK_SHIFT);
        }
        if ((vk_modifiers & 0x06) == 0x06) {
            strokes.push_back(VK_RMENU); //AltGr
            if (alt_opt) strokes.push_back(VK_MENU);
            if (control) strokes.push_back(VK_CONTROL);
        }
        else {
            if (alt_opt || vk_modifiers & 0x4) strokes.push_back(VK_MENU);
            if (control || vk_modifiers & 0x2) strokes.push_back(VK_CONTROL);
        }

        // construct input event.
        INPUT ip{};
        constexpr auto kSizeIp = sizeof(ip);
        ip.type = INPUT_KEYBOARD;
        //ki: wVk, wScan, dwFlags, time, dwExtraInfo
        ip.ki = {0, 0, 0, 0, 0};

        //send key down strokes
        std::lock_guard<decltype(mutex_sending)> lock(mutex_sending);
        for (auto it = strokes.crbegin(); it != strokes.crend(); ++it) {
            ip.ki.wVk = *it;
            const auto result = SendInput(1, &ip, kSizeIp);
            if (result == 0)
                throw WindowsFunctionError();
        }
        //send key up strokes
        ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
        for (const auto it : strokes) {
            ip.ki.wVk = it;
            const auto result = SendInput(1, &ip, kSizeIp);
            if (result == 0)
                throw WindowsFunctionError();
        }
#else
        static_assert(sizeof(UniChar) == sizeof(UChar),
            "For Unicode handling, assuming Mac wide char is same as ICU 16-bit Unicode char.");
        try { //In MacOS, KeyCodeForChar will throw if key not in map
            CGEventRef d;
            CGEventRef u;
            uint64_t flags = 0;
            auto dr = gsl::finally([&d] {if (d) CFRelease(d); });//release at end of scope
            auto ur = gsl::finally([&u] {if (u) CFRelease(u); });

            if (in_keymap) {
                const auto vk = mapped_key->second;
                d = CGEventCreateKeyboardEvent(NULL, vk, true);
                u = CGEventCreateKeyboardEvent(NULL, vk, false);
            }
            else {
                const UChar uc{icu::UnicodeString::fromUTF8(key)[0]};
                const auto key_code = KeyCodeForChar(uc);
                d = CGEventCreateKeyboardEvent(NULL, key_code, true);
                u = CGEventCreateKeyboardEvent(NULL, key_code, false);
                flags = CGEventGetFlags(d); //in case KeyCode has associated flag
            }

            if (alt_opt) flags |= kCGEventFlagMaskAlternate;
            if (command) flags |= kCGEventFlagMaskCommand;
            if (control) flags |= kCGEventFlagMaskControl;
            if (shift) flags |= kCGEventFlagMaskShift;
            if (flags) {
                CGEventSetFlags(d, static_cast<CGEventFlags>(flags));
                CGEventSetFlags(u, static_cast<CGEventFlags>(flags));
            }

            if (kCFCoreFoundationVersionNumber >= kCFCoreFoundationVersionNumber10_11) {
                static const pid_t lr_pid{GetPid()};
                if (lr_pid) {
                    std::lock_guard<decltype(mutex_sending)> lock(mutex_sending);
                    CGEventPostToPid(lr_pid, d);
                    CGEventPostToPid(lr_pid, u);
                }
            }
            else { //use if OS version < 10.11 as CGEventPostToPid first supported in 10.11
                static ProcessSerialNumber psn{[]() {
                    ProcessSerialNumber temp_psn{0};
                    pid_t lr_pid{GetPid()};
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                    if (lr_pid)
                        GetProcessForPID(lr_pid, &temp_psn); //first deprecated in macOS 10.9
#pragma GCC diagnostic pop
                    return temp_psn;
                }()};
                if (psn.highLongOfPSN || psn.lowLongOfPSN) {
                    std::lock_guard<decltype(mutex_sending)> lock(mutex_sending);
                    CGEventPostToPSN(&psn, d);
                    CGEventPostToPSN(&psn, u);
                }
            }
        }
        catch (const std::out_of_range& e) {
            juce::NativeMessageBox::showMessageBox(juce::AlertWindow::WarningIcon, "Error",
                "Unsupported character was used: " + key + ". " + e.what());
        }
#endif
    }
    catch (const std::exception& e) {
        juce::NativeMessageBox::showMessageBox(juce::AlertWindow::WarningIcon, "Error",
            "Exception in key sending function for key: " + key + ". " + e.what());
    }
    catch (...) {
        juce::NativeMessageBox::showMessageBox(juce::AlertWindow::WarningIcon, "Error",
            "Exception in key sending function for key: " + key + ". Non-standard exception.");
    }
}