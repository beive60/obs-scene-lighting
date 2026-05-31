<#
.SYNOPSIS
  Builds and installs the obs-scene-lighting plugin into a portable OBS Studio installation.

.DESCRIPTION
  This script configures, builds, and installs the obs-scene-lighting OBS plugin
  into a local portable OBS Studio directory. It uses CMake presets for configuration
  and supports WhatIf/Confirm via SupportsShouldProcess.

  The target OBS installation must be a portable build (indicated by the presence of
  portable_mode.txt in the root directory).

.PARAMETER ObsRoot
  Path to the portable OBS Studio installation directory.
  Default: 'C:\Apps\OBS-Studio\OBS-Studio-32.1.2-Windows-x64'

.PARAMETER Preset
  CMake preset name used for the configure step.
  Default: 'windows-x64'

.PARAMETER BuildDir
  Path to the CMake build output directory. Can be relative to the repository root.
  Default: 'build_x64'

.PARAMETER Configuration
  CMake build configuration (e.g., Debug, Release, RelWithDebInfo).
  Default: 'RelWithDebInfo'

.PARAMETER CMakeExecutable
  Path or command name for the CMake executable.
  Default: 'C:\Program Files\CMake\bin\cmake.exe'

.PARAMETER SkipConfigure
  When specified, skips the CMake configure step. Useful when the build directory
  is already configured.

.PARAMETER SkipBuild
  When specified, skips the CMake build step. Useful when the plugin is already
  built and only the install step is needed.

.EXAMPLE
  .\install-portable-obs.ps1
  Configures, builds, and installs the plugin using default parameters.

.EXAMPLE
  .\install-portable-obs.ps1 -SkipConfigure -SkipBuild
  Installs a previously built plugin without re-configuring or re-building.

.EXAMPLE
  .\install-portable-obs.ps1 -ObsRoot 'D:\OBS\portable' -Configuration 'Debug'
  Builds a Debug configuration and installs into a custom OBS directory.

.EXAMPLE
  .\install-portable-obs.ps1 -WhatIf
  Shows what actions would be performed without executing them.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
  [string]$ObsRoot = 'C:\Apps\OBS-Studio\OBS-Studio-32.1.2-Windows-x64',
  [string]$Preset = 'windows-x64',
  [string]$BuildDir = 'build_x64',
  [string]$Configuration = 'RelWithDebInfo',
  [string]$CMakeExecutable = 'C:\Program Files\CMake\bin\cmake.exe',
  [switch]$SkipConfigure,
  [switch]$SkipBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-AbsolutePath {
  <#
  .SYNOPSIS
    Resolves a file path to its absolute form.

  .DESCRIPTION
    If the given Path is already rooted (absolute), returns it normalized.
    Otherwise, joins it with BasePath and returns the resulting absolute path.

  .PARAMETER Path
    The file path to resolve. May be relative or absolute.

  .PARAMETER BasePath
    The base directory used to resolve relative paths.

  .OUTPUTS
    [string] The resolved absolute path.
  #>
  param(
    [Parameter(Mandatory = $true)]
    [string]$Path,
    [Parameter(Mandatory = $true)]
    [string]$BasePath
  )

  if ([System.IO.Path]::IsPathRooted($Path)) {
    return [System.IO.Path]::GetFullPath($Path)
  }

  return [System.IO.Path]::GetFullPath((Join-Path $BasePath $Path))
}

# Resolve all paths relative to the repository root.
$repoRoot = Resolve-AbsolutePath -Path '..' -BasePath $PSScriptRoot
$resolvedBuildDir = Resolve-AbsolutePath -Path $BuildDir -BasePath $repoRoot
$resolvedObsRoot = Resolve-AbsolutePath -Path $ObsRoot -BasePath $repoRoot
$portableMarker = Join-Path $resolvedObsRoot 'portable_mode.txt'
$obsExecutable = Join-Path $resolvedObsRoot 'bin\64bit\obs64.exe'

# Locate the CMake executable, checking both direct path and PATH lookup.
if (Test-Path $CMakeExecutable) {
  $resolvedCMake = $CMakeExecutable
} else {
  $cmakeCommand = Get-Command $CMakeExecutable -ErrorAction SilentlyContinue
  if (-not $cmakeCommand) {
    throw "CMake executable was not found: $CMakeExecutable"
  }
  $resolvedCMake = $cmakeCommand.Source
}

# Validate that the target is a portable OBS installation.
if (-not (Test-Path $portableMarker)) {
  throw "portable_mode.txt was not found under $resolvedObsRoot"
}

if (-not (Test-Path $obsExecutable)) {
  throw "obs64.exe was not found under $resolvedObsRoot"
}

# Step 1: Configure the CMake project using the specified preset.
if (-not $SkipConfigure) {
  $configureDescription = "Configure preset $Preset"
  if ($PSCmdlet.ShouldProcess($resolvedBuildDir, $configureDescription)) {
    & $resolvedCMake --preset $Preset
    if ($LASTEXITCODE -ne 0) {
      exit $LASTEXITCODE
    }
  }
}

# Step 2: Build the plugin using the specified configuration.
if (-not $SkipBuild) {
  $buildDescription = "Build $resolvedBuildDir ($Configuration)"
  if ($PSCmdlet.ShouldProcess($resolvedBuildDir, $buildDescription)) {
    & $resolvedCMake --build $resolvedBuildDir --config $Configuration
    if ($LASTEXITCODE -ne 0) {
      exit $LASTEXITCODE
    }
  }
}

# Step 3: Install the built plugin into the portable OBS directory.
$installDescription = "Install plugin to $resolvedObsRoot"
if ($PSCmdlet.ShouldProcess($resolvedObsRoot, $installDescription)) {
  & $resolvedCMake --install $resolvedBuildDir --config $Configuration --prefix $resolvedObsRoot
  if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
  }
}
