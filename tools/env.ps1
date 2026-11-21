# TH01 (Highly Responsive to Prayers) Android port - toolchain environment.
# ASCII-only on purpose: PowerShell 5.1 reads BOM-less .ps1 as ANSI.
#
# Usage:
#   Set-ExecutionPolicy -Scope Process Bypass -Force
#   . .\tools\env.ps1

$env:TH01_TOOLS        = "C:\th08-toolchain"
$env:JAVA_HOME         = Join-Path $env:TH01_TOOLS "jdk17"
$env:GRADLE_USER_HOME  = Join-Path $env:TH01_TOOLS "gradle-home"
$env:ANDROID_HOME      = Join-Path $env:TH01_TOOLS "android-sdk"
$env:ANDROID_SDK_ROOT  = $env:ANDROID_HOME
$env:ANDROID_NDK_HOME  = Join-Path $env:ANDROID_HOME "ndk\27.1.12297006"

# Keep native/Gradle intermediates off the exFAT workspace.
$env:TH01_BUILD_DIR    = Join-Path $env:TH01_TOOLS "build-target-th01"

$env:GRADLE_OPTS = "-Dorg.gradle.jvmargs=-Xmx2048m -XX:MaxMetaspaceSize=512m -Dorg.gradle.daemon=false -Dorg.gradle.parallel=false -Dorg.gradle.workers.max=2"

$env:TH01_PORT_ROOT = Split-Path $PSScriptRoot -Parent
$env:TH01_GRADLE    = Join-Path $env:TH01_TOOLS "gradle\gradle-9.8.2\bin\gradle.bat"

# ---------------------------------------------------------------- ASCII alias
# GNU make / Gradle / CMake do not cope with the Chinese workspace path.
# C:\th01ws is a junction to the real workspace; nothing is copied.
$env:TH01_ALIAS = "C:\th01ws"
if (-not (Test-Path -LiteralPath $env:TH01_ALIAS)) {
  try {
    New-Item -ItemType Junction -Path $env:TH01_ALIAS -Target $env:TH01_PORT_ROOT -ErrorAction Stop | Out-Null
  } catch {
    Write-Warning "could not create junction $($env:TH01_ALIAS): $($_.Exception.Message)"
  }
}
if (Test-Path -LiteralPath (Join-Path $env:TH01_ALIAS "th01-port")) {
  $env:TH01_ALIAS_ROOT = $env:TH01_ALIAS
} else {
  $env:TH01_ALIAS_ROOT = $env:TH01_PORT_ROOT
}

$candidates = @(
  (Join-Path $env:TH01_TOOLS "git\cmd"),
  (Join-Path $env:TH01_TOOLS "mingw64\bin"),
  (Join-Path $env:JAVA_HOME "bin"),
  (Join-Path $env:ANDROID_HOME "platform-tools"),
  (Join-Path $env:TH01_TOOLS "gradle\gradle-9.8.2\bin"),
  (Join-Path $env:TH01_TOOLS "cmake\bin")
)
foreach ($p in $candidates) {
  if (Test-Path -LiteralPath $p) { $env:PATH = "$p;$env:PATH" }
}
