# Alsasua Simulator — Altsasu Manifa en Unreal Engine 5.8

Simulador del pueblo de **Alsasua (Altsasu, Navarra)** en **UE 5.8**, migrado
desde el prototipo Unity. Genera el pueblo proceduralmente a partir de datos
reales (calles, edificios, fachadas, POI, terreno LiDAR, UTM 30N) y simula el
conflicto social con misiones, manifestaciones, multitudes y presión policial.

> **Versión exacta: UE 5.8** (`EngineAssociation=5.8`, C++20, Unity build off).

## Estado actual

- Mundo procedural real: `DirectorArranque` genera terreno (LiDAR), río, plaza,
  **1030 fachadas** (una sección de malla por edificio), 19 landmarks, señales,
  calles y carreteras.
- **Ortofoto PNOA real** (satélite + montes) drapada sobre el terreno y los
  tejados, alineada al grid UTM 30N del mundo (ver sección más abajo).
- Ciclo día/noche y clima (niebla, lluvia, nubosidad, MPC) con setters de render
  **limitados a 4x/seg** para no invalidar el draw-cache de la escena.
- Sistema de misiones (`MisionesSubsystem`) con encadenado M00→M01→…→M12,
  diálogos, apoyo popular y economía.
- Manifestaciones (`ManifestacionSubsystem`): convocatoria, concentración,
  marcha, dispersión por presión policial.
- Perfilado automático: el GameMode arranca `CsvProfile` en `StartPlay` y sale
  solo a los 240 s de wall time escribiendo el CSV.

## Ortofoto PNOA (satélite real)

Imágenes reales del IGN (servicio WMS `pnoa-ma`) georreferenciadas al mismo
grid que el terreno: `world_cm = (UTM_m − 566033, UTM_m − 4741332) × 100`,
con X=east, Y=north. Texturas con el **sur arriba** (v=0 en Ymin).

| Textura | Cobertura | Resolución | Bounds mundo (cm) |
|---|---|---|---|
| `Content/Terreno/alsasua_satelite_pnoa_8192.png` → `/Game/Terreno/T_Satelite_Alsasua` | 7200×7200 m (todo el terreno, montes) | 0.879 m/px | X −168200..551800, Y 497000..1217000 |
| `Content/Textures/ortofoto_pnoa_plaza_8192.png` → `/Game/Textures/T_Ortofoto` | 2750×2750 m (núcleo urbano) | 0.336 m/px | X 54300..329300, Y 719500..994500 |

- `M_Terreno_Orto` (terreno en `-game`) drapea el satélite completo;
  `M_Tejado_Orto` (tejados) usa el detalle urbano y hace fallback al satélite
  fuera de la plaza (máscara "dentro de núcleo", borde suave). `M_TerrenoAlsasua`
  (editor) regenera el drape en cada Play.
- **Regeneración**: `Tools/DescargarOrtofotoPNOA.py` descarga ambos mosaicos
  (WMS `OI.OrthoimageCoverage` + style Default; `OI.MosaicElement` está roto) y
  verifica la alineación por round-trip (corr ≈ 0.999). Después
  `Tools/ImportSatellite.py` (consola Python del editor) reimporta las texturas
  y regenera los materiales.
- Verificación empírica del flip (sur arriba): el río Arakil queda sobre agua
  (media 19.7) con esa convención y sobre tierra (59.0) con la opuesta.

## Módulos C++

| Módulo | Rol |
|---|---|
| `AlsasuaCore` | Datos geo (UTM 30N, `UnityaUnreal`), bus de eventos |
| `AlsasuaWorld` | Generación procedural: terreno, calles, edificios, POI, puentes |
| `AlsasuaEntities` | Base de entidades: NPC, daño, vida |
| `AlsasuaGameplay` | GameMode, misiones, manifestación, ciclo visual, clima, guardado |
| `AlsasuaManifa` | Sistemas de mundo (~51): multitud, tráfico, iluminación, fachadas, optimización, hitch protector, GAS |
| `AlsasuaSimulator` | Generadores de edificios y carreteras |
| `AlsasuaUI` | Widgets (pausa, ajustes) |
| `AlsasuaEditor` | Generadores de assets/materiales en editor |

## Plugins

`EnhancedInput`, `Water`, `Niagara`, `ModelingToolsEditorMode`,
`GameplayAbilities`, `MotionTrajectory`, `PythonScriptPlugin`,
`EditorScriptingUtilities`.

## Compilación (UE 5.8)

```
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
    AlsasuaSimulatorEditor Win64 Development ^
    -Project="<ruta>\AlsasuaSimulator.uproject" -WaitMutex
```

## Ejecución

**Juego standalone** (compila shaders la primera vez, ~1-5 min de boot):

```
UnrealEditor.exe "AlsasuaSimulator.uproject" /Game/Maps/L_Alsasua ^
    -game -DX11 -windowed -ResX=1280 -ResY=720 -NOSPLASH -log
```

Salida CSV: `Saved/Profiling/CSV/Profile(<timestamp>).csv`. El proceso se
autocierra a los 240 s.

**Modos de validación:**

| Flag | Efecto |
|---|---|
| `-NoMisiones` | Omite misiones y multitud (benchmark del pueblo sin carga de IA) |

## Rendimiento

- **Fachadas**: cada ventana era una sección de `ProceduralMesh` (~60k draw
  calls). Ahora **una sección por edificio** → 1030 draw calls.
- **Multitud**: NPCs lejanos fuerzan el LOD más grueso y apagan sombras
  dinámicas (`AlsasuaOptimizerSubsystem`).
- **Sombreado/GI**: Virtual Shadow Maps, Nanite (proyecto), Lumen, instancing
  dinámico en `DefaultEngine.ini`.

Resultado medido con misiones activas: GPU 157 ms → **19 ms/frame**, 48 FPS
estables (antes ~0.3 FPS), sin hitches de 8 s.

## Estructura

```
Config/        # DefaultEngine/Game/Editor.ini (VSM, Nanite, Lumen, streaming)
Source/        # 8 módulos C++ (ver tabla)
Tools/         # Python: importación de assets, paisaje, niagara, setup
Content/       # Assets (NO versionado en git: 18.9 GB, ver abajo)
```

## Versionado

`Source/`, `Config/`, `Tools/`, texturas PNG y datos ligeros se versionan en
git. Quedan fuera los **uassets derivados** (reimportables desde los PNG vía
`Tools/ImportSatellite.py`) y el contenido pesado de importación (raw LiDAR,
meshes). Para eso, usa **Git LFS** o un repositorio de assets aparte.
