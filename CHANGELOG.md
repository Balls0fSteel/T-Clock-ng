# Changelog

All notable changes of the **T-Clock-ng** fork (based on [White-Tiger/T-Clock](https://github.com/White-Tiger/T-Clock) "T-Clock Redux" 2.4.4).
Timestamps: `YYYY-MM-DD_HHMMSS_epoch` (local time).

## 2026-10-12_170121_1791817281 — About page renamed to T-Clock-ng

### Changed
- About page: title "T-Clock-ng", history line credits White-Tiger's T-Clock Redux, startup checkbox
  "Start T-Clock-ng when Windows starts".
- About links show compact text (`github.com/Balls0fSteel/T-Clock-ng`, "report a bug / request a feature") —
  the full fork URLs were cut off by the dialog layout.
- The startup shortcut file name (`CONF_START`, "T-Clock Redux.lnk") and the file version resources are
  unchanged, so existing "start with Windows" shortcuts keep working.

## 2026-10-12_164546_1791816346 — Windows 11 support

### Added
- **Windows 11 XAML taskbar support ("overlay clock").** Windows 11's taskbar (22H2+/24H2) no longer has the classic
  `TrayClockWClass` window T-Clock used to hook, so T-Clock silently did nothing. When no usable classic clock is
  found on Windows 11, the injected DLL now creates its own layered clock window (`TClockOverlayWClass`) as a child
  of `Shell_TrayWnd`, drawn with `UpdateLayeredWindow` (per-pixel alpha), positioned over the original clock.
  - `Clock64.exe` runs a helper thread (`src/Clock/win11.c`) that locates the original clock via UI Automation
    (element whose name contains the current time; independent of AutomationIds that change between builds),
    samples the taskbar color above it to cover the original clock, and reports both to the overlay.
  - Clicking the overlay opens/closes the Windows 11 calendar (UIA Invoke on the original clock; fallback `Win+N`).
  - Default text color follows the light/dark system theme; hover highlight like Windows 11.
  - Settings (`[Taskbar]` in `T-Clock.ini`, or registry `HKCU\Software\Stoic Joker's\T-Clock 2010\Taskbar`):
    `Win11Overlay` (0 = auto, 1 = always, -1 = never), `Win11OverlayPopup` (1 = use an owned popup instead of a
    child window), `Win11OverlayOffsetX` / `Win11OverlayOffsetY` (pixel nudges).
- `TOS_WIN11` OS level (build >= 22000).
- GitHub Actions workflow (`.github/workflows/build.yml`): MSVC x86 + x64 builds, artifacts, releases on tags.

### Changed
- `src/compile.bat` finds MSBuild via `vswhere` (VS 2017+, tested with VS 2026 / v145) instead of the hard-coded
  Windows SDK 7.1 / VS 2013 paths; `RELEASE_TAG` env var marks release builds.
- Update checker and About page point to this fork (`Balls0fSteel/T-Clock-ng`); the old author's e-mail link was
  replaced by the fork's issue tracker.
- "Use old calendar" (`UseWin32TrayClockExperience`) is disabled on Windows 11, where it has no effect.
- `Clock_Inject()` retries instead of hooking nothing when neither a classic clock nor overlay mode is available.
- `Clock_GetCalendar()` recognizes the Windows 11 calendar/notification center flyout (for "click again to close").

### Fixed
- Build with C23 compilers (GCC 15+): `ReloadXPClockTheme()` and `OnDestroy()` were called with arguments while
  declared without parameters.

### Removed
- `.travis.yml` (Travis CI, Coverity and deploy secrets belonged to the upstream project).

### Notes / issues encountered
- Tested on Windows 11 IoT Enterprise LTSC 2024 (24H2, build 26100) with StartAllBack (classic taskbar):
  classic mode works unchanged; forced overlay mode (`Win11Overlay=1`) renders, hovers, shows tooltips, toggles the
  calendar and unloads cleanly from explorer.
- 2026-10-12_165800_1791817080 — tested on the **stock Windows 11 24H2 taskbar** (StartAllBack's classic taskbar
  temporarily disabled): overlay mode is selected automatically; UIA finds the clock (`SystemTray.OmniButton`,
  name "Clock 4:56 PM 10/12/2026", full taskbar height); the layered child window draws above the XAML island;
  the sampled background matches the accent-colored taskbar so the original clock is hidden; hover, tooltip,
  calendar open/close and clean unload all work.
- A tray icon with sensor readings (numbers in its tooltip) also matched the time digits; the year-match scoring
  correctly preferred the real clock.
- On taskbars with a vertical gradient (some StartAllBack themes) a forced overlay's single sampled background color
  can't match exactly; set `BackColor` manually if needed. The stock Windows 11 taskbar is flat.
- MinGW: sources compile with GCC 16 (MSYS2), but a full `make` build wasn't run as `mingw32-make` isn't installed.
