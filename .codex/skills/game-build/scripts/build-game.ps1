param(
    [ValidateSet("Debug", "Release", "Retail")]
    [string]$Configuration = "Debug",

    [ValidateSet("x64")]
    [string]$Platform = "x64",

    [switch]$Rebuild
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..\..\..\..")).Path
$solutionPath = Join-Path $repoRoot "Game.sln"

if (-not (Test-Path -LiteralPath $solutionPath)) {
    throw "Could not find Game.sln at '$solutionPath'."
}

$vswherePath = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path -LiteralPath $vswherePath)) {
    throw "Could not find vswhere at '$vswherePath'. Install Visual Studio Installer components."
}

$msbuildPath = (& $vswherePath -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1)
if (-not $msbuildPath) {
    throw "Could not locate MSBuild.exe via vswhere."
}

$target = if ($Rebuild) { "Rebuild" } else { "Build" }
$msbuildArgs = @(
    $solutionPath,
    "/m",
    "/t:$target",
    "/p:Configuration=$Configuration",
    "/p:Platform=$Platform"
)

Write-Host "Using MSBuild: $msbuildPath"
Write-Host "Building $solutionPath ($Configuration|$Platform, target: $target)"

Push-Location $repoRoot
try {
    & $msbuildPath @msbuildArgs
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}
