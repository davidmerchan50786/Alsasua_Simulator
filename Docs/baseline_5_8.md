# Línea base 5.8 — primera medida del refactor

Sesión del 2026-08-23, `Profile(20260823_144745).csv`, sobre el árbol con los
21 plugins `GF_*` y el fix de `IMPLEMENT_MODULE` en `AlsasuaKernel`.

**Es la primera vez que este código se ejecuta.** Los seis CSV que había en el
proyecto original son del 9-ago, una semana antes del refactor, y los números de
`RESUMEN_TECNICO.md` son de una sesión en **5.4**. No son comparables sin más;
esta tabla es el nuevo punto de partida.

Máquina: Ryzen 7 PRO 4750G (8c/16t), 32 GB, Radeon RX 6650 XT (8 GB),
Windows 11 25H2, D3D12 / SM6, Development Editor.

## Medidas

126 filas de datos útiles. Mediana, p95 y máximo:

| | mediana | p95 | máx |
|---|---:|---:|---:|
| `FrameTime` (ms) | **15.47** | 523.72 | 49522.52 |
| `GameThreadTime` (ms) | 9.33 | 25.24 | 5272.48 |
| `RenderThreadTime` (ms) | **15.31** | 503.67 | 41963.51 |
| `GPUTime` (ms) | 10.85 | 19.57 | 312.91 |
| `RHI/DrawCalls` | **4256** | 6034 | 9792 |
| `RHI/PrimitivesDrawn` | 1 521 325 | 1 995 003 | 9 449 075 |
| `Basic/TicksQueued` | 215 | 2279 | 2280 |

## Lectura

1. **~65 FPS de mediana** (15.47 ms). Mejor que los 48 FPS del
   `RESUMEN_TECNICO.md`, pero ese dato es de 5.4 y de otra escena: no cantes
   victoria, cuenta como punto de partida nuevo.

2. **El cuello de botella es el render thread, no la GPU.** `RenderThreadTime`
   (15.31) va pegado a `FrameTime` (15.47) mientras la GPU se queda en 10.85.
   O sea que sobra GPU y falta hilo de render — que es exactamente lo que
   producen los draw calls, no los shaders.

3. **4256 draw calls de mediana frente a los ~819 de referencia: 5,2 veces
   más.** Es el hallazgo importante. Encaja con la FASE 5.1 del plan: quedan
   `SpawnActor` en bucle sin convertir a instanciado en `GF_Edificios` (15),
   `GF_Vegetacion` (13), `GF_Trafico` (9) y `GF_Carreteras` (6), y 97
   `LoadObject` repartidos por `Plugins/*/Private`.

4. **Los máximos son compilación, no simulación.** `FrameTime` máximo de 49,5 s
   y p95 de 523 ms son los hitches de compilar shaders y texturas la primera
   vez. Para medir de verdad hace falta una segunda pasada con la DDC ya
   caliente. **Estos p95/máx no valen como referencia todavía.**

5. La sesión **no llegó a los 240 s**: murió en el frame 903 (~90 s) sin volcado
   de crash ni cierre limpio. El log deja el motivo probable a la vista:

   ```
   BEWARE: AssetCompile memory estimate is greater than available, but we're
   running it [TextureDerivedData] anyway!
   RequiredMemory = 5547.67 MiB, MemoryLimit = 5190.96 MiB
   ```

   Una sola normal map (`T_GV_Shrub_Typ1_N`, 8192², BC6H) pide 5,5 GB para
   comprimirse, por encima del límite que el propio motor se pone. Es el primer
   objetivo de la fase de memoria: esa textura no necesita 8192².

## Cómo repetir

```bat
UnrealEditor.exe "AlsasuaSimulator.uproject" /Game/Maps/L_Alsasua ^
    -game -windowed -ResX=1280 -ResY=720 -NOSPLASH -log
```

El CSV sale solo en `Saved/Profiling/CSV/`. Para leerlo, buscar las columnas
**por nombre**: son 342 y las posiciones cambian entre runs.

**Ojo con lanzarlo desde Git Bash**: MSYS reescribe `/Game/Maps/L_Alsasua` a
`C:/Program Files/Git/Game/Maps/L_Alsasua` y el mapa no carga, sin decir por
qué. Usa PowerShell o `cmd`.
