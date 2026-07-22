# Arquitectura — Mosaico V3 (GPU-Driven Terrain & JIT Collisions)

Prompt Definitivo IV · Terreno GPU-driven que prescinde del componente Terrain de Unity, manteniendo intacto el contrato Core ITerrainService / TerrenoGlobal (alturas matematicas) que consume todo el juego.
Estado (2026-06-14): diseno. No implementado. Depende del Omni-Grid (ya en Core, Fase 0/1) para las JIT collisions.

## 0. Que reemplaza y que se mantiene

| Pieza | Hoy (V2) | V3 |
|-------|----------|-----|
| Render del suelo | 48 Terrain (CargadorMosaicoTerreno) → 48+ draw calls, memoria de heightmaps en CPU+GPU | 1–2 draw calls via clipmap GPU + RenderMeshIndirect |
| Colision | 48 TerrainCollider siempre activos | JIT MeshColliders solo bajo entidades fisicas (pool) |
| Mezcla de materiales | splatmap 8 capas tope de muestreos | Texture2DArray + SVT, capas "infinitas" |
| Alturas CPU (ITerrainService.AlturaMundo) | Terrain.SampleHeight (tile-aware en ServicioTerreno) | igual contrato, pero resuelto por muestreo matematico del RAW en RAM |
| Contrato Core | ITerrainService / TerrenoGlobal | SIN CAMBIOS — los ~20 lectores no se enteran |

Principio rector: V3 es un nuevo proveedor dentro de ServicioTerreno (FuenteTerreno.MosaicoGPU), no un sistema paralelo. La maquina de estados (Inicializando→Generando→Listo), TerrenoListoEvent y la cadena de fallback se conservan; si la GPU/SVT no esta disponible, cae al Mosaico V2 con Terrain. Cero regresion por diseno.

## 1. Separacion fisica: render (GPU) vs alturas (CPU)

La clave para "mantener intacta la capa Core" es desacoplar la altura matematica del render:

```
RAW uint16 (lattice 1/64) en disco  ──┬──> CPU: NativeArray<ushort> por tile (RAM)
  manifest_v2.json (48 tiles)         |        └─ MuestreadorAlturaMosaico → ITerrainService.AlturaMundo
                                      |           (decode: alturaMundo = tile.y + q/64, bilineal)
                                      └──> GPU: Texture2D R16 / heightmap atlas (VRAM)
                                                 └─ clipmap vertex shader (solo VISUAL)
```

- **Alturas** (gameplay, IA, spawn, NavMesh, arboles): nunca tocan la GPU. Se resuelven leyendo el mismo RAW ya cargado en RAM, con el decode lattice que ya documenta MosaicoManifest (y_tile + q/64). Esto es mas rapido y mas preciso que Terrain.SampleHeight y elimina la dependencia del componente.
- **Render**: el heightmap vive en VRAM; el vertex shader desplaza una malla teselada. Que el render exista o no es irrelevante para la simulacion.

```csharp
// Systems — implementa ITerrainService sin Terrain. Alturas = matematica pura sobre RAM.
public sealed class MuestreadorAlturaMosaico
{
    NativeArray<ushort>[] _tilesRaw;   // un RAW por tile, Persistent
    MosaicoManifest _man;

    public float AlturaMundo(Vector3 p)
    {
        int t = TileEn(p.x, p.z);                       // O(1): indice por anillo + rejilla
        if (t < 0) return _man.datumYBase;              // fuera → datum
        var tile = _man.tiles[t];
        float u = (p.x - tile.x) / tile.ancho * (tile.res - 1);
        float v = (p.z - tile.z) / tile.ancho * (tile.res - 1);
        ushort q = MuestreoBilineal(_tilesRaw[t], tile.res, u, v);  // lattice
        return tile.y + q / 64f;                        // alturaMundo bit-exacta con el bake
    }
}
```

El `ITerrainService.Terreno` (que hoy devuelve el tile ancla) puede devolver null en V3 (no hay Terrain); los lectores ya hacen null-check. Se anyade `FuenteTerreno.MosaicoGPU` al enum.

## 2. Pilar 1 — Render: Geometry Clipmap GPU + RenderMeshIndirect

### 2.1 Clipmap (anillos concentricos que siguen a la camara)

En vez de 48 mallas gigantes, una sola malla teselada en anillos centrada en la camara:

- **L niveles** (p.ej. 6): el nivel 0 = parche fino (1 m) junto al jugador; cada nivel exterior duplica el tamano de celda (2, 4, 8... m) y cubre 4x area → resolucion cae con la distancia sin costuras (los anillos encajan por construccion).
- La malla es estatica (un grid de vertices en espacio local por nivel + "stitching ring" para unir niveles); solo cambia un `_CamPos` y un snap por nivel (cuantizado al tamano de celda → sin shimmer).
- El vertex shader muestrea el heightmap atlas (VRAM) y desplaza y. La normal se calcula por diferencias finitas del heightmap (3 samples) — normales sin malla pesada.

```hlsl
// Vertex (Shader Graph Custom Function o HLSL):
float2 worldXZ = LocalToWorldClip(IN.posOS, _CamSnap, _NivelEscala);
float  h = SampleHeightAtlas(worldXZ);              // R16, lattice decode en GPU
float3 n = NormalFromHeight(worldXZ, _Texel);       // diferencias finitas
posWS.y = h;
```

### 2.2 1–2 draw calls? RenderMeshIndirect

- Todos los anillos del clipmap → una `Graphics.RenderMeshIndirect` con un `GraphicsBuffer.IndirectArguments` (instancia por parche de anillo).
- Para el quadtree adaptativo (LOD por pendiente/distancia), un compute shader de culling + seleccion de LOD escribe el buffer indirecto (que parches dibujar) → la CPU no itera tiles. El `OptimizadorVisualHDRP` ya usa `Unity.RenderPipelines.GPUDriven.Runtime` (esta en el asmdef Core).
- Reaprovechamos el know-how del `RenderizadorMultitudBRG`: buffer crudo + MetadataValue + culling en callback. El terreno es el mismo patron con un heightmap en el vertex en lugar de matrices por instancia.

**Alternativa BRG**: si se quiere unificar con el pipeline de culling del BRG, los parches del clipmap se registran como un batch y el `OnPerformCulling` filtra por frustum. Para el terreno el clipmap+indirect es mas simple; el BRG es preferible si ya hay un cull GPU global.

### 2.3 Heightmap a VRAM

- `ServicioTerreno` (Systems) sube cada RAW a un `Texture2D(R16, mipChain)` y los compone en un atlas (o `Texture2DArray` por anillo). Multi-resolucion V2 (anillo 0 @0.59 m, anillo 2 @3.5 m) → mapea a niveles del clipmap directamente.
- Subida asincrona (`Texture2D.Apply(false, false)` + `GraphicsBuffer.SetData` en hilo de carga) escalonada por tile, igual que `CargadorMosaicoTerreno` hace hoy con los Terrain (anillo 0 primero).

## 3. Pilar 2 — Splatmapping sin limites (Texture2DArray + SVT)

- **Index Map** (splatmap reinterpretado): textura por tile donde cada texel guarda hasta 4 indices+pesos de capa (RGBA = id capa / blend). Sustituye al limite de 8 capas del Terrain.
- **Texture2DArray** de materiales PBR (Albedo/Normal/MaskMap ARM), todas las texturas de `Assets/Textures_AAA/TerrainLayers/` + asfalto + roca/barro como slices. El pixel shader muestrea el array por el indice del index map → N capas con coste fijo (4 samples, no 8xN).
- **SVT** (Streaming Virtual Texturing): las slices grandes (orto 0.25 m/px, `ortofoto_alsasua_REAL.png`) van por SVT → solo se residen en VRAM los texels visibles. Transicion fotorrealista asfalto (anillo 0) ↔ roca/barro (anillo 2) mezclando dos indices del index map con un factor de pendiente/altura, sin romper el presupuesto de muestreos por pase.
- Implementacion: Shader Graph con un Custom Function HLSL para el muestreo indexado del array (Shader Graph no expone `Texture2DArray.Sample(index)` nativamente con blend de 4).

```hlsl
// Muestreo de 4 capas indexadas desde el array (1 pase):
half4 albedo = 0; half3 nrm = 0;
[unroll] for (int k = 0; k < 4; k++) {
    int  capa = idx[k];                         // del index map
    half w    = pesos[k];
    albedo += w * SAMPLE_TEXTURE2D_ARRAY(_AlbedoArray, sampler_lin, uvTriplanar, capa);
    nrm    += w * UnpackNormal(SAMPLE_TEXTURE2D_ARRAY(_NormalArray, sampler_lin, uvTriplanar, capa)).xyz;
}
```

## 4. Pilar 3 — JIT Havok Collisions (consume el Omni-Grid)

Sin TerrainCollider, el coche y los NPCs necesitan colision. La generamos solo donde hay entidades fisicas, en celdas de 64x64 m, con pool — y quien la necesita lo dice el Omni-Grid.

### 4.1 Quien pide colision → Omni-Grid

Las entidades fisicas (jugador, vehiculos, policias, props grandes) ya se inyectan en el Omni-Grid con `TipoEspacial.Jugador | Vehiculo | Policia | PropDestruible`. Cada frame, ServicioTerreno consulta:

```csharp
// celdas que necesitan collider = union de discos alrededor de entidades fisicas
grid.QueryRadio(jugador.pos, RADIO_FISICA, TipoEspacial.Jugador | TipoEspacial.Vehiculo, _fisicas);
// → de cada entidad, su celda 64x64 + las 8 vecinas (margen para velocidad)
```

El set de "celdas calientes" es pequeno y estable; el grid lo da en O(k).

### 4.2 Pipeline async (Burst + Jobs + Physics.BakeMesh)

Por celda nueva caliente:

- **Job A** (Burst, IJobParallelFor): genera vertices+indices del parche 64x64 muestreando el RAW (lattice) a resolucion de fisica (p.ej. 1 m → 65x65 verts)
- **Job B** (Burst): `Physics.BakeMesh(meshId, ...)` ← BakeMesh SI es Burst/Job-safe
- **Main thread** (barato): `meshCollider.sharedMesh = mesh` (ya horneado) → sin stall

`Physics.BakeMesh` (el paso caro) corre en worker threads; asignar el mesh ya horneado al MeshCollider en el hilo principal es casi gratis → cero hitch.

### 4.3 Pool de "Physics Patches" (ciclo de vida en ServicioTerreno)

```csharp
sealed class ParchesFisica
{
    readonly Stack<GameObject> _libres = new();              // pool de GO con MeshCollider
    readonly Dictionary<uint, GameObject> _activos = new();  // Morton celda → parche

    public void Sincronizar(NativeList<uint> celdasCalientes)
    {
        // 1. soltar parches que salieron del set (al pool, no Destroy)
        // 2. para cada celda nueva: rentar del pool, lanzar Job A→B, asignar al completar
        // 3. histeresis: una celda aguanta unos frames antes de soltarse (evita thrash al borde)
    }
}
```

- Sin asignaciones en runtime: GO + MeshCollider + Mesh se reciclan; solo se re-hornea el Mesh con datos de la nueva celda.
- Presupuesto: maximo K bakes en vuelo; el resto espera (el suelo bajo el jugador siempre tiene prioridad 0). Encaja con `FactorCarga` del Orquestador (bajo presion, K baja).
- Capa de fisica dedicada para que los raycasts de Foot IK (Prompt III) golpeen estos parches.

## 5. Pilar 4 — Micro-deformacion dinamica (roderas, barro, nieve)

### 5.1 Mapa de deformacion delta (clipmap toroidal centrado en el jugador)

- Un `RenderTexture/GraphicsBuffer` R16 local (p.ej. 1024² @ 0.25 m = 256 m alrededor del jugador), direccionado toroidalmente (wrap): al moverse el jugador, solo se limpia la franja que entra (no se recopia todo).
- Es un delta: `alturaFinal = alturaBase(heightmap) + delta(deformMap)`. El terreno base no se modifica (sigue bit-exacto en disco).

### 5.2 Escritura desde las ruedas (compute dispatch)

`ControladorJugador` / el vehiculo, en el punto de contacto de cada rueda, dispara un brush (compute shader) que resta profundidad en el deform map (rodera) con forma del neumatico y acumulacion (mas pasadas → surco mas hondo). Nieve = blanco que se desplaza; barro = oscurecimiento + normal.

```
Rueda (mundo) → UV del deform map (toroidal) → ComputeDispatch(brush)
   deformMap[uv] = min(deformMap[uv], -profundidad·presion)   // idempotente, acumulativo
```

### 5.3 Lectura: vertices + normales en tiempo real

- El mismo vertex shader del clipmap suma delta a la altura cuando el vertice cae dentro del deform map, y recalcula la normal por diferencias finitas del (base+delta) → roderas con sombreado correcto, gratis en el render.
- **Fisica opcional** (avanzado): el Job A de las JIT collisions puede sumar el delta al hornear el parche bajo el coche → roderas que tambien se sienten. v1: deformacion visual; la version fisica queda como extension (re-hornear es caro; limitar a la celda bajo la rueda).

## 6. Integracion y orden de capas

- **Systems** (`ServicioTerreno`): dueno de heightmap VRAM, splat/SVT, pool de fisica, deform map.
- **Core** (`ITerrainService/TerrenoGlobal`): contrato intacto; `AlturaMundo` ahora via `MuestreadorAlturaMosaico` (matematico). El Omni-Grid (Core) alimenta que celdas necesitan fisica.
- **Render**: HDRP custom (Shader Graph + clipmap). Usa `Unity.RenderPipelines.GPUDriven.Runtime` (ya referenciado).
- **MultiTileTerrainEdit** (escritores del terreno, rios/excavacion): pasa a escribir en el RAW en RAM + invalidar el tile en VRAM (en vez de `TerrainData.SetHeights`). Kernels idempotent `min()` se conservan.

## 7. Riesgos y mitigaciones

| Riesgo | Mitigacion |
|--------|-----------|
| Clipmap/SVT es mucho shader nuevo | V3 es proveedor opcional; fallback a V2 Terrain intacto. Detras de un toggle. |
| Physics.BakeMesh en job: version de API | Verificar firma Job-safe en la version de Physics del proyecto; si no, bake en worker Task + ensamblar en main. |
| Deform map fisico re-hornea caro | v1 solo visual; fisica limitada a la celda bajo la rueda. |
| Alturas CPU deben coincidir con el bake al bit | Reusar el decode lattice exacto de MosaicoManifest (ya validado por el gate Python). |
| SVT no disponible en la plataforma | Fallback a Texture2DArray con mips (sin streaming). |

## 8. Plan de fases

1. **MuestreadorAlturaMosaico** (alturas CPU desde RAW en RAM) bajo ITerrainService — sustituye SampleHeight sin tocar render. Validar AlturaMundo V3 == V2 (RMSE ~0).
   - ✅ HECHO (scaffold, 2026-06-15): `IMuestreadorAlturaPrecisa.cs` (Core, contrato AlturaMundo/NormalMundo independiente de Terrain) + `MuestreadorAlturaMosaico.cs` (Systems, carga RAW de los 48 tiles a RAM en background con presupuesto por frame, decode lattice 1/64 bilineal, registra el servicio en ServiceLocator cuando esta listo). Es opt-in (~126 MB en RAM) y aditivo: ITerrainService.AlturaMundo sigue siendo la entrada por defecto; los consumidores que necesiten precision bit-exacta (Foot IK avanzado, spawn reproducible, NavMesh) hacen `ServiceLocator.Get<IMuestreadorAlturaPrecisa>()?.AlturaMundo(p)` con fallback. Compila limpio (0 errores).
   - ✅ HECHO (2026-06-15): `SistemaFootIK.cs` es el primer consumidor real — `ResolverPie()` pide `ServiceLocator.Get<IMuestreadorAlturaPrecisa>()`. Compila limpio.
   - ✅ HECHO (2026-06-15): `BootstrapMuestreadorAltura.cs` (Systems) — unico punto de decision para activar MuestreadorAlturaMosaico. Compila limpio.
   - Pendiente: validacion RMSE V3==V2 en Play.

2. **Heightmap → VRAM** (atlas R16) + clipmap basico render-only (sin splat) → comparar contra Terrain.

3. **Splat index map + Texture2DArray** (sin SVT).

4. **JIT collisions + pool** (consume Omni-Grid). Retirar TerrainCollider.

5. **SVT** para orto/capas grandes.

6. **Micro-deformacion visual**; (7) fisica opcional de roderas.

**Gate**: ninguna fase entra sin que AlturaMundo y la silueta visual coincidan con V2. El gate Python del terreno (`ValidarMosaicoV2.py`) sigue siendo la fuente de verdad de los datos.
