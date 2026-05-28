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

$repoRoot = Resolve-AbsolutePath -Path '..' -BasePath $PSScriptRoot
$resolvedBuildDir = Resolve-AbsolutePath -Path $BuildDir -BasePath $repoRoot
$resolvedObsRoot = Resolve-AbsolutePath -Path $ObsRoot -BasePath $repoRoot
$portableMarker = Join-Path $resolvedObsRoot 'portable_mode.txt'
$obsExecutable = Join-Path $resolvedObsRoot 'bin\64bit\obs64.exe'

if (Test-Path $CMakeExecutable) {
  $resolvedCMake = $CMakeExecutable
} else {
  $cmakeCommand = Get-Command $CMakeExecutable -ErrorAction SilentlyContinue
  if (-not $cmakeCommand) {
    throw "CMake executable was not found: $CMakeExecutable"
  }
  $resolvedCMake = $cmakeCommand.Source
}

if (-not (Test-Path $portableMarker)) {
  throw "portable_mode.txt was not found under $resolvedObsRoot"
}

if (-not (Test-Path $obsExecutable)) {
  throw "obs64.exe was not found under $resolvedObsRoot"
}

if (-not $SkipConfigure) {
  $configureDescription = "Configure preset $Preset"
  if ($PSCmdlet.ShouldProcess($resolvedBuildDir, $configureDescription)) {
    & $resolvedCMake --preset $Preset
    if ($LASTEXITCODE -ne 0) {
      exit $LASTEXITCODE
    }
  }
}

if (-not $SkipBuild) {
  $buildDescription = "Build $resolvedBuildDir ($Configuration)"
  if ($PSCmdlet.ShouldProcess($resolvedBuildDir, $buildDescription)) {
    & $resolvedCMake --build $resolvedBuildDir --config $Configuration
    if ($LASTEXITCODE -ne 0) {
      exit $LASTEXITCODE
    }
  }
}

$installDescription = "Install plugin to $resolvedObsRoot"
if ($PSCmdlet.ShouldProcess($resolvedObsRoot, $installDescription)) {
  & $resolvedCMake --install $resolvedBuildDir --config $Configuration --prefix $resolvedObsRoot
  if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
  }
}