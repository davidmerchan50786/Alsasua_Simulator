param(
    [string]$Destination = (Resolve-Path "$PSScriptRoot\\.."),
    [string]$Repository = "davidmerchan50786/Alsasua_Simulator",
    [string]$ReleaseTag = "assets-2026.08"
)

$ErrorActionPreference = "Stop"
$baseUrl = "https://github.com/$Repository/releases/download/$ReleaseTag"
$manifestPath = Join-Path $env:TEMP "alsasua-external-assets.json"
Invoke-WebRequest "$baseUrl/alsasua-external-assets.json" -OutFile $manifestPath
$manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json

foreach ($asset in $manifest.assets) {
    $archive = Join-Path $env:TEMP $asset.name
    Invoke-WebRequest "$baseUrl/$($asset.name)" -OutFile $archive
    if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne $asset.sha256) {
        throw "Checksum inválido: $($asset.name)"
    }
    tar -xzf $archive -C $Destination
}
