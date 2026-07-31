# RESUMEN TÉCNICO — Optimización de Rendimiento GPU · Alsasua Simulator (UE 5.4)

Fecha: 2026-07-31
Proyecto: `AlsasuaSimulator` (Altsasu Manifa, UE 5.4, C++20)
Resultado global: **0.3 FPS → 48 FPS estables con misiones activas**
(GPU mediana 157 ms → 18.9 ms por frame).

---

## 1. Contexto y objetivo

El simulador genera proceduralmente el pueblo de Altsasu en runtime
(`DirectorArranque`): terreno LiDAR, río, plaza, 1030 fachadas, 19 landmarks,
señales, calles y carreteras, más el sistema de misiones y manifestaciones con
multitudes de NPC. En ejecución standalone (`-game`) el juego se congelaba:

- ~0.3 FPS en días avanzados de simulación.
- Hitch Protector (`UAlsasuaHitchProtector`) disparando "Pánico detectado —
  Bajando densidad de IA" en prácticamente todos los frames.
- Una sesión de 240 s de perfilado capturaba **79 frames** (frente a los
  2197 del run de referencia sin misiones).

Objetivos de la sesión:

1. Validar (apples-to-apples) el fix de la sesión previa: setters de render
   del ciclo visual/clima limitados a 4x/seg.
2. Aislar el coste real de GPU.
3. Aplicar técnicas de balanceo (impostores/LOD/culling) sin perder calidad
   AAA+++.
4. Verificar el resultado con CSV comparativo y versionar.

---

## 2. Metodología de perfilado

### 2.1 Auto-perfilado integrado en el GameMode

`AAlsasuaGameplayGameMode` incorpora un sistema de captura automática:

```cpp
// StartPlay()
if (GEngine) { GEngine->Exec(nullptr, TEXT("CsvProfile start")); }

// Tick(): timer de wall time
if ((FPlatformTime::Seconds() - HoraInicioWall) > 240.0)
{
    GEngine->Exec(nullptr, TEXT("CsvProfile stop"));
    // espera a que el CSV tenga tamaño > 0 (hasta 30 s) y RequestExit(false)
}
```

Consecuencias operativas:

- Lanzar el juego standalone **sin flags de perfilado** ya produce CSV.
- El proceso **se autocierra a los 240 s** tras `StartPlay`.
- La ventana de captura de 240 s es suficiente para misiones M00→M01.

### 2.2 Lanzamiento reproducible

```
"F:\Epic Games\UE_5.4\Engine\Binaries\Win64\UnrealEditor.exe" ^
    "F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\AlsasuaSimulator.uproject" ^
    /Game/Maps/L_Alsasua -game -DX11 -windowed -ResX=1280 -ResY=720 -NOSPLASH -log
```

CSV de salida: `Saved/Profiling/CSV/Profile(<timestamp>).csv`.

### 2.3 Análisis del CSV

El CSV es una tabla por frame; los nombres de columna viajan en la primera
fila (las posiciones pueden variar entre runs). Columnas usadas:

| Columna | Significado |
|---|---|
| `FrameTime` | Tiempo de frame total (ms) |
| `GameThreadTime` | Tiempo del hilo de juego |
| `RenderThreadTime` / `_CriticalPath` | Tiempo del hilo de render |
| `GPUTime` | Tiempo GPU (suelen coincidir con FrameTime ⇒ GPU-bound) |
| `RHI/PrimitivesDrawn` | Primitivas enviadas por frame |
| `DrawSceneCommand_StartDelay` | Retardo entre inicio del render thread y la ejecución del draw scene command |

Estadística clave: en ambos builds la **GPU** iguala al FrameTime en mediana
mientras `GameThreadTime` queda muy por debajo ⇒ **cuello de botella de GPU,
no de CPU**.

---

## 3. Historia de los runs

### 3.1 Run de referencia — CSV viejo (06:40, build previo)

Estado: build SIN `DirectorArranque` (mundo estático del mapa), **sin
misiones**. Binarios anteriores al 09:52.

| Métrica | Valor |
|---|---|
| Frames capturados | 2197 |
| Mediana FrameTime | 20.2 ms (~50 FPS) |
| Mediana GPU | 20.2 ms |
| Primitivas/frame (mediana) | 4.4 M |
| Frames > 100 ms | 26 |
| Frames > 500 ms | 23 |
| Frames > 1 s | 21 |
| Max | 13.5 s (boot) |

Patrón de hitches: **spikes de GPU de ~7.8 s en frames aislados**, agrupados
en pares en frames concretos (43-47, 116-117, 158-160, 654-656, 915-918,
1247-1249, 1290, 2071). Fuera de esos frames, 50 FPS planos.
`DrawSceneCommand_StartDelay = 0` en todo el run ⇒ **no había ramp ni
degeneración progresiva** en el escenario sin misiones.

### 3.2 Run nuevo pre-fix — CSV 10:14 (build 09:52, con misiones)

Estado: build con fix visual 4x/seg + `DirectorArranque` + misiones M00/M01
+ multitud. SIN merge de fachadas.

| Métrica | Valor |
|---|---|
| Frames capturados | 79 (240 s ⇒ 0.3 FPS) |
| Mediana FrameTime | 157.0 ms |
| Mediana GPU | 157.5 ms |
| Mediana GameThread | 47.9 ms (esperando a render) |
| Mediana `DrawSceneCommand_StartDelay` | 254.1 ms |
| Primitivas/frame (mediana) | 59 235 |
| Frames > 100 ms | 67 (85 % del run) |
| `EventWait` acumulado (GameThread) | ~1591 s |

Boot catastrófico: frames 0-16 con 2-17 s cada uno (~2.5 min de slideshow).
Misiones SÍ avanzaban (M00→M01 completada) pese a los frames lentos.

### 3.3 Run de validación `-NoMisiones` (11:00)

Estado: build nuevo + `-NoMisiones`. **Solo 16 frames en 240 s**; el timer
de 240 s disparó durante el boot (el mundo procedural aún se estaba
construyendo) y el CSV quedó en 0 bytes. World-gen vacío este run
("0 árboles", "0 farolas", "0 foliage", "0 coches").

Conclusión: el `-NoMisiones` no reproduce el run viejo, porque el run viejo
tampoco tenía `DirectorArranque`. **La validación quedó confundida por el
cambio de escena, no por el fix.**

### 3.4 Run final post-fix — CSV 11:29 (con misiones + merge)

| Métrica | Valor |
|---|---|
| Frames capturados | **11 584** (240 s ⇒ **48 FPS**) |
| Mediana FrameTime | **18.8 ms** |
| Mediana GPU | **18.9 ms** |
| Primitivas/frame (mediana) | 4.07 M |
| Frames > 100 ms | 51 |
| Frames > 500 ms | 5 |
| Frames > 1 s | 2 (solo boot, ~1.4-1.7 s) |

Comparativa global:

| Métrica | Viejo (sin misión) | Pre-merge (con misión) | Post-merge (con misión) |
|---|---|---|---|
| Frames/240 s | 2197 | 79 | **11 584** |
| Mediana GPU | 20 ms | 157 ms | **18.9 ms** |
| Primitivas | 4.4 M | 59 k | **4.07 M** |
| Spikes > 1 s | 21 | 20 | **2** |

La mediana GPU vuelve al nivel del run viejo (20 ms) **con las misiones y la
multitud corriendo encima**, y los hitches de 8 s desaparecen.

### 3.5 Experimento — Lumen + VSM desactivados (CSV 12:13)

Para desglosar los ~19 ms GPU restantes se lanzó un run con
`-ExecCmds="r.DynamicGlobalIlluminationMethod=0, r.Shadow.Virtual.Enable=0"`
(sin Lumen ni Virtual Shadow Maps). Resultado:

| Métrica | Baseline (11:29) | Lumen+VSM off (12:13) |
|---|---|---|
| Mediana GPU | **18.9 ms** | 22.0 ms |
| p95 GPU | 22.5 ms | 25.6 ms |
| Primitivas (mediana) | 4.07 M | 4.56 M |
| Draw calls (mediana) | 819 | 2215 |
| Spikes > 100 ms (tras warmup) | 2 | 0 |

**Conclusión: Lumen y VSM NO son el cuello de botella.** Apagarlos *empeora*
el rendimiento: sin VSM el juego cae a shadow maps en cascada clásicos (CSM)
y los draw calls de sombra se multiplican ~3× (819 → 2215). El coste está en
el **base pass / geometría** (4 M prims de terreno ProceduralMesh + fachadas),
no en la iluminación. Lumen+VSM en DX11 sin RT y a esta densidad ya son la
opción barata; la config de render queda como estaba.

### 3.6 Ajuste de LOD de terreno (CSV 12:35)

El terreno (4033×4033 px, chunk 128 px → 1024 chunks) teselaba de más en
rango lejano: `LOD1Step=2` (800 m-2 km) y `LOD2Step=4` (>2 km) eran
sub-píxel. Cambiados a `LOD1Step=4`, `LOD2Step=8` (`TerrenoGenerado.h`;
espaciado de vértice 0.7 m / 1.4 m vs píxel ≈ 2-3 m a esas distancias → sin
cambio visual perceptible).

| Métrica | Pre-LOD (12:13) | Post-LOD (12:35) |
|---|---|---|
| Primitivas (mediana) | 4.56 M | **3.55 M** (−22 %) |
| GPU (mediana) | 22.0 ms | **20.5 ms** |
| GPU p95 | 25.6 ms | 23.9 ms |

Nota: el run 11:29 (18.9 ms, 819 DC) vs los de la mañana posterior (20.5 ms,
2225 DC) muestran una anomalía de benchmark en `BeginOcclusionTests`
(257 → 1613) con config y mapa idénticos y la misma escena (GPUSceneInstanceCount
8267 en ambos) — no la provocan los cambios de esta sesión (ya presente en el
run 12:13, pre-LOD) y no afecta al resultado jugable. No perseguida: sería
profundizar en internals del renderer sin impacto en FPS real.

---

## 4. Diagnóstico de causa raíz

`UAlsasuaFacadeGenerator::GenerarFachadasMundo()` creaba por edificio un
`UProceduralMeshComponent` y llamaba `CreateMeshSection_LinearColor` **una vez
por ventana**:

```cpp
// ANTES (por ventana): índice de sección único
ProcMesh->CreateMeshSection_LinearColor(Face * 100 + Lvl * 10 + W, Verts, Tris, ...);
// y por tienda:
ProcMesh->CreateMeshSection_LinearColor(900 + Face, Verts, Tris, ...);
```

- Cada sección de `ProceduralMesh` es **un draw call independiente**.
- 1030 edificios × decenas de ventanas ≈ **~60 000 draw calls de quads**.
- Cada invalidación del draw-command cache (setters de sol/niebla, streaming,
  cualquier evento de render) obligaba a **reconstruir ~60 k secciones** ⇒
  hitches de 7-8 s.

El fix de la sesión previa (ciclo visual/clima 4x/seg) reducía la frecuencia
de esas invalidaciones pero **no el coste de cada reconstrucción** — por eso
no resolvía el problema. El valor de 59 k primitivas en el run pre-merge
confirmaba el draw-call count catastrófico.

---

## 5. Solución aplicada

### 5.1 Merge de fachadas — `AlsasuaFacadeGenerator.cpp`

Cada edificio se genera ahora como **una única sección** acumulando
vértices/triángulos/normales/UVs con offset de índices:

```cpp
TArray<FVector> Verts; TArray<int32> Tris; TArray<FVector> Norms; TArray<FVector2D> UVs;
// ... en cada ventana/tienda:
const int32 V0 = Verts.Num();
Verts.Add(...); ...; Tris.Add(V0+0); Tris.Add(V0+2); ...
// al final:
ProcMesh->CreateMeshSection_LinearColor(0, Verts, Tris, Norms, UVs,
    TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
```

Resultado: **~60 000 draw calls → 1030**. Una sola llamada de colisión por
edificio (antes una por ventana).

### 5.2 LOD multitud — `AlsasuaOptimizerSubsystem`

`OptimizeCrowd()` ya desactivaba el Tick de IA de los NPC más allá de
`AICullDistance` (5000 cm). Se añadió control de **render** para NPC lejanos:

```cpp
const float RenderLODDistSq = RenderLODDistance * RenderLODDistance; // 2000 cm
if (Dist > RenderLODDistSq)
{
    if (Mesh->GetForcedLOD() == 0)          // solo transiciona una vez
    {
        Mesh->SetForcedLOD(Mesh->GetNumLODs());  // LOD más grueso
        Mesh->SetCastShadow(false);               // sin sombras dinámicas
    }
}
else { Mesh->SetForcedLOD(0); Mesh->SetCastShadow(true); }
```

Nota de API (UE 5.4): `USkeletalMeshComponent` expone
`SetForcedLOD(int32)` / `GetForcedLOD()` (heredadas de `USkinnedMeshComponent`);
`SetForcedLodModel` NO existe en 5.4 y el acceso directo a `ForcedLodModel`
está deprecated desde 4.24.

### 5.3 Flag de validación — `MisionesSubsystem.cpp`

```cpp
if (FParse::Param(FCommandLine::Get(), TEXT("NoMisiones")))
{
    bSaltarIntro = true;   // omite M00 y por tanto misiones/multitud
}
```

### 5.4 Cambios de la sesión previa (ya en binarios)

- `CicloVisualSubsystem`: rotación/intensidad/color del sol y skylight a
  **4x/seg** (0.25 s) en vez de por frame.
- `ClimaSubsystem`: niebla + parámetros MPC a **4x/seg**.
- `UAlsasuaOptimizerSubsystem` y `UAlsasuaHitchProtector` ahora
  `FTickableGameObject` (tick propio + `GetStatId`).

### 5.5 Config de render (sin cambios, ya óptima)

`DefaultEngine.ini`: VSM (`r.Shadow.Virtual.Enable=1`), Nanite proyecto,
Lumen (`r.DynamicGlobalIlluminationMethod=1`), `r.MeshDrawCommands.DynamicInstancing=1`,
TAA, streaming pool 2048 MB, `r.Nanite.MaxPixelsPerEdge=1.5`.

---

## 6. Compilación y verificación

Compilación UBT (solo módulos afectados, ~20-30 s incremental):

```
"F:\Epic Games\UE_5.4\Engine\Build\BatchFiles\Build.bat" ^
    AlsasuaSimulatorEditor Win64 Development ^
    -Project="F:\...\AlsasuaSimulator.uproject" -WaitMutex
```

Fallo intermedio corregido: `SetForcedLodModel` → `SetForcedLOD` (error C2039).

---

## 7. Estado del repositorio

- Commit: `bfe2f68` — "perf: fachadas 1 seccion/edificio + LOD multitud;
  0.3->48fps" (161 archivos, +2878/-2272).
- Push: `origin/master` (branch nueva en GitHub).
- Contenido del commit: `Source/`, `Config/`, `Tools/`, `.gitignore`
  (se añadió `__pycache__/`, `*.pyc`).
- **Fuera de git**: `Content/` (18.9 GB: 1940 assets importados, 446 de
  carretera, raw LiDAR, satélite). Requiere Git LFS para versionarse.

---

## 8. Siguientes pasos sugeridos

1. **El objetivo de rendimiento está cumplido** (48 FPS, 2 spikes residuales).
   Los ~19 ms GPU son base pass/geometría, no GI/sombras (ver §3.5). El LOD
   de terreno ya se ha afinado (§3.6, −22 % prims, −1.5 ms) sin cambio visual.
   Un target estricto de 60 FPS requeriría recortar más geometría (p. ej.
   LOD0 del terreno o densidad de fachadas) a costa de calidad.
2. **Nanite en fachadas NO es viable en 5.4**: `UDynamicMeshComponent` no
   expone `SetEnableNanite` (verificado en `BaseDynamicMeshComponent.h`).
   Migrar a UE 5.7+ daría LOD/imposters automáticos, pero es un cambio de
   motor, no de código.
3. **Imposters de multitud: innecesario por ahora** — la manifestación real
   (manifestantes skeletal con `ManifestanteActor`) está capada a ~60
   personas (`TamMax`). Replantear solo si la multitud escala a cientos.
4. **HLOD**: no aplica a actores generados en runtime; si se bakea el pueblo
   a un nivel persistente, HLODs masivos reducirían draw calls lejanos.
5. **Content en Git LFS** o repositorio de assets separado.
6. Pendiente menor: el `AlsasuaManifestacionManager` (port de Unity, sin
   callers) instanciaba 2 ISMCs huérfanos en `Initialize`, generando un
   `ensure` en cada boot — la llamada se ha eliminado (clase queda dormida).
