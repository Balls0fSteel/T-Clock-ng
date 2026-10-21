# PLAN — T-Clock-ng

Last updated: 2026-10-21_223300_1792614780

## Current work
- v2.5.0 released (first T-Clock-ng release: Windows 11 overlay clock, modern MSVC builds, fork URLs).

## Next steps
- [x] Overlay mode on the stock Windows 11 24H2 taskbar: UIA lookup, child window above XAML, accent-color
      background, hover/tooltip, calendar toggle, clean unload (2026-10-12).
- [ ] Still to test on the stock taskbar: light theme, transparency effects on, autohide, DPI scaling
      (125/150 %), left-aligned taskbar, explorer restart while running, virtual desktops, "show desktop".
- [ ] Overlay is right-aligned and can be wider than the original clock (covers part of the next tray icon with
      long formats); consider shrinking the default format/padding on Windows 11.
- [ ] Full MinGW build (`mingw32-make`, needs `pacman -S mingw-w64-x86_64-make` + i686 toolchain for 32-bit).
- [x] First tagged release from GitHub Actions: v2.5.0 (2026-10-21).
- Release checklist: regenerate `version.h` with AutoVersion (`-D "MAJOR=…;MINOR=…;BUILD=…;STATUS=3;REVISION=<commit
  count of the release commit>"`), update `src/version`, commit, tag `vX.Y.Z`, push the tag.

## Open questions
- (resolved 2026-10-12: product renamed to T-Clock-ng; settings key `Stoic Joker's\T-Clock 2010` kept)
- Move settings to a `T-Clock-ng` key with a one-time migration? Only worth it if the old key causes confusion.
- Should overlay mode also hide the original clock via Windows' own setting instead of covering it? (Hiding it makes
  the tray reflow, so there'd be no free space at the clock position — covering seemed the better default.)

## Future improvements
- Options UI for the `Win11Overlay*` settings (currently ini/registry only).
- Overlay clocks on secondary taskbars (`Shell_SecondaryTrayWnd`) of the Windows 11 XAML taskbar.
- ARM64 build (ARM64 Windows 11 explorer can't load the x64 DLL).
- Dark mode for the options dialogs.
- Per-monitor DPI awareness for Clock.exe UI.
- Background as a sampled strip instead of a single color (gradient taskbars).

## Known TODOs
- `src/common/version.h` and `src/common/AutoVersion.exe` are regenerated/rebuilt by every MSVC Win32 build;
  don't commit them accidentally.
- `src/sign-n-zip.bat` still references Travis/AppVeyor variables and an external signing setup (optional, unused by CI).
