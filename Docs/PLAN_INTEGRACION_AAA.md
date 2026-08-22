# Plan de integración AAA — Alsasua Simulator

Estado medido el 2026-08-22 sobre `ue5-clean-integration` @ `fafc08a`.
No es un plan de deseos: cada fase parte de algo comprobado en el repo y termina
en un criterio que se puede verificar sin creerse el log.

**Regla de orden**: la limpieza va la última, como pediste. Pero la *red de
seguridad* va la primera, porque ahora mismo no existe y sin ella ninguna de las
fases siguientes se puede dar por buena.

> **§2 ordena por dependencia. §4 ordena por importancia — empieza por ahí.**
> Y lee P0 antes que nada: el mapa no está en este clon (por `.gitignore`, es
> deliberado), y el refactor de los 21 plugins no se ha ejecutado nunca.

---

## 0. Lo que hay de verdad

### 0.1 Tamaño real

| | ficheros | LOC | auditado |
|---|---:|---:|---|
| `Source/` (9 módulos) | 139 cpp | 24 171 | sí |
| `Plugins/GF_*` (21 plugins) | 456 cpp+h | 36 158 | **no** |
| total | | **60 329** | 40 % |

Los 126 k LOC que salen con un `find` ingenuo sobre `Plugins/` son
`Intermediate/`: código generado por UHT. El código escrito son 36 k.

### 0.2 Densidad por plugin — quién es sistema y quién es cascarón

LOC por fichero. Por debajo de ~60 es declaración sin cuerpo.

| plugin | fich | LOC | LOC/f | lectura |
|---|---:|---:|---:|---|
| GF_Vegetacion | 11 | 2002 | 182 | implementado |
| GF_AI | 26 | 4296 | 165 | implementado |
| GF_Edificios | 24 | 3232 | 135 | implementado |
| GF_Trafico | 18 | 2355 | 131 | implementado |
| GF_Clima | 12 | 1505 | 125 | implementado |
| GF_Carreteras | 22 | 2176 | 99 | implementado |
| GF_World | 71 | 6810 | 96 | implementado |
| GF_Ferrocarril | 6 | 525 | 88 | implementado |
| GF_NPCs | 14 | 1117 | 80 | implementado |
| GF_Audio | 18 | 1346 | 75 | implementado |
| GF_Optimization | 11 | 681 | 62 | mixto |
| GF_Vehiculos | 25 | 1448 | 58 | **cascarón** |
| GF_Debug | 11 | 631 | 57 | **cascarón** |
| GF_UI | 17 | 957 | 56 | **cascarón** |
| GF_Systems | 51 | 2827 | 55 | **cascarón** |
| GF_Social | 26 | 1311 | 50 | **cascarón** |
| GF_Core | 26 | 1130 | 43 | **cascarón** |
| GF_Politica | 15 | 599 | 40 | **cascarón** |
| GF_Dialogos | 17 | 671 | 39 | **cascarón** |
| GF_Misiones | 7 | 230 | 33 | **cascarón** |
| GF_Abilities | 14 | 309 | 22 | **cascarón** |

Siete de los ocho cascarones son exactamente los siete sistemas de gameplay que
faltan. La extracción movió la fachada; el cuerpo nunca existió.

### 0.3 Grafo de módulos — está bien

Comprobado sobre los 30 `.Build.cs`. Es un DAG limpio, sin ciclos:

```
AlsasuaKernel ─┐
AlsasuaCore ───┼─► GF_{Clima,Carreteras,Edificios,Ferrocarril,Misiones,
               │      Optimization,Politica,Trafico,Vegetacion,Vehiculos}
               │        └─► GF_{Social,NPCs,Audio,Dialogos,UI,Abilities,
               │                AI,World,Systems,Core,Debug}
               ├─► AlsasuaWorld ──► AlsasuaGameplay ──► AlsasuaUI
               └─► AlsasuaEntities ─┘
AlsasuaManifa ◄─ (todavía dependencia de World, Gameplay y UI)
```

El ciclo `AlsasuaManifa ↔ AlsasuaUI` que confesaba `CLAUDE.md` **ya no está**.
Las dependencias de editor (`UnrealEd`, `MaterialEditor`) en `AlsasuaWorld` van
guardadas tras `if (Target.bBuildEditor)` — correcto, el target Game compila.

Esto es lo mejor del refactor y hay que no romperlo: cualquier fase que añada un
`PublicDependencyModuleNames` nuevo entre plugins pasa por revisar el DAG.

---

## 1. Hallazgos que condicionan el plan

### H1 — La red de seguridad está ciega ⟨bloqueante⟩

`Tools/AuditarSistemas.py:27` y `Tools/VerificarFuentes.py:64` hacen
`FUENTE = os.path.join(RAIZ, "Source")`. Los 36 k LOC de `Plugins/` no los mira
nadie.

Consecuencia medida ahora mismo:

- `AuditarSistemas.py` imprime **"Ninguno"** en las cuatro secciones
  (duplicados, pasivos, arrancan solos, inertes). No porque esté limpio: porque
  todos los sistemas que auditaba se mudaron a `Plugins/`.
- `VerificarFuentes.py` saca **52 hallazgos**, de los que ~45 son el mismo falso
  positivo: `#include "World/AlsasuaAtmosphereController.h" y esa cabecera no
  está en Source/`. Está en `Plugins/GF_Clima/`. El verificador no lo sabe.
- `.github/workflows/verificar.yml` corre esos scripts en cada PR. **CI está
  verde y no significa nada.**

CLAUDE.md §7 dice literalmente: *"Sin compilador en Linux, `VerificarFuentes.py`
y `VerificarDatasets.py` son lo único que hay entre un error tonto y `main`."*
Ese "único" está desconectado desde el refactor.

### H2 — Los plugins `GF_` no son Game Features

`GF_Trafico.uplugin` (y los 20 restantes) no tienen `GameFeatureData`, no
declaran `"ExplicitlyLoaded": true`, y sus módulos cargan en `PreDefault` /
`Default`. Son plugins de código normales, enlazados estáticamente. El prefijo
`GF_` promete algo que no hace:

- no hay activación/desactivación en runtime,
- no hay exclusión por plugin en el cook,
- desactivar un subsistema exige recompilar,
- `GameFeatures` y `ModularGameplay` están habilitados en el `.uproject` sin que
  nadie los use.

No es un bug — funciona — pero es una mentira de nombre que va a costar cara
cuando alguien intente "desactivar GF_Politica para el benchmark".

### H3 — El port en inglés nunca se fusionó con la capa en castellano

El patrón es siempre el mismo: la versión **en castellano** vive en
`AlsasuaGameplay`, está cableada al `.ini` o al director, y es corta; la versión
**en inglés** vive en un plugin, tiene la lógica, y no la llama nadie.

| concepto | cableado (castellano) | con la lógica (inglés) | tercero |
|---|---|---|---|
| clima | `UClimaSubsystem` — Gameplay, 74 L, conduce `MPC_Clima` | `AlsasuaWeatherSystem` — GF_Clima, 119 L | `UClimateSubsystem` 33 L + `UWeatherSubsystem` 41 L, ambos GF_Clima |
| diálogos | `UDialogoSubsystem` — Gameplay, 90 L | `UDialogSubsystem` — GF_Dialogos, 16 L | — |
| misiones | `UMisionesSubsystem` — Gameplay, 94 L | `UMisionManager` — GF_Misiones, 45 L | — |
| tráfico | `UTraficoSubsystem` — Gameplay, 36 L | `AlsasuaTrafficSystem` — GF_Trafico, 73 L | `AlsasuaDynamicTrafficSystem` 81 L |
| audio ambiente | `UAudioAmbienteSubsystem` — Gameplay, 39 L | `AlsasuaAmbientAudioSystem` — GF_Audio, 98 L | `AlsasuaAudioManager` 29 L |
| interacción | — | `IInteractuable` — GF_AI, 19 L | `InteractableInterface` — GF_AI, 25 L (**mismo plugin**) |
| guardado | `UAlsasuaLegacySaveGame` — Gameplay, 73 L | `UAlsasuaSaveGame` — Manifa, 43 L | `UDEPRECATED_AlsasuaSaveGame` |
| vegetación | `AlsasuaVegetationSpawner` — World, 28 L | `UVegetationSpawnerSubsystem` — GF_Vegetacion, 122 L | `TreeSpawnerComponent` + `AlsasuaTreePlacer` |
| depuración | — | `UAlsasuaDebugVisualizer` — GF_Debug | `AlsasuaDebugComponent`, `AlsasuaDebugHUD`, `DebugManager` |

Cuatro sistemas de clima es el caso extremo. `MPC_Clima` sólo tiene un conductor
legítimo (`UClimaSubsystem`, CLAUDE.md §6); los otros tres o no corren o pelean
por los mismos escalares.

### H4 — Cuatro jerarquías de personaje y ningún guardia civil en el mundo

| clase | módulo | LOC | qué trae | ¿vivo? |
|---|---|---:|---|---|
| `AAlsasuaCharacter` | Manifa | 181 | GAS + `IDamageable` + trayectoria Motion Matching | **sí** — `DefaultPawnClass` en `DefaultEngine.ini:8` |
| `AAlsasuaPlayerCharacter` | Entities | 101 | Enhanced Input propio, `IDamageable`, **sin GAS** | no — duplicado completo del anterior |
| `AAlsasuaEntitiesCharacter` | Entities | 16 | cámara y nada más | no — esqueleto |
| `ANPCCharacter` | Entities | 16 | `Morale` | base de GuardiaCivil |
| `AGuardiaCivil` | Entities | **19** | `PatrolRadius` | no — **nadie lo referencia fuera de su módulo, nadie lo spawnea** |
| `ANPCGuardCharacter` | Manifa | 48 | GAS, `SuspicionLevel`, aggro, `TryDeescalate()` | no — sin llamantes |
| `AOperativeCharacter` | GF_Politica | — | agente del deep state | no |
| `APoliciaActor` + `APoliciaController` | Gameplay | — | — | no |
| `AAlsasuaPoliciaController` | Gameplay | — | — | no |
| `AAlsasuaPoliceVan` + `UPoliceScannerComponent` | GF_Vehiculos | — | — | no |
| `URefuerzosSubsystem` | Gameplay | — | refuerzos | no |

Once piezas de "fuerza del orden". Cero puntos de spawn. La lógica de guardia que
pides (`SuspicionLevel`, deescalación, GAS) existe en `ANPCGuardCharacter`; el
nombre que pides existe en `AGuardiaCivil`; y no se tocan.

De las 312 clases exportadas del proyecto, **199 no aparecen en ningún fichero
fuera de su propio módulo**. Parte son legítimas (GameModes por `.ini`,
subsistemas que arranca el motor), pero es el orden de magnitud del problema.

### H5 — World Partition activado sobre un mundo generado en runtime

`DefaultEngine.ini:86-93` y `:129` activan WP con `CellSize=25600`,
`LoadingRange=128000`. Pero:

- `Content/Maps/` está **vacío** — los `.umap` están en `.gitignore`. No hay
  nivel que particionar hasta que corra `Tools/SetupLevel.py`.
- `ADirectorArranque` spawnea el pueblo entero en `BeginPlay()`. Un actor creado
  en runtime **no pertenece a ninguna celda de WP**: queda siempre cargado. WP no
  ahorra ni un byte de lo que genera el director.
- CLAUDE.md §11: `UImportadorLandscape` **aborta** si `IsPartitionedWorld()`.
  Activar WP rompe el importador de landscape.

Hoy WP es coste de configuración con beneficio cero y un riesgo concreto.

### H6 — `ADirectorArranque` es el único punto de integración, y es un god-object

757 líneas, ~52 fases, 40 `#include` de cabeceras de plugins. Todo lo que quieras
que corra entra ahí o no corre. Los sistemas "nuevos" del último commit (biomas,
tráfico dinámico, megasión, sabotaje) hay que comprobar uno a uno si llegaron a
la cadena.

### H7 — Sin línea base de memoria

`r.Streaming.PoolSize=2048` fijo, sin perfil por tier. `RESUMEN_TECNICO.md` es el
acta de una sesión en **5.4** y sólo mide FPS y draw calls, no memoria. No hay un
`stat memory` / `LLM` de referencia contra el que comparar nada.

---

## 2. Fases

Cada fase tiene **criterio de salida verificable**. No se pasa a la siguiente sin
él. Las fases 1-3 son secuenciales; 4-6 pueden solaparse.

---

### FASE 0 — Devolver la vista a los verificadores ⟨bloqueante⟩

Sin esto, todo lo demás se hace a ciegas y "compila" es la única señal.

1. `Tools/VerificarFuentes.py:64` y `Tools/AuditarSistemas.py:27`: sustituir
   `FUENTE = Source` por una lista de raíces `[Source/, Plugins/*/Source/]`,
   **excluyendo `Intermediate/` y `Binaries/`**. Los `.generated.h` de plugins
   viven en `Plugins/<X>/Intermediate/Build/.../UHT/`; la resolución de includes
   tiene que aceptarlos igual que acepta los de `Source/`.
2. Mismo cambio en `AuditarAssets.py`, `VerificarGuardado.py`,
   `VerificarDatasets.py` (este último busca el C++ que consume cada JSON — y los
   consumidores ya están en plugins).
3. Volver a pasar los seis. Triar los hallazgos reales de los falsos positivos
   que queden. Los ~45 includes "no está en Source/" deben desaparecer solos.
4. Añadir un verificador nuevo, `Tools/VerificarModulos.py`: lee los 30
   `.Build.cs`, construye el DAG, y falla si hay ciclo o si un módulo `Runtime`
   depende de uno `Editor` fuera de un `bBuildEditor`. Es el invariante que más
   caro sale de romper y ahora no lo comprueba nadie.
5. `.github/workflows/verificar.yml`: añadir el paso nuevo.

**Salida**: los seis verificadores corren sobre 60 k LOC en vez de 24 k, y el
informe de `AuditarSistemas.py` deja de decir "Ninguno" en las cuatro secciones.
Guardar ese informe como `Docs/baseline_sistemas_2026-08.txt` — es el inventario
contra el que se mide todo lo demás.

---

### FASE 1 — Inventario y línea base medida

No se arregla lo que no se ha medido. Dos entregables, ninguno de código.

1. **Inventario de vivos vs muertos**. Del informe de la fase 0, clasificar las
   199 clases sin referencia externa en tres cubos:
   - *arranca solo* — `UWorldSubsystem` con `Initialize`/`Tick` sobreescrito, o
     clase puesta por `.ini`. Corre aunque nadie la nombre. **Hay que mirar qué
     hace**, no darla por dormida (CLAUDE.md §11 ya se comió este error).
   - *pasivo* — componente o actor que alguien adjunta/coloca. Comprobar el
     llamante, y sobre qué lista itera (el error de `GetAllActorsOfClass`).
   - *inerte de verdad* — candidato de la fase 8, no de ahora.
2. **Línea base de rendimiento en 5.8**. `RESUMEN_TECNICO.md` es de 5.4 y no
   sirve de referencia. Lanzar standalone (el CSV sale solo a los 240 s) y
   registrar medianas de `FrameTime`, `GPUTime`, `RHI/PrimitivesDrawn`,
   `RHI/DrawPrimitiveCalls`. Y añadir memoria, que hoy no se mide: `stat memory`,
   `LLM`, `r.Streaming.PoolSize` real usado, número de instancias HISM. Guardar
   como `Docs/baseline_5_8.md`.

**Salida**: `Docs/baseline_sistemas_2026-08.txt` + `Docs/baseline_5_8.md`
commiteados. Cualquier "mejora" posterior se justifica contra estos números.

---

### FASE 2 — Fusionar los duplicados (H3)

**Regla única**: *se queda el que está cableado; se le mete dentro la lógica del
otro; el otro se borra en la fase 8, no ahora.* Marcar el perdedor con
`UE_DEPRECATED` para que nadie lo use mientras tanto.

Orden, de más barato a más caro:

| # | fusión | superviviente | absorbe | riesgo |
|---|---|---|---|---|
| 2.1 | interacción | `IInteractuable` (ya usado por GF_AI) | `InteractableInterface` | bajo — mismo plugin |
| 2.2 | audio | `UAudioAmbienteSubsystem` | `AlsasuaAmbientAudioSystem` (98 L de zonas) + `AlsasuaAudioManager` | bajo |
| 2.3 | vegetación | `UVegetationSpawnerSubsystem` (122 L, HISM) | `AlsasuaVegetationSpawner` (28 L) — ojo, este es el que llama el director | medio — cambia fase del director |
| 2.4 | diálogos | `UDialogoSubsystem` (90 L, cableado) | `UDialogSubsystem` (16 L) + tipos de GF_Dialogos | bajo |
| 2.5 | guardado | `UAlsasuaSaveGame` | `UAlsasuaLegacySaveGame` + `UDEPRECATED_*` | **alto** — pasar `VerificarGuardado.py` antes y después; un campo que se guarda y no se carga es pérdida de partida |
| 2.6 | tráfico | `AlsasuaDynamicTrafficSystem` (el nuevo, con StreetGraph + AIDriver) | `UTraficoSubsystem` (36 L) + `AlsasuaTrafficSystem` (73 L) | medio |
| 2.7 | misiones | `UMisionesSubsystem` (94 L, cableado) | `UMisionManager` (45 L) | medio — ver fase 4 |
| 2.8 | clima | `UClimaSubsystem` (**único conductor legítimo de `MPC_Clima`**) | `AlsasuaWeatherSystem` (119 L, la lógica); borrar `UClimateSubsystem` y `UWeatherSubsystem` | **alto** — CLAUDE.md §8.2: los setters de render van a 4 Hz, no por frame. Si `AlsasuaWeatherSystem` los toca por frame, se corrige al fusionar |
| 2.9 | depuración | `UAlsasuaDebugVisualizer` (GF_Debug, el nuevo) | `AlsasuaDebugComponent`, `AlsasuaDebugHUD`, `DebugManager` | bajo |

**Salida**: nueve conceptos con una implementación cada uno. El informe de
`AuditarSistemas.py` (sección "RIESGO DE DUPLICADO") deja de listarlos.
`VerificarGuardado.py` limpio.

---

### FASE 3 — Vertical slice: jugador + guardia civil (H4)

Es lo que pediste explícitamente y además es el mejor test de integración que
hay: si un guardia persigue al jugador por el pueblo generado, están vivos el
terreno, el navmesh, la IA, GAS, el HUD y el audio a la vez.

#### 3.1 Colapsar la jerarquía de personaje

```
ACharacter
 └─ AAlsasuaCharacterBase      (nuevo, en AlsasuaEntities — GAS + IDamageable + locomoción)
     ├─ AAlsasuaCharacter      (jugador — hoy en Manifa; mover a Entities)
     └─ AAlsasuaNPCBase
         ├─ AGuardiaCivil      (absorbe ANPCGuardCharacter)
         └─ AManifestante / APeaton
```

- La base sale de `AAlsasuaCharacter` (181 L, la única con GAS y trayectoria).
- `AAlsasuaPlayerCharacter` (101 L) y `AAlsasuaEntitiesCharacter` (16 L) se marcan
  deprecados. **No borrar aún** — fase 8.
- `DefaultEngine.ini:8` sigue apuntando al jugador; si cambia de módulo, cambia la
  ruta `/Script/<Módulo>.<Clase>`. Es el fallo más fácil de la fase: el `.ini` no
  avisa, el juego arranca con el pawn por defecto de UE y parece que "no pasa
  nada".

#### 3.2 Guardia civil de verdad

`AGuardiaCivil` (19 L) absorbe `ANPCGuardCharacter` (48 L): GAS,
`SuspicionLevel`, `IsAggro()`, `TryDeescalate()`, `ReduceAggression()` — esa
última es la que engancha el megáfono y el disfraz (`MegaphoneTool`,
`DisguiseComponent` en GF_Social).

Piezas a conectar, todas ya existen:

| pieza | dónde | papel |
|---|---|---|
| `AAlsasuaPoliciaController` | Gameplay | controller — patrulla, persecución |
| `URefuerzosSubsystem` | Gameplay | llamar refuerzos al subir la sospecha |
| `AAlsasuaPoliceVan` | GF_Vehiculos | vehículo de refuerzo |
| `UPoliceScannerComponent` | GF_Vehiculos | detección desde el vehículo |
| `USecurityRadarComponent` | GF_Systems | detección de amenaza (nuevo) |
| `UDetentionMinigameComponent` | Manifa | detención |
| `UFactionSubsystem` | GF_Politica | ya nombra `GuardiaCivil` |
| `AOperativeCharacter` | GF_Politica | **decidir**: ¿guardia civil de paisano o clase aparte? Si es lo primero, fusionar |

#### 3.3 Punto de spawn — lo que falta de verdad

Ningún sitio del código spawnea un guardia. Hay que añadir una fase al director.
Colocación: los puntos de `poi_data.json` y las patrullas sobre el grafo de calles
de `UAlsasuaStreetGraph` (ya existe, se construye de `roads_unity.json`).

Restricciones de CLAUDE.md que aplican **literalmente** aquí:

- **Coordenadas**: si el POI trae `lat/lon`, mandan ellas (`LatLonToUE5`), no el
  `x/z` — el factor entre los dos marcos es **−10**, no +10 (§11).
- **Cota**: `AbsLocalToUE5(X,0,Z)` deja la Z en cero, que son 531 m bajo el
  pueblo. Apoyar con `AlturaSueloUE5(World, X, Y)`.
- **Filtrado**: pasar por `UAlsasuaGeoData::DentroDelTerreno` y **decir cuántos se
  descartan**, no en silencio.
- **Determinismo**: `FRandomStream` sembrado por id del POI
  (`Id * 2654435761u + sal`), nunca `FMath::RandRange` — si no, el CSV de
  perfilado deja de ser comparable (§11).
- **Orden en el director**: después del navmesh y después de los túneles/tren
  (fases 51b/52), o una patrulla que se apoye por raycast se sube al techo de un
  vagón.
- **Coste**: una docena de guardias son actores normales. Aquí *no* aplica la
  regla del actor-por-pieza (§8.0) — esa es para miles.

#### 3.4 Navegación

`DefaultEngine.ini:136`: `bGenerateNavigationOnlyAroundNavigationInvokers=True`.
El jugador necesita un `UNavigationInvokerComponent` o los guardias no tienen
navmesh donde caminar. Comprobar que `AAlsasuaCharacter` lo lleva.

**Salida**: en standalone, un guardia civil patrulla, detecta al jugador, sube
`SuspicionLevel`, llama refuerzos y termina en el minijuego de detención. Sin
`ensure`, sin actores en cota cero, y el CSV de 240 s sale igual dos arranques
seguidos (determinismo).

---

### FASE 4 — Llenar los cascarones (§0.2)

Ahora sí, en orden de impacto sobre el bucle de juego. Cada uno con la lógica que
ya está fusionada en la fase 2, no partiendo de cero.

| orden | plugin | LOC/f hoy | qué falta de verdad |
|---|---|---:|---|
| 4.1 | **GF_Misiones** | 33 | grafo de misión, objetivos, recompensas, ramificación. Enganchar a `UMisionesSubsystem` (superviviente de 2.7) y a `UConsecuenciasSubsystem` y `UProgresionSubsystem`, que ya existen en Gameplay y tampoco los llama nadie |
| 4.2 | **GF_Dialogos** | 39 | `Content/Dialogs/` ya tiene árboles y `VerificarDialogos.py` los valida. Falta el runtime: nodos, condiciones, `DialogMemoryComponent`. Es el sistema con más infraestructura hecha y menos código |
| 4.3 | **GF_UI** | 56 | HUD real. Hoy `AlsasuaHUD` sólo pinta la barra de carga de `ArranqueMundo::Progreso`. Faltan minimapa (`AlsasuaMinimapWidget` está incluido y no existe), vida, sospecha, objetivo activo. **Ojo capa**: Gameplay no puede referenciar UI (CLAUDE.md §3) — se sigue asignando por `.ini` |
| 4.4 | **GF_Abilities** | 22 | 14 ficheros, 309 líneas: el cascarón más vacío del proyecto. Las abilities de infiltración (`Lockpick`, `Rally`, `Sprint`) siguen en Manifa y sus includes están rotos (`Abilities/AlsasuaGameplayAbility.h` no resuelve). Arreglar los includes es el primer paso, no escribir abilities nuevas |
| 4.5 | **GF_Vehiculos** | 58 | físicas, daño, cámara. `AAlsasuaPoliceVan` lo necesita la fase 3 |
| 4.6 | **GF_Politica** | 40 | influencia, traiciones, `StateOfAlarm`. Contenido de endgame — después de que el bucle base funcione |
| 4.7 | **GF_Social** | 50 | redes sociales, trending, fachada de reputación |
| 4.8 | **GF_Systems** | 55 | 51 ficheros y 2827 líneas. Auditar antes de escribir: parte son los sistemas nuevos (megasión, sabotaje) y parte cascarón heredado |
| 4.9 | **GF_Core** | 43 | procesador async, budget, chain reaction, replay. Infraestructura — sólo si algo de arriba la necesita. YAGNI si no |

**Salida por plugin**: LOC/fichero > 80 y al menos un llamante fuera del propio
plugin, verificado por `AuditarSistemas.py` (que ya ve `Plugins/` desde la fase 0).

---

### FASE 5 — Mejora gráfica

La configuración de `DefaultEngine.ini` ya es AAA de partida (Lumen + Nanite +
VSM + niebla volumétrica + nubes volumétricas, DX12/SM6 obligatorio). El techo no
está en los flags, está en la geometría y en los materiales.

**Lo que NO hay que hacer** (§8.4, medido): apagar Lumen o VSM. Sin VSM los CSM
triplican los draw calls de sombra (819 → 2215). El coste está en base pass.

Por impacto real:

1. **Cerrar los `SpawnActor` que quedan**. 111 `SpawnActor` repartidos por 16
   módulos. Los ocho sistemas grandes ya están convertidos a HISM (§8.0), pero
   `GF_Edificios` (15), `GF_Vegetacion` (13), `GF_Trafico` (9) y `GF_Carreteras`
   (6) hay que revisarlos uno a uno. El olor sigue siendo el mismo: **97
   `LoadObject` en `Plugins/*/Private`** — si alguno está dentro de un bucle de
   colocación, hay un actor-por-pieza al lado.
2. **Fachadas a Nanite**. §8.6 dice que no es viable porque
   `UDynamicMeshComponent` no expone `SetEnableNanite`. La salida es *bake* a
   `StaticMesh` en el editor (`AlsasuaEditor` ya tiene los generadores). 1030
   edificios horneados = Nanite real sobre la geometría más pesada del pueblo. Es
   la mejora gráfica de mayor retorno del proyecto.
3. **Ortofoto PNOA por zonas**. `UCargadorPoligonos` pinta 278 actores de color
   plano encima de la ortofoto (278 draw calls, §5). Teñir la ortofoto por zona
   **en el material** en vez de superponer geometría: −278 draw calls y se ve la
   foto debajo.
4. **Materiales que faltan**. `UCreadorMaterialesSimples` genera los 29 materiales
   sueltos (§6). Confirmar con `AuditarAssets.py` — sección "pack externo sin
   degradación" — que ningún sistema nuevo carga una malla de un pack ausente sin
   `AlsasuaMallaFab::Resolver` de respaldo.
5. **Anillo lejano**. `alsasua_relieve_lejano_4096.png` (29 MB) no se versiona
   (LFS agotado). Sin él el anillo se ve con color procedural. Correr
   `Tools/DescargarRelieveLejano.py` una vez (~80 s) — paso de puesta a punto, no
   bug.
6. **Perfiles gráficos**. `UAlsasuaGraphicsSettingsSubsystem` ya sale de
   `PerfilArranque` en `DefaultGame.ini` y sólo se aplica en `EWorldType::Game`.
   Añadir escalabilidad por tier de GPU en vez de un único perfil.

**Salida**: mediana de `RHI/DrawPrimitiveCalls` por debajo de la línea base de la
fase 1, con Lumen y VSM encendidos. Ninguna regresión de `FrameTime`.

---

### FASE 6 — Balanceo de memoria

Hoy no hay ni una medida. Primero medir (fase 1), luego:

1. **Pool de streaming por tier**. `r.Streaming.PoolSize=2048` fijo. Con la
   ortofoto de 8192² × 2 + Megascans, 2 GB es optimista en una GPU de 8 GB.
   Perfilar con `stat streaming` y derivar el pool del VRAM detectado.
2. **Presupuesto por sistema**. Contar instancias HISM: vegetación (12 000),
   aceras (5038 losas), persianas (17 537), árboles (2783). Cada HISM guarda una
   transform por instancia — 17 537 persianas son ~1,7 MB sólo de transforms, más
   el buffer de culling. Fijar un techo por sistema y que el sistema lo respete,
   no que se descubra en el crash.
3. **Descarga de datos JSON**. 25 ficheros en `Content/Datos/`,
   `building_facades.json` pesa 7 MB. Se parsean en el arranque y se quedan en
   memoria si alguien guarda el `FJsonObject`. Comprobar que los cargadores
   sueltan el JSON al terminar su fase.
4. **`bForceGCAfterLevelStreamedOut=True`** ya está puesto. Sin WP real (H5) no
   hace nada.

**Salida**: `Docs/presupuesto_memoria.md` con techo por sistema, y una sesión de
240 s cuyo pico de memoria quepa en el presupuesto declarado.

---

### FASE 7 — Decisiones de arquitectura pendientes

Dos que no se pueden aplazar hasta la limpieza porque condicionan lo anterior.

**7.1 — ¿Game Features de verdad, o renombrar?** (H2)

- *Opción A*: convertir a Game Feature real **sólo los cuatro plugins de
  contenido opcional** — `GF_Misiones`, `GF_Politica`, `GF_Social`, `GF_Debug`.
  Añadirles `GameFeatureData`, `"ExplicitlyLoaded": true` y las acciones. Los
  constructores de mundo (`GF_Edificios`, `GF_Carreteras`, `GF_Vegetacion`,
  `GF_Trafico`, `GF_Clima`, `GF_World`) se quedan estáticos — el director los
  necesita en `BeginPlay` y no ganan nada activándose tarde.
- *Opción B*: renombrar los 21 a `AlsasuaX` y quitar `GameFeatures` +
  `ModularGameplay` del `.uproject`. Menos código, cero promesas incumplidas.

Coste A ≈ 1 día por plugin. Coste B ≈ medio día total. **B si no vas a usar
activación en runtime; A si quieres poder cocinar sin la capa política.**

**7.2 — ¿World Partition o no?** (H5)

- *Opción A — quitarlo*: `bEnableWorldPartition=False`. El mundo es procedural en
  runtime, WP no gestiona lo que genera el director, y su presencia rompe
  `UImportadorLandscape`. Recuperas el importador y no pierdes nada medible.
- *Opción B — usarlo de verdad*: hornear el pueblo generado a actores estáticos en
  el editor (`AlsasuaEditor` ya tiene los generadores), meterlos en celdas, y que
  el director sólo construya lo dinámico. Es la ruta AAA de verdad — y el único
  camino que hace compatible Nanite en fachadas (fase 5.2) con streaming. **Es un
  proyecto en sí mismo, no un ajuste de `.ini`.**

Recomendación: **A ahora, B como fase 9** si el pueblo horneado llega a existir.
Mantener WP encendido sin nivel ni bake es lo peor de las dos.

---

### FASE 8 — Limpieza ⟨sólo con todo lo anterior verde⟩

Tal como pediste: al final. La condición de entrada es estricta — **los seis
verificadores en verde, la línea base de la fase 1 batida, y la vertical slice de
la fase 3 corriendo 240 s sin `ensure`.** Borrar antes de eso es borrar a ciegas.

1. **Clases duplicadas de la fase 2** — los nueve perdedores, ya deprecados y sin
   llamantes. Verificar con `AuditarSistemas.py` que no aparecen.
2. **Jerarquía de personaje muerta** — `AAlsasuaPlayerCharacter` (101 L),
   `AAlsasuaEntitiesCharacter` (16 L), `ANPCGuardCharacter` (absorbido en 3.2).
3. **Inertes de verdad** — el tercer cubo de la fase 1. CLAUDE.md ya nombra 13
   ficheros de `AlsasuaManifa/World/*` sin llamantes (`EnvironmentalDecals`,
   `DecalSystem`, `RoadDecalSystem`, `GroundCoverSystem`, `FacadeDetailSystem`…).
   Reconfirmar con el auditor ya arreglado antes de tocarlos.
4. **`AlsasuaManifa` restante** — 21 cpp, 2903 LOC. Si tras las fases 2-4 no queda
   nada propio, el módulo desaparece y con él la última dependencia cruzada del
   DAG. Es el cierre natural del refactor.
5. **`AlsasuaSimulator`** — 10 cpp, 471 LOC, **9 de sus clases sin referencia
   externa** (`AAlsasuaBuildingGenerator`, `AAlsasuaRoadManager`,
   `UAlsasuaChatManager`, `UAlsasuaWhisperManager`, `UAlsasuaVisualGlitch`,
   `UAlsasuaParanoiaComponent`…). Su trabajo lo hacen `GF_Edificios` y
   `GF_Carreteras`. Candidato completo a desaparecer.
6. **Docs**: la raíz tiene ocho `.md` de estado (`MEJORAS_IMPLEMENTADAS.md`,
   `README_MEJORAS.md`, `PLAN_MEJORA_REALISMO.md`,
   `PLAN_ARQUITECTURA_MICROSERVICIOS.md`, `QUICK_START_VEGETACION.md`,
   `INSTRUCCIONES_VEGETACION.md`, `EJECUTAR_AHORA.txt`…) que se contradicen entre
   sí y con `CLAUDE.md`. Consolidar en `README.md` + `Docs/` y borrar el resto.
7. **CVars muertos** — `VerificarFuentes.py` ya caza uno:
   `AlsasuaSettingsWidget.cpp` escribe `g.CameraShakeIntensity` y no la registra
   nadie; ese ajuste no hace nada. Barrer los que salgan con el verificador ya
   arreglado.
8. **`Content/`** — `AuditarAssets.py` lista mallas descargadas que nadie pide.
   Sobre 19 GB de contenido, es donde está el peso muerto real.

**Salida**: `AuditarSistemas.py` con la sección "INERTES" vacía **de verdad** (no
por ceguera), DAG de módulos sin `AlsasuaManifa` ni `AlsasuaSimulator`, y una sola
versión de cada concepto.

---

## 3. Riesgos

| riesgo | dónde muerde | mitigación |
|---|---|---|
| No hay CI que compile | el job `compilar` de `verificar.yml` está tras la variable `RUNNER_UE58`, que no existe. **Verde no significa que compile** | montar el runner self-hosted, o compilar a mano tras cada fase |
| Ruta `/Script/` en `.ini` | mover una clase de módulo rompe `DefaultEngine.ini:8-10` en silencio: el juego arranca con el pawn por defecto | grep de `/Script/` en `Config/` en cada movimiento de clase; añadirlo a `VerificarFuentes.py` |
| Ejes de `UnityaUnreal` | seis sistemas se lo comieron a la vez; la vertical va **en medio** | `VerificarFuentes.py` ya lo caza — pero sólo desde la fase 0, cuando vea `Plugins/` |
| Cota cero | `AbsLocalToUE5(X,0,Z)` deja Z=0 = 531 m bajo el pueblo | `AlturaSueloUE5` siempre; nunca `Pos.Z += N` sobre cero |
| Raíz de JSON | `TArray` contra raíz de objeto devuelve `false` sin log | `JsonDatos::CargarArray` siempre |
| Aleatoriedad | rompe la comparabilidad del CSV de perfilado | `FRandomStream` sembrado por id |
| Regresión de draw calls | cada fusión puede meter un `SpawnActor` en bucle | CSV de 240 s tras cada fase, comparado con `Docs/baseline_5_8.md` |

---

## 4. Orden por importancia

Reordenado por **coste de no hacerlo ahora**, no por dependencia. Donde el orden
por importancia choca con el orden por dependencia, manda la dependencia y se
indica.

### P0 — El refactor no se ha ejecutado nunca  ⟨0,5 día⟩

El mapa **sí existe** — en el proyecto original, no en este clon:

```
F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Content\Maps\L_Alsasua.umap   52 KB, 31-jul
```

Y allí el pueblo corría bien: seis CSV completos en `Saved/Profiling/CSV/`, del
9-ago, el mayor de 23 MB. Sesiones de 240 s enteras.

Que aquí no esté es **deliberado**: `.gitignore:34` excluye `Content/Maps/*.umap`
(el `Content/` completo pesa ~19 GB). `Content/Maps/.gitkeep` lo dice al pie de la
letra — *"Crea L_Alsasua desde el editor y guárdalo en esta carpeta"* — y
`Docs/PRIMER_COMPILADO_5_8.md` §2 lo documenta. No es un fallo del repo: es un
paso de puesta a punto que en este clon nadie ha dado.

Lo que sí es un problema es la fecha. Cronología:

| fecha | qué |
|---|---|
| 31-jul | último guardado de `L_Alsasua.umap` |
| 9-ago | seis sesiones completas en el original — los únicos datos de perf que existen |
| 15–22-ago | **todo el refactor**: 21 plugins, ~200 clases cambiadas de módulo |
| 17-ago | único intento de arranque en este clon → `Failed to enter /Game/Maps/L_Alsasua` |
| 22-ago | `fafc08a`, DLLs de editor construidas |

**Los seis CSV son de una semana antes del refactor.** Ninguna de las ~52 fases
del director ha corrido una sola vez con el código tal como está hoy. "Compila
limpio, 0 errores" es cierto y verifica el enlazado, no la ejecución.

Y hay una trampa concreta al traer el mapa: **no hay un solo `CoreRedirects` en
`Config/`**. El refactor movió ~200 clases entre módulos, y un actor colocado en
un nivel guarda la ruta completa (`/Script/AlsasuaManifa.X`). Todo lo que se
colocara a mano en aquel mapa y cuya clase haya cambiado de módulo **desaparece al
cargar, con un warning y sin parar el arranque**.

Qué hacer, en este orden:

1. Copiar `L_Alsasua.umap` del original a `Content/Maps/`.
2. Abrir y lanzar. Luego `grep -i "Failed to load\|Missing Class\|Cannot find"`
   sobre `Saved/Logs/AlsasuaSimulator.log`.
3. Si salen clases perdidas → añadir `[CoreRedirects]` en `DefaultEngine.ini`, una
   entrada por clase movida. Si no salen → el mapa casi no tiene nada colocado (el
   pueblo es procedural en runtime) y no hace falta nada más.
4. Alternativa si el mapa da guerra: nivel **Empty** nuevo + `Tools/SetupLevel.py`
   (está en `RunAll.py`). Empty, no World Partition — ver P1.
5. Dejar que salga el CSV de 240 s. Ese es el primer dato real del refactor, y la
   línea base de P5.

Bug colateral que ya sale en ese mismo log, y que confirma H4:

```
Error: CDO Constructor (AlsasuaPlayerCharacter): Failed to find
       /Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin
Error: CDO Constructor (AlsasuaPlayerCharacter): Failed to find
       /Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C
```

`AAlsasuaPlayerCharacter` — el duplicado sin GAS marcado para borrar — carga
rutas fijas en el constructor sin respaldo. Viola la regla de degradación elegante
de CLAUDE.md §6 y ensucia el arranque de cada sesión del editor.

**Salida**: un CSV en `Saved/Profiling/CSV/` y un log sin `Fatal`. Hasta que eso
exista, no hay proyecto que integrar.

---

### P1 — Apagar World Partition  ⟨1 línea⟩

`bEnableWorldPartition=False` en `DefaultEngine.ini:129`. Sube aquí porque es la
mejor relación coste/beneficio del plan y porque **bloquea P0**:
`UImportadorLandscape` aborta si `IsPartitionedWorld()` (CLAUDE.md §11), o sea que
con WP encendido el nivel que hay que crear en P0 no puede importar el landscape.

Y no se pierde nada: el pueblo lo genera el director en `BeginPlay()`, y un actor
creado en runtime no pertenece a ninguna celda de WP. Detalle en §2 FASE 7.2.

---

### P2 — Verificadores ven `Plugins/`  ⟨1 día⟩

Detalle en §2 FASE 0. Sube al podio porque a partir de aquí toda afirmación sobre
el código es fe: el 60 % del proyecto no lo audita nadie, `AuditarSistemas.py`
dice "Ninguno" en las cuatro secciones por ceguera, y CI está verde sin significar
nada. Cada día que pasa sin esto se escribe código nuevo sin red.

---

### P3 — Jugador + guardia civil  ⟨5 días⟩

Detalle en §2 FASE 3. Es lo que pediste, y además es el único test que ejercita
terreno, navmesh, IA, GAS, HUD y audio a la vez. Con P0 hecho, es el que convierte
"el pueblo carga" en "el pueblo es un juego".

Lo caro no es el personaje: es que **no hay un solo punto de spawn de guardia en
todo el código**, y hay once piezas de fuerza del orden repartidas por cinco
módulos sin conectar.

---

### P4 — Fusionar clima y guardado  ⟨2 días⟩

Los dos de riesgo alto de la FASE 2 (2.8 y 2.5), adelantados sobre las otras siete
fusiones:

- **Clima**: cuatro sistemas peleando por los escalares de `MPC_Clima`, que sólo
  admite un conductor. Cada día que se retrasa es más código escrito contra el
  sistema equivocado.
- **Guardado**: tres clases de save. Un campo que se guarda y no se carga es
  pérdida de partida del jugador — el único bug de esta lista con consecuencia
  irreversible.

---

### P5 — Línea base medida  ⟨1 día⟩

Detalle en §2 FASE 1. Sólo es posible después de P0 (sin CSV no hay medida). Por
debajo de P4 porque no arregla nada — pero por encima de todo lo gráfico, porque
sin ella "mejora AAA" no es verificable y `RESUMEN_TECNICO.md` es de 5.4.

---

### P6 — Bucle de juego: GF_Misiones, GF_Dialogos, GF_UI  ⟨2-3 semanas⟩

FASE 4.1-4.3. Los tres cascarones que separan "simulador de pueblo" de "juego".
`GF_Dialogos` primero dentro del grupo: `Content/Dialogs/` ya tiene los árboles y
`VerificarDialogos.py` ya los valida — es el que más infraestructura hecha tiene y
menos código.

---

### P7 — Fachadas a Nanite  ⟨1 semana⟩

FASE 5.2. La mejora gráfica de mayor retorno del proyecto: 1030 edificios son la
geometría más pesada del pueblo y hoy son `UDynamicMeshComponent`, que no admite
Nanite. Requiere bake a `StaticMesh` en el editor. Debajo de P6 porque es caro y
porque sin P5 no se puede demostrar que mejoró.

---

### P8 — Las otras siete fusiones  ⟨2 días⟩

FASE 2.1-2.4, 2.6, 2.7, 2.9. Riesgo bajo, se pueden hacer en huecos entre lo
demás. La de interacción (2.1) conviene antes de P3 si el guardia va a implementar
`IInteractuable`.

---

### P9 — Cerrar los `SpawnActor` que quedan  ⟨3 días⟩

FASE 5.1. 111 `SpawnActor` y 97 `LoadObject` en `Plugins/*/Private`. El olor de
CLAUDE.md §8.0 sigue vivo en `GF_Edificios`, `GF_Vegetacion`, `GF_Trafico` y
`GF_Carreteras`. Se hace con la línea base de P5 delante, para poder demostrar la
bajada de draw calls.

---

### P10 — Memoria  ⟨3 días⟩

FASE 6. Depende de P5 (medir primero). Importante pero no urgente: no hay evidencia
de que la memoria sea hoy el cuello de botella, porque no hay evidencia de nada —
ver P0.

---

### P11 — Resto de cascarones  ⟨3-4 semanas⟩

FASE 4.4-4.9: `GF_Abilities`, `GF_Vehiculos`, `GF_Politica`, `GF_Social`,
`GF_Systems`, `GF_Core`. Contenido, no arquitectura. `GF_Abilities` primero, que
además tiene los includes rotos.

---

### P12 — Decidir el nombre de los `GF_`  ⟨0,5-1 día, o 21 días⟩

FASE 7.1. Cosmético mientras nadie intente desactivar un plugin en runtime. La
opción barata (renombrar a `AlsasuaX` y quitar `GameFeatures` del `.uproject`) se
puede hacer cualquier tarde.

---

### P13 — Limpieza  ⟨3 días⟩

FASE 8. Última, como pediste, y con la condición de entrada intacta: verificadores
en verde, línea base batida, y la slice de P3 corriendo 240 s sin `ensure`.

---

## 5. Camino crítico

```
P0 nivel + primer arranque    ← nada tiene sentido antes de esto
 └─ P1 apagar WP              ← bloquea P0 (importador de landscape)
     └─ P2 verificadores
         └─ P3 jugador + guardia civil
             └─ P6 bucle de juego
```

P4 puede ir en paralelo desde el principio. P5 en cuanto haya CSV. P7-P11 después
de P5. P13 al final.

**Si sólo haces una cosa esta semana**: P1 + P0. Un nivel que arranca y un CSV.
Todo el resto del plan está escrito contra un proyecto que nadie ha visto correr.
