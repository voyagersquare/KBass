param(
    [Parameter(Mandatory=$true)][string]$Bundle,
    [Parameter(Mandatory=$true)][string]$Validator,
    [Parameter(Mandatory=$true)][string]$ResultsDirectory
)
$ErrorActionPreference = 'Stop'
# This test writes only to an ephemeral GitHub Actions Windows runner.
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Run this system-directory check in GitHub Actions, not on a personal computer.' }
if (![Environment]::Is64BitProcess) { throw 'The check needs 64-bit PowerShell.' }
$source = (Resolve-Path -LiteralPath $Bundle).Path
$results = [IO.Path]::GetFullPath($ResultsDirectory)
New-Item -ItemType Directory -Force -Path $results | Out-Null
$systemRoot = Join-Path $env:CommonProgramFiles 'VST3'
$systemBundle = Join-Path $systemRoot 'KBass.vst3'
$relativeModule = 'Contents\x86_64-win\KBass.vst3'
$module = Join-Path $source $relativeModule
if (!(Test-Path -LiteralPath $module)) { throw 'The source bundle has no Windows x64 module.' }
if (Test-Path -LiteralPath $systemBundle) { throw 'Refusing to replace an existing system plugin.' }
$rawMetadata = Get-Content -LiteralPath (Join-Path $source 'Contents\Resources\moduleinfo.json') -Raw
# The pinned Steinberg SDK emits JSON5-style trailing commas. Read its fields
# without altering the shipped manifest or any bytes in the tested bundle.
$metadata = [regex]::Replace($rawMetadata, ',\s*([}\]])', '$1') | ConvertFrom-Json
if ($metadata.'Factory Info'.Vendor -ne 'Voyager Square') { throw 'Unexpected plugin manufacturer.' }
$instrument = $metadata.Classes | Where-Object { $_.Category -eq 'Audio Module Class' }
if ($instrument.Name -ne 'KBass' -or $instrument.'Sub Categories' -notcontains 'Instrument') { throw 'Not the expected VST3 instrument.' }

function Get-BundleHashes([string]$Root) {
    $base = (Resolve-Path -LiteralPath $Root).Path
    Get-ChildItem -LiteralPath $base -File -Recurse | Sort-Object FullName | ForEach-Object {
        $_.FullName.Substring($base.Length + 1).Replace('\','/') + ' ' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
}
function Invoke-Validation([string]$Path, [string]$Label) {
    $logDir = Join-Path $results $Label
    $arguments = @('--strictness-level','5','--timeout-ms','60000','--output-dir',('"' + $logDir + '"'),'--validate',('"' + $Path + '"'))
    $process = Start-Process -FilePath $Validator -ArgumentList $arguments -Wait -PassThru -NoNewWindow
    if ($process.ExitCode -ne 0) { throw "Validation failed for $Label, exit $($process.ExitCode)." }
    $logs = Get-ChildItem -LiteralPath $logDir -Filter '*.txt'
    if (!$logs -or !(Select-String -Path $logs.FullName -Pattern '^SUCCESS$' -Quiet)) { throw "Missing successful report for $Label." }
}
$report = [ordered]@{
    status='FAIL'; release='v0.1.0-preview.4'; name='KBass'; manufacturer='Voyager Square';
    sourceModuleHash=(Get-FileHash -LiteralPath $module -Algorithm SHA256).Hash.ToLowerInvariant();
    systemPath=$systemBundle; fullBundleChecksumMatch=$false; sourceValidation=$false;
    systemPathValidation=$false; cleanup=$false; installerBuilt=$false;
    installerExecuted=$false; abletonLiveTested=$false
}
$copied = $false
try {
    Invoke-Validation $source 'source'
    $report.sourceValidation = $true
    New-Item -ItemType Directory -Force -Path $systemRoot | Out-Null
    Copy-Item -LiteralPath $source -Destination $systemRoot -Recurse
    $copied = $true
    if (Compare-Object @(Get-BundleHashes $source) @(Get-BundleHashes $systemBundle)) { throw 'The copied system bundle differs from the published plugin.' }
    $report.fullBundleChecksumMatch = $true
    Invoke-Validation $systemBundle 'system-path'
    $report.systemPathValidation = $true
    $report.status = 'PASS'
} catch {
    $report.error = $_.Exception.Message
    throw
} finally {
    if ($copied) { Remove-Item -LiteralPath $systemBundle -Recurse -Force }
    $report.cleanup = !(Test-Path -LiteralPath $systemBundle)
    $report | ConvertTo-Json -Depth 5 | Set-Content (Join-Path $results 'system-path-test.json') -Encoding UTF8
    $report | ConvertTo-Json -Depth 5 | Write-Host
}
