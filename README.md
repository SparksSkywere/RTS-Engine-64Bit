
# RTS Game Engine (64Bit)

This repository includes source code for Command & Conquer Generals, and its expansion pack Zero Hour. This release provides support to the Steam Workshop for both games ([C&C Generals](https://steamcommunity.com/workshop/browse/?appid=2229870) and [C&C Generals - Zero Hour](https://steamcommunity.com/workshop/browse/?appid=2732960)).

**Author:** SparksSkywere  
**Repository:** [electronicarts/CnC_Generals_Zero_Hour](https://github.com/electronicarts/CnC_Generals_Zero_Hour)  
**Branch:** `main`  
**Status:** Active development — *build in progress as of 2th Feb 2025*

## Dependencies (WORKING TO CLEANUP OR REMOVE)
If you wish to rebuild the source code and tools successfully you will need to find or write new replacements (or remove the code using them entirely) for the following libraries;

- DirectX SDK (Version 9.0 or higher) (expected path `\Code\Libraries\DirectX\`) - (DX9 64bit June 2010)
- STLport (4.5.3) - (expected path `\Code\Libraries\STLport-4.5.3`)
- 3DSMax 4 SDK - (expected path `\Code\Libraries\Max4SDK\`) - Unable to find
- NVASM - (expected path `\Code\Tools\NVASM\`) - Nvidia based, cannot replace
- BYTEmark - (expected path `\Code\Libraries\Source\Benchmark`)
- RAD Miles Sound System SDK - (expected path `\Code\Libraries\Source\WWVegas\Miles6\`)
- RAD Bink SDK - REMOVED. Movie playback now uses the in-repo Media Foundation backend and expects loose converted video files such as `.mp4`.
- SafeDisk API - (expected path `\Code\GameEngine\Include\Common\SafeDisk` and `\Code\Tools\Launcher\SafeDisk\`) - REMOVED
- Miles Sound System "Asimp3" - (expected path `\Code\Libraries\WPAudio\Asimp3`)
- GameSpy SDK - (expected path `\Code\Libraries\Source\GameSpy\`) - (DEPRECATED, due removal)
- ZLib (1.1.4) - (expected path `\Code\Libraries\Source\Compression\ZLib\`)
- LZH-Light (1.0) - (expected path `\Code\Libraries\Source\Compression\LZHCompress\CompLibSource` and `CompLibHeader`)

## Compiling (Win64)
To use the compiled binaries, you must own the game. The C&C Ultimate Collection is available for purchase on [EA App](https://www.ea.com/en-gb/games/command-and-conquer/command-and-conquer-the-ultimate-collection/buy/pc) or [Steam](https://store.steampowered.com/bundle/39394/Command__Conquer_The_Ultimate_Collection/).

The quickest way to build all configurations in the project is to open `rts.sln` in Microsoft Visual Studio C++ 2026 Community (Zero Hour patch 1.04) and select Build -> Batch Build, then hit the “Rebuild All” button.

The Batch Build now includes `WorldBuilder`. To build it on x64 with VS 2026, install the Visual Studio MFC desktop component for the active MSVC toolset/architecture; otherwise the solution will fail with `MSB8041` before compiling the editor project.

When the workspace has finished building, the compiled binaries will be copied to the folder called `/Run/` found in the root of the game directory.

## Movie Playback Replacement
The proprietary RAD Bink playback path has been removed from the runtime. Movie playback now uses Windows Media Foundation through the engine's existing `VideoPlayer` abstraction and looks for loose converted movie files with the same base names as the original `.bik` assets.

Supported replacement extensions are `.wmv`, `.mp4`, `.avi`, `.mov`, and `.m4v`. The runtime currently prefers `.wmv` before `.mp4` when multiple converted files with the same base name are present.

If your installation still only contains `.bik` files, convert them first. A helper script is included at `Tools\convert-bik-movies.ps1` and will recursively scan the selected root for `.bik` files and convert them to `.wmv` by default when `ffmpeg.exe` is available in `PATH`, found under that root, or passed explicitly. Pass `-OutputFormat mp4` if you still want H.264 `.mp4` output.

The helper defaults are tuned for the current replacement backend:

- `wmv` is the default output format because it has been the lighter path for the current Media Foundation playback integration.
- output is normalized to a constant `30 FPS` stream to avoid uneven frame pacing from source files with awkward timing.
- converted files are written beside the original `.bik` files using the same base name.
- audio is stripped by default because the current engine-side backend still renders video frames only; add `-IncludeAudio` only if you want the converted container to preserve that track for future use or external playback.

Key parameters:

- `-Root` scans any selected game/content root recursively for `.bik` files.
- `-FfmpegPath` points at a specific `ffmpeg.exe` if it is not on `PATH`.
- `-Overwrite` replaces existing converted outputs.
- `-IncludeAudio` keeps the embedded movie audio track in the converted file.
- `-OutputFormat wmv|mp4` chooses the output container.

Example:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\convert-bik-movies.ps1

# Or point it at any other root that contains legacy .bik files
powershell -ExecutionPolicy Bypass -File .\Tools\convert-bik-movies.ps1 -Root "D:\Games\Zero Hour"

# Or force MP4 output instead of the WMV default
powershell -ExecutionPolicy Bypass -File .\Tools\convert-bik-movies.ps1 -OutputFormat mp4

# Or overwrite existing conversions and keep embedded audio in the output container
powershell -ExecutionPolicy Bypass -File .\Tools\convert-bik-movies.ps1 -Overwrite -IncludeAudio
```

If you want the converted files to keep embedded audio tracks, add `-IncludeAudio`. The current engine-side replacement backend decodes video frames only, so audio in converted containers is not yet routed through the game audio system.

## Known Issues
Windows has a policy where executables that contain words “version”, “update” or “install” in their filename will require UAC Elevation to run. This will affect “versionUpdate” and “buildVersionUpdate” projects from running as post-build events. Renaming the output binary name for these projects to not include these words should resolve the issue for you.

## STLport
STLport will require changes to successfully compile this source code. The file [stlport.diff](stlport.diff) has been provided for you so you can review and apply these changes. Please make sure you are using STLport 4.5.3 before attempting to apply the patch.

## Contributing
This repository will accept contributions (pull requests, issues, etc). If you wish to create changes to the source code and encourage collaboration, please create a fork of the repository under your GitHub user/organization space.

## License
This repository and its contents are licensed under the GPL v3 license, with additional terms applied. Please see [LICENSE.md](LICENSE.md) for details.