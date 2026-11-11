param(
    [Parameter(Mandatory=$true)][string]$SetupPath,
    [Parameter(Mandatory=$true)][string]$SourceBundle,
    [Parameter(Mandatory=$true)][string]$ValidatorPath
)
$ErrorActionPreference = 'Stop'
# This test installs to Program Files. Only run it in an isolated Actions VM.
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Installer smoke tests require an isolated GitHub Actions runner.' }
$setup = (Resolve-Path $SetupPath).Path
$source = (Resolve-Path $SourceBundle).Path
$validator = (Resolve-Path $ValidatorPath).Path
$packages = Join-Path $PSScriptRoot '..\build\packages'
$bundle = Join-Path $env:CommonProgramW6432 'VST3\KBass.vst3'
$app = Join-Path $env:ProgramFiles 'Independent Audio\KBass'
$registration = 'HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{3582A89D-57AD-4B61-A3C1-C23442F8FBA3}_is1'
if ((Test-Path $bundle) -or (Test-Path $app) -or (Test-Path $registration)) {
    throw 'Expected a clean runner without an existing KBass installation.'
}
$presetFolder = Join-Path $env:APPDATA 'Independent Audio\KBass\Presets'
New-Item -ItemType Directory -Force $presetFolder | Out-Null
$preset = Join-Path $presetFolder ('installer-preservation-' + [guid]::NewGuid().ToString() + '.kbasspreset')
'KBass installer preservation fixture' | Set-Content $preset -Encoding UTF8
$presetHash = (Get-FileHash $preset -Algorithm SHA256).Hash
function Assert-PresetPreserved {
    if (!(Test-Path $preset) -or (Get-FileHash $preset -Algorithm SHA256).Hash -ne $presetHash) {
        throw 'The installer modified a user preset.'
    }
}
function Assert-InstalledBundle {
    foreach ($file in (Get-ChildItem $source -File -Recurse)) {
        $relative = $file.FullName.Substring($source.Length).TrimStart([char[]]'\/')
        $installed = Join-Path $bundle $relative
        if (!(Test-Path $installed) -or (Get-FileHash $installed -Algorithm SHA256).Hash -ne (Get-FileHash $file.FullName -Algorithm SHA256).Hash) {
            throw "Missing or different installed VST3 file: $relative"
        }
    }
    $entry = Get-ItemProperty $registration
    if ($entry.Publisher -ne 'Independent Audio' -or $entry.DisplayVersion -ne '0.3.0') {
        throw 'The installed publisher or version is wrong.'
    }
    if (!(Test-Path (Join-Path $app 'Documentation\SATURATION.md'))) { throw 'Documentation was not installed.' }
    Assert-PresetPreserved
}
function Invoke-Setup([string]$phase) {
    $log = Join-Path $packages ("installer-$phase.log")
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/SP-','/NORESTART',('/LOG="' + $log + '"'))
    $process = Start-Process -FilePath $setup -ArgumentList $arguments -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Installer $phase failed: $($process.ExitCode)" }
    Assert-InstalledBundle
}
try {
    # Simulate updating a manually copied ZIP bundle, including an obsolete binary.
    New-Item -ItemType Directory -Force (Split-Path $bundle) | Out-Null
    Copy-Item $source $bundle -Recurse -Force
    'Old manually copied KBass binary' | Set-Content (Join-Path $bundle 'Contents\x86_64-win\KBass.vst3')
    Invoke-Setup 'install'

    # Reinstalling must repair the payload without creating a second registration.
    'Damaged KBass binary' | Set-Content (Join-Path $bundle 'Contents\x86_64-win\KBass.vst3')
    Invoke-Setup 'repair'

    $logDir = Join-Path $PSScriptRoot '..\build\installed-pluginval'
    $arguments = @('--strictness-level','5','--timeout-ms','60000','--output-dir',('"' + $logDir + '"'),'--validate',('"' + $bundle + '"'))
    $process = Start-Process -FilePath $validator -ArgumentList $arguments -Wait -PassThru -NoNewWindow
    if ($process.ExitCode -ne 0) { throw 'pluginval rejected the installed system VST3.' }
    $logs = Get-ChildItem $logDir -Filter '*.txt'
    if (!$logs -or !(Select-String -Path $logs.FullName -Pattern '^SUCCESS$' -Quiet)) { throw 'Missing installed pluginval success report.' }
    Copy-Item $logs.FullName $packages -Force

    $uninstaller = Join-Path $app 'Uninstall\unins000.exe'
    if (!(Test-Path $uninstaller)) { throw 'Uninstaller is missing.' }
    $log = Join-Path $packages 'installer-uninstall.log'
    $process = Start-Process -FilePath $uninstaller -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/LOG="' + $log + '"')) -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Uninstall failed: $($process.ExitCode)" }
    if ((Test-Path (Join-Path $bundle 'Contents\x86_64-win\KBass.vst3')) -or (Test-Path $registration)) { throw 'Uninstall left the plugin or its registration behind.' }
    Assert-PresetPreserved

    $report = [ordered]@{
        version='0.3.0'; manufacturer='Independent Audio'; sourceCommit=$env:GITHUB_SHA;
        installedPath=$bundle; installedBinarySHA256=(Get-FileHash (Join-Path $source 'Contents\x86_64-win\KBass.vst3') -Algorithm SHA256).Hash.ToLowerInvariant();
        installerSHA256=(Get-FileHash $setup -Algorithm SHA256).Hash.ToLowerInvariant();
        installerBuilt=$true; installerExecuted=$true; manualBundleUpgrade=$true;
        repairPassed=$true; installedPluginvalPassed=$true; uninstallPassed=$true; userPresetsPreserved=$true
    }
    $report | ConvertTo-Json | Set-Content (Join-Path $packages 'installer-test-results.json') -Encoding UTF8
    Write-Host 'PASS: system VST3 installation, manual-bundle upgrade, repair, installed pluginval, uninstall and user preset preservation.'
} finally {
    Remove-Item $preset -ErrorAction SilentlyContinue
}
