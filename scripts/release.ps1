<#
.SYNOPSIS
  Build, sign, tag, and publish a release of obs-scene-lighting.

.DESCRIPTION
  Automates the full release workflow:
    1. Build the plugin and generate an NSIS installer via CPack.
    2. Sign the DLL and installer with signtool.
    3. Create a git tag for the release version.
    4. Upload signed assets to a GitHub release via gh CLI.

.PARAMETER Version
  Semantic version string for the release (e.g. "1.0.0").
  Defaults to the version in buildspec.json.

.PARAMETER Configuration
  CMake build configuration. Defaults to RelWithDebInfo.

.PARAMETER CertificateThumbprint
  SHA-1 thumbprint of the code-signing certificate in the Windows certificate store.
  If omitted, signtool automatically selects the best available certificate (/a),
  which works with USB token certificates.

.PARAMETER TimestampServer
  RFC 3161 timestamp server URL for countersigning.

.PARAMETER SkipBuild
  Skip the CMake build step (use existing build artifacts).

.PARAMETER SkipSign
  Skip code signing (for local testing without a certificate).

.PARAMETER SkipTag
  Skip git tag creation.

.PARAMETER SkipPublish
  Skip GitHub release creation and asset upload.

.PARAMETER DryRun
  Show what would be executed without making changes.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
  [string]$Version,
  [string]$Configuration = 'RelWithDebInfo',
  [string]$BuildDir = 'build_x64',
  [string]$Preset = 'windows-x64',
  [string]$CertificateThumbprint,
  [string]$TimestampServer = 'http://timestamp.digicert.com',
  [switch]$SkipBuild,
  [switch]$SkipSign,
  [switch]$SkipTag,
  [switch]$SkipPublish,
  [switch]$DryRun
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot

if (-not $Version) {
  $buildspec = Get-Content (Join-Path $repoRoot 'buildspec.json') -Raw | ConvertFrom-Json
  $Version = $buildspec.version
}

$resolvedBuildDir = Join-Path $repoRoot $BuildDir
$installerPattern = "obs-scene-lighting-*-windows-x64.exe"
$dllRelativePath = "${Configuration}\obs-scene-lighting.dll"

Write-Host "Release: obs-scene-lighting v${Version}" -ForegroundColor Cyan
Write-Host "Build directory: ${resolvedBuildDir}" -ForegroundColor Gray

# --- Step 1: Build and generate NSIS installer ---

if (-not $SkipBuild) {
  Write-Host "`n[1/4] Building and packaging..." -ForegroundColor Yellow

  $cmakeArgs = @('--build', $resolvedBuildDir, '--config', $Configuration)
  if ($DryRun) {
    Write-Host "  DRY RUN: cmake $($cmakeArgs -join ' ')"
  } else {
    if ($PSCmdlet.ShouldProcess($resolvedBuildDir, "CMake build")) {
      cmake @cmakeArgs
      if ($LASTEXITCODE -ne 0) { throw "CMake build failed with exit code $LASTEXITCODE" }
    }
  }

  $cpackArgs = @('-G', 'NSIS', '-C', $Configuration, '-B', $resolvedBuildDir)
  if ($DryRun) {
    Write-Host "  DRY RUN: cpack $($cpackArgs -join ' ')"
  } else {
    if ($PSCmdlet.ShouldProcess($resolvedBuildDir, "CPack NSIS generation")) {
      Push-Location $resolvedBuildDir
      cpack @cpackArgs
      if ($LASTEXITCODE -ne 0) { throw "CPack failed with exit code $LASTEXITCODE" }
      Pop-Location
    }
  }
} else {
  Write-Host "`n[1/4] Skipping build (SkipBuild specified)." -ForegroundColor DarkGray
}

# --- Step 2: Code signing with signtool ---

$installerPath = Get-ChildItem -Path $resolvedBuildDir -Filter $installerPattern -ErrorAction SilentlyContinue |
  Select-Object -First 1 -ExpandProperty FullName
$dllPath = Join-Path $resolvedBuildDir $dllRelativePath

if (-not $SkipSign) {
  Write-Host "`n[2/4] Signing artifacts..." -ForegroundColor Yellow

  $filesToSign = @()
  if (Test-Path $dllPath) { $filesToSign += $dllPath }
  if ($installerPath -and (Test-Path $installerPath)) { $filesToSign += $installerPath }

  if ($filesToSign.Count -eq 0) {
    throw "No artifacts found to sign. Ensure the build step completed successfully."
  }

  foreach ($file in $filesToSign) {
    if ($CertificateThumbprint) {
      $signArgs = @('sign', '/sha1', $CertificateThumbprint, '/fd', 'sha256', '/tr', $TimestampServer, '/td', 'sha256', '/v', $file)
    } else {
      $signArgs = @('sign', '/a', '/fd', 'sha256', '/tr', $TimestampServer, '/td', 'sha256', '/v', $file)
    }

    if ($DryRun) {
      Write-Host "  DRY RUN: signtool $($signArgs -join ' ')"
    } else {
      if ($PSCmdlet.ShouldProcess($file, "Sign with signtool")) {
        signtool @signArgs
        if ($LASTEXITCODE -ne 0) { throw "signtool failed for ${file}" }
        Write-Host "  Signed: $file" -ForegroundColor Green
      }
    }
  }
} else {
  Write-Host "`n[2/4] Skipping code signing (SkipSign specified)." -ForegroundColor DarkGray
}

# --- Step 3: Create git tag ---

$tagName = "v${Version}"

if (-not $SkipTag) {
  Write-Host "`n[3/4] Creating git tag ${tagName}..." -ForegroundColor Yellow

  $existingTag = git tag -l $tagName 2>$null
  if ($existingTag) {
    Write-Host "  Tag ${tagName} already exists, skipping." -ForegroundColor DarkYellow
  } else {
    if ($DryRun) {
      Write-Host "  DRY RUN: git tag -a ${tagName} -m `"Release ${tagName}`""
    } else {
      if ($PSCmdlet.ShouldProcess($tagName, "Create git tag")) {
        git tag -a $tagName -m "Release ${tagName}"
        if ($LASTEXITCODE -ne 0) { throw "git tag creation failed" }
        git push origin $tagName
        if ($LASTEXITCODE -ne 0) { throw "git push tag failed" }
        Write-Host "  Tag ${tagName} created and pushed." -ForegroundColor Green
      }
    }
  }
} else {
  Write-Host "`n[3/4] Skipping tag creation (SkipTag specified)." -ForegroundColor DarkGray
}

# --- Step 4: Create GitHub release and upload assets ---

if (-not $SkipPublish) {
  Write-Host "`n[4/4] Publishing GitHub release..." -ForegroundColor Yellow

  $assets = @()
  if ($installerPath -and (Test-Path $installerPath)) { $assets += $installerPath }

  if ($assets.Count -eq 0) {
    throw "No assets found to upload. Ensure the build and packaging steps completed."
  }

  $ghArgs = @('release', 'create', $tagName, '--title', "Release ${tagName}", '--generate-notes')
  $ghArgs += $assets

  if ($DryRun) {
    Write-Host "  DRY RUN: gh $($ghArgs -join ' ')"
  } else {
    if ($PSCmdlet.ShouldProcess($tagName, "Create GitHub release")) {
      gh @ghArgs
      if ($LASTEXITCODE -ne 0) { throw "gh release create failed" }
      Write-Host "  Release ${tagName} published with $($assets.Count) asset(s)." -ForegroundColor Green
    }
  }
} else {
  Write-Host "`n[4/4] Skipping publish (SkipPublish specified)." -ForegroundColor DarkGray
}

Write-Host "`nRelease workflow complete." -ForegroundColor Cyan
