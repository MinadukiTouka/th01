# Fetch pinned third-party sources into third_party/.
#   SDL2      2.32.10   https://github.com/libsdl-org/SDL
#   FreeType  2.15.0    https://github.com/freetype/freetype
#
# Usage:
#   powershell -NoProfile -ExecutionPolicy Bypass -File tools\fetch-deps.ps1

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

$root = Split-Path $PSScriptRoot -Parent
$thirdParty = Join-Path $root "third_party"
$cache = Join-Path "C:\th08-toolchain" "downloads"
New-Item -ItemType Directory -Force $thirdParty, $cache | Out-Null

$deps = @(
  @{ Name = "SDL2";     Version = "2.32.10"; File = "SDL2-2.32.10.zip";
     Url = "https://codeload.github.com/libsdl-org/SDL/zip/refs/tags/release-2.32.10";
     ZipRoot = "SDL-release-2.32.10" },
  @{ Name = "freetype"; Version = "2.15.0";  File = "freetype-2.15.0.zip";
     Url = "https://codeload.github.com/freetype/freetype/zip/refs/tags/VER-2-15-0";
     ZipRoot = "freetype-VER-2-15-0" }
)

foreach ($dep in $deps) {
  $target = Join-Path $thirdParty $dep.Name
  $marker = Join-Path $target "CMakeLists.txt"
  if (Test-Path -LiteralPath $marker) {
    Write-Host "$($dep.Name) already present ($($dep.Version))"
    continue
  }

  $zip = Join-Path $cache $dep.File
  if (-not (Test-Path -LiteralPath $zip)) {
    Write-Host "downloading $($dep.Name) $($dep.Version) ..."
    Invoke-WebRequest -Uri $dep.Url -OutFile $zip -UseBasicParsing -TimeoutSec 600
  }

  Write-Host "extracting $($dep.Name) ..."
  $stage = Join-Path $root "_stage"
  New-Item -ItemType Directory -Force $stage | Out-Null
  Expand-Archive -LiteralPath $zip -DestinationPath $stage -Force
  if (Test-Path -LiteralPath $target) { Remove-Item -Recurse -Force $target }
  Move-Item -LiteralPath (Join-Path $stage $dep.ZipRoot) -Destination $target
  Remove-Item -Recurse -Force $stage
  Write-Host "$($dep.Name) -> $target"
}

Write-Host "third-party sources ready."
