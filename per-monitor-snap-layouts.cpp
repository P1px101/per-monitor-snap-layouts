// ==WindhawkMod==
// @id              per-monitor-snap-layouts
// @name            Per Monitor Snap Layouts
// @description     Makes Windows 11 Snap Layouts only suggest windows that are on the same monitor, preventing it from pulling windows across monitors.
// @version         1.0
// @author          P1px
// @github          https://github.com/P1px101
// @include         explorer.exe
// @include         ShellExperienceHost.exe
// @architecture    x86-64
// @compilerOptions -luser32 -ldwmapi
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Per Monitor Snap Layouts

When you snap a window using Windows 11's Snap Layouts ( dragging to the top of
the screen or using Win+Z ), Windows suggests other open windows to fill the
remaining spaces. By default, it shows windows from **all** monitors, which is
annoying when you have a multi-monitor setup which it pulls windows from your main
monitor to fill a layout on your secondary monitor ( or vice versa ).

This mod fixes that by filtering the Snap Assist suggestions so that only
windows currently on the **same monitor** as where your cursor is are offered.

![Per Monitor Snap Layouts Demo](https://raw.githubusercontent.com/P1px101/per-monitor-snap-layouts-gif/main/per-monitor-snap-layouts-gif.gif)
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- enabled: true
  $name: Enable per monitor snap layouts
  $description: Toggle the per-monitor filtering on or off without uninstalling.
*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <dwmapi.h>

struct {
    bool enabled;
} settings;

using EnumWindows_t = decltype(&EnumWindows);
EnumWindows_t EnumWindows_Original;

struct EnumWindowsFilterContext {
    WNDENUMPROC originalCallback;
    LPARAM originalLParam;
    HMONITOR targetMonitor;
};

static BOOL CALLBACK FilteredEnumProc(HWND hwnd, LPARAM lParam) {
    auto* ctx = reinterpret_cast<EnumWindowsFilterContext*>(lParam);

    HMONITOR windowMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

    if (windowMonitor != ctx->targetMonitor) {
        return TRUE;
    }

    return ctx->originalCallback(hwnd, ctx->originalLParam);
}

BOOL WINAPI EnumWindows_Hook(WNDENUMPROC lpEnumFunc, LPARAM lParam) {
    if (!settings.enabled) {
        return EnumWindows_Original(lpEnumFunc, lParam);
    }

    POINT pt;
    GetCursorPos(&pt);
    HMONITOR targetMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

    if (!targetMonitor) {
        return EnumWindows_Original(lpEnumFunc, lParam);
    }

    EnumWindowsFilterContext ctx;
    ctx.originalCallback = lpEnumFunc;
    ctx.originalLParam = lParam;
    ctx.targetMonitor = targetMonitor;

    return EnumWindows_Original(FilteredEnumProc,
                                reinterpret_cast<LPARAM>(&ctx));
}

using EnumDesktopWindowsW_t = decltype(&EnumDesktopWindows);
EnumDesktopWindowsW_t EnumDesktopWindows_Original;

BOOL WINAPI EnumDesktopWindows_Hook(HDESK hDesktop, WNDENUMPROC lpfn,
                                     LPARAM lParam) {
    if (!settings.enabled) {
        return EnumDesktopWindows_Original(hDesktop, lpfn, lParam);
    }

    POINT pt;
    GetCursorPos(&pt);
    HMONITOR targetMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

    if (!targetMonitor) {
        return EnumDesktopWindows_Original(hDesktop, lpfn, lParam);
    }

    EnumWindowsFilterContext ctx;
    ctx.originalCallback = lpfn;
    ctx.originalLParam = lParam;
    ctx.targetMonitor = targetMonitor;

    return EnumDesktopWindows_Original(hDesktop, FilteredEnumProc,
                                       reinterpret_cast<LPARAM>(&ctx));
}

void LoadSettings() {
    settings.enabled = Wh_GetIntSetting(L"enabled");
}

BOOL Wh_ModInit() {
    LoadSettings();

    HMODULE user32 = GetModuleHandle(L"user32.dll");
    if (!user32) {
        user32 = LoadLibrary(L"user32.dll");
    }

    if (user32) {
        void* pEnumWindows =
            (void*)GetProcAddress(user32, "EnumWindows");
        if (pEnumWindows) {
            Wh_SetFunctionHook(pEnumWindows, (void*)EnumWindows_Hook,
                               (void**)&EnumWindows_Original);
        }

        void* pEnumDesktopWindows =
            (void*)GetProcAddress(user32, "EnumDesktopWindowsW");
        if (pEnumDesktopWindows) {
            Wh_SetFunctionHook(pEnumDesktopWindows,
                               (void*)EnumDesktopWindows_Hook,
                               (void**)&EnumDesktopWindows_Original);
        }
    }

    return TRUE;
}

void Wh_ModUninit() {}

void Wh_ModSettingsChanged() {
    LoadSettings();
}