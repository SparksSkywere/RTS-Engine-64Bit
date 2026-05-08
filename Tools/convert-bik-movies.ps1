# Convert legacy .bik movies under a chosen root into loose modern container files.
param(
	# Root folder to scan recursively for source .bik files.
	[string]$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
	# Optional explicit ffmpeg.exe path.
	[string]$FfmpegPath,
	# Replace existing converted outputs when present.
	[switch]$Overwrite,
	# Keep the embedded audio track in the converted container.
	[switch]$IncludeAudio,
	# Output container to generate for each .bik file.
	[ValidateSet("wmv", "mp4")]
	[string]$OutputFormat = "wmv"
)

$ErrorActionPreference = "Stop"

# Find ffmpeg from an explicit path, PATH, or under the chosen root.
function Resolve-Ffmpeg {
	param([string]$RootPath, [string]$ConfiguredPath)

	if ($ConfiguredPath) {
		if (-not (Test-Path $ConfiguredPath)) {
			throw "ffmpeg not found at '$ConfiguredPath'."
		}
		return (Resolve-Path $ConfiguredPath).Path
	}

	$command = Get-Command ffmpeg -ErrorAction SilentlyContinue
	if ($command) {
		return $command.Source
	}

	$localFfmpeg = Get-ChildItem -Path $RootPath -Filter ffmpeg.exe -File -Recurse -ErrorAction SilentlyContinue |
		Select-Object -First 1
	if ($localFfmpeg) {
		return $localFfmpeg.FullName
	}

	throw "ffmpeg.exe was not found. Install FFmpeg or pass -FfmpegPath."
}

# Enumerate all legacy movie inputs under the chosen root.
function Get-BinkMovieFiles {
	param([string]$RootPath)

	Get-ChildItem -Path $RootPath -Filter *.bik -File -Recurse -ErrorAction SilentlyContinue |
		Sort-Object FullName
}

# Convert a single source movie into the requested output container.
function Convert-BinkMovie {
	param(
		[string]$Ffmpeg,
		[string]$InputFile,
		[string]$OutputFile,
		[bool]$ForceOverwrite,
		[bool]$KeepAudio,
		[string]$Format
	)

	# Build the ffmpeg argument list explicitly to avoid quoting issues.
	$arguments = @()
	$arguments += if ($ForceOverwrite) { "-y" } else { "-n" }
	$arguments += "-i"
	$arguments += $InputFile

	# Default to silent video because the current engine-side backend only renders frames.
	if (-not $KeepAudio) {
		$arguments += "-an"
	}

	# Normalize output cadence so Media Foundation sees a steady 30 FPS stream.
	$arguments += @("-vf", "fps=30", "-vsync", "cfr", "-r", "30")

	# Prefer a lighter WMV encode by default for the legacy full-screen playback path.
	if ($Format -eq "wmv") {
		$arguments += @(
			"-c:v", "wmv2",
			"-b:v", "3500k",
			"-maxrate", "3500k",
			"-bufsize", "7000k"
		)
		if ($KeepAudio) {
			$arguments += @("-c:a", "wmav2", "-b:a", "192k")
		}
	}
	else {
		# Keep MP4 available for compatibility when WMV is not desired.
		$arguments += @(
			"-c:v", "libx264",
			"-preset", "medium",
			"-profile:v", "main",
			"-level", "3.1",
			"-crf", "20",
			"-pix_fmt", "yuv420p"
		)
		if ($KeepAudio) {
			$arguments += @("-c:a", "aac", "-b:a", "192k")
		}
	}

	# Fast-start helps MP4 playback and inspection tools open more quickly.
	if ($Format -eq "mp4") {
		$arguments += @("-movflags", "+faststart")
	}
	$arguments += $OutputFile

	& $Ffmpeg @arguments
	if ($LASTEXITCODE -ne 0) {
		throw "ffmpeg failed for '$InputFile'."
	}
}

# Resolve the working root and encoder before scanning for source movies.
$rootPath = (Resolve-Path $Root).Path
$ffmpeg = Resolve-Ffmpeg -RootPath $rootPath -ConfiguredPath $FfmpegPath

$bikFiles = @(Get-BinkMovieFiles -RootPath $rootPath)

if (-not $bikFiles) {
	Write-Host "No .bik files found under $rootPath"
	return
}

Write-Host "Found $($bikFiles.Count) .bik file(s) under $rootPath"

$converted = 0
$skipped = 0

# Convert each movie beside its original .bik source file.
foreach ($file in $bikFiles) {
	$outputFile = Join-Path $file.DirectoryName ($file.BaseName + "." + $OutputFormat)
	if ((Test-Path $outputFile) -and -not $Overwrite) {
		Write-Host "Skipping existing $outputFile"
		$skipped++
		continue
	}

	Write-Host "Converting $($file.FullName) -> $outputFile"
	Convert-BinkMovie -Ffmpeg $ffmpeg -InputFile $file.FullName -OutputFile $outputFile -ForceOverwrite:$Overwrite.IsPresent -KeepAudio:$IncludeAudio.IsPresent -Format $OutputFormat
	$converted++
}

Write-Host "Completed movie conversion. Converted: $converted Skipped: $skipped"