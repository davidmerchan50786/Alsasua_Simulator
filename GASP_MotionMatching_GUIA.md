# GASP / Motion Matching — guía de integración (Paso AAA-1)

Tienes las **887 secuencias FBX de GASP** extraídas en `E:\Desk\GASP_FBX\`
(AimOffset, Idle, Crouch, Walk, Run, Sprint, Jump, Traversal). El código del personaje
(`AAlsasuaCharacter`) ya está preparado: cuerpo Mannequin + cámara/movimiento AAA + Enhanced Input.

Los plugins de Motion Matching ya están activados en el `.uproject`:
**PoseSearch, MotionTrajectory, AnimationWarping, AnimationLocomotionLibrary, Chooser.**

> Estos pasos son de **editor/contenido** (assets binarios), no se pueden hacer fuera de Unreal.

## 0. Recompila con los plugins nuevos
Abre el proyecto en UE 5.8 (recompila los módulos) o haz Build en Visual Studio. Al abrir el
editor, confirma en **Edit → Plugins** que los 5 de arriba están **Enabled** (si pide reiniciar, reinicia).

## 1. Importa las secuencias sobre el esqueleto del Mannequin
1. Content Browser → crea la carpeta `/Game/Animaciones/GASP`.
2. **Import** (o arrastra) **todos** los `.fbx` de `E:\Desk\GASP_FBX\` (puedes seleccionar todas las
   subcarpetas a la vez).
3. En el diálogo de import de animación:
   - **Skeleton** = el del Mannequin: `SK_Mannequin_Skeleton` (en `/Game/Characters/Mannequins/Meshes/`).
   - Import Animations = **ON**, Import Mesh = OFF.
   - Mantén "Use Source Name", frame rate por defecto.
4. Importa. Te quedan ~887 `AnimSequence` en `/Game/Animaciones/GASP`.

## 2. Motion Matching — dos caminos

**Opción A (recomendada, turnkey): traer el AnimBP de GASP ya montado.**
El pack que tienes es "sequences only" (sin grafo). Lo más rápido para Motion Matching de verdad es
instalar el **Game Animation Sample** completo desde **Fab** (gratis), abrirlo, y **Migrate**
(click derecho → Asset Actions → Migrate) su `ABP_SandboxCharacter` + la **Pose Search Database** +
los Chooser a este proyecto. Vienen ya construidos sobre estas mismas secuencias.

**Opción B (manual, si quieres montarlo tú):**
1. Crea un **Pose Search Schema** (`/Game/Animaciones/GASP/PSS_Locomocion`): esqueleto = Mannequin;
   canales = posición/velocidad de pies + trayectoria.
2. Crea una **Pose Search Database** con ese schema y añade las secuencias de Walk/Run/Sprint/Idle/Crouch.
3. Crea un **Animation Blueprint** sobre `SK_Mannequin_Skeleton`:
   - Añade un componente/origen de **Motion Trajectory** (lo alimenta el Character Movement).
   - En el AnimGraph, nodo **Motion Matching** apuntando a la Database, con la trayectoria como query.
   - Salida → output pose (con Anim Warping para los pies si quieres).

## 3. Engánchalo al personaje
1. Crea un **Blueprint derivado** de `AAlsasuaCharacter` (Content Browser → Blueprint Class →
   busca `AlsasuaCharacter`), llámalo `BP_JugadorAlsasua`.
2. En su componente **Mesh**: asigna el AnimBP (el de GASP migrado, o el tuyo de la Opción B).
3. Crea los assets de **Enhanced Input** y asígnalos en el Blueprint (categoría "Input"):
   - `IMC_Jugador` (Input Mapping Context) con: WASD→`IA_Mover` (Vector2D), ratón→`IA_Mirar`,
     Espacio→`IA_Saltar`, Shift→`IA_Correr`, Ctrl→`IA_Agacharse`.
   - Asigna `ContextoMapeo = IMC_Jugador` y cada `IA_*` a su Input Action.
4. En el GameMode (`AAlsasuaGameMode`) pon `DefaultPawnClass = BP_JugadorAlsasua`
   (o ponlo desde World Settings del mapa).

Resultado: jugador con cuerpo, locomoción Motion Matching y controles modernos.
Si no haces el paso 3.3, el personaje **igual se mueve** con el input clásico (WASD) por el fallback ya programado.

## Notas
- Si al importar las FBX el retargeting no cuadra (esqueleto distinto), usa **IK Retargeter** del
  Mannequin; las secuencias de GASP están hechas para el Mannequin de UE5, así que normalmente encajan directas.
- `ExperimentalStateMachineData` del pack son datos auxiliares; no hace falta importarlos para Motion Matching.
