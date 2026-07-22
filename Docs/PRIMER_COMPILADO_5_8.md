# Primer compilado — Alsasua Simulator (UE 5.8)

Guía paso a paso para llevar el puerto C++ de cero a una escena jugable. Orden
pensado para que cada paso desbloquee el siguiente. Marca ✅ a medida que avances.

El `.uproject` ya fija `EngineAssociation: "5.8"`.

---

## 0. Requisitos

- Unreal Engine **5.8** instalado.
- Visual Studio 2022 con el workload *Game development with C++* (toolchain de UE 5.8).
- Python 3 (para `Tools/PrepararLandscape.py`), con `numpy`.
- El repo `Alsasua_Simulator` clonado; trabajamos en `UnrealProject/`.

### Compatibilidad 5.8 (verificado)
- El único cambio rompedor de materiales en 5.8 es la firma de `ComputeFinalGBuffer`
  (Substrate) en shaders **`.usf/.ush` personalizados**. Aquí **no** hay shader custom:
  todos los materiales se generan por grafo de nodos (`UMaterialEditingLibrary`), así
  que no afecta. Substrate (activado en `DefaultEngine.ini`) sigue autoconvirtiendo los
  pines legacy (BaseColor/Roughness/Normal/Emissive/Opacity) que usan estos materiales.
- `ProceduralMeshComponent` sigue presente en 5.8. La función C++ `CreateMeshSection`
  (con `FColor`) que usan los generadores de geometría sigue siendo válida (lo que se
  marcó *deprecated* es el nodo expuesto a **Blueprint**, no la función C++). Si en
  algún momento Epic retira el overload C++, la migración es a `CreateMeshSection_LinearColor`.
- Navmesh por invokers, lights/sky/fog, Niagara y el import de Landscape: sin cambios en 5.8.

---

## 1. Generar proyecto y primer build de código

1. Click derecho en `AlsasuaSimulator.uproject` → **Generate Visual Studio project files**.
2. Abre el `.sln` y compila en configuración **Development Editor**.
3. Los módulos compilan solos en orden de dependencia. El grafo es:

   ```
   AlsasuaCore  →  AlsasuaWorld  →  AlsasuaGameplay  →  AlsasuaUI
                →  AlsasuaEntities ↗
   AlsasuaEditor (solo editor) → depende de Core + World
   ```

   Si algún módulo no aparece, revisa que esté en `AlsasuaSimulator.uproject`
   (sección `Modules`) y en los `.Build.cs`.

> **Editor (resuelto)**: `CreadorMaterialAgua.cpp` fija `BlendMode`/`TwoSided`
> **por reflexión** (`FindPropertyByName` + `SetPropertyValue_InContainer`), así
> que compila tanto si esos campos son públicos como privados en 5.8. No requiere
> ajuste. Es código de editor; no afecta al runtime del juego.

---

## 2. Nivel `L_Alsasua`

El `DefaultEngine.ini` ya apunta el mapa por defecto a `/Game/Maps/L_Alsasua`.

1. **File → New Level**. Elige **Empty Level** (no "Open World") para el primer
   arranque: evita el flujo World Partition en el import de landscape (ver §4).
2. Guárdalo en `Content/Maps/` como `L_Alsasua`.
3. **World Settings**:
   - *GameMode Override* = `AlsasuaGameMode` (ya fijado por ini, pero confírmalo).
   - El `AlsasuaGameMode` ya define `DefaultPawnClass=AAlsasuaCharacter` y
     `PlayerControllerClass=AAlsasuaPlayerController` en C++.

---

## 3. HUD (capa UI no la puede poner el GameMode de Gameplay)

El GameMode (capa Gameplay) **no** puede referenciar la UI por las reglas de capa.
Asigna el HUD por config o Blueprint:

- **Opción ini** — añade a `Config/DefaultEngine.ini`, bajo la sección del GameMode:
  ```ini
  [/Script/AlsasuaGameplay.AlsasuaGameMode]
  HUDClass=/Script/AlsasuaUI.AlsasuaHUD
  ```
- **Opción Blueprint** — crea un `BP_AlsasuaGameMode` hijo de `AlsasuaGameMode`,
  pon *HUD Class = AlsasuaHUD*, y úsalo como GameMode del mapa.

### Controles (teclado + mando)
- **Enhanced Input** se construye en **runtime** si no asignas assets en el editor:
  mover (WASD / stick izq.), mirar (ratón / stick der.), saltar (Espacio / A),
  correr (Shift / LB), agacharse (C / B). Si prefieres autorar los `IA_*`/`IMC` como
  uasset, asígnalos en el Blueprint del personaje y tienen prioridad.
- **Acciones**: disparar (ratón izq. / RT), interactuar-diálogo (E / Y), disfraz
  (H / L3), menú (Esc / Start), navegar menú (flechas o DPad / A). El resto de
  teclas de acción (1-8 armas/sustancias, M manifa, F5/F9) son de teclado por ahora.

### Menú principal y pausa (flujo de juego)
- **Pausa** (en el juego): ya funciona sin setup — `Esc` abre el menú (Reanudar /
  Guardar / Cargar / Opciones / Salir), flechas + Enter. `F5`/`F9` guardado/carga rápida.
- **Menú principal** (pantalla de inicio): crea un nivel vacío `/Game/Maps/L_MainMenu`,
  ponle *GameMode Override* = `AMenuPrincipalGameMode` (su HUD ya está en el ini), y
  cambia `GameDefaultMap` a `L_MainMenu`. Opciones: *Nueva partida* (abre `L_Alsasua`),
  *Continuar* (abre `L_Alsasua` y autocarga el slot 0 al estar listo el mundo), *Salir*.
  El nombre del nivel de juego es la propiedad `NivelJuego` del controller (por defecto
  `L_Alsasua`); ajústalo si tu mapa jugable se llama distinto.

Con esto verás la pantalla de carga, el HUD de juego, el diálogo y las misiones.

---

## 4. Terreno (Landscape)

1. Genera el heightmap listo para UE (una vez):
   ```
   python3 Tools/PrepararLandscape.py <ruta heightmap_unificado.r16> UnrealProject/Content/Terreno
   ```
   Produce `Content/Terreno/alsasua_landscape_4033.r16` + `landscape_import.json`.

2. Importa, dos vías:
   - **C++**: ejecuta `UImportadorLandscape::ImportarLandscape(<ruta r16>)` desde
     un Editor Utility Widget/Blueprint o la consola. Valores por defecto ya correctos.
   - **Manual** (Landscape Mode → Import from File) con:

     | Parámetro        | Valor       |
     |------------------|-------------|
     | Section size     | 63×63 quads |
     | Sections/comp.   | 1×1 (64×64 componentes) |
     | Scale X / Y      | **178.5714** |
     | Scale Z          | **200** |
     | Location Z (cm)  | **49567** |
     | Location X / Y   | centrar en Herriko Plaza |

3. Comprobación: la cota de Herriko Plaza (531.94 m) debe quedar en `WorldZ ≈ 2061 cm`.

> **World Partition (blindado)**: `UImportadorLandscape` detecta si el nivel es
> World Partition (`World->IsPartitionedWorld()`) y **aborta con aviso** en vez de
> crear un landscape roto sin proxies de streaming. En ese caso usa **Landscape
> Mode → Import from File** (crea los proxies solo) con los valores de la tabla de
> arriba, o un nivel **Empty** para el primer arranque. El flag
> `bPermitirWorldPartition=true` fuerza el import C++ (no recomendado).

---

## 5. Cielo, niebla y atmósfera

`UCicloVisualSubsystem` **crea solo** Sun + SkyAtmosphere + SkyLight (captura en
tiempo real) + ExponentialHeightFog si el mapa no los trae. No tienes que ponerlos.
- Si prefieres controlarlos a mano, colócalos en el mapa y el subsistema los
  reutilizará (busca-o-crea, no duplica).
- Exposición: con Lumen + auto-exposure, los valores de sol (`IntensidadDia=8`) son
  relativos; si la escena sale quemada/oscura, ajusta el Post Process Volume
  (Exposure) o `IntensidadDia/Noche` en el subsistema.

---

## 6. Materiales (agua + edificios)

Ejecuta una vez cada utilidad (Editor Utility/consola); los cargadores los aplican solos:

1. `UCreadorMaterialAgua::CrearMaterialAgua()` → `/Game/Materiales/M_AguaRio`.
   `UCargadorVias` lo pone en las cintas de río. Bajo **Substrate** (activado en tu
   ini) los pines legacy se autoconvierten; sale traslúcido y reflectante. Oleaje
   (normal map en paneo) = opcional, textura de artista.
2. `UCreadorMaterialEdificio::CrearMaterialEdificio()` → crea **`MPC_Clima`**
   (Material Parameter Collection con el escalar `Wetness`) y luego
   `/Game/Materiales/M_Edificio`, material opaco **vertex-color** genérico de
   superficies que lee `Wetness`: con lluvia oscurece el color y baja la rugosidad
   (aspecto mojado). `UClimaSubsystem` conduce `Wetness` en runtime (sube con
   lluvia, seca despacio). Lo aplican:
   - `UCargadorEdificios` → edificios con paleta vasca por id (arenisca en muros,
     teja terracota o pizarra en tejados).
   - `UCargadorCalles` → calzada (asfalto) y peatonal/sendero (adoquín claro).
   - `UCargadorVias` → aceras (adoquín) y vía de tren (balasto); los ríos usan `M_AguaRio`.
   - `UCargadorPoligonos` → plazas (piedra) y zonas verdes (verde del propio dato).

   Así toda la capa de suelo y los edificios salen teñidos en vez de grises. Sin el
   material, cada superficie usa su color base por defecto.
   - `UCargadorEdificios` prefiere **`M_Fachada`** si existe (ver punto 3); si no,
     usa `M_Edificio`.
3. `UCreadorMaterialFachada::CrearMaterialFachada()` → `/Game/Materiales/M_Fachada`
   (**ejecútalo después** de `CrearMaterialEdificio`, que crea el `MPC_Clima`).
   Fachada vertex-color + mojado + **ventanas procedurales** (rejilla por UV, ~2.6 m
   horizontal × 3.1 m por planta) que se **encienden de noche** vía el escalar
   `Night` del MPC (lo conduce `UClimaSubsystem` según la hora), gateadas a las
   caras verticales (no salen ventanas en tejados/suelo) y con **encendido
   pseudo-aleatorio por ventana** (~55% encendidas, hash de la celda) para que no
   brillen todas a la vez. `UCargadorEdificios` la aplica a los edificios.
4. **Ortofoto en tejados (opcional, máxima fidelidad)**:
   - Importa `Content/Datos/ortofoto_unity.png` (o `ortofoto_alsasua_REAL.png`) como
     textura `/Game/Textures/T_Ortofoto` (marca **sRGB**, sin mipmaps agresivos).
   - Ejecuta `UCreadorMaterialTejadoOrto::CrearMaterialTejadoOrto()` →
     `/Game/Materiales/M_Tejado_Orto`. Proyecta la ortofoto PNOA (25 cm/px) en planta
     sobre los tejados mapeando `WorldPosition.XY` al bbox real de la ortofoto
     (constantes ya calculadas de `orto_tiles_meta.json`). `UCargadorEdificios` la
     aplica a la **sección 1** (tejado); los muros (sección 0) siguen con la fachada.
   - También tiene parámetro `Detalle` (textura de teja tileada ~1 m) que da nitidez de
     cerca y se funde por distancia, igual que el terreno.
   - **Caveat de orientación**: el mapeo asume norte arriba / oeste a la izquierda en
     el PNG. Si la ortofoto sale girada o espejada en los tejados, invierte `texU` o
     `texV` en `CreadorMaterialTejadoOrto.cpp` (un signo) y regenera. (El mismo signo
     hay que tocar en `CreadorMaterialTerrenoOrto.cpp` — comparten el mapeo.)
5. **Ortofoto en el terreno (opcional, casa con su entorno real)**:
   - Con `T_Ortofoto` ya importada, ejecuta
     `UCreadorMaterialTerrenoOrto::CrearMaterialTerrenoOrto()` →
     `/Game/Materiales/M_Terreno_Orto`. Proyecta la misma ortofoto en planta sobre el
     Landscape y se **funde a un verde neutro fuera del área cubierta** (~2,7 km del
     casco), así caminos, prados y parking entre edificios son la foto aérea real.
   - **Si importas el Landscape DESPUÉS de crear este material**, `UImportadorLandscape`
     lo asigna solo como *Landscape Material*. Si ya existía el Landscape, asígnalo a
     mano en el actor Landscape → *Landscape Material* = `M_Terreno_Orto`.
   - Comparte el parámetro `Ortofoto`: asigna la misma `T_Ortofoto` a ambos materiales.
   - **Detalle de cerca**: parámetros `Detalle` (albedo tileado de hierba/asfalto/ruido,
     ~2 m) que modula el brillo a ras de suelo para dar nitidez —la ortofoto a 25 cm/px
     se ve borrosa de cerca— y `DetalleNormal` (normal map tileado, **marca la textura
     como Normal Map al importar**) que aporta **relieve** real. Ambos se **funden por
     distancia** (>60 m vuelve a ortofoto plana, sin tiling). El material de tejado
     (`M_Tejado_Orto`) tiene los mismos dos parámetros (~1 m). Sin texturas asignadas,
     queda solo la ortofoto.
6. `UCreadorMaterialArbol::CrearMaterialArbol()` → `/Game/Materiales/M_Arbol`.
   Material con parámetro vectorial `Color`. `UCargadorArboles` crea una instancia
   dinámica por especie y le fija el tono de copa (pino verde oscuro, abedul claro,
   roble/haya/chopo intermedios…). Sin el material, los árboles usan el placeholder
   por defecto.

---

## 7. Navegación (NavMesh por invokers)

Ya configurado en `DefaultEngine.ini`:
```ini
[/Script/NavigationSystem.NavigationSystemV1]
bAutoCreateNavigationData=True
bGenerateNavigationOnlyAroundNavigationInvokers=True
```
`UNavMeshAlsasua` registra el invoker del jugador al arrancar. La malla se teje
sola alrededor del jugador (90 m). No necesitas `NavMeshBoundsVolume`.
- Verifica que el `RecastNavMesh` se autocrea (aparece en el Outliner al jugar).
- Si la policía/peatones no se mueven: confirma que los edificios procedurales
  exportan navegación (los `ProceduralMeshComponent` con colisión lo hacen) y que
  el agente del nav (radio/alto) encaja con la cápsula del personaje.

---

## 8. Assets opcionales (el juego arranca sin ellos)

Estos cuelgan de rutas blandas; si faltan, esa pieza queda en silencio/sin VFX
pero **nada peta**:

| Ruta esperada                         | Qué es                          |
|---------------------------------------|---------------------------------|
| `/Game/VFX/NS_Lluvia`                 | Niagara de lluvia (clima)       |
| `/Game/Audio/SC_Trueno`               | Sonido de trueno (tormenta)     |
| `/Game/Audio/Amb_Lluvia/Viento/Multitud/Dia/Noche` | Camas de ambiente en bucle |

También conviene asignar **skeletal mesh** a `APeatonActor`/`AManifestanteActor`
(de momento salen como cápsula invisible; el coche sí se ve por el cubo).

---

## 9. Probar (Play)

Flujo esperado al darle a **Play**:

1. **Pantalla de carga** (HUD) con barra de progreso.
2. El **director de arranque** puebla por fases: terreno → edificios (marca
   *baseline*, te libera el movimiento) → calles → vías → suelos → árboles.
3. Aparece el HUD de juego; arranca **M00 "Esnatu, Altsasu"** con su diálogo.
4. Muévete y llega a la **Herriko Plaza** → completas M00 → salta **M01**, que
   convoca una manifestación con marcha.
5. Teclas: `1-4` armas / `5-8` sustancias / `H` disfraz / `E` interactuar-diálogo
   / `M` convocar manifa / ratón izq. disparar.

---

## 10. Render AAA (ya en el ini, requiere hardware)

`DefaultEngine.ini` activa Lumen HWRT, Virtual Shadow Maps, MegaLights, Substrate,
Nanite, TSR, DX12. Objetivo gama alta; Lumen cae a software sin RT. Si el editor
va a tirones en el primer arranque, baja temporalmente la *Scalability* (Engine
Scalability Settings) mientras validas la lógica.

---

### Resumen de orden mínimo
**Compilar código → crear `L_Alsasua` (Empty) + GameMode → asignar HUD →
`PrepararLandscape.py` + importar Landscape → `CrearMaterialAgua()` → Play.**
Lo demás (cielo, navmesh, audio) se autoconfigura o es opcional.
