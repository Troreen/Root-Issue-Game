param(
    [ValidateSet("Debug", "Release", "Retail")]
    [string]$Configuration = "Debug",

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$GameArgs
)

$scriptRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptRoot "..\..\..\..")).Path
$exePath = Join-Path $repoRoot "Bin\GameMain_$Configuration.exe"

if (-not (Test-Path -LiteralPath $exePath)) {
    throw "Could not find '$exePath'. Build the '$Configuration' configuration first."
}

Write-Host "Running $exePath"

Push-Location $repoRoot
try {
    & $exePath @GameArgs
    if ($LASTEXITCODE -ne $null -and $LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
finally {
    Pop-Location
}
