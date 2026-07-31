# Alsasua Simulator — Altsasu Manifa en Unreal Engine 5.4

Simulador del pueblo de **Alsasua (Altsasu, Navarra)** en **UE 5.4**, migrado
desde el prototipo Unity. Genera el pueblo proceduralmente a partir de datos
reales (calles, edificios, fachadas, POI, terreno LiDAR, UTM 30N) y simula el
conflicto social con misiones, manifestaciones, multitudes y presión policial.

> **Versión exacta: UE 5.4** (`EngineAssociation=5.4`, C++20, Unity build off).

## Estado actual

- Mundo procedural real: `DirectorArranque` genera terreno (LiDAR), río, plaza,
  **1030 fachadas** (una sección de malla por edificio), 19 landmarks, señales,
  calles y carreteras.
- Ciclo día/noche y clima (niebla, lluvia, nubosidad, MPC) con setters de render
  **limitados a 4x/seg** para no invalidar el draw-cache de la escena.
- Sistema de misiones (`MisionesSubsystem`) con encadenado M00→M01→…→M12,
  diálogos, apoyo popular y economía.
- Manifestaciones (`ManifestacionSubsystem`): convocatoria, concentración,
  marcha, dispersión por presión policial.
- Perfilado automático: el GameMode arranca `CsvProfile` en `StartPlay` y sale
  solo a los 240 s de wall time escribiendo el CSV.

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

## Compilación (UE 5.4)

```
"F:\Epic Games\UE_5.4\Engine\Build\BatchFiles\Build.bat" ^
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

`Source/`, `Config/`, `Tools/` se versionan en git. **`Content/` queda fuera**
(18.9 GB: raw LiDAR, satélite, meshes importados). Para versionarlo, usa
**Git LFS** o un repositorio de assets aparte.
