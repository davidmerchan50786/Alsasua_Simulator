$ErrorActionPreference = 'Stop'

$ProjectRoot = 'F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject'
$ProjectFile = Join-Path $ProjectRoot 'AlsasuaSimulator.uproject'
$DocsRoot = Join-Path $env:USERPROFILE 'Documents\Unreal Projects'
$ProjectLink = Join-Path $DocsRoot 'AlsasuaSimulator'
$MegascansSrc = 'J:\Documentos\Unreal Projects\CitySample\Content\Megascans'
$MegascansDst = Join-Path $ProjectRoot 'Content\External\Megascans'
$LogFile = Join-Path $ProjectRoot 'Tools\setup_alsasua_project.log'

function Write-Log($msg) {
    $stamp = Get-Date -Format 'yyyy-MM-dd HH:mm:ss'
    $line = "[$stamp] $msg"
    Add-Content -LiteralPath $LogFile -Value $line
    Write-Host $line
}

New-Item -ItemType Directory -Path $DocsRoot -Force | Out-Null
New-Item -ItemType Directory -Path $ProjectRoot\Tools -Force | Out-Null
New-Item -ItemType File -Path $LogFile -Force | Out-Null

if (-not (Test-Path -LiteralPath $ProjectFile)) {
    throw "No se encontró el proyecto: $ProjectFile"
}

# Crear acceso desde Documents/Unreal Projects
if (Test-Path -LiteralPath $ProjectLink) {
    if ((Get-Item -LiteralPath $ProjectLink).LinkType -eq 'Junction') {
        Remove-Item -LiteralPath $ProjectLink -Force
    } else {
        Remove-Item -LiteralPath $ProjectLink -Force
    }
}

New-Item -ItemType Junction -Path $ProjectLink -Target $ProjectRoot -Force | Out-Null
Write-Log "Junction creado: $ProjectLink -> $ProjectRoot"

# Ajustar EngineAssociation a 5.8 si no está ya
$uproject = Get-Content -LiteralPath $ProjectFile -Raw
if ($uproject -notmatch '"EngineAssociation"\s*:\s*"5\.8"') {
    $uproject = $uproject -replace '"EngineAssociation"\s*:\s*"[^"]+"', '"EngineAssociation": "5.8"'
    Set-Content -LiteralPath $ProjectFile -Value $uproject -NoNewline
    Write-Log 'EngineAssociation ajustado a 5.8'
} else {
    Write-Log 'EngineAssociation ya apunta a 5.8'
}

# Copiar Megascans si existe la fuente
if (Test-Path -LiteralPath $MegascansSrc) {
    New-Item -ItemType Directory -Path $MegascansDst -Force | Out-Null
    Copy-Item -LiteralPath $MegascansSrc -Destination $MegascansDst -Recurse -Force
    Write-Log "Megascans copiado a $MegascansDst"
} else {
    Write-Log "No se encontró la fuente de Megascans: $MegascansSrc"
}

Write-Log 'Listo. Abre el proyecto desde Documents/Unreal Projects/AlsasuaSimulator o desde el .uproject.'
