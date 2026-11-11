param([switch]$SkipInstaller)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Scripts\PortableZip.ps1')
$root = $PSScriptRoot
Set-Location $root
$version = '0.1.0'
$module = Join-Path $root 'build\windows-x64\KBass_artefacts\Release\VST3\KBass.vst3'
$binary = Join-Path $module 'Contents\x86_64-win\KBass.vst3'
if (!(Test-Path $binary) -or (Get-Item $binary).Length -eq 0) { throw 'Windows x64 VST3 module is missing.' }
$bytes = [IO.File]::ReadAllBytes($binary)
$pe = [BitConverter]::ToInt32($bytes, 0x3c)
if ($bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a -or [BitConverter]::ToUInt16($bytes, $pe + 4) -ne 0x8664) { throw 'The module is not a Windows x64 PE binary.' }
$stage = Join-Path $root 'build\stage'
$packages = Join-Path $root 'build\packages'
New-Item -ItemType Directory -Force $stage, $packages | Out-Null
Copy-Item $module $stage -Recurse -Force
Copy-Item (Join-Path $root 'README.md'), (Join-Path $root 'LICENSE') $stage -Force
Copy-Item (Join-Path $root 'ThirdParty') $stage -Recurse -Force
$portable = Join-Path $packages "KBass-$version-Windows-x64-VST3.zip"
Write-PortableZip $stage $portable
$source = Join-Path $packages 'KBass-Source.zip'
git archive --format=zip --output=$source HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot archive the matching KBass source.' }
$juceSource = Join-Path $root 'build\windows-x64\_deps\juce-src'
if (!(Test-Path "$juceSource\LICENSE.md")) { throw 'Matching JUCE source is missing.' }
Write-PortableZip $juceSource (Join-Path $packages 'JUCE-8.0.6-Source.zip')
if (!$SkipInstaller) {
    $compiler = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'
    if (!(Test-Path $compiler)) { throw 'Install Inno Setup 6 before packaging the installer.' }
    & $compiler "/DAppVersion=$version" "/DStageDir=$stage" "/DOutputDir=$packages" (Join-Path $root 'Installer\KBass.iss')
    if ($LASTEXITCODE -ne 0) { throw 'Installer compilation failed.' }
}
$info = [ordered]@{ version=$version; sourceCommit=(git rev-parse HEAD); format='VST3'; architecture='Windows x64'; standalone=$false }
$info | ConvertTo-Json | Set-Content (Join-Path $packages 'release-info.json') -Encoding UTF8
Write-Host "Packages prepared in $packages"
