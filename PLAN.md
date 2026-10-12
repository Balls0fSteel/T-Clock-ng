# PLAN — T-Clock-ng

Last updated: 2026-10-12_164546_1791816346

## Current work
- Windows 11 support (branch `win11`): overlay clock for the XAML taskbar, build modernization, fork URLs.

## Next steps
- [ ] Test overlay mode on the **stock Windows 11 taskbar** (StartAllBack's classic taskbar disabled):
      clock found via UI Automation? background matches (light/dark, accent color, transparency)? calendar toggles?
      autohide, DPI scaling (125/150 %), taskbar alignment left/center, explorer restart.
- [ ] Verify the layered *child* window draws above the XAML island; if not, default `Win11OverlayPopup` to 1.
- [ ] Test virtual desktop switching and "show desktop" with the overlay.
- [ ] Full MinGW build (`mingw32-make`, needs `pacman -S mingw-w64-x86_64-make` + i686 toolchain for 32-bit).
- [ ] First tagged release from GitHub Actions; bump `src/version` (update checker) when releasing.

## Open questions
- Rename the product ("T-Clock Redux" in titles/About) to "T-Clock-ng"? Keep settings key `Stoic Joker's\T-Clock 2010`
  for compatibility either way.
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
