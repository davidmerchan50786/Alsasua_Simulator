param(
    [string]$SourceProject = "F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject",
    [string]$OutputDirectory = "H:\Temp\opencode\AlsasuaReleases",
    [int64]$MaxBytes = 1500MB
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
$assets = @(
    "DZ_Assets", "FreeAnimationLibrary", "GV_FreeShrubsPack", "HighPoly_Tree_Model",
    "Man", "Megaplant_Library", "Nanite_Plants_Sample_Collection", "OWD_Flowers_Pack",
    "Vefects", "VehicleVarietyPack"
)
$archives = @()

function Add-Archive([string]$Name, [string[]]$Paths) {
    $archive = Join-Path $OutputDirectory $Name
    & tar -czf $archive -C $SourceProject @Paths
    if ($LASTEXITCODE) { throw "tar falló: $Name" }
    $script:archives += [PSCustomObject]@{
        name = $Name
        sha256 = (Get-FileHash $archive -Algorithm SHA256).Hash
        bytes = (Get-Item $archive).Length
    }
}

foreach ($asset in $assets) { Add-Archive "alsasua-$asset.tar.gz" @("Content/$asset") }

$cityRoot = Join-Path $SourceProject "Content\UnrealDrive_CitySample"
$cityOther = Get-ChildItem -LiteralPath $cityRoot -Force | Where-Object Name -ne "Textures" | ForEach-Object { "Content/UnrealDrive_CitySample/$($_.Name)" }
Add-Archive "alsasua-UnrealDrive_CitySample-core.tar.gz" $cityOther

$chunk = @(); $bytes = 0; $index = 1
Get-ChildItem -LiteralPath "$cityRoot\Textures" -Recurse -File | ForEach-Object {
    if ($chunk.Count -and $bytes + $_.Length -gt $MaxBytes) {
        Add-Archive ("alsasua-UnrealDrive_CitySample-textures-{0:d2}.tar.gz" -f $index) $chunk
        $index++; $chunk = @(); $bytes = 0
    }
    $chunk += "Content/UnrealDrive_CitySample/Textures/$($_.FullName.Substring((Join-Path $cityRoot 'Textures').Length).TrimStart('\'))"
    $bytes += $_.Length
}
if ($chunk.Count) { Add-Archive ("alsasua-UnrealDrive_CitySample-textures-{0:d2}.tar.gz" -f $index) $chunk }

@{ version = "assets-2026.08"; assets = $archives } | ConvertTo-Json -Depth 3 |
    Set-Content -Encoding utf8 (Join-Path $OutputDirectory "alsasua-external-assets.json")
