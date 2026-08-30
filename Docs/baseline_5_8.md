# Línea base 5.8 — primera ejecución real del refactor

Medido el 2026-08-23 sobre `integracion-total` @ `778bf96`, UE 5.8, standalone,
1280×720, DX12/SM6. GPU: AMD Radeon RX 6650 XT. CPU: Ryzen 7 PRO 4750G.

**Es la primera vez que el pueblo se ejecuta con el código refactorizado en 21
plugins.** Los seis CSV que había (`Profile(20260809_*)`) son del proyecto
original y de una semana antes del refactor: no sirven de comparación directa.
`RESUMEN_TECNICO.md` es de UE 5.4 y menos aún.

## Qué desbloqueó la ejecución

Dos fallos, ninguno visible al compilar:

1. **`AlsasuaKernel` sin `IMPLEMENT_MODULE`** (`e639a7b`). Era el único módulo
   del proyecto sin fichero de módulo. El linker producía un DLL válido —de ahí
   que el build saliera siempre limpio— pero sin el objeto de módulo que busca
   el `ModuleManager`. El motor abortaba con *"The game module 'AlsasuaKernel'
   could not be successfully initialized after it was loaded"* y cero líneas de
   log entre la carga y el fallo. El `EXCEPTION_ACCESS_VIOLATION` posterior era
   el motor desmontándose a medias, no un bug aparte.

2. **`UMuestreadorAltura::EstaDisponible()` mentía** (`778bf96`). Miraba el
   puntero `Terreno` a pelo, y ese puntero lo rellena `BuscarTerreno()` desde
   dentro de `AlturaMundo()`: el primero en preguntar recibía `false` aunque el
   terreno llevara rato construido. `CargadorArboles` lo usa como guarda, así
   que descartaba los 2783 árboles del LiDAR y el log los contaba como "fuera
   del terreno" — cierto de 280 y de ninguno de los otros 2503.

## Rendimiento (mediana de 126 muestras, sesión de 240 s)

| métrica | mediana | mín | máx |
|---|---:|---:|---:|
| `FrameTime` | **15,5 ms** (≈64 FPS) | 12,2 | 49 522 |
| `GPUTime` | 10,9 ms | 5,8 | 312,9 |
| `RenderThreadTime` | 15,3 ms | 12,2 | 41 963 |
| `GameThreadTime` | 9,3 ms | 6,0 | 5 272 |
| `RHI/PrimitivesDrawn` | 1 521 325 | 649 141 | 9 449 075 |
| `PhysicalUsedMB` | 16 486 | 15 599 | 16 616 |

Los máximos de decenas de segundos son el arranque: compilación de shaders y
construcción del mundo, no régimen de juego. La mediana es lo comparable.

`RHI/DrawPrimitiveCalls` **no sale en este CSV** — hay que añadirlo al conjunto
de stats si se quiere seguir la métrica de §8 de `CLAUDE.md` (los 819 draw calls
de referencia). Sin ella no se puede verificar la regla del actor-por-pieza.

Memoria: 16,5 GB de pico. No hay presupuesto declarado contra el que juzgarlo
(fase 6 del plan de integración).

## Qué se construye (52 fases, `Progreso=1.00`)

| sistema | cantidad |
|---|---|
| Terreno procedural | 1024 chunks (4033², origen −168200, 497000) |
| Suelos poligonales | 278 (plazas + zonas verdes) |
| Relieve lejano | 315 768 tris, 1 sección, anillo de 60 km |
| Árboles LiDAR | **2503** en 14 especies (280 descartados) |
| Vías | 374 (206 aceras, 86 ferrocarril, 74 ríos, 8 caminos) |
| Calles | 489 |
| Edificios | 1030 (938 con altura LiDAR; 2357 huellas medidas) |
| Vegetación | 5000 hierbas + 2068 flores + 500 arbustos en 273 zonas |
| Aparcamiento | 1880 plazas, 452 coches instanciados |
| Señalización | 79 señales, 932 marcas viales, 120 placas bilingües |
| Semáforos | 12 en intersecciones reales (271 candidatos) |
| Fachadas | 717 toldos + 17 537 persianas, 1030 puertas |
| Cubiertas | 552 detalles (antenas, chimeneas, placas solares) |
| Cables aéreos | 887 vanos sobre 1095 postes |
| Otros | 1647 barandillas, 100 contenedores, 23 murales, 10 bocas de túnel, 21 vehículos ferroviarios |

## Lo que sigue roto

1. **Materiales que no compilan** → Default Material gris en superficies
   grandes. La causa es siempre la misma: `TextureSampleParameter2D` con la
   textura en NULL porque el uasset no existe (los uassets derivados de textura
   están en `.gitignore`, se reimportan).
   - `M_Terreno_Orto` y `M_Tejado_Orto`: **resueltos**. Las dos ortofotos eran
     punteros LFS de 134 bytes en este worktree; las imágenes reales (116 MB y
     108 MB) estaban en el worktree original. Copiadas e importadas con
     `Tools/ImportSatellite.py`.
   - `M_Terreno_Calles` y `M_Terreno_Acera`: dos texturas NULL cada uno. Los 37
     PNG de `Content/Textures/T_*.png` sí están versionados pero nunca se
     importaron. Los importa `Tools/ImportTexturasPBR.py` (nuevo).
2. **Packs de mallas ausentes**, con degradación funcionando pero pobre:
   - `/Game/AssetsImportados/Naturaleza/*` — nada importado; 7568 quads de
     respaldo en las zonas verdes.
   - `/Game/Meshes/Arboles/SM_*` — los 14 árboles caen a forma básica.
   - `/Game/Nanite_Plants_Sample_Collection/*` — 0 plantas nanite.
   - `/Game/VehicleVarietyPack/*` — los coches caen a cubo escalado.
3. **`lidar_dtm_05m.raw` no está** en `Content/Terreno/`. El heightmap principal
   (`alsasua_landscape_4033.r16`, 32,5 MB) sí carga; este segundo no, y el
   terreno tira del procedural.
4. **`VegetationSpawnerSubsystem` sin DataAssets** de bioma y sin landscape.
   Corre y no hace nada.
5. **`TerrenoLejano`: `CeldaM=150` no divide exacto** el borde (176.000 celdas);
   el anillo solapa 150 m bajo el terreno. El propio log dice la solución: usar
   100, 150, 200 o 300.
6. **Sin jugador ni guardia civil verificados.** El mundo se construye; nadie lo
   habita. Es la fase 3 del plan de integración.


## Puesta a punto tras clonar (no está en git)

Tres pasos, en este orden. Ninguno es opcional si quieres ver el pueblo con su
aspecto real en vez de gris.

1. **Traer las dos ortofotos.** `Content/Terreno/alsasua_satelite_pnoa_8192.png`
   (116 MB) y `Content/Textures/ortofoto_pnoa_plaza_8192.png` (108 MB) están en
   LFS y en este worktree se quedaron como punteros de 134 bytes. El presupuesto
   LFS de la cuenta está agotado (`CLAUDE.md` §5b), así que `git lfs pull` no las
   trae: hay que copiarlas de otro worktree que las tenga, o regenerarlas con
   `Tools/DescargarOrtofotoPNOA.py`.

   Cómo saber si te faltan: si pesan ~134 bytes, es un puntero.

2. **Importar las ortofotos**: `Tools/ImportSatellite.py` en el editor, o
   headless con `-run=pythonscript`. Arregla `M_Terreno_Orto` y `M_Tejado_Orto`.

3. **Importar las texturas PBR**: `Tools/ImportTexturasPBR.py`. Los 37
   `Content/Textures/T_*.png` sí están versionados, pero sus uassets no. Arregla
   `M_Terreno_Calles` y `M_Terreno_Acera`.

Tras los tres, los cuatro materiales del proyecto compilan. El único que sigue
cayendo a Default Material es `MS_DefaultMaterial` del pack de terceros
`UnrealDrive_CitySample`, cuyas texturas 8K también son punteros LFS; no lo usa
nada del pueblo.

**Nota sobre el commandlet**: los dos scripts terminan con `exit code 1` y
`Failure - 2 error(s)` aunque el import haya ido bien. Los dos errores son
preexistentes y ajenos: *"Asset manager settings do not include a rule for assets
of type GameFeatureData"* — los plugins `GF_` no son Game Features reales
(§H2 de `PLAN_INTEGRACION_AAA.md`). Mira `Python script executed successfully` en
el log, no el código de salida.


## Segunda pasada — assets de tu biblioteca (2026-08-23, tarde)

Petición: "faltan los assets de coches de Epic Engine, mi biblioteca" + "añade
más assets del estilo" (árboles, edificios, casas, asfalto, más coches, todo lo
que hubiera en la biblioteca ya descargada, un vehículo de Guardia Civil).

### Encontrado y arreglado

**11 rutas con el mismo bug** (`fix(assets)` / `4c32750`): el objeto interno de
una malla FBX/glTF importada siempre se llama como el paquete (`SM_Cosa` →
objeto `SM_Cosa`, nunca `Cosa`), y media docena de sitios pedían la forma corta.
El `.uasset` existía, `LoadObject` fallaba en silencio, y todo caía al
placeholder más básico. Verificado el nombre interno real de cada uno con el
name-table del binario antes de tocar nada:

- `VehicleVarietyPack`: SM_Hatchback/SUV/SportsCar/Pickup/Truck_Box — 4 ficheros,
  8 apariciones (coches, furgonetas, autobús del tráfico dinámico).
- `DZ_Assets/DZ_Trees`: 5 especies de árbol de alta calidad (pino, roble, chopo,
  cocotero, palmera).
- `Nanite_Plants_Sample_Collection`: 3 mallas de vegetación de zona verde.
- `UnrealDrive_CitySample/Meshes/Rail_Guard`: el guardarraíl — **1647 apariciones**,
  todas las barandillas del pueblo.

**Packs copiados del worktree original** (mismo disco físico, `cp` instantáneo,
no descarga): `VehicleVarietyPack` (1,4 GB), `AssetsImportados/Naturaleza`
(638 MB, hierba/setos/rocas/hiedra), `CitySample` (942 MB, props). Los tres ya
estaban en `.gitignore` con el nombre correcto — sólo faltaban en disco en este
worktree.

**Import headless de 106 FBX de Naturaleza** — bloqueado por un bug real en
`Tools/ue5_import_all_assets.py`: `FbxImportUI.convert_scene` ya no existe con
ese nombre en 5.8, y la excepción sin capturar abortaba el bucle entero antes de
importar nada. Envuelto en `try/except` (`bd3a613`): 106 FBX → 169 uassets.

**Generador procedural** (`UAlsasuaAssetGenerator::GenerarTodosLosAssets`, ya
existía, nunca se había ejecutado): 10 especies de árbol, 8 piezas de
mobiliario urbano, 7 landmarks (iglesia, ayuntamiento, escuela, estación,
frontón, nave, bloque cívico), 51 materiales. De paso, un `ensure` real
(`SetupAttachment` después de `RegisterComponent`, orden invertido) en los
generadores de río y puente — corregido, aunque esas dos herramientas son
redundantes con el runtime (`UCargadorVias`/`UCargadorPuentes` ya construyen
ríos y puentes reales en cada arranque).

**Resultado**: `VehicleVarietyPack` y `AssetsImportados/Naturaleza` a **0**
fallos de carga (eran 89 y 18). Materiales del proyecto rotos: **0** (era 4,
luego el `MS_DefaultMaterial` de un pack de terceros no usado por el pueblo).

### Guardia Civil — no resuelto, necesita tu sesión de Epic

No hay ningún vehículo policial en la biblioteca, ni versionado ni en el
worktree original. Fab no es accesible por MCP — es la pestaña del editor con
tu login. Candidatos encontrados por búsqueda web:
[Police Car - Interactable Vehicles](https://www.fab.com/listings/de417e86-8ffa-4adc-a734-6fde5110315b)
(recomendado, pensado para colocación en mundo, no conducible),
[Low Poly Vehicles Police Pack 3](https://www.fab.com/listings/bc97b138-592c-4a94-a313-e8edca48c775).
`AlsasuaPoliceVan.cpp` es un `USkeletalMeshComponent` vacío sin ruta
hardcodeada — listo para recibir lo que añadas.

### Hueco real que queda: variedad de fachada por barrio

Los 1030 edificios **ya tienen material** (`M_Fachada`, con ventanas nocturnas)
— no salen grises. `buildings_final.json` trae `material_type` (ladrillo /
piedra_hormigon / ladrillo_industrial) + `barrio` por edificio, y
`CargadorEdificios.cpp:300` intenta *sobre-escribir* con `M_<tipo>_<barrio>`
(p. ej. `M_ladrillo_Intxostia`); si no existe, ni el genérico `M_<tipo>`
tampoco, se queda con `M_Fachada`. Ninguna de esas ~24 variantes (3 tipos × 8
barrios) tiene generador. Efecto real: todos los edificios comparten el mismo
ladrillo genérico en vez de variar por barrio — no bloqueante, trabajo nuevo con
alcance propio (escribir ~24 entradas de material en un generador, no hay bug
que arreglar). Mismo patrón con `/Game/LUTs/LUT_Neutral`
(`AlsasuaBarrioStyleSystem.h:142`): LUT de color grading por barrio, sin
generador, cae en silencio sin aplicar nada.

---

## Corrección: la medida anterior estaba hecha con los plugins apagados

La tabla de arriba (mediana 15,5 ms, ~64 FPS, 4256 draw calls) se tomó cuando
**ninguno de los 21 plugins `GF_*` se estaba cargando**. No era el pueblo
completo: era el esqueleto sin la mitad de los sistemas.

Salieron tres fallos de arranque más, todos de configuración y todos invisibles
al compilar:

| # | qué | dónde |
|---|---|---|
| 2 | los 21 `GF_*` con `"Enabled": false` | `AlsasuaSimulator.uproject` |
| 3 | los 21 `.uplugin` con `"ExplicitlyLoaded": true` mientras 14 están enlazados en duro | `Plugins/*/*.uplugin` |
| 4 | `USocialMediaSubsystem::Initialize` desreferencia un GameInstance nulo | `GF_Social` |

El 3 es una contradicción de raíz: `ExplicitlyLoaded` le dice al motor "no
cargues este DLL al arrancar", pero `PublicDependencyModuleNames` genera una
importación estática que el loader de Windows exige antes de ejecutar una sola
línea. Da `GetLastError=126` y el motor culpa al módulo que importa, no al
descriptor que lo causa.

El 4 es la trampa de `CLAUDE.md` §11 al pie de la letra: `Initialize` de un
`UWorldSubsystem` lo llama el motor en todos los mundos —el del editor, los
transitorios, cada PIE y la cocción— y en varios no hay GameInstance.

## Línea base real (2026-08-30, sesión completa de 240 s, cierre limpio)

`Profile(20260830_220923).csv`, 229 filas, 21/21 plugins cargados, 0 fallos de
módulo, 0 crashes, `Log file closed`.

| | sin plugins (medida vieja) | **con los 21 (real)** |
|---|---:|---:|
| `FrameTime` | 15,47 ms (~65 FPS) | **33,75 ms (~30 FPS)** |
| `GameThreadTime` | 9,33 | 13,27 |
| `RenderThreadTime` | 15,31 | 21,63 |
| `GPUTime` | 10,85 | **11,50** |
| `RHI/DrawCalls` | 4256 | **6657** |
| `RHI/PrimitivesDrawn` | 1 521 325 | 1 788 047 |
| `Basic/TicksQueued` | 215 | **969** |

### Lectura

1. **El pueblo va a ~30 FPS, no a 65.** El 65 medía un build lisiado.

2. **Limitado por CPU, no por GPU.** La GPU casi no se mueve (10,85 → 11,50)
   mientras el frame se duplica. El coste entra por hilo de render (21,63) y de
   juego (13,27). Apagar Lumen o VSM no arreglaría nada — §8.4 de `CLAUDE.md` ya
   lo decía.

3. **6657 draw calls: 8,1 veces los ~819 de referencia.** Es el número a batir,
   y confirma la FASE 5.1 del plan: quedan `SpawnActor` en bucle sin convertir a
   instanciado.

4. **969 ticks encolados frente a 215**: los plugins multiplican por 4,5 lo que
   tica. Candidato directo para `UAlsasuaOptimizerSubsystem` (§8.3).

5. Los p95 ya son sanos (177 ms) frente a los 523 de la primera pasada: la DDC
   ya estaba caliente. Los máximos de 47 s siguen siendo el arranque.
