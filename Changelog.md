# Command & Conquer Generals: Zero Hour — Changelog

> This changelog covers the complete release history of *Command & Conquer Generals*, its
> expansion *Zero Hour*, all official Electronic Arts patches, the 2025 EA open-source
> release, and the continuing community development effort under version 1.05.
>
> All entries are given in reverse-chronological order within each product generation, with
> the most recent work at the top.

---

## Version 1.05 — Community Build (x64 Port & Modernisation)
**Author:** SparksSkywere  
**Repository:** [electronicarts/CnC_Generals_Zero_Hour](https://github.com/electronicarts/CnC_Generals_Zero_Hour)  
**Branch:** `main`  
**Status:** Active development — *build in progress as of 2th Feb 2025*

This release marks the first community-driven port of the Zero Hour engine to a native
64-bit (x86-64) target platform under Microsoft Visual Studio 2026 Community. It builds on
the EA open-source release (see below) and aims to produce a binary that runs correctly
on modern 64-bit Windows without any compatibility shims, 32-bit runtime bridges, or
legacy operating-system stubs.

### Movie Playback Replacement
A full replacement pass removed the dead proprietary Bink playback stub and replaced it with a Media Foundation-backed decoder that keeps the original engine-side `VideoPlayer` API intact.

- Replaced the old `BinkVideoPlayer` implementation with a custom loose-file movie backend that decodes modern video containers through Windows Media Foundation and uploads frames into the existing `VideoBuffer` path used by shell movies, load screens, and UI videos.
- Removed the obsolete `bink.h` stub and the last project-level Bink include dependency from `GameEngineDevice.vcxproj`, so the runtime no longer depends on unavailable RAD Bink SDK headers.
- Fixed movie-path resolution so legacy explicit `.bik` references now correctly fall through to converted files with the same base name and supported extensions such as `.wmv`, `.mp4`, `.avi`, `.mov`, and `.m4v`, with the runtime now preferring `.wmv` before `.mp4` when both exist.
- Added the helper script `Tools\convert-bik-movies.ps1` to recursively convert legacy `.bik` assets under any chosen root into loose movie replacements using FFmpeg.
- Updated `Tools\convert-bik-movies.ps1` to support recursive root scanning, explicit `ffmpeg.exe` discovery, `-Overwrite`, `-IncludeAudio`, and `-OutputFormat wmv|mp4`, while defaulting to constant-frame-rate WMV output for the lighter Media Foundation playback path.
- Added inline PowerShell `#` comments to `Tools\convert-bik-movies.ps1` so the conversion workflow and encoder choices are maintainable in-repo.
- Added `Tools\cleanup.ps1` and `Tools\reset.ps1` to stop stale build/runtime processes and clear locked/intermediate build state after interrupted local builds.
- Hardened the new recovery scripts so they now enumerate and remove individual stale `obj`, `ipch`, `.pdb`, `.idb`, `.ilk`, `.ipdb`, `.iobj`, and root build-log paths correctly in both dry-run and real execution.
- Verified the local WorldBuilder x64 path end to end: the project still requires ATL/MFC for the active `v145` x64 toolset, and after installing that prerequisite the remaining Release blocker was a `GameEngineDevice` warning in `BinkVideoPlayer.cpp` (`hr` initialized but not referenced). That warning is now fixed, `GameEngineDevice.lib` is produced for Release, and `WorldBuilder.vcxproj` Release builds cleanly.

### Options Menu Restoration, UI Modernisation & Startup Polish
A substantial follow-up pass focused on restoring the full Zero Hour options experience,
ensuring the menu behaves correctly on modern displays, and cleaning up several x64-era
regressions that only surfaced once the menu override files were expanded.

- Re-enabled the complete `OptionsMenu` flow from the shell and in-game UI, restoring
  reliable `Save`, `Cancel`, and `Defaults` behaviour through `OptionPreferences` and the
  existing shell callbacks.
- Reworked the `Audio`, `Video`, `Display`, `Accessibilities`, and `Network` tabs in the
  override WND files with corrected spacing, consistent tab sizing, and normalized H2/H3
  typography for labels versus interactive controls.
- Improved the menu's practical usability by fixing combobox z-ordering, resolution-aware
  font scaling, fullscreen hover alignment, and the transition from the initial splash-sized
  startup window to the real render window at the configured game resolution.
- Simplified replay handling to a single `Replay` checkbox while still synchronising the
  legacy `SaveCameraInReplays` and `UseCameraInReplays` preference keys for compatibility.
- Removed the problematic `Controls` / keyboard-mapping tab from the visible tab strip,
  retaining the rest of the menu layout and rebalancing the remaining tabs to fill the bar
  evenly.
- Hardened WND parsing after a Debug CRT assertion by updating
  `GameWindowManagerScript.cpp::readUntilSemicolon()` to cast to `unsigned char` before
  calling `isspace`, and by normalising custom WND text to ASCII-safe content.
- Removed invalid override-script flags such as `WRAP_CENTERED`, preventing menu-open
  parser failures while keeping the live `Run/Window/` and mirrored `Run/Data/Window/`
  copies in sync.
- Fixed a verified Debug shutdown leak report by releasing map-string tables in
  `GameTextManager::deinit()`, freeing persistent weather-setting roots during final
  `GameLogic` teardown, and making `SnowManager::init()` safely re-entrant so repeated
  initialisation no longer leaks stale snow-data allocations.

### Project Structure
- Removed the original *C&C Generals* (non-Zero Hour) source tree entirely; Zero Hour
  (`GeneralsMD/`) is now the sole build target, reflecting the fact that Zero Hour is a
  superset of all Generals functionality.
- Solution retargeted from `Win32` to `x64` across all configurations:
  `Release|x64`, `Debug|x64`, `DebugW3D|x64`, `Internal|x64`, and `Profile|x64`.
- All `.vcxproj` project files migrated to Visual Studio 2026 toolset (`v144`).
- Pre-built 32-bit libraries replaced with freshly compiled x64 equivalents:
  `WWLib`, `WWMath`, `WWUtil`, `WWDebug`, `WW3D2` (all configurations).
- DirectX SDK updated from the legacy August 2007 SDK to the **June 2010 DirectX SDK**
  (64-bit capable), with updated `d3dx9.h`, `d3dx9math.h`, `d3dx9tex.h`,
  `d3dx9mesh.h`, `d3dx9effect.h`, `d3dx9shader.h`, `d3dx9anim.h`, `d3dx9core.h`,
  `basetsd.h`, and 64-bit import libraries (`d3dx9.lib`, `d3dx9d.lib`).
- `BuildVersion.h` introduced as a fallback version header for local builds,
  defining `VERSION_STRING`, `VERSION_MAJOR`, `VERSION_MINOR`, and
  `VERSION_BUILDNUM` to prevent compiler errors when `GenerateVersion.bat` has not
  been run.
- `GenerateVersion.bat` updated to reflect version 1.05.

### 64-Bit Pointer & Type Correctness
The original codebase was written to target 32-bit Windows exclusively. On a 64-bit
target, `sizeof(void*) == 8` rather than `sizeof(void*) == 4`, meaning that any code
storing a pointer value in a 32-bit integer type causes silent data truncation and
potential crashes or memory corruption.

The following categories of 64-bit type errors were identified and corrected across
the codebase:

#### `HWND` / `HINSTANCE` stored in 32-bit integer types
Window handles and instance handles are pointer-sized on Win64. All sites that stored
an `HWND`, `HINSTANCE`, or `HRESULT` in a plain `int`, `DWORD`, or `UINT` were
updated to use the correct Win64-safe type (`HWND` directly, or `INT_PTR`/`UINT_PTR`
where an integer representation is unavoidable).

**Files affected:** `WinMain.cpp`, `Win32GameEngine.cpp`, `W3DDisplay.cpp`,
`W3DMouse.cpp`, `W3DWebBrowser.cpp`, `AnimateWindowManager.cpp`,
`ProcessAnimateWindow.cpp`, `GameWindow.h`, `WindowVideoManager.h`,
`GadgetComboBox.cpp`, `GadgetListBox.cpp`, `GadgetProgressBar.cpp`,
`W3DCheckBox.cpp`, `W3DComboBox.cpp`, `W3DHorizontalSlider.cpp`,
`W3DListBox.cpp`, `W3DProgressBar.cpp`, `W3DPushButton.cpp`,
`W3DRadioButton.cpp`, `W3DStaticText.cpp`, `W3DTabControl.cpp`,
`W3DTextEntry.cpp`, `W3DVerticalSlider.cpp`, `W3DGameWindow.cpp`,
`Win32DIKeyboard.cpp`, `Win32DIMouse.cpp`.

#### `void*` cast to `int` / `DWORD` (C4311 / C4312 warnings)
Callback identifiers, network tokens, and object identifiers that were historically
passed as `void*` but stored as `int` at call sites were corrected to use
`reinterpret_cast<intptr_t>` or `reinterpret_cast<uintptr_t>` as appropriate,
avoiding any truncation of the upper 32 bits on a 64-bit process.

**Files affected:** `LANAPICallbacks.cpp`, `LobbyUtils.cpp`, `PeerThread.cpp`,
`BuddyThread.cpp`, `PersistentStorageThread.cpp`, `GameInfo.cpp`,
`PartitionManager.cpp`, `ConnectionManager.cpp`, `DisconnectManager.cpp`,
`FirewallHelper.cpp`, `FrameMetrics.cpp`, `GUIUtil.cpp`, `NetPacket.cpp`,
`Transport.cpp`, `NAT.cpp`, `Chat.cpp`, `GameSpyChat.cpp`, `MainMenuUtils.cpp`,
`GameSpyChat.cpp`;  
also GameLogic-side: `CountermeasuresBehavior.cpp`,
`FlightDeckBehavior.cpp`, `SlowDeathBehavior.cpp`, `ActiveBody.cpp`,
`UndeadBody.cpp`, `SabotageInternetCenterCrateCollide.cpp`,
`GarrisonContain.cpp`, `JetAIUpdate.cpp`, `RailroadGuideAIUpdate.cpp`,
`DockUpdate.cpp`, `SpawnPointProductionExitUpdate.cpp`,
`StructureToppleUpdate.cpp`, `Weapon.cpp`, `WeaponSet.cpp`,
`ScriptActions.cpp`, `GameLogicDispatch.cpp`, `AIPathfind.cpp`,
`AIStates.cpp`, `TurretAI.cpp`, `PolygonTrigger.cpp`, `TerrainLogic.cpp`.

#### `WPARAM` / `LPARAM` truncation at message-handling sites
Windows message parameters are pointer-sized on Win64. Message handlers that
packed pointer values into the 32-bit word component of a `WPARAM` or `LPARAM`
were corrected to use the full 64-bit parameter value.

**Files affected:** `CommandXlat.cpp`, `Keyboard.cpp`, `MessageStream.cpp`,
`W3DControlBar.cpp`, `W3DMainMenu.cpp`, various `W3DGadget` files.

#### `sizeof` assumptions assuming 4-byte pointers
All hard-coded `4` / `sizeof(int)` values used in memory layout calculations that
should have been `sizeof(void*)` or `sizeof(size_t)` were corrected.

**Files affected:** `GameMemory.cpp`, `INI.cpp`, `DataChunk.h` (serialisation),
`Dict.h`, `Module.h`, `ThingFactory.cpp`, `ThingTemplate.cpp`.

### DirectX 9 Rendering Compatibility
The engine was originally written against DirectX 8 (`IDirect3D8`, `IDirect3DDevice8`,
`D3DCAPS8`, etc.) via the WW3D2 `DX8Wrapper` layer. EA had already added an
`#ifdef WW3D_USE_D3D9_ALIASES` translation layer that maps D3D8 identifiers to their
D3D9 equivalents at the preprocessor level. This port verified and completed that
migration:

#### `Peek_Current_Caps()` — safe capability accessor
`DX8Wrapper::Get_Current_Caps()` contains a `WWASSERT(CurrentCaps)` which fires a
controlled crash if called before Direct3D initialisation completes. A new non-asserting
accessor was added:

```cpp
// dx8wrapper.h
static const DX8Caps* Peek_Current_Caps() { return CurrentCaps; }  // safe — returns nullptr before init
static const DX8Caps* Get_Current_Caps()  { WWASSERT(CurrentCaps); return CurrentCaps; }
```

All early-startup call sites that could execute before `DX8Wrapper::Init()` has been
called were updated to use `Peek_Current_Caps()` with a null-pointer check and a
sensible capability fallback. **Files corrected:**

- `W3DRadar.cpp` — `findFormat()` capability query for radar texture format selection.
  Added `getSafeFallbackFormat()` helper; falls back to `D3DFMT_R5G6B5` when caps
  are unavailable.
- `W3DDisplay.cpp` — `createVideoBuffer()` format query; falls back to
  `D3DFMT_X8R8G8B8` when caps pointer is null.
- `W3DSnow.cpp` — `ReAcquireResources()` and `render()` calls to
  `Support_PointSprites()`; point-sprite rendering disabled gracefully when caps are
  unavailable.
- `W3DShaderManager.cpp` — `Support_Dot3()`, `Get_Max_Simultaneous_Textures()`,
  `Get_Pixel_Shader_Major_Version()`, and `Get_Pixel_Shader_Minor_Version()` all
  guarded; falls back to lowest-common-denominator shader path.
- `W3DScene.cpp` — `D3DPMISCCAPS_COLORWRITEENABLE` capability flag query guarded.
- `W3DVolumetricShadow.cpp` — Same `D3DPMISCCAPS_COLORWRITEENABLE` guard
  applied.

#### D3DX utility call correctness
All D3DX helper calls (`D3DXCreateTexture`, `D3DXLoadSurfaceFromMemory`,
`D3DXFilterTexture`, `D3DXCreateRenderToSurface`, etc.) were verified against the
June 2010 D3DX9 header declarations to ensure parameter counts and types match the
64-bit-compiled headers. Implicit narrowing conversions on pointer-to-`DWORD` casts
in surface-stride arguments corrected.

**Files affected:** `W3DAssetManager.cpp`, `TerrainTex.cpp`, `W3DTreeBuffer.cpp`,
`W3DSmudge.cpp`, `HeightMap.cpp`, `BaseHeightMap.cpp`, `FlatHeightMap.cpp`,
`W3DWater.cpp`, `W3DWaterTracks.cpp`, `W3DModelDraw.cpp`, `W3DBufferManager.cpp`,
`W3DShadow.cpp`, `W3DProjectedShadow.cpp`.

#### Camera shake integration
`camerashakesystem.cpp` / `camerashakesystem.h`: camera-shake accumulator updated
to use `float` arithmetic in place of the original `INT_PTR`-masking shortcut that
broke on 64-bit due to sign-extension of addresses.

### Deprecated Windows API Modernisation

#### `GetVersionExA` — six call sites replaced
`GetVersionExA` was deprecated in Windows 8.1 and may return incorrect data on
Windows 10 / 11 without an explicit application manifest. All six call sites were
replaced with `RtlGetVersion` (kernel-mode safe, always returns true version numbers):

| File | Purpose |
|------|---------|
| `GlobalLanguage.cpp` | Locale auto-detection for language selection |
| `PopupPlayerInfo.cpp` | Player info overlay OS display string |
| `HeaderTemplate.cpp` | GUI header version banner |
| `GameState.cpp` | Save-game compatibility metadata |
| `W3DDisplay.cpp` | D3D adapter enumeration fallback path |
| `GlobalData.cpp` | System information logging on startup |

#### `GetTickCount` → `GetTickCount64`
`GetTickCount` overflows after approximately 49.7 days of continuous Windows uptime.
All performance and timing sites changed to `GetTickCount64` with appropriate
`ULONGLONG` storage variables to prevent wrap-around errors in long-running sessions
or on machines with high uptime.

**Files affected:** `PerfTimer.cpp`, `Recorder.cpp`, `FrameMetrics.cpp`,
`ConnectionManager.cpp`, `Transport.cpp`.

#### Unsafe string handling
`lstrcpyA`, `lstrcat`, `lstrcpyn` replaced with `StringCchCopy`, `StringCchCat`,
`StringCchCopyN` (from `<strsafe.h>`) at network packet assembly and file-path
handling sites, eliminating potential buffer overflows.

**Files affected:** `NetPacket.cpp`, `Win32BIGFile.cpp`, `Win32BIGFileSystem.cpp`,
`LANGameInfo.cpp`.

#### `BinkVideoPlayer` — pointer width
`BinkVideoPlayer.cpp`: Bink handle stored in a platform-pointer `HANDLE` rather than
`DWORD` to support 64-bit Bink library pointers.

#### `MilesAudioManager` — Miles Sound System 64-bit
`MilesAudioManager.cpp`: Miles audio handle types corrected to `HDIGDRIVER` and
`HMDIDRIVER` pointer types (not `DWORD`) to support the Miles 6 64-bit library.

### Crash Fixes (Start-up Sequence)

The following hard crashes (manifesting as the in-engine "serious error" dialogue and
a `ReleaseCrashInfo.txt` entry) were diagnosed and resolved:

#### Crash 1 — Null `TheRadar` pointer in `GameEngine::update()`
**Symptom:** Fault offset `0x104AF` on first game-loop iteration.  
**Root cause:** `TheRadar->Update()` called unconditionally before `TheRadar` had been
constructed by the subsystem initialisation sequence.  
**Fix:** `GameEngine.cpp` — null guard around `TheRadar->Update()` with lazy
re-initialisation via `initSubsystem()` when `TheSubsystemList` is non-null.

#### Crash 2 — `WWASSERT` in `Get_Current_Caps()` during `W3DRadar::init()`
**Symptom:** Fault offset `0x387638` during radar subsystem initialisation.  
**Root cause:** `W3DRadar::init()` → `findFormat()` → `DX8Wrapper::Get_Current_Caps()`
fired its internal assertion because `DX8Wrapper::CurrentCaps` was still null at the
point the radar subsystem initialised.  
**Fix:** Added `Peek_Current_Caps()` accessor (see above); updated `W3DRadar::findFormat()`
to use the safe accessor with a sensible format fallback.

#### Crash 3 — Per-subsystem attribution in `GameEngine::update()`
**Symptom:** `ReleaseCrashInfo.txt` showed only `Uncaught Exception in GameEngine::update`,
with no indication of which subsystem threw.  
**Fix:** `GameEngine.cpp` updated with individual `try`/`catch` blocks per subsystem
(`TheRadar`, `TheAudio`, `TheGameClient`, `TheMessageStream`, `TheNetwork`,
`TheCDManager`, `TheGameLogic`), each logging a `RELEASE_CRASH` diagnostic message
naming the specific subsystem before re-throwing.

### Build Configuration
- STLport 4.5.3 patch file (`stlport.diff`) provided to guide necessary modifications
  to the STLport headers for 64-bit MSVC compilation (primarily `_REENTRANT` and
  `_NOTHREADS` guard adjustments).
- `AI.h` — corrected `#pragma warning` push/pop nesting broken by 64-bit compile.
- `BaseType.h` — `INT32`/`UINT32` typedefs now conditional on `_WIN64` to avoid
  conflicts with the MSVC 2026 standard library definitions.
- `profile.h` — `__int64` usage updated to `INT64`/`UINT64` typedefs for portability.

---

## EA Open Source Release — Electronic Arts
**Date:** 21 February 2025  
**Repository:** [electronicarts/CnC_Generals_Zero_Hour](https://github.com/electronicarts/CnC_Generals_Zero_Hour)  
**Licence:** GNU General Public Licence v3.0 with additional terms (see `LICENSE.md`)

Electronic Arts released the complete source code for both *Command & Conquer
Generals* and *Command & Conquer Generals: Zero Hour* under the GPU v3 licence
as part of a broader initiative to preserve and continue the C&C franchise in the
community. The release includes:

- Full `GameEngine` C++ source code (AI, scripting, game logic, networking, GUI)
- Full `GameEngineDevice` Win32/W3D device-layer source code (Direct3D rendering,
  Miles audio integration, DirectInput handling, Bink video playback)
- `Libraries/` tree including the WWVegas WW3D2 rendering library, WWLib,
  WWMath, WWUtil, and WWDebug
- All INI data tables, shader files, and asset references in the `/Run/` directory
- GameSpy SDK integration code (deprecated network layer, included for historical
  completeness)
- Tooling source: `CRCDiff`, `textureCompress`, `assetcull`, `versionUpdate`,
  `buildVersionUpdate`, `wolSetup`, `PATCHGET`
- Steam Workshop integration support for both game app IDs
  (Generals: `2229870`, Zero Hour: `2732960`)
- `stlport.diff` patch file for STLport 4.5.3 compatibility
- `README.md` with build instructions and known dependency list

The release was based on the final retail codebase corresponding to version **1.04**
of *Zero Hour*, the last patch distributed by Electronic Arts.

---

## Command & Conquer Generals: Zero Hour

### About Zero Hour
*Command & Conquer Generals: Zero Hour* is the official expansion pack to *Command &
Conquer Generals*, developed by EA Pacific and published by Electronic Arts. It was
released in **North America on 22 September 2003** and in **Europe on 26 September 2003**.

The expansion significantly expanded the game with the *General's Challenge* mode,
sub-general specialisations for each of the three factions, new units across all
factions, new superweapons, and additional single-player campaign missions. The
underlying SAGE/W3D engine received several performance improvements and rendering
additions including smudge effects, improved shadow volumes, and dynamic particle
lighting.

### Original Development Credits — Zero Hour

**EA Pacific (Las Vegas, Nevada)**

*Engine Programming (Zero Hour additions):*
- **Mark Wilczynski** — Smudge effects (`W3DSmudge`), Level-of-Detail (June/Sept 2003)
- **Ian Barkley-Yeung** — Dynamic audio event system (June 2003)
- **Graham Smallwood** — Replace-object upgrade system (July 2003), cave system improvements
- **Colin Day** — Ongoing engine maintenance; INI audio animation system (July 2002)
- **Mark Lorenzen** — Passengers-fire-from-transport upgrade system (May 2003)
- **Steven Johnson** — Upgrade module joint authorship (September 2002)

*WW3D2 / DX8Wrapper (Zero Hour revisions):*
- **Kenny Mitchell** — DX8Wrapper D3D9 compatibility revisions, Camera improvements

*(See Zero Hour 1.00 for full team listing)*

---

### Version 1.04 — Zero Hour (Official Final Patch)
**Developer:** Electronic Arts (EA Los Angeles)  
**Release Date:** c. June 2004

The final official patch produced by Electronic Arts for *Generals: Zero Hour*
and the last version distributed through the EA Download Manager and official patch
servers. This patch closed out support for the WOL (Westwood Online) successor
GameSpy multiplayer service with several stability and anti-cheat improvements.

**Balance & Gameplay:**
- Global Liberation Army: `ToxinTractor` damage-per-second reduced; `Anthrax Beta`
  upgrade toxin cloud duration reduced to prevent indefinite area denial.
- USA: `Particle Cannon` targeting reticule corrected to properly track the aim
  point rather than the camera origin in edge cases.
- China: `Nuclear Missile` silo construction time slightly increased for balance.
- `Jarmen Kell` (GLA Hero) — one-shot vehicle-capture ability was corrected so that
  captured vehicles retain the correct player ownership flag in all circumstances
  (previously could produce orphaned units under certain network desynchronisation
  conditions).
- `Black Lotus` (China Hero) — structure-capture timer now always properly displayed
  in the command bar.
- `Combat Cycle` (GLA Zero Hour unit) — pathfinding near ramps and bridges improved.
- `Helix Helicopter` (China Zero Hour unit) — `Nationalisms` propaganda tower upgrade
  aura radius corrected; no longer clips through buildings.
- `Sneak Attack` GLA tunnel emergence animation fixed in games with more than four
  players.
- Sub-general skirmish AI improved to more reliably use superweapons and stage
  late-game assaults.

**Networking & Multiplayer:**
- MD5 file-integrity verification extended to cover INI data tables, preventing
  common cheats achieved by editing game data files.
- GameSpy authentication handshake timeout extended to accommodate high-latency
  connections.
- `ConnectByAddress` direct-IP connection dialogue made more reliable under
  Windows XP SP2 / SP3 firewall restrictions.
- Desynchronisation detection improved; games with desync now terminate cleanly
  rather than hanging indefinitely.

**Single Player & Campaign:**
- Several mission scripts corrected to prevent objectives from becoming uncompletable
  when the player eliminated scripted units ahead of their trigger time.

**Stability:**
- Fixed a crash that could occur when loading replays recorded under version 1.02
  or earlier.
- Memory leak in the `AISideInfo` script evaluation loop fixed, reducing long-session
  memory growth.

---

### Version 1.03 — Zero Hour
**Developer:** Electronic Arts  
**Release Date:** c. March 2004

**Balance & Gameplay:**
- `Toxin Tractor` damage type reclassified from `FLAME` to `POISON` to resolve
  inconsistency with armour modifiers.
- USA `Ranger` combat drop scatter radius reduced to prevent units landing outside
  the target area on large maps.
- GLA `Rebel Ambush` general's power timing adjusted to prevent immediate capture of
  structures in base Defence scenarios.
- `Suicide Bomber` cycle detonation damage radius normalised across all
  difficulty levels.
- `Desolator` general AI (Zero Hour sub-general) improved to properly call in
  `Desolation` ability during base attacks rather than reserving it indefinitely.
- China `Inferno Cannon` adjusted so that `Napalm` upgrade fire stacks are cleared
  on death.

**Networking:**
- Large-game lobby (8-player) desyncs caused by differing `RandomValue` seed
  timing corrected.
- Chat flooding protection added to WOL/GameSpy lobby.

**Stability:**
- Fixed crash when loading a Zero Hour map in Generals (non-ZH) mode.
- Fixed intermittent crash exiting to main menu from a completed campaign mission.

---

### Version 1.02 — Zero Hour
**Developer:** Electronic Arts  
**Release Date:** c. January 2004

**Balance & Gameplay:**
- `Black Lotus` instant-capture exploit removed: capture now requires the correct
  duration regardless of frame-rate or network conditions.
- `Demo General` (USA sub-general) cluster mines adjusted to prevent unbreakable
  perimeter rings around chokepoints.
- `Laser General` (USA sub-general) `Laser Crusader` damage-per-shot corrected;
  upgrade tier scaling normalised.
- China `Nuke General` `Nuclear Storm` general's power radius reduced.
- GLA `Dr. Thrax` (Anthrax specialist) biohazard container drop ability adjusted
  to prevent splash damage from exceeding primary-fire values.
- GLA `Angry Mob` population count reduced when spawned via general's power.

**Multiplayer:**
- Restored missing `ReplayRecorder` frame header on replays, resolving desynchronised
  replay playback introduced in 1.01.
- Several `LAN` game-host-migration edge cases resolved.

---

### Version 1.01 — Zero Hour
**Developer:** Electronic Arts  
**Release Date:** c. November 2003

First patch for the expansion. Primarily addressed crashes and balance issues
identified in the first weeks following launch.

**Stability:**
- Corrected crash on mission load when the player's starting waypoint lacked a
  defined facing angle.
- Fixed crash in the General's Challenge mode when defeating a general whose
  narration audio file was unavailable.
- Corrected a startup crash on systems without a `Creative Audigy` or compatible
  EAX-capable audio device when `EAX` was enabled in options.

**Balance:**
- Early rush balance adjustments to `Combat Cycle` and `Technical` unit speeds.
- `Inferno Cannon` minimum range corrected to prevent self-damage at maximum elevation.
- `Helix` drop-off passengers action corrected to deposit units at the designated
  drop zone rather than at the Helix's closest landing point.

---

### Version 1.00 — Zero Hour (Initial Release)
**Developer:** EA Pacific (Las Vegas, Nevada)  
**Publisher:** Electronic Arts  
**Release Date:** 22 September 2003 (North America); 26 September 2003 (Europe)  
**Engine:** SAGE (Strategy Action Game Engine) / W3D v2 — 64-bit port in progress

#### Original Development Credits

**EA Pacific — Direction & Production**

| Role | Name |
|------|------|
| Executive Producer | Mike Verdu |
| Producer | Aaron Kaufman |
| Lead Designer | Mark Skaggs |
| Designer | Jason Bender |

**EA Pacific — Programming**

*GameEngine / GameEngineDevice:*
| Name | Primary Contributions |
|------|-----------------------|
| Michael S. Booth | Game engine architecture (from Oct 2000), GameLogic, MessageStream system, View architecture, RandomValue, ParticleSystem |
| Colin Day | WinMain entry point, W3DScene, W3DDisplay, W3DGameClient, W3DView, W3DTerrainVisual, W3DTerrainLogic, W3DInGameUI, terrain and road rendering, weapon system, INI loading, radar (Jan 2002), volumetric shadows (Jan 2001) |
| Matthew D. Campbell | Version system, UDP networking layer, LAN game session management, GameSpy/WOL integration, WOL lobby menus, multiplayer settings, user preferences, CRC/diff tooling, file transfer |
| Chris Huybregts | WOL quick-match and lobby menus, LAN API callbacks, INI control bar scheme |
| Mark Wilczynski | Level-of-detail system, shadow rendering (projected and shadow-volume), ghost objects, mouse cursor system |
| Bryan Cleveland | NAT traversal and firewall helper, web-page launch utility |
| Steven Johnson | Damage effects system, weapon set system, upgrade modules |
| Graham Smallwood | Crate system, tunnel tracker, cave system, weapon set upgrade, stealth-restore upgrades, special-power pause/unpause |
| John Ahlquist | AI scripting engine, script actions and conditions, tile data, world height map, W3D file system, dynamic lighting |
| John McDonald | Video/Bink movie integration |
| Kris Morness | Stealth upgrade modules |
| Amit Kumar | Power plant upgrade system |

*WWVegas WW3D2 Rendering Library (Westwood Studios heritage):*
| Name | Primary Contributions |
|------|-----------------------|
| Jani Penttinen | Original DX8Wrapper Direct3D abstraction layer |
| Kenny Mitchell | DX8Wrapper D3D9 revisions, camera system improvements |
| Greg Hjelstrom | WW3D core engine, collision detection, mathematical utilities, W3D file format, camera system |
| Hector Yee | Texture format conversion, colour space utilities |
| Nathaniel Hoffman | WW3D format header specification |
| Byon Garrabrant | CRC calculation utilities |

**New in Zero Hour:**

*General's Challenge Mode:* 12 unique AI generals across three factions, each with
distinct strategies, voiced taunts, and unique unit compositions.

| Faction | Sub-General | Speciality |
|---------|------------|------------|
| USA | General Alexis Alexander | Super Weapons |
| USA | General Malcolm Granger | Air Force |
| USA | General Townes | Laser Technology |
| USA | General Brigadier General Bronislav | Infantry |
| USA | General Rodall Juhziz | Demolitions |
| China | General Ta Hun Kwai | Tank Warfare |
| China | General Shin Fai | Infantry |
| China | General Leang | Nuke / Strategic |
| China | General Tao | Super Weapons |
| China | General Fai | Air Force |
| GLA | Prince Kassad | Stealth |
| GLA | Dr. Thrax | Anthrax / Biochemical |
| GLA | Rodall Juhziz | Explosives |
| GLA | Desolator | Radiation |
| GLA | Sulaymaan | Guerrilla Warfare / Vehicles |

*New Units:*
- USA: `Sentry Drone` (autonomous scout/attack drone), `Microwave Tank` (crowd
  dispersal / base-clearing weapon for Townes)
- China: `Helix` heavy-lift helicopter (troop transport / propaganda), `Listening
  Outpost` (forward radar structure)
- GLA: `Rocket Buggy` (long-range artillery vehicle), `Combat Cycle` (fast suicide
  / raider unit), `Toxin Tractor` (area-denial chemical sprayer)

*New Superweapons:*
- USA: `Particle Cannon` — precision-directed energy weapon
- China: `Nuclear Missile` — area-effect ballistic weapon (improved over the
  Generals warhead)
- GLA: `SCUD Storm` — inaccurate but devastating ballistic rocket barrage

*Engine Additions (Zero Hour):*
- Smudge/decal system for long-lasting terrain marks (burn scars, tyre marks)
- Dynamic audio event system for context-sensitive ambient audio
- Improved volumetric shadow volumes for Zero Hour structures
- Replace-object upgrade capability (building upgrades with visual model substitution)
- Passengers-fire-from-transport capability (unit-specific flag)

---

## Command & Conquer Generals

### About Generals
*Command & Conquer Generals* is a real-time strategy game developed by **EA Pacific**
and published by **Electronic Arts**. It was released in **North America on
10 February 2003** and in **Europe on 21 February 2003**.

*Generals* was notable as the first entry in the *Command & Conquer* franchise to
abandon the Tiberium and Red Alert settings in favour of a near-future geopolitical
conflict, and the first to feature a fully three-dimensional engine (the SAGE / W3D
engine, an evolution of the Westwood Studios W3D technology used in *Tiberian Sun* and
*Red Alert 2*). The game featured three asymmetric factions — the **United States of
America**, the **People's Republic of China**, and the **Global Liberation Army** — each
with distinct unit rosters, economies, and special abilities mediated by a
*General's Points* progression system during play.

Multiplayer was provided via **GameSpy** (replacing the legacy WOL — Westwood Online —
infrastructure used by earlier C&C titles) with support for LAN play and up to
eight players on a single skirmish map. The game also featured a
*Generals Challenge* mode pitting the player sequentially against twelve AI generals,
a feature carried over and significantly expanded in *Zero Hour*.

### Original Development Credits — Generals

**EA Pacific (Las Vegas, Nevada)**

| Role | Name |
|------|------|
| Executive Producer | Mike Verdu |
| Producer | Aaron Kaufman |
| Lead Designer | Mark Skaggs |
| Designer | Jason Bender |

**Programming** — *same core team as documented under Zero Hour 1.00 (above)*,
with development spanning from **October 2000** (earliest source-code dates) through
February 2003 (gold master). The SAGE / W3D2 engine represents the work of engineers
originally at Westwood Studios (Las Vegas) whose team transferred to EA Pacific
following Electronic Arts' acquisition.

---

### Version 1.08 — Generals (Official Final Patch)
**Developer:** Electronic Arts  
**Release Date:** c. 2004

The final official patch for *Command & Conquer Generals*. All subsequent support was
directed exclusively toward *Zero Hour*.

**Balance & Gameplay:**
- USA: `Comanche` helicopter stinger AoE corrected; `Microwave Tank` (pre-ZH
  variant) rebalanced.
- China: `MiG` aircraft bombing run scatter pattern made consistent.
- GLA: `Tunnel Network` multi-building distance exploit resolved; units could
  previously emerge from any tunnel entrance regardless of whether the originating
  structure was destroyed. Emergence now correctly blocked when source tunnel is
  under construction or destroyed.
- `Jarmen Kell` one-shot vehicle capture now properly grants ownership on the
  first shot in multiplayer (previously required two shots in some network
  conditions).

**Anti-Cheat & Integrity:**
- MD5 file-integrity verification introduced for INI game data; prevents balance
  modification by editing script files locally.
- Replay recording improved; replay files from 1.08 correctly store client version
  to prevent loading mismatched replays.

**Networking:**
- Fixed lobby desynchronisation when reconnecting to a GameSpy room after a
  disconnect.

---

### Version 1.07 — Generals
**Developer:** Electronic Arts  
**Release Date:** c. Late 2003

**Balance:**
- GLA: `Jarmen Kell` sniper ability cost increased via general's points.
- USA: `Aurora Bomber` speed and bomb yield adjusted for better risk/reward balance.
- China: `Red Guard` hit-point buff reverted following competitive feedback;
  `Gattling Cannon` turret damage normalised.
- `Worker` (all factions' supply collectors) pathfinding improvements near supply
  depots to prevent gridlock.

**Stability:**
- Replay desynchronisation on maps with non-standard waypoint counts corrected.

---

### Version 1.06 — Generals
**Developer:** Electronic Arts  
**Release Date:** c. October 2003

**Multiplayer:**
- Formal tournament-mode support added: 1v1 GameSpy ladder integration, automated
  game-result reporting to the WOL statistics service.
- Ping-display in game lobby made more accurate.
- Spectator-mode improvements for tournament observation.

**Balance:**
- Global balance pass across all three factions following community competitive
  feedback from the first extended multiplayer season.
- GLA `SCUD Storm` superweapon (Generals variant) targeting overhauled.
- USA `Spy Drone` veterancy scaling corrected.

---

### Version 1.05 — Generals
**Developer:** Electronic Arts  
**Release Date:** c. August 2003

**Balance:**
- `Stinger Soldier` and `Stinger Site` anti-air damage against aircraft rebalanced.
- China `Battlemaster Tank` cost adjusted.
- GLA `Technical` vehicle reload time standardised across difficulty levels.
- `Supply Centre` supply truck spawn timing corrected to prevent supply-stacking
  exploits on some maps.

**Stability:**
- Fixed a crash when loading custom maps with more than 512 waypoints.

---

### Version 1.04 — Generals
**Developer:** Electronic Arts  
**Release Date:** c. July 2003

**Balance:**
- Further adjustments to `Overlord Tank` crush damage.
- `Black Lotus` capture ability: capture time display corrected in UI.
- GLA `Rebel` unit detection radius adjusted.

**Stability:**
- Black screen on startup resolved for certain Intel integrated graphics
  configurations.
- Bink video player crash on systems without a dedicated sound device corrected.

---

### Version 1.03 — Generals
**Developer:** Electronic Arts  
**Release Date:** c. May 2003

**Balance:**
- `China MiG` aircraft attack frequency adjusted.
- GLA `Arms Dealer` and `Black Market` income rates normalised.
- USA `Tomahawk` launcher range corrected in multiplayer to match single-player
  values.

**Stability:**
- Crash on exit when a `VRAM` allocation failure occurred during render-target
  creation resolved.

---

### Version 1.02 — Generals
**Developer:** Electronic Arts  
**Release Date:** c. April 2003

**Balance:**
- `General's Challenge` AI difficulty scaling tuned; Easy opponents now less
  aggressive in early game rushes.
- USA `Paladin Tank` point-defence laser activation delay corrected.
- GLA `Demo Trap` detonation radius displayed in-game.

**Stability:**
- Crash when deselecting a unit mid-build-queue corrected.
- Multiplayer desynchronisation caused by time-of-day script timing corrected for
  replays recorded at frame rates above 30 fps.

---

### Version 1.01 — Generals
**Developer:** Electronic Arts  
**Release Date:** c. March 2003

First post-launch patch. Primarily addressed balance concerns surfaced by early
multiplayer feedback and a small number of critical crash bugs.

**Balance:**
- USA `Artillery General`'s `Paladin` artillery firing rate reduced from initial
  release values.
- GLA starting build order adjusted to prevent base completion before the first
  wave of a three-player match.

**Stability:**
- Crash when exiting a skirmish game while a supply truck was in transit resolved.
- Fixed hang when connecting to a GameSpy game session that had already started.

---

### Version 1.00 — Generals (Original Release)
**Developer:** EA Pacific (Las Vegas, Nevada)  
**Publisher:** Electronic Arts  
**Release Date:** 10 February 2003 (North America); 21 February 2003 (Europe)  
**Engine:** SAGE (Strategy Action Game Engine) / W3D v2  
**Development period:** October 2000 — February 2003

*Initial release.* See original development credits listed under this section's
header above.

**Factions:**
- **United States of America:** Conventional military with air superiority,
  precision-strike capabilities, and advanced superweapons (MOAB, Particle Cannon
  prototype in some sub-general variants). Strongest individual unit quality and
  special abilities; relatively expensive.
- **People's Republic of China:** Quantity-over-quality doctrine. Tank-heavy ground
  army, nuclear superweapon, propaganda aura for troop morale, and the powerful
  `Overlord Tank`. Mid-tier unit cost and production speed.
- **Global Liberation Army (GLA):** Asymmetric guerrilla force. Cheapest economy;
  no air force; workers can re-place fallen structures via salvage; tunnel network
  for rapid cross-map logistics; `SCUD Storm` and chemical warfare superweapons.

**Game Modes:**
- Single-player campaign: three separate campaign sequences (one per faction), each
  of approximately seven missions covering a fictionalised near-future global conflict.
- Skirmish: configurable AI opponents on any of the included maps, with General's
  Challenge (12 AI generals, one per sub-general, each with escalating difficulty).
- Multiplayer: LAN and GameSpy online, up to eight players.

**Engine Features (SAGE / W3D v2):**
- Fully three-dimensional real-time rendering (vs. the pre-rendered isometric sprites
  of the Tiberian and Red Alert series).
- DirectX 8 via the `DX8Wrapper` abstraction layer originally authored at Westwood
  Studios; hardware transform-and-lighting support.
- Ray-cast height map terrain with dynamic deformation (craters from explosions
  persist on the terrain mesh).
- Bink video integration for FMV cut-scenes.
- Miles Sound System 6 for positional 3D audio and EAX environmental audio effects.
- GameSpy SDK for online matchmaking and statistics.
- INI-driven data tables for units, weapons, upgrades, and maps — enabling extensive
  modding.
- W3D model format (`.w3d`) for units and structures, W3X for maps.
- `WWASSERT` assertion framework used throughout for development-time invariant
  checking (some assertions survive into the Release build via `RELEASE_CRASH`
  macros).

---