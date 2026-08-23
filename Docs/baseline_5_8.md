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
