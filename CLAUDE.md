# CLAUDE.md

Guía para asistentes de IA que trabajen en este repositorio. Describe la
arquitectura, los flujos de trabajo y las convenciones que hay que respetar.

> **Estilo de respuesta**: `AGENTS.md` (Ponytail + Caveman) está activo y manda
> sobre el estilo. Lazy senior dev + salida terse. Léelo antes de escribir código.

---

## 1. Qué es esto

Simulador del pueblo de **Alsasua (Altsasu, Navarra)** en **Unreal Engine 5.8**,
portado desde un prototipo Unity. El pueblo **no está autorado a mano**: se genera
proceduralmente en runtime a partir de datos geográficos reales (LiDAR, catastro,
OSM, ortofoto PNOA del IGN) y encima corre una simulación social (misiones,
manifestaciones, multitudes, presión policial, economía).

Idioma del proyecto: **castellano** en nombres de clases, comentarios, docs y
mensajes de commit. Hay islas de inglés heredadas del port (`AlsasuaManifa`).
Mantén el idioma del fichero que tocas; no traduzcas código existente.

Documentos de referencia (léelos antes de tocar su área):

| Documento | Contenido |
|---|---|
| `README.md` | Estado actual, ortofoto PNOA, módulos, comandos |
| `Docs/PRIMER_COMPILADO_5_8.md` | Puesta en marcha paso a paso: nivel, HUD, landscape, materiales, navmesh |
| `RESUMEN_TECNICO.md` | Informe de optimización GPU (0.3 → 48 FPS); metodología de perfilado |
| `Docs/ADR-001_Terrain_V3_GPU_Driven_UE5.md` | Diseño de terreno GPU-driven — **no implementado**, es un ADR |
| `GASP_MotionMatching_GUIA.md` | Integración de Motion Matching (pasos de editor/contenido) |
| `Content/ModelosDescargados/LEEME.md` | Props CC0 de Poly Haven y su importación |
| `Content/Terreno/README_Landscape.md` | Datos del heightmap |

---

## 2. Compilar y ejecutar

**Motor: UE 5.8 exacto** (`EngineAssociation="5.8"` en el `.uproject`). C++20,
Unity build desactivado en los módulos grandes, LiveCoding deshabilitado.

```bat
:: Build (Windows, VS 2022 + workload C++)
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
    AlsasuaSimulatorEditor Win64 Development ^
    -Project="<ruta>\AlsasuaSimulator.uproject" -WaitMutex

:: Standalone (compila shaders la 1ª vez, ~1-5 min de boot)
UnrealEditor.exe "AlsasuaSimulator.uproject" /Game/Maps/L_Alsasua ^
    -game -DX11 -windowed -ResX=1280 -ResY=720 -NOSPLASH -log
```

- **Este repo no compila en Linux ni en CI**: es un proyecto UE de Windows sin
  workflows de GitHub. No hay forma de verificar un cambio de C++ compilando
  desde un contenedor. Si no puedes compilar, **dilo explícitamente** en vez de
  afirmar que el cambio está verificado.
- **El juego se autocierra a los 240 s.** `AAlsasuaGameplayGameMode::StartPlay()`
  lanza `CsvProfile start` y `Tick()` sale a los 240 s de wall time escribiendo
  `Saved/Profiling/CSV/Profile(<timestamp>).csv`. No es un bug; es el arnés de
  perfilado. Si añades una sesión larga, tenlo en cuenta.
- Flag `-NoMisiones`: omite misiones y multitud (benchmark del pueblo solo).
- Tests: `Source/AlsasuaManifa/Private/AlsasuaSubsystemTests.cpp`, automation
  tests bajo `Alsasua.*` (`WITH_AUTOMATION_WORKER`). Cobertura mínima y varios
  están *skipped* por requerir world vivo. No los uses como red de seguridad.

---

## 3. Arquitectura de módulos

8 módulos declarados en `AlsasuaSimulator.uproject` + los dos `.Target.cs`.

| Módulo | Rol | Cifras |
|---|---|---|
| `AlsasuaCore` | Geo/UTM 30N, event bus, spatial grid, helpers de carga de material | 4 cpp |
| `AlsasuaWorld` | Generación procedural: `DirectorArranque`, terreno, calles, edificios, POI, puentes | 22 cpp |
| `AlsasuaEntities` | Base de entidades: NPC, daño, vida | 6 cpp |
| `AlsasuaGameplay` | GameMode, misiones, manifestación, ciclo visual, clima, guardado, economía | 40 cpp |
| `AlsasuaManifa` | Port Unity: ~70 sistemas de mundo, GAS, IA, multitud, optimización, UI interna | **216 cpp** |
| `AlsasuaSimulator` | Generadores de edificios y carreteras | 11 cpp |
| `AlsasuaUI` | Widgets (HUD, pausa, ajustes, menú principal) | 6 cpp |
| `AlsasuaEditor` | Solo editor: generadores de materiales/mallas, importador de landscape | 21 cpp |

Grafo de dependencias:

```
AlsasuaCore ──► AlsasuaWorld ──► AlsasuaGameplay ──► AlsasuaUI
            └─► AlsasuaEntities ─┘                      │
            └─► AlsasuaManifa ◄──────────────────────────┘  (ciclo confesado)
AlsasuaEditor (Editor) ──► Core + World
```

### Reglas de capa (respétalas)

- Cada header empieza con `// Nombre.h (capa GAMEPLAY | UI | EDITOR)`. **Mantén
  ese comentario** al crear ficheros nuevos.
- La capa **Gameplay no puede referenciar UI**. Por eso el HUD se asigna por ini
  (`HUDClass=/Script/AlsasuaUI.AlsasuaHUD`) o por Blueprint, no en C++.
- Existe **un ciclo heredado**: `AlsasuaManifa` ↔ `AlsasuaUI`. Está confesado en
  `AlsasuaManifa.Build.cs` con `CircularlyReferencedDependentModules.Add("AlsasuaUI")`.
  **No añadas ciclos nuevos**; UBT 5.8 los marca como error.
- `AlsasuaManifa` compila con `bUseUnity = false` y `bEnableExceptions = true`
  (detecta includes que faltan). No lo cambies para "acelerar el build".

---

## 4. Sistemas de coordenadas — la fuente de bugs nº 1

Todo lo geográfico pasa por `AlsasuaCore/Public/GeoDataAlsasua.h`. Léelo entero
antes de tocar posiciones. Resumen:

| Espacio | Definición |
|---|---|
| **Local relativo** | Metros; hay que sumar `OX=1918.0`, `OZ=8570.0` para pasar a absoluto |
| **Local absoluto** | Metros; X=este, Z=norte |
| **UTM 30N (EPSG:32630)** | Centro Alsasua E=567951.0, N=4749902.0 |
| **UE5** | Centímetros; **X=este, Y=norte, Z=arriba**; 1 m = 100 uu |

**Qué JSON viene en qué espacio** (documentado en el header, y es fácil equivocarse):

- Relativo (necesitan `RelLocalToUE5`): `roads_unity.json`, `buildings_final.json`.
- **Absoluto** (usar `AbsLocalToUE5`): `trees_unity.json`, `signage_data.json`,
  `waterways_unity.json`, `greenspaces_unity.json`.
- **Mezclado**: `street_furniture.json` trae los dos. 191 de sus 220 piezas en
  relativo (papeleras, bancos, bolardos…) y 29 en absoluto (las 12 paradas de
  bus, las 5 fuentes, las señales, los cruces): lo escribieron dos generadores.
  Los dos grupos están separados por 4115 m de hueco en la coordenada norte, así
  que se distinguen sin ambigüedad. Usa `MobiliarioAUE5`, que lo decide por
  pieza; convertirlo todo como relativo manda esas 29 a 8,6 km del pueblo. Y no
  se distinguen por tipo: hay `papelera` en relativo y `papelera_reciclaje` en
  absoluto, así que un sistema que lea "las papeleras" toca los dos marcos.
  Los cuatro que leen ese fichero —`DetailDressing`, `Fountain`, `Farola`,
  `Container`— ya pasan por `MobiliarioAUE5`.

Usa siempre las funciones (`AbsLocalToUE5`, `RelLocalToUE5`, `LatLonToUE5`,
`UTMToUE5`, `UE5ToLatLon`), nunca aritmética a mano. El frame UTM↔UE5 usa el
origen LiDAR `(566033, 4741332)`, **distinto** de `OriginLocalX/Z` — están ahí
para centrado local, no para UTM.

**`UnityaUnreal` es la que muerde.** Recibe `(este, arriba, norte)` y devuelve
`(este_cm, norte_cm, arriba_cm)` — la vertical va **en medio**, no al final.
Escribir `UnityaUnreal(FVector(X, Z, 0))` con `Z` = norte compila, no avisa, y
mete la coordenada norte en el eje vertical: la pieza acaba sobre la línea
norte=0 y flotando a la altura de su propia coordenada norte, que en este pueblo
son más de 800 m en el aire. Le pasó a seis sistemas a la vez. Para un par
(este, norte) usa `AbsLocalToUE5(FVector(X, 0, Z))` o `RelLocalToUE5`, que no
tienen ese hueco; `UnityaUnreal` sólo cuando el dato ya trae la vertical en
medio, como los `pts` planos `[x,y,z,...]`. `Tools/VerificarFuentes.py` lo caza;
si tu caso es de los legítimos, márcalo con `// ejes ok`.

Y la cota tampoco sale del conversor. `AbsLocalToUE5` y `RelLocalToUE5` propagan
el **segundo** componente del vector de entrada a la Z de salida, así que con el
patrón habitual `(X, 0, Z)` la Z sale en cero — que es cota cero del mundo, 531 m
por debajo del pueblo. Hay que apoyarla con `AlturaSueloUE5(World, X, Y)` o usar
`RelLocalASueloUE5`. Un `Pos.Z += 300` sobre ese cero deja la pieza medio
kilómetro bajo tierra.

Cota de referencia: Herriko Plaza a 531.94 m → `CotaPlazaCm = 53194`.

### Ortofoto PNOA

Dos texturas georreferenciadas al mismo grid, con **el sur arriba** (v=0 en Ymin;
verificado empíricamente contra el cauce del Arakil). Ver tabla de bounds en
`README.md`. Regeneración: `Tools/DescargarOrtofotoPNOA.py` (WMS
`OI.OrthoimageCoverage`; `OI.MosaicElement` está roto) → `Tools/ImportSatellite.py`
en la consola Python del editor.

---

## 5. Arranque del mundo

`ADirectorArranque` (`AlsasuaWorld`, 626 líneas) es el orquestador: `BeginPlay()`
→ `IniciarConstruccion()`, que construye el pueblo en **~52 fases numeradas y
comentadas**, en orden de dependencia, publicando progreso en
`ArranqueMundo::Progreso` (lo lee `AlsasuaHUD` para la barra de carga):

1. `ATerrenoGenerado` — heightmap real `Content/Terreno/alsasua_landscape_4033.r16`
   (4033², procedural mesh; en `-game` no se usa Landscape).
1b. `UCargadorPoligonos` — 5 plazas + 273 zonas verdes como superficies drapeadas.
1c. `ATerrenoLejano` — anillo de relieve de 60×60 km con el hueco del terreno
   jugable, para que el mundo no se corte en seco a 3,6 km (ver §5b).
2. `UCargadorArboles` (LiDAR) → `UCargadorVias` (ferrocarril, aceras, caminos;
   también genera los ríos como cintas drapeadas) → `UCargadorCalles` →
   `UCargadorEdificios` → tejado modular → Herriko Plaza → `UCargadorPuentes` →
   `UCargadorPOI` → vegetación.
3. Fases 13-51: sistemas de `AlsasuaManifa/World/*` (atmósfera, post-process por
   zonas, estilos de barrio, fachadas, farolas, señales, tráfico, aceras,
   marcas viales, semáforos, cables aéreos, clima, audio…).
46. `UAlsasuaTrafficLightSystem` — semáforos con ciclo, detrás de
   `ADirectorArranque::bSemaforos`. La fase estuvo saltada con un log de "skip
   para perfilado", así que el sistema no corría nunca; para volver a medir sin
   ellos se baja la bandera, no se comenta la fase.
51b. `ATunelAlsasua` — las diez bocas de los cinco túneles.
52. `UAlsasuaFerrocarrilSystem` — material rodante en la playa de vías.

Las dos últimas van ahí **a propósito**: un tren y un marco de hormigón tienen
colisión, y colocados antes, cualquier sistema que se apoye por raycast y pase
por la estación o por una boca se subiría al techo de un vagón o al dintel (ver
el aviso de más abajo).

Si añades una fase, ponla **donde toque en la cadena** y mantén la numeración y
el `Progreso`. El terreno va siempre primero: el resto hace raycast contra él
para apoyarse (`MuestreadorAltura`, `GeoDataAlsasua::TraceUp/TraceDown`).

**Los datasets de `UCargadorVias` no tienen todos la misma forma de raíz.** Cuatro
son un array; `railways_unity.json` es un objeto `{"rails", "stations"}` porque
además de los 86 trazados lleva los dos apeaderos. `Encolar` acepta las dos formas
y recibe el nombre del campo cuando hace falta. Deserializar a `TArray` contra una
raíz de objeto devuelve `false` sin más, así que la vía férrea entera —38,7 km—
estuvo perdiéndose en silencio mientras el director registraba "Vías férreas
cargadas". `Tools/VerificarVias.py` replica el parseo y canta si un dataset deja
de producir trazados.

**Cuidado con el orden y los raycast de altura.** Varios cargadores muestrean Z
con `LineTraceSingleByChannel(ECC_Visibility)` **sin filtrar por actor** (lo hacen
`APoligonoSuelo` y `UCargadorArboles`): cogen lo más alto que haya debajo. Por eso
`UCargadorPoligonos` va en la fase **1b**, pegado al terreno y antes de árboles,
calles y edificios — detrás de los árboles, un vértice de zona verde bajo una copa
drapearía a la altura de la copa. Si añades algo que se apoye por raycast,
colócalo antes de lo que pueda taparle el suelo.

Coste: `UCargadorPoligonos` son 278 actores, uno por polígono, cada uno con su
sección de `ProceduralMesh` → 278 draw calls sobre los ~819 de referencia del
`RESUMEN_TECNICO.md`. Y pinta color plano sobre la ortofoto PNOA del terreno; si
hace falta ver la foto debajo, se tiñe la ortofoto por zona en el material, no se
sube el epsilon.

### 5b. Relieve lejano (`ATerrenoLejano`)

El terreno jugable son 7200×7200 m y acaba en seco. `ATerrenoLejano` dibuja
detrás un cuadrado de 60×60 km con un **agujero del tamaño exacto** del terreno,
así que no se solapan: al oeste Aizkorri (1543 m a 14,8 km), al este San
Donato/Andía (1410 m a 15,1 km), al norte Aralar, al sur Urbasa/Lokiz.

- **Datos**: `Tools/DescargarRelieveLejano.py` baja el MDT25 y el PNOA del IGN
  (verificado por round-trip, corr 1.000) → `alsasua_relieve_lejano_2048.r16` +
  `_meta.json` + `_4096.png`. La caja y la codificación viven en el meta, no en
  el C++. El `.r16` se versiona; **el PNG de 29 MB no**, porque el presupuesto de
  Git LFS de la cuenta está agotado y el push lo rechaza — hay que ejecutar el
  script una vez tras clonar (~80 s). Sin él, el anillo se ve con el color
  procedural y `ImportSatellite.py` avisa en vez de fallar.
- **Cota**: el mundo usa `worldZ_cm = alt_m*100 − 51133`, no metros sobre el mar.
  El `.r16` del anillo guarda altitud real, así que hay que restar esa base
  (`CotaBaseCm`); sin ella el relieve flota 511 m. La codificación del heightmap
  principal (`495 + q/64`) **no vale aquí**: topa en 1518,98 m y Aizkorri pasa de
  1520, así que el anillo usa datum y paso propios.
- **Costura**: las alturas se funden del borde del terreno jugable a las del MDT
  a lo largo de `BandaFusionM`, con proyección radial en Chebyshev, para que en
  el borde exacto ambas superficies coincidan y no quede escalón.
- **Es decorado**: sin colisión (si la tuviera, los `LineTrace` de altura de
  árboles y suelos se engancharían a él) y sin sombras dinámicas. Una sola
  sección de malla = un draw call para los 60 km.

### Datos (`Content/Datos/*.json`)

25 ficheros con el pueblo real: `buildings_final.json`, `building_facades.json`
(7 MB), `roads_unity.json`, `trees_unity.json`, `street_furniture.json`,
`landmarks_real.json`, `poi_data.json`, `signage_data.json`, `nighborhoods.json`
(sic, sin la `e`), `asset_manifest.json`, `asset_mapping.json`… Se versionan y
son la fuente de verdad del mundo. **No los regeneres a la ligera** — cambiarlos
mueve geometría ya validada contra fotos de referencia
(`Content/Datos/referencia_fotos/`).

---

## 6. Assets y materiales

### Degradación elegante, siempre

El proyecto tiene que arrancar **sin** los assets pesados (Megascans/Fab son
decenas de GB fuera de git). Dos patrones a respetar:

- `AlsasuaCore/Public/CargarMaterialComun.h` — `MaterialesAAADisponibles()`
  comprueba 5 paquetes "gate" antes de usar la librería `Content/Road`. Sin ellos
  un MI carga pero compila a Default Material gris, así que se cae a
  `M_Terreno_Calles`/`M_Terreno_Acera`. Usa
  `CargarMaterialConFallbackSeguro(AAA, Propia, Final)` para material nuevo.
- `AlsasuaManifa/Public/World/AlsasuaMallaFab.h` — `Resolver(Tipo, FormaBasica)`
  escoge malla por orden de calidad: Fab/Epic → `/Game/Mobiliario` propio →
  forma básica del motor. `VieneDeFab()` decide si hay que reescalar;
  `LimpiarCache()` tras importar sin reiniciar el editor.

Rutas blandas que pueden faltar sin romper nada: `/Game/VFX/NS_Lluvia`,
`/Game/Audio/SC_Trueno`, camas de ambiente. Mantén esa propiedad.

### Materiales

Se generan **por grafo de nodos** desde `AlsasuaEditor` (`UMaterialEditingLibrary`),
no hay shaders `.usf/.ush` custom — por eso el breaking change de Substrate en 5.8
(`ComputeFinalGBuffer`) no afecta.

Los 29 materiales sueltos que una docena de sistemas cargaba por ruta —`M_Madera`,
`M_Piedra`, `M_Metal_Guardia`, `M_Toldo`, los `M_Asphalt_*` por barrio…— y que no
creaba nadie los genera `UCreadorMaterialesSimples`, dentro de `CrearMaterialesPBR`.
No rompían nada, porque quien los carga comprueba el null y sigue con el material
de la malla; simplemente esas funciones visuales no se veían nunca, y
`AuditarAssets.py` los daba por buenos porque la carpeta sí la genera el proyecto.
Ese hueco ahora sale en su propia sección del informe.

Orden obligatorio: `CrearMaterialEdificio()` crea el **`MPC_Clima`** (escalares
`Wetness`, `Night`) del que dependen todos los demás. `UClimaSubsystem` conduce
esos parámetros en runtime. Crear un material que lea el MPC antes de que exista
= material gris.

---

## 7. Pipeline de Python (`Tools/`)

54 scripts. Dos familias:

- **34 con `import unreal`** — se ejecutan **dentro del editor** (Output Log →
  consola Python, o Tools → Execute Python Script). `Tools/RunAll.py` es el
  maestro: ejecuta todo el setup en orden de dependencias (assets/materiales →
  nivel → capas visuales → VFX → foliage de Fab), aislando cada paso y sacando
  un resumen de lo que falló. Deriva rutas de `unreal.Paths.project_dir()`.
- **20 standalone** (`DescargarOrtofotoPNOA.py`, `PrepararLandscape.py`,
  `DownloadTextures.py`, `enrich_*.py`, `asset_manifest.py`…) — Python 3 normal,
  algunos con `numpy`/`requests`.

Los que empiezan por `Verificar`/`Auditar` no montan nada: contrastan lo que hay
contra su fuente y sacan un informe. Sirven de red donde no hay compilador —
`VerificarVias.py` (formas de raíz de los datasets de vía y sitio para el material
rodante), `VerificarDatasets.py` (campos que el C++ pide y el JSON no tiene, marcos
mezclados, elementos fuera del terreno), `AuditarSistemas.py` (sistemas de mundo
que no llama nadie, y con cuál chocarían; también los `UActorComponent` que no
adjunta nadie, que son 23 y antes se daban por buenos suponiendo que ya los
adjuntaría su actor),
`VerificarFuentes.py` (el `;` que se lleva un comentario, delimitadores
descuadrados, `UnityaUnreal` con los ejes cambiados, CVars `g.*` que nadie
registra, `.generated.h` ausente en cabecera reflejada, rutas
`/Engine/EngineMeshes/` que no existen, y conversor de coordenadas metido en
línea donde se espera una posición de mundo — ahí ya no hay dónde apoyar la
cota), `VerificarCallesNavarra.py` (trazado contra el eje catastral),
`AuditarAssets.py` (rutas sin respaldo y mallas bajadas que nadie pide),
`VerificarGuardado.py` (campos del save que se guardan y no se cargan),
`VerificarDialogos.py` (los árboles de Content/Dialogs entran enteros),
`AlturasLidarEdificios.py --verificar`, `DescargarCatastroNavarra.py --verificar`.

Sin compilador en Linux, `VerificarFuentes.py` y `VerificarDatasets.py` son lo
único que hay entre un error tonto y `main`. Pásalos antes de subir C++.

**Regla de rutas**: ningún script puede llevar rutas absolutas de una máquina
concreta. Los standalone derivan la raíz de su propia ubicación
(`os.path.dirname(os.path.dirname(os.path.abspath(__file__)))`, están en `Tools/`)
y los de editor usan `unreal.Paths.project_dir()`. Los 11 ficheros que quedaban
con `F:\Epic Games\UE_5.7\altsasu_gtavii\...` ya están migrados; si añades uno,
sigue el patrón. Única excepción legítima: rutas a instalaciones externas al repo
(CitySample, Blender), que van como parámetro con valor por defecto —
ver `SetupAlsasuaProject.ps1`.

---

## 8. Rendimiento: reglas que no se negocian

`RESUMEN_TECNICO.md` documenta cómo se pasó de 0.3 a 48 FPS. Las lecciones que
hay que conservar al escribir código nuevo:

0. **Nada de un actor por pieza cuando las piezas se cuentan por miles.** Va
   antes que el resto porque es el error más fácil de cometer y el más caro.
   `UAlsasuaFoliagePainter` estaba escrito con un `AStaticMeshActor` por mata:
   con las 273 zonas verdes salían decenas de miles de actores. Ahora siembra en
   `UHierarchicalInstancedStaticMeshComponent`, uno por tipo de planta — doce mil
   instancias en ocho draw calls, con culling y LOD de serie. Lo mismo
   `UAlsasuaSidewalkSystem`, que ponía un actor por losa de acera: 5038 losas,
   5038 draw calls, y encima dos `LoadObject` de material por losa dentro del
   bucle. Ahora son dos capas instanciadas, una por acabado. Y
   `UAlsasuaAwningShutterSystem`, el peor de todos: 17537 persianas a actor por
   pieza, apiladas además en el centroide del edificio — un bloque de 7,7 m con
   132 ventanas se llevaba una columna de 398 m atravesando el tejado. Ahora se
   reparten por el perímetro y por planta, instanciadas. La otra vía válida
   es coser la geometría en una sola sección de `ProceduralMesh`, que es lo que
   hace `AlsasuaVegetationSpawner` con el césped procedural.

   Van ya convertidos `FoliagePainter`, `SidewalkSystem`, `AwningShutterSystem`,
   `GuardrailSystem`, `RooftopDetailSystem`, `DoorEntranceSystem`,
   `ParkingSystem`, `ContainerSystem`, `StreetArtSystem` y
   `PaintedStreetSignSystem`. Ojo con el patrón que los delataba a
   todos: `LoadObject` de la malla o del material **dentro** del bucle de
   colocación. Si lo ves, casi seguro que también hay un `SpawnActor` al lado.
   Los que quedan con `SpawnActor<AStaticMeshActor>` colocan decenas de piezas,
   no miles, y ahí la regla no aplica.
1. **Una sección de `ProceduralMesh` = un draw call.** Las fachadas creaban una
   sección por ventana (~60 000 draw calls y hitches de 8 s al invalidar el
   draw-command cache). Ahora se acumulan verts/tris/normales/UVs con offset de
   índices y se emite **una sección por edificio** (1030). Nunca crees secciones
   en bucle sobre elementos pequeños.
2. **Setters de render limitados a 4x/seg.** `CicloVisualSubsystem` y
   `ClimaSubsystem` aplican rotación/intensidad/color de sol, skylight, niebla y
   parámetros del MPC cada 0.25 s, no por frame — cada llamada invalida el
   draw-cache de la escena. Si añades algo que toque render, respétalo.
3. **LOD de multitud**: `UAlsasuaOptimizerSubsystem` apaga el tick de IA más allá
   de `AICullDistance = 5000` cm y fuerza el LOD más grueso + sin sombras
   dinámicas más allá de `RenderLODDistance = 2000` cm, transicionando **una sola
   vez** (guarda con `GetForcedLOD() == 0`).
4. **Lumen y VSM no son el cuello de botella** — apagarlos empeora el resultado
   (sin VSM, los CSM triplican los draw calls de sombra: 819 → 2215). El coste
   está en base pass/geometría. No "optimices" desactivándolos.
5. **LOD de terreno**: `TerrenoGenerado.h` con `LOD1Step = 4`, `LOD2Step = 16`
   (valores ya afinados; bajarlos vuelve a teselar sub-píxel).
6. **Nanite en fachadas no es viable** (verificado en 5.8):
   `UDynamicMeshComponent` no expone `SetEnableNanite`. Requeriría bake a
   StaticMesh.
7. `UAlsasuaHitchProtector` y `UAlsasuaOptimizerSubsystem` son
   `FTickableGameObject` (necesitan `GetStatId()`).

Para medir: lanza standalone (el CSV sale solo), y compara medianas de
`FrameTime` / `GPUTime` / `RHI/PrimitivesDrawn` frente a los números del
`RESUMEN_TECNICO.md`. La primera fila del CSV trae los nombres de columna; las
posiciones cambian entre runs.

---

## 9. Convenciones de código

- **C++20**, prefijos UE estándar (`A` actor, `U` UObject, `F` struct, `E` enum,
  `I` interfaz). Macros de export por módulo: `ALSASUACORE_API`,
  `ALSASUAWORLD_API`, `ALSASUAMANIFA_API`, …
- **Nombres en castellano** para lo nuevo (`CargadorEdificios`, `MisionesSubsystem`,
  `TerrenoGenerado`, `AvanzarObjetivo`). `AlsasuaManifa` conserva inglés del port.
- Header con comentario de cabecera: nombre, capa y una o dos frases de qué hace y
  de qué es puerto. Sigue ese formato.
- Subsistemas: `UGameInstanceSubsystem` / `UWorldSubsystem`. Los que necesitan
  tick implementan `FTickableGameObject` con `RETURN_QUICK_DECLARE_CYCLE_STAT` e
  `IsTickable() { return !IsTemplate(); }`.
- API expuesta a Blueprint con `UFUNCTION(BlueprintCallable, Category="X")`;
  delegados con `DECLARE_DYNAMIC_MULTICAST_DELEGATE*` y `UPROPERTY(BlueprintAssignable)`.
- Punteros a UObject **siempre** bajo `UPROPERTY()`, aunque sean privados.
- Logs: `UE_LOG(LogTemp, Log, TEXT("Clase: mensaje..."))` con el nombre de la
  clase como prefijo.
- Comentarios: explican **por qué**, no qué. El código existente comenta las
  decisiones raras (fallbacks, ciclos de módulo, flips de UV). Mantén ese nivel.
- Indentación mixta (4 espacios en la mayoría, tabs en algunos ficheros de
  `AlsasuaGameplay`). **Copia la del fichero que tocas.**

---

## 10. Git y contenido

- Se versionan: `Source/`, `Config/`, `Tools/`, `Docs/`, `Content/Datos/`,
  `Content/Textures/*.png`, `Content/Terreno/` (heightmap + metadatos),
  `Content/ModelosDescargados/` (glTF CC0), `Content/Dialogs/`.
- **Fuera de git** (`.gitignore`): `Binaries/`, `Intermediate/`, `Saved/`,
  `DerivedDataCache/`, `Content/Megascans|Fab|GASP|Road|Materiales|AssetsImportados/`,
  `Content/Maps/*.umap`, LiDAR raw, y los **uassets derivados de textura**
  (reimportables con `Tools/ImportSatellite.py`). El `Content/` completo pesa
  ~19 GB.
- **Git LFS** para las dos ortofotos de 8192² (ver `.gitattributes`).
- Mensajes de commit: **castellano**. Dos estilos conviven — convencional
  (`fix:`, `perf:`, `feat(gis):`) y descriptivo en imperativo
  ("Mapear Village y ForestPack, y enchufarlos donde había primitivas"). Ambos
  valen; explica el *qué* y el *por qué*, no el fichero.
- Sin CI: no hay `.github/workflows`. La verificación es manual, en Windows.
- `.opencode/` contiene el vendoring del plugin Ponytail (agente de OpenCode) y
  `graphify-out/` es salida de herramienta. **No los toques** salvo petición
  explícita.

---

## 11. Trampas conocidas

- **La forma de la raíz de los JSON de `Content/Datos/` no es uniforme, y
  equivocarse no duele.** Unos son un array en la raíz (`roads_unity.json`,
  `trees_unity.json`, `street_furniture.json`, `greenspaces_unity.json`) y otros
  envuelven el array en un objeto (`railways_unity.json` → `rails`,
  `poi_data.json` → `pois`, `nighborhoods.json` → `barrios`). Deserializar a
  `TArray` contra una raíz de objeto —o a `FJsonObject` contra una de array—
  devuelve `false` y ya está: ni excepción ni línea en el log. El sistema hace
  `return` y el arranque sigue. Así estuvieron muertos a la vez la vía férrea,
  las superficies de calle, los coches aparcados, las señales de tráfico, los
  árboles con especie, las farolas, el foliage de zonas verdes y las calles y
  ríos del minimapa. **Usa siempre `JsonDatos::CargarArray`**
  (`AlsasuaCore/Public/CargarJsonComun.h`), que se traga las dos formas y avisa
  cuando no encuentra nada. `Tools/VerificarDatasets.py` compara lo que pide el
  C++ con lo que hay en el dato.
- **Antes de revivir un sistema, mira si su trabajo ya lo hace otro.** Varias
  fases de `AlsasuaManifa/World/*` duplican lo que ya construyó un cargador de
  `AlsasuaWorld` sobre el mismo JSON, y mientras estuvieron rotas no se notó.
  `AlsasuaTreePlacer` (fase 23) replantaba los 2783 árboles que `UCargadorArboles`
  (fase 2) ya siembra en HISM, uno por actor y encima de los primeros;
  `AlsasuaRoadSurfaceSystem` (fase 26) ponía un cubo aplastado sobre cada cinta
  de `UCargadorCalles` (fase 4). Los dos siguen ahí, pero el primero sólo aporta
  su ficha botánica y el segundo publica el firme por id de vía para que lo
  aplique el director sobre las cintas que ya existen.
- **Hay datos que caen fuera del mundo y colocarlos no falla.** 31 de las 126
  señales de `signage_data.json` traen coordenadas de hasta 216 km, y 280 de los
  2783 árboles del LiDAR quedan fuera de los 7200 m del terreno jugable. El actor
  se crea igual, su trazo de suelo no encuentra nada y acaba en cota cero o en la
  de la plaza. `VerificarDatasets.py` los cuenta; quien los lea debe filtrarlos
  **diciendo cuántos**, no en silencio. El filtro es
  `UAlsasuaGeoData::DentroDelTerreno`; la caja no se copia a mano en cada
  sistema, y `VerificarDatasets.py` la lee del propio header para no medir
  contra un terreno que ya no existe.
- **Lo que va en una pared necesita la pared, y nadie la tenía.** Tres sistemas
  colgaban cosas de una fachada colocándolas en el centroide del barrio o en el
  eje de la calzada, con el giro sorteado: los 23 murales y grafitis de
  `AlsasuaStreetArtSystem` (los dos de Herriko en el mismo punto exacto, uno
  dentro del otro), las placas de calle de `AlsasuaPaintedStreetSignSystem` y los
  escaparates de `AlsasuaShopFrontSystem`. La pared está en
  `AlsasuaManifa/Public/World/AlsasuaMuros.h`: los ~6000 tramos del perímetro de
  los 1030 footprints, con largo y normal saliente, calculados una vez. La normal
  se decide contra el centroide del footprint porque `buildings_final.json` **no
  garantiza el sentido de giro** del polígono; sin eso, la mitad de lo que se
  cuelgue queda pintado por dentro del muro. Y la fachada que da a la calle —la
  misma para puerta, portal, garaje y escaparate— la elige
  `AlsasuaDirecciones::LadoDeEntrada`, que es donde vive el punto de calle de OSM.
- **Escalar un `Plane` en Z no lo pone de pie.** `/Engine/BasicShapes/Plane` es un
  plano en XY que mira hacia arriba; `SetActorScale3D(Ancho, 0.05, Alto)` da una
  tira tumbada en el suelo, no un cartel. Le pasaba a los murales, a los grafitis
  y a las placas de calle a la vez, y no salta a la vista en el código porque la
  escala *parece* la de un cartel. Para algo vertical, cubo con el grueso en el
  eje que mira afuera. Y ojo con ese eje: si el yaw mira **afuera** de la fachada
  (`LadoDeEntrada`, `AlsasuaMuros`), el grueso va en X y el ancho en Y; si el yaw
  va **a lo largo** del muro (`AwningShutterSystem`), al revés. Las 1030 puertas
  iban con la escala del otro convenio: 10 cm de ancho y un metro de fondo,
  clavadas de canto en el muro.
- **Dos ficheros traen lat/lon Y x/z, y no dicen lo mismo.** `landmarks_real.json`
  (19) y `poi_data.json` (30 de sus 78) llevan las dos cosas. Medidas entre
  elementos, las distancias por x/z salen **diez veces más pequeñas** que por
  coordenada geográfica, y la z va **al revés**: el factor es −10, no +10.
  Colocados por x/z, los 19 landmarks quedan apiñados en 121×134 m y a 120 m de
  mediana del edificio más cercano, sin que ninguno caiga sobre uno; por lat/lon
  la mediana baja a 29 m y seis caen justo encima de su footprint —la iglesia, el
  ayuntamiento, la biblioteca, el mercado—. **Cuando el elemento trae lat/lon,
  mandan ellas** (`LatLonToUE5`); los 47 POI que no la traen se quedan con su
  x/z, que además sí alcanzan edificios, o sea que vienen de otra pasada.
  `VerificarDatasets.py` compara los dos marcos y saca el factor real.
- **Hay datos cuyas coordenadas no sirven, y tampoco lo dicen.** Distinto de caer
  fuera del mundo: los 56 `señal_comercio` de `signage_data.json` traen 40
  amontonados en diez metros alrededor de `(1891.5, 8572.0)` —que es
  `OriginLocalX/Z`, la constante de centrado, no una dirección— a 126 m del
  edificio más cercano, y los otros 16 a hasta 216 km. Están dentro de rango, no
  los caza `DentroDelTerreno`, y colocarlos deja las tiendas en corro en mitad de
  un prado. Lo aprovechable es el contenido —nombre, tipo de negocio, barrio—, y
  eso es lo que se usa: la tienda va a una fachada de su barrio, elegida por FNV
  del nombre para que no se mueva entre arranques. Antes de fiarte de una
  coordenada, mira la dispersión del conjunto, no un elemento.
- **Un `LoadObject` dentro del bucle de colocación es el olor del actor por
  pieza.** Los ocho sistemas convertidos a instanciado lo tenían todos, y en
  varios el asset ni siquiera existía: `AlsasuaContainerSystem` pedía la
  papelera a un pack que no está en el repo, sin fallback — cien
  actores sin malla, invisibles, ocupando su sitio en la lista de fases y en el
  log. Lo mismo las farolas y los doce semáforos, de los que sólo quedaba la
  luz puntual flotando a 3,25 m. `AuditarAssets.py` lo caza ahora en su sección
  "pack externo sin degradación": ruta de pack externo + `SetStaticMesh` y ni
  `AlsasuaMallaFab::Resolver` ni `CargarMaterialConFallback` en el fichero. Malla nueva, por `AlsasuaMallaFab::Resolver`; material nuevo, por
  `CargarMaterialConFallbackSeguro`; los dos, **fuera** del bucle.
- **Colocar por `FMath::FRand`/`RandRange` hace el pueblo irrepetible.** No es
  un fallo visible, pero rompe el arnés de perfilado: si la geometría cambia en
  cada arranque, comparar el CSV de hoy con los números del `RESUMEN_TECNICO.md`
  no mide nada. Lo nuevo va con `FRandomStream` sembrado por el id del elemento
  (`Id * 2654435761u + <sal>`), que además deja razonar sobre lo que se ve. Si
  el elemento no tiene id —una tienda, un mural— se siembra por FNV del nombre o
  por el índice, nunca por `GetTypeHash`, cuyo valor no está garantizado entre
  compilaciones.

  De los 22 ficheros de `AlsasuaManifa/World/*` que quedaban con el patrón
  antiguo, **13 no los llama nadie** (`EnvironmentalDecals`, `DecalSystem`,
  `RoadDecalSystem`, `GroundCoverSystem`, `FacadeDetailSystem`…: están en la
  lista de `AuditarSistemas.py`, así que su aleatoriedad no llega al mundo). De
  los vivos, los que colocan geometría fija ya van sembrados; los que quedan son
  de tiempo y parpadeo —`StreetLightController`, `InteriorLightComponent`,
  `WeatherSystem`, `AmbientAudioSystem`— y ahí el azar por frame es lo que se
  quiere. `DetailDressingSystem` conserva `RandRange` sólo en sus cinco
  funciones de respaldo, que corren si falta `street_furniture.json`.
- **Los datasets son heterogéneos entre elementos.** `street_furniture.json`
  tiene 220 piezas y no todas traen los mismos campos: las fuentes llevan
  `nombre` y `activa`, las paradas `linea` y `con_techo`, y la mayoría ninguno de
  los dos. Antes de dar por ausente un campo, míralo en **todo** el fichero, no
  en los primeros elementos.
- `RESUMEN_TECNICO.md` es el **acta de una sesión que corrió en 5.4**, con un
  aviso al principio que lo dice. Sus medidas y sus rutas de motor son las de
  aquel día; el diagnóstico y las reglas de §5 siguen vigentes. Para compilar y
  lanzar hoy, `README.md` o este fichero. (El `.uproject` decía 5.4 en
  `Description`; corregido a 5.8, que es lo que fija `EngineAssociation`.)
- **Los túneles son bocas, no galería.** `ATunelAlsasua` levanta los diez
  portales de los cinco túneles de `tunnels_unity.json` (los dos ferroviarios,
  el de la N-1, el de la A-10 y el del Plazaola). No agujerea el terreno:
  `ATerrenoGenerado` es una malla procedural de 4033² afinada al detalle y
  recortarle un hueco es una operación de terreno, así que **el túnel no se
  atraviesa**. Cavar la galería sin poder entrar sería geometría enterrada
  pagando draw calls. Antes `UCargadorVias` los encolaba y los descartaba,
  contándolos como construidos.
- `UImportadorLandscape` **aborta** si el nivel es World Partition
  (`IsPartitionedWorld()`) en vez de crear un landscape roto. Para el primer
  arranque, usa un nivel **Empty**, o importa a mano por Landscape Mode con los
  valores de la tabla de `PRIMER_COMPILADO_5_8.md` (§4).
- Enhanced Input se construye **en runtime** si no hay assets `IA_*`/`IMC`
  asignados; si los autoras en el editor, tienen prioridad.
- `CreadorMaterialAgua.cpp` fija `BlendMode`/`TwoSided` **por reflexión**
  (`FindPropertyByName`) para compilar independientemente de la visibilidad de
  esos campos en 5.8. No lo "simplifiques" a acceso directo.
- `CreateMeshSection` (overload C++ con `FColor`) sigue siendo válida en 5.8; lo
  deprecado es el nodo de Blueprint. La ruta de migración, si llega, es
  `CreateMeshSection_LinearColor`.
- `AlsasuaManifestacionManager` (port Unity sin callers) está **dormido** a
  propósito: se le quitó la instanciación de dos ISMCs huérfanos que disparaban
  un `ensure` en cada boot.
- `Docs/ADR-001_*` describe un terreno GPU-driven **que no existe**. No lo cites
  como si estuviera implementado.
