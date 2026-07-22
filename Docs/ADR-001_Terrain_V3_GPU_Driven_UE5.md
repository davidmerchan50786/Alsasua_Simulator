# ADR-001: Mosaico V3 — GPU-Driven Terrain & JIT Collisions (UE5 Adaptation)

Terreno GPU-driven que prescinde del Landscape de UE5, manteniendo intacto el contrato
Core `ITerrainService` / `TerrenoGlobal` (alturas matematicas) que consume todo el juego.

**Estado (2026-07-22):** Adaptacion a UE5 del diseno Unity V3. No implementado.
**Depende del:** Omni-Grid (Core, Fase 0/1) para JIT collisions.

---

## 0. Que reemplaza y que se mantiene

| Pieza | Hoy (V2 — Unity) | V3 UE5 |
|-------|-------------------|--------|
| Render del suelo | 48 Landscape tiles → 48+ draw calls | 1–2 draw calls via clipmap GPU + `FMeshDrawCommand` indirect |
| Colision | 48 LandscapeColliders siempre activos | JIT `UBodySetup` + `UProceduralMeshComponent` solo bajo entidades fisicas (pool) |
| Mezcla de materiales | splatmap 8 capas tope | `UTexture2DArray` + Runtime Virtual Texture, capas "infinitas" |
| Alturas CPU | `Landscape::SampleHeight` (tile-aware) | igual contrato, muestreo matematico del RAW en RAM |
| Contrato Core | `ITerrainService` / `TerrenoGlobal` | **SIN CAMBIOS** — los ~20 lectores no se enteran |

**Principio rector:** V3 es un nuevo proveedor dentro de `ServicioTerreno`
(`FuenteTerreno::MosaicGPU`), no un sistema paralelo. La maquina de estados
(`Inicializando → Generando → Listo`), `TerrenoListoEvent` y la cadena de fallback
se conservan; si la GPU/RVT no esta disponible, cae al Mosaico V2 con Landscape.
**Cero regresion por diseno.**

---

## 1. Separacion fisica: render (GPU) vs alturas (CPU)

La clave es desacoplar la altura matematica del render:

```
RAW uint16 (lattice 1/64) en disco  ──┬──> CPU: TArray<uint16> por tile (RAM)
  manifest_v2.json (48 tiles)         |        └─ MuestreadorAlturaMosaico → ITerrainService.AlturaMundo
                                      |           (decode: alturaMundo = tile.y + q/64, bilineal)
                                      └──> GPU: UTexture2D(PF_R16) / heightmap atlas (VRAM)
                                                 └─ clipmap vertex shader (solo VISUAL)
```

- **Alturas** (gameplay, IA, spawn, NavMesh, arboles): nunca tocan la GPU.
  Se resuelven leyendo el mismo RAW ya cargado en RAM, con el decode lattice
  que documenta `MosaicoManifest` (`y_tile + q/64`). Mas rapido y preciso que
  `Landscape::SampleHeight` y elimina la dependencia del componente Landscape.
- **Render**: el heightmap vive en VRAM; el vertex shader desplaza una malla teselada.
  Que el render exista o no es irrelevante para la simulacion.

```cpp
// Alturas = matematica pura sobre RAM. (AlsasuaCore)
class FMuestreadorAlturaMosaico : public IMuestreadorAlturaPrecisa
{
    TArray<TArray<uint16>> TilesRaw;   // un RAW por tile, Persistent
    const FMosaicoManifest* Man = nullptr;

    virtual float AlturaMundo(const FVector& P) const override
    {
        int32 T = TileEn(P.X, P.Z);                     // O(1): indice por anillo + rejilla
        if (T < 0) return Man->DatumYBase;              // fuera → datum
        const FMosaicoTile& Tile = Man->Tiles[T];
        float U = (P.X - Tile.X) / Tile.Ancho * (Tile.Res - 1);
        float V = (P.Z - Tile.Z) / Tile.Ancho * (Tile.Res - 1);
        uint16 Q = MuestreoBilineal(TilesRaw[Tile.Id], Tile.Res, U, V);
        return Tile.Y + Q / 64.0f;                      // alturaMundo bit-exacta
    }
};
```

`ITerrainService::Terreno` puede devolver `nullptr` en V3 (no hay Landscape);
los lectores ya hacen null-check. Se anyade `FuenteTerreno::MosaicGPU` al enum.

---

## 2. Pilar 1 — Render: Geometry Clipmap GPU + DrawMeshIndirect

### 2.1 Clipmap (anillos concentricos que siguen a la camara)

En vez de 48 mallas gigantes, una sola malla teselada en anillos centrada en la camara:

- **L niveles** (p.ej. 6): el nivel 0 = parche fino (1 m) junto al jugador;
  cada nivel exterior duplica el tamano de celda (2, 4, 8... m) y cubre 4x area →
  resolucion cae con la distancia sin costuras (los anillos encajan por construccion).
- La malla es estatica (un `FMeshBatch` de vertices en espacio local por nivel +
  stitching ring para unir niveles); solo cambia un `_CamPos` y un snap por nivel
  (cuantizado al tamano de celda → sin shimmer).
- El vertex shader muestrea el heightmap atlas (VRAM) y desplaza Y. La normal
  se calcula por diferencias finitas del heightmap (3 samples).

```hlsl
// Vertex Shader (Custom Node en Material o FGlobalShader):
float2 WorldXZ = LocalToWorldClip(IN.PositionOS, _CamSnap, _NivelEscala);
float  H = SampleHeightAtlas(WorldXZ);              // PF_R16, lattice decode en GPU
float3 N = NormalFromHeight(WorldXZ, _TexelSize);   // diferencias finitas
Out.PositionWS.Y = H;
```

### 2.2 1–2 draw calls? DrawMeshIndirect

- Todos los anillos del clipmap → una `FMeshDrawCommand` con
  `FRHICommandList::SubmitMeshDrawCommands` + `FMeshDrawIndirectArgs`
  (instancia por parche de anillo).
- Para el quadtree adaptativo (LOD por pendiente/distancia), un
  **Compute Shader** (`FGlobalShader`) de culling + seleccion de LOD
  escribe el `FRHIBuffer` de argumentos indirectos (que parches dibujar) →
  la CPU no itera tiles.
- Alternativa **Batched Renderer** (BRG): si ya hay un cull GPU global,
  los parches del clipmap se registran como batch y `OnPerformCulling`
  filtra por frustum. Para el terreno el clipmap+indirect es mas simple.

```cpp
// Compute Shader de culling de clipmap (FGlobalShader)
class FClipmapCullCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FClipmapCullCS);
    // ...
    void Execute(const FClipmapCullParams& Params)
    {
        // Por cada parche candidato: frustum test + LOD selection
        // Escribe FMeshDrawIndirectArgs en UAV buffer
    }
};
```

### 2.3 Heightmap a VRAM

- `ServicioTerreno` (Systems) sube cada RAW a un `UTexture2D(PF_R16, MipGen)` y
  los compone en un atlas (o `UTexture2DArray` por anillo). Multi-resolucion V2
  (anillo 0 @0.59 m, anillo 2 @3.5 m) → mapea a niveles del clipmap directamente.
- Subida asincrona (`ENQUEUE_RENDER_COMMAND` + `RHICreateTexture2D` en GameThread
  + `RHIUpdateTexture` escalonada por tile), igual que `CargadorMosaicoTerreno`
  hace hoy con Landscape (anillo 0 primero).

```cpp
ENQUEUE_RENDER_COMMAND(UploadHeightmapTile)(
    [TextureRef, RawData = MoveTemp(TileData)](FRHICommandListImmediate& RHICmdList)
    {
        // UpdateTextureRegion en hilo de render — sin stall del GameThread
        FRHITexture* Tex = TextureRef->GetResource()->TextureRHI;
        RHICmdList.UpdateTexture(Tex, 0, 0, FUpdateTextureRegion2D(0,0,0,0, Res, Res), RawData);
    }
);
```

---

## 3. Pilar 2 — Splatmapping sin limites (Texture2DArray + RVT)

- **Index Map** (`UTexture2D` PF_R8G8B8A8): textura por tile donde cada texel
  guarda hasta 4 indices+pesos de capa (RGBA = id capa / blend). Sustituye al
  limite de 8 capas del Landscape.
- **`UTexture2DArray`** de materiales PBR (Albedo/Normal/MaskMap ARM), todas las
  texturas de `Content/Textures_AAA/TerrainLayers/` como slices. El pixel shader
  muestrea el array por el indice del index map → N capas con coste fijo
  (4 samples, no 8×N).
- **Runtime Virtual Texture** (RVT): las slices grandes (orto 0.25 m/px) van por
  `URuntimeVirtualTexture` → solo se residen en VRAM los tiles visibles.
  Transicion fotorrealista asfalto (anillo 0) ↔ roca/barro (anillo 2) mezclando
  dos indices del index map con factor de pendiente/altura, sin romper el
  presupuesto de muestreos por pase.
- Implementacion: Material con Custom Node HLSL para muestreo indexado del array.

```hlsl
// Muestreo de 4 capas indexadas desde Texture2DArray (1 pase):
half4 Albedo = 0; half3 Nrm = 0;
[unroll] for (int K = 0; K < 4; K++)
{
    int   Capa = Idx[K];                         // del index map
    half  W    = Pesos[K];
    Albedo += W * Texture2DSampleArray(_AlbedoArray, _AlbedoArraySampler, UVTriplanar, Capa);
    Nrm    += W * UnpackNormal(Texture2DSampleArray(_NormalArray, _NormalArraySampler, UVTriplanar, Capa)).xyz;
}
```

---

## 4. Pilar 3 — JIT Collisions (Chaos Physics + Omni-Grid)

Sin LandscapeCollider, el coche y los NPCs necesitan colision. La generamos
solo donde hay entidades fisicas, en celdas de 64×64 m, con pool — y quien la
necesita lo dice el Omni-Grid.

### 4.1 Quien pide colision → Omni-Grid

Las entidades fisicas (jugador, vehiculos, policias, props grandes) ya se
inyectan en el Omni-Grid con `TipoEspacial::Jugador | Vehiculo | Policia`.
Cada frame, `ServicioTerreno` consulta:

```cpp
// Celdas que necesitan collider = union de discos alrededor de entidades fisicas
TArray<uint32> CeldasCalientes;
Grid.QueryRadio(JugadorPos, RADIO_FISICA, TipoEspacial::Jugador | TipoEspacial::Vehiculo, CeldasCalientes);
// → de cada entidad, su celda 64×64 + las 8 vecinas (margen para velocidad)
```

El set de "celdas calientes" es pequeno y estable; el grid lo da en O(k).

### 4.2 Pipeline async (AsyncTask + Chaos Physics Cooking)

Por celda nueva caliente:

- **Task A** (`FAutoDeleteAsyncTask<FCookTerrainPatchJob>`):
  genera vertices+indices del parche 64×64 muestreando el RAW (lattice)
  a resolucion de fisica (1 m → 65×65 verts). Escribe en `TArray<FVector>`
  + `TArray<int32>`.
- **Task B** (GameThread, barato):
  `UBodySetup::CookPhysicsData()` + asigna a `UProceduralMeshComponent`.
  El cooking ya corre en worker threads via `FBodyInstance::SetMesh`.

```cpp
class FCookTerrainPatchJob : public IAsyncTask
{
    virtual void DoWork() override
    {
        // 1. Muestrear RAW → TArray<FVector> Vertices, TArray<int32> Triangles
        // 2. UBodySetup::CookPhysicsData(Vertices, Triangles, ...) — Burst-free
    }
};
```

- `UBodySetup` ya maneja convex decomposition y cooking internamente.
- Asignar el mesh ya horneado al `UProceduralMeshComponent` en GameThread
  es casi gratis → **cero hitch**.

### 4.3 Pool de "Physics Patches" (ciclo de vida en ServicioTerreno)

```cpp
class FParchesFisica
{
    TStack<AActor*> Libres;                                         // pool de GO con ProceduralMesh
    TMap<uint32, AActor*> Activos;                                  // Morton celda → parche

    void Sincronizar(const TArray<uint32>& CeldasCalientes)
    {
        // 1. Soltar parches que salieron del set (al pool, no DestroyActor)
        // 2. Para cada celda nueva: rentar del pool, lanzar Task A→B, asignar al completar
        // 3. Histeresis: una celda aguanta ~10 frames antes de soltarse (evita thrash al borde)
    }
};
```

- Sin asignaciones en runtime: `AActor` + `UProceduralMeshComponent` + `UBodySetup`
  se reciclan; solo se re-hornea el `UBodySetup` con datos de la nueva celda.
- Presupuesto: maximo K cooks en vuelo; el resto espera (el suelo bajo el jugador
  siempre tiene prioridad 0). Encaja con `FactorCarga` del Orquestador.
- Capa de fisica dedicada (`ECC_GameChannel1`) para que los raycasts de Foot IK
  golpeen estos parches.

---

## 5. Pilar 4 — Micro-deformacion dinamica (roderas, barro, nieve)

### 5.1 Mapa de deformacion delta (clipmap toroidal centrado en el jugador)

- Un `UTextureRenderTarget2D` R16 local (1024×1024 @ 0.25 m = 256 m alrededor
  del jugador), direccionado toroidalmente (`TA_Clamp`): al moverse el jugador,
  solo se limpia la franja que entra (no se recopia todo).
- Es un delta: `alturaFinal = alturaBase(heightmap) + delta(deformMap)`.
  El terreno base no se modifica (bit-exacto en disco).

```cpp
// Setup del deform map toroidal
FTextureRenderTarget2D* DeformMap = NewObject<FTextureRenderTarget2D>();
DeformMap->InitCustomFormat(1024, 1024, PF_R16F, false);  // R16 float para delta con signo
DeformMap->AddressX = TA_Clamp;                           // toroidal via UV offset
DeformMap->AddressY = TA_Clamp;
```

### 5.2 Escritura desde las ruedas (Compute Shader dispatch)

`AAlsasuaPlayerController` / el vehiculo, en el punto de contacto de cada rueda,
dispara un brush (**Compute Shader** `FGlobalShader`) que resta profundidad
en el deform map (rodera) con forma del neumatico y acumulacion.

```hlsl
// Compute Shader de deformacion (brush de rodera)
[numthreads(8, 8, 1)]
void DeformBrushCS(
    uint3 Id : SV_DispatchThreadID,
    RWTexture2D<float> DeformMap,
    float2 WheelUV,
    float  BrushRadius,
    float  Depth)
{
    float2 Offset = (float2(Id.xy) / 1024.0) - WheelUV;
    float  Dist = length(Offset);
    if (Dist > BrushRadius) return;

    float Falloff = 1.0 - saturate(Dist / BrushRadius);
    float Current = DeformMap[Id.xy];
    DeformMap[Id.xy] = min(Current, -Depth * Falloff);   // idempotente, acumulativo
}
```

### 5.3 Lectura: vertices + normales en tiempo real

- El mismo vertex shader del clipmap suma delta a la altura cuando el vertice
  cae dentro del deform map, y recalcula la normal por diferencias finitas del
  (base+delta) → roderas con sombreado correcto, gratis en el render.
- **Fisica opcional** (avanzado): el Task A de las JIT collisions puede sumar
  el delta al hornear el parche bajo el coche → roderas que tambien se sienten.
  v1: deformacion visual; la version fisica queda como extension.

---

## 6. Integracion y orden de capas (UE5)

- **AlsasuaWorld** (`ServicioTerreno`): dueno de heightmap VRAM, splat/RVT,
  pool de fisica, deform map.
- **AlsasuaCore** (`ITerrainService / TerrenoGlobal`): contrato intacto;
  `AlturaMundo` ahora via `FMuestreadorAlturaMosaico` (matematico). El Omni-Grid
  (Core) alimenta que celdas necesitan fisica.
- **Render**: Material UE5 (Material Editor + Custom Node HLSL) + clipmap via
  `FMeshDrawCommand` indirect. Usa `FMeshPassProcessor` para el draw path.
- **MultiTileTerrainEdit** (escritores del terreno, rios/excavacion): pasa a
  escribir en el RAW en RAM + invalidar el tile en VRAM (en vez de
  `FLandscapeDataAccess::SetHeight`). Kernels idempotent `min()` se conservan.

### UE5: Pipeline de render del clipmap

```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────────┐
│  FClipmapCullCS  │───>│ FMeshDrawCommand │───>│ FRHICommandList     │
│  (Compute)       │    │ (Indirect Args)  │    │ SubmitMeshDrawCmds  │
└─────────────────┘    └──────────────────┘    └─────────────────────┘
         │                        │
         │ Frustum + LOD          │ Vertex displacement
         ▼                        ▼
  ┌──────────────┐       ┌──────────────────┐
  │ FRHIBuffer   │       │ Material Clipmap │
  │ IndirectArgs │       │ (Custom HLSL)    │
  └──────────────┘       └──────────────────┘
                                  │
                           SampleHeightAtlas()
                                  │
                           ┌──────────────────┐
                           │ UTexture2D(R16)  │
                           │ Heightmap Atlas  │
                           └──────────────────┘
```

---

## 7. Riesgos y mitigaciones (UE5)

| Riesgo | Mitigacion |
|--------|-----------|
| Clipmap/RVT es mucho shader nuevo | V3 es proveedor opcional; fallback a V2 Landscape intacto. Detras de un `CVar` toggle (`r.Alsasua.TerrainV3`). |
| `UBodySetup::CookPhysicsData` puede ser caro | Cooking en `FAutoDeleteAsyncTask`; el cook se hace una vez por celda y se cachea en pool. |
| Deform map fisico re-hornea caro | v1 solo visual; fisica limitada a la celda bajo la rueda. |
| Alturas CPU deben coincidir con el bake al bit | Reusar el decode lattice exacto de `MosaicoManifest` (validado por gate Python). |
| RVT no disponible en todas las GPUs | Fallback a `UTexture2DArray` con mips manuales (sin streaming virtual). |
| `FMeshDrawIndirectArgs` requiere SM5+ | Verificar `GRHISupportsDrawMeshIndirect`; fallback a `DrawMesh` por-instance si no. |

---

## 8. Plan de fases (UE5)

1. **`FMuestreadorAlturaMosaico`** — alturas CPU desde RAW en RAM bajo
   `ITerrainService`. Validar `AlturaMundo V3 == V2` (RMSE ~0).
   - ✅ HECHO (Unity scaffold, 2026-06-15): `IMuestreadorAlturaPrecisa` +
     `MuestreadorAlturaMosaico` + `BootstrapMuestreadorAltura`.
   - **Pendiente UE5**: Adaptar a `UWorldSubsystem` + `UGameInstanceSubsystem`
     con `FStreamableManager` para carga async de RAWs.

2. **Heightmap → VRAM** (atlas `PF_R16`) + clipmap basico render-only (sin splat)
   → comparar contra Landscape. **Crear `FClipmapCullCS`** y `UMaterial` base.

3. **Splat index map + `UTexture2DArray`** (sin RVT). Material con Custom Node.

4. **JIT collisions + pool** (consume Omni-Grid). Retirar Landscape collider.
   Crear `FParchesFisica` con `UProceduralMeshComponent` pool.

5. **Runtime Virtual Texturing** para orto/capas grandes.
   Configurar `URuntimeVirtualTextureVolume` + `URuntimeVirtualTexture`.

6. **Micro-deformacion visual** — `UTextureRenderTarget2D` toroidal + Compute.
   (7) Fisica opcional de roderas.

**Gate**: ninguna fase entra sin que `AlturaMundo` y la silueta visual coincidan
con V2. El gate Python del terreno (`ValidarMosaicoV2.py`) sigue siendo la fuente
de verdad de los datos.

---

## 9. Equivalencia Unity → UE5

| Unity | UE5 | Modulo |
|-------|-----|--------|
| `Graphics.RenderMeshIndirect` | `FRHICommandList::SubmitMeshDrawCommands` | RHI |
| `GraphicsBuffer.IndirectArguments` | `FRHIBuffer` (BUF_IndirectArgs) | RHI |
| `ComputeShader` dispatch | `FGlobalShader` + `FComputeShaderUtils::Dispatch` | RenderCore |
| `Texture2D(R16)` | `UTexture2D` (PF_R16) | Engine |
| `Texture2DArray` | `UTexture2DArray` | Engine |
| SVT (`StreamingVirtualTexturing`) | Runtime Virtual Texture (`URuntimeVirtualTexture`) | Engine |
| `RenderTexture` | `UTextureRenderTarget2D` | Engine |
| `Physics.BakeMesh` | `UBodySetup::CookPhysicsData` | Engine/PhysicsCore |
| `MeshCollider` | `UProceduralMeshComponent` + `UBodySetup` | ProceduralMeshComponent |
| `Burst` + `IJobParallelFor` | `FAutoDeleteAsyncTask` / `ParallelFor` | Core |
| `NativeArray<T>` | `TArray<T>` | Core |
| `ServiceLocator` | `UGameInstanceSubsystem` | Engine |
| `GameObject` pool | `AActor` pool / `TObjectPool` | Core |
| `Shader Graph Custom Node` | Material Editor + Custom Node HLSL | Engine |
| `MonoBehaviour.Start` | `UActorComponent::BeginPlay` / `UGameInstanceSubsystem::Initialize` | Engine |
| `GameObject.FindObjectOfType` | `UGameplayStatics::FindActorOfClass` / Subsystem lookup | Engine |
| `DontDestroyOnLoad` | `UGameInstanceSubsystem` (vive toda la sesion) | Engine |
