# Plan de Mejora: Realismo y Fidelidad Visual
## Alsasua Simulator - Objetivo: Fotorealismo 1:1

**Fecha**: 15 de agosto de 2026  
**Estado actual**: 48 FPS, Lumen+VSM+Nanite, 1030 edificios procedurales  
**Objetivo**: Fidelidad visual idéntica a la realidad (colores, texturas, vegetación, edificios, carreteras)

---

## 1. VEGETACIÓN: INTEGRACIÓN DE ASSETS NANITE

### Estado Actual
- Sistema procedural con Poisson disc (buena distribución)
- Densidad uniforme artificial (0.5/m², 25% probabilidad global)
- Assets nuevos sin integrar: Nanite Plants, HighPoly Trees, arbustos, flores

### Mejoras a Implementar

#### 1.1 Integrar Assets Nanite Ready
```cpp
// AlsasuaManifa/Public/Environment/VegetationType.h
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Nanite")
bool bEnableNanite = true;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vegetation|Performance")
TArray<TSoftObjectPtr<UStaticMesh>> NaniteMeshes;
```

**Assets a integrar**:
- `Content/Nanite_Plants_Sample_Collection/` → arbustos, hierbas altas
- `Content/HighPoly_Tree_Model/` → árboles principales (robles, hayas)
- `Content/GV_FreeShrubsPack/` → sotobosque denso
- `Content/OWD_Flowers_Pack/` → flores silvestres (bordes carretera, prados)
- `Content/Megaplant_Library/` → plantas grandes (helechos, arbustos densos)

**Acción**: Crear Data Assets para cada especie nativa de Navarra:
- Roble pedunculado (*Quercus robur*)
- Haya (*Fagus sylvatica*)
- Abedul (*Betula pendula*)
- Avellano (*Corylus avellana*)
- Espino (*Crataegus monogyna*)

#### 1.2 Distribución Natural (No Uniforme)
```cpp
// AlsasuaManifa/Private/Environment/VegetationSpawnerSubsystem.cpp
float GetNaturalDensity(const FVector& WorldPos, const UVegetationType* Type)
{
    // Noise 3D para clustering
    float Noise3D = FMath::PerlinNoise3D(WorldPos / 5000.0f); // 50m clusters
    float ClusterMask = FMath::Clamp((Noise3D + 0.3f) / 0.6f, 0.0f, 1.0f);
    
    // Máscaras ecológicas
    float SlopeMask = GetSlopeMask(WorldPos);
    float WaterDistMask = GetWaterProximityMask(WorldPos); // 0-30m río
    float HeightMask = GetElevationMask(WorldPos);
    
    return Type->BaseDensity * ClusterMask * SlopeMask * WaterDistMask * HeightMask;
}
```

**Clustering realista**:
- Ribera del Arakil: alisos, sauces (densos 0-15m del río)
- Laderas norte: hayas (30% cobertura, clusters cada 50m)
- Laderas sur: robles dispersos (15% cobertura)
- Bordes de carretera: arbustos bajos + flores (lineal)

#### 1.3 LOD y Optimización Nanite
```cpp
UPROPERTY(EditAnywhere, Category = "Vegetation|LOD")
float NaniteTrianglePercent = 1.0f; // 100% calidad cerca, auto-reduce lejos

UPROPERTY(EditAnywhere, Category = "Vegetation|Culling")
float MaxDrawDistance = 5000.0f; // 50m árboles pequeños, 5km árboles grandes
```

---

## 2. EDIFICIOS: FOTOREALISMO DE FACHADAS

### Estado Actual
- 1030 fachadas procedurales (1 draw call c/u)
- Materiales por barrio (`M_Fachada_*`)
- Ortofoto en tejados (0.336 m/px)

### Mejoras a Implementar

#### 2.1 Materiales de Fachada Avanzados
```hlsl
// Materiales/M_Fachada_Master.usf
// Añadir paralaje occlusion mapping
float2 ParallaxOffset = ParallaxOcclusionMapping(UV, ViewDir, HeightMap, 0.05);
UV += ParallaxOffset;

// Weathering procedural (suciedad bajo ventanas, humedad base)
float DirtMask = saturate(WorldNormal.z * -2.0 + 1.0); // acumula abajo
float Wetness = GetMPCParameter("Wetness"); // del AlsasuaVisualEffectsManager
BaseColor = lerp(BaseColor, DarkenColor, DirtMask * 0.3);
Roughness = lerp(Roughness, 0.2, Wetness * 0.5);
```

**Texturas necesarias**:
- Height map para paralaje (generar de albedo)
- Dirt/grime masks (Megascans `Decals/Grime_*`)
- Normal maps detallados (ladrillo, estuco, piedra)

**Por barrio**:
- Casco viejo: piedra natural, juntas profundas, musgo en orientación norte
- Ensanche: revoque liso, colores pasteles (alineados a ortofoto)
- Zona industrial: hormigón, graffiti decals, óxido en metales

#### 2.2 Ventanas Realistas
```cpp
// AlsasuaManifa/Public/World/AlsasuaFacadeGenerator.h
struct FWindowData 
{
    // ... existente
    
    UPROPERTY()
    bool bHasCurtain = true; // 70% probabilidad
    
    UPROPERTY()
    FLinearColor CurtainColor; // random warm colors
    
    UPROPERTY()
    float ReflectionIntensity = 0.8f; // vidrio reflectante
};
```

**Material ventanas**:
- Reflexiones de entorno (Screen Space Reflections + Lumen)
- Cortinas interiores (plane simple, color variado)
- Reflejos del cielo (Cubemap azul durante día)
- Suciedad en vidrio (noise texture, 20% opacidad)

#### 2.3 Detalles de Fachada (Instanced)
```cpp
// AlsasuaManifa/Public/World/AlsasuaBuildingFacadeSystem.h
class FAlsasuaFacadeDetailGenerator
{
    void AddAirConditioners(); // 40% ventanas orientación sur
    void AddSatelliteDishes(); // 15% edificios (tejado/balcón)
    void AddFlowerPots(); // 25% balcones
    void AddAwnings(); // tiendas planta baja
    void AddGuttersAndPipes(); // canalones verticales
};
```

**Meshes a añadir** (HISM, bajo coste):
- Aires acondicionados (3 variantes)
- Antenas parabólicas (2 variantes)
- Macetas con flores (5 colores)
- Toldos de tiendas (stripe patterns)

#### 2.4 Color Matching con Ortofoto
```cpp
// Herramienta editor: ExtractBuildingColors.py
def ExtractAverageColorFromOrtophoto(BuildingID, OrtophotoTexture):
    """
    Sample 100 puntos en área de fachada de edificio en ortofoto.
    Retorna color promedio (RGB) para material MI.
    """
    pass

// Aplicar a cada MI_Fachada_XXXX
MI->SetVectorParameterValue("TintColor", SampledColor);
```

---

## 3. CARRETERAS: REALISMO DE SUPERFICIES

### Estado Actual
- JSON con datos de vías (`roads_unity.json`)
- Materiales base: `M_Carretera_Asfalto`, `M_Terreno_Acera`
- Sistemas de marcas viales y decals básicos

### Mejoras a Implementar

#### 3.1 Variación de Desgaste por Zona de Tráfico
```cpp
// AlsasuaManifa/Public/World/AlsasuaRoadSurfaceSystem.h
UENUM()
enum class ERoadWearLevel : uint8
{
    Pristine,      // carreteras secundarias
    Light,         // vías residenciales
    Moderate,      // avenidas principales
    Heavy,         // intersecciones, rotondas
    Damaged        // zonas en obras, baches
};

struct FRoadSegmentData
{
    ERoadWearLevel WearLevel;
    TArray<FVector> PotholeLocations; // baches procedurales
    float CrackDensity; // 0-1
};
```

**Material procedural**:
```hlsl
// M_Carretera_Asfalto
float CrackNoise = Noise(WorldPos / 50.0, 8); // grietas finas
float WearMask = GetRoadWearMask(WorldPos); // lookup por zona
BaseColor = lerp(FreshAsphalt, AgedAsphalt, WearMask);
Roughness = lerp(0.7, 0.9, WearMask); // más rugoso desgastado
Normal = BlendNormals(BaseNormal, CrackNormal * WearMask);
```

#### 3.2 Manchas de Aceite y Fluidos
```cpp
// AlsasuaManifa/Public/World/AlsasuaRoadDecalSystem.h
void SpawnOilStains()
{
    // Spawn en parking, gasolineras, talleres
    for (FVector ParkingSpot : GetParkingLocations())
    {
        if (FMath::FRand() < 0.3f) // 30% spots tienen mancha
        {
            SpawnDecal("M_Decal_OilStain", ParkingSpot, RandomRotation());
        }
    }
}
```

**Decals a añadir**:
- Manchas de aceite (iridiscente)
- Marcas de neumáticos (frenazos)
- Chicles en aceras (pequeños puntos negros)
- Hojas caídas en otoño (particle decals)

#### 3.3 Marcas Viales Desgastadas
```cpp
// AlsasuaRoadMarkingsSystem.cpp
void ApplyWearToMarkings()
{
    // Marcas en intersecciones: 50% opacidad
    // Marcas en rectas: 80% opacidad
    MarkingMaterial->SetScalarParameterValue("Opacity", OpacityByWear);
}
```

#### 3.4 Charcos Dinámicos (Lluvia)
```cpp
// AlsasuaManifa/Public/Environment/AlsasuaWeatherSystem.h
void UpdateRainPuddles(float DeltaTime)
{
    if (RainIntensity > 0.3f)
    {
        // Spawn charcos en depresiones de carretera
        for (FVector LowPoint : GetRoadDepressions())
        {
            PuddleSurface->SetWorldLocation(LowPoint);
            PuddleSurface->SetVisibility(true);
            // Material con ripples animados
        }
    }
}
```

**Material charco**:
- Reflexión del cielo (planar reflection)
- Ripples con normal map animado
- Opacidad 0.6 (se ve asfalto debajo)

---

## 4. ILUMINACIÓN: NOCTURNA Y VOLUMÉTRICA

### Estado Actual
- Sistema `AlsasuaNightLightingSystem`
- Emissive en ventanas (`AlsasuaBuildingEmissiveComponent`)
- Farolas básicas

### Mejoras a Implementar

#### 4.1 Farolas con Volumetric Light Beams
```cpp
// AlsasuaManifa/Public/World/AlsasuaStreetLightController.h
void SetupStreetLight(APointLight* Light)
{
    Light->SetIntensity(3000); // lumens realistas
    Light->SetLightColor(FLinearColor(1.0, 0.9, 0.7)); // cálido sodio
    Light->SetAttenuationRadius(1500); // 15m
    Light->SetSourceRadius(10); // soft shadows
    Light->bCastVolumetricShadow = true; // clave para beams
    Light->VolumetricScatteringIntensity = 2.0f; // visible en niebla
}
```

**Volumetric fog**:
```ini
; DefaultEngine.ini
r.VolumetricFog.GridPixelSize=8  ; más detalle
r.VolumetricFog.GridSizeZ=64     ; altura
r.VolumetricFog.HistoryWeight=0.9
```

#### 4.2 Ventanas con Variación Interior
```cpp
// AlsasuaBuildingEmissiveComponent.cpp
void RandomizeWindowLights()
{
    for (FWindowData& Window : Building->Ventanas)
    {
        // 60% encendidas por noche, variación de color (temperatura luz)
        bool bLightOn = FMath::FRand() < 0.6f;
        float ColorTemp = FMath::RandRange(2700, 6500); // K
        FLinearColor EmissiveColor = ColorTempToRGB(ColorTemp);
        Window.EmissiveIntensity = bLightOn ? FMath::RandRange(2.0, 8.0) : 0.0;
        Window.EmissiveColor = EmissiveColor;
    }
}
```

#### 4.3 Light Probes Densos
```cpp
// AlsasuaManifa/Public/World/AlsasuaLightProbeSystem.h
void GenerateLightProbeGrid()
{
    // Grid 5×5m en zona urbana
    // Grid 20×20m en montes
    // Auto-placement en altura media edificios
}
```

---

## 5. CLIMA Y ATMÓSFERA

### Estado Actual
- `AlsasuaWeatherSystem`: lluvia, nieve, viento
- Wetness en MPC (0-1)
- Nubes volumétricas procedurales

### Mejoras a Implementar

#### 5.1 Asfalto Mojado con Reflejos
```hlsl
// M_Carretera_Asfalto
float Wetness = GetMPCParameter("Wetness");
Roughness = lerp(0.7, 0.15, Wetness); // muy reflectante mojado
Specular = lerp(0.5, 0.9, Wetness);
// Screen Space Reflections se activan automáticamente
```

#### 5.2 Gotas en Cámara (Post-Process)
```cpp
// AlsasuaManifa/Public/Visuals/AlsasuaEnhancedPostProcessComponent.h
UPROPERTY(EditAnywhere, Category = "Weather")
UMaterialInterface* RainDropsMaterial; // M_PP_RainDrops

void UpdateRainEffect(float RainIntensity)
{
    PostProcessSettings.WeightedBlendables.Array[0].Weight = RainIntensity;
}
```

**Material PP**:
- Normal map de gotas (animadas, scroll down)
- Distorsión sutil (refraction)
- Alpha mask (gotas no cubren 100% pantalla)

#### 5.3 Niebla Dinámica por Zona
```cpp
// AlsasuaWorld/Public/AlsasuaWorldSubsystem.h
void UpdateFogByLocation()
{
    // Río Arakil: niebla densa al amanecer (6-9 AM)
    if (IsNearRiver() && TimeOfDay < 0.375f)
    {
        ExponentialHeightFog->SetFogDensity(0.02f); // denso
        ExponentialHeightFog->SetFogHeightFalloff(0.2f); // bajo
    }
    else
    {
        ExponentialHeightFog->SetFogDensity(0.005f); // normal
    }
}
```

---

## 6. DETALLES URBANOS (CLUTTER)

### Mejoras a Implementar

#### 6.1 Sistema de Basura y Residuos
```cpp
// AlsasuaManifa/Public/World/AlsasuaDetailDressingSystem.h
void SpawnUrbanClutter()
{
    // Aceras: papeles, colillas, hojas
    // Esquinas: bolsas, cartones (solo barrios industriales)
    // Parques: bancos, papeleras, farolas decorativas
    
    SpawnHISM("SM_Paper_Scatter", Locations, Density = 0.1f); // 1 cada 10m²
    SpawnHISM("SM_Leaves_Pile", Locations, Density = 0.05f); // otoño
}
```

#### 6.2 Carteles y Señalización
```cpp
void SpawnStreetSigns()
{
    // Señales tráfico (STOP, YIELD, límites velocidad)
    // Carteles tiendas (letreros luminosos noche)
    // Graffiti en zonas específicas (material decal)
}
```

**Meshes necesarios**:
- Señales DGT españolas (12 tipos comunes)
- Bancos de parque (3 variantes)
- Papeleras (2 variantes)
- Farolas decorativas (casco antiguo)

---

## 7. AUDIO Y AMBIENTE

### Mejoras a Implementar

#### 7.1 Sonido Ambiental Espacializado
```cpp
// AlsasuaManifa/Public/Audio/AlsasuaAmbientAudioSystem.h
void SpawnAmbientSounds()
{
    // Río Arakil: agua fluyendo (atenuación 0-50m)
    // Carreteras: tráfico lejano (loop continuo)
    // Pájaros: gorjeo diurno (solo 6-20h)
    // Viento: intensidad variable por altura
}
```

#### 7.2 Reverb Zones
```cpp
// AlsasuaReverbZoneSystem.cpp
void SetupReverbZones()
{
    // Calles estrechas: reverb medio (RT60 = 1.2s)
    // Plazas abiertas: reverb bajo (RT60 = 0.3s)
    // Túneles/puentes: reverb alto (RT60 = 2.5s)
}
```

---

## 8. RAY-TRACING HARDWARE (Opcional, GPU RTX)

### Configuración
```ini
; DefaultEngine.ini
[/Script/Engine.RendererSettings]
r.RayTracing=True
r.RayTracing.Shadows=True
r.Lumen.HardwareRayTracing=True
r.Lumen.HardwareRayTracing.LightingMode=2  ; hit lighting
r.RayTracing.Reflections.MaxRoughness=0.6
```

**Beneficios**:
- Sombras precisas en contacto (Contact Shadows mejoradas)
- Reflejos en vidrios/agua (sin artefactos SSR)
- Oclusión ambiental exacta
- Lumen más preciso (menos leaking)

**Coste**: -15 FPS estimado (48 → 33 FPS en RTX 3060)

---

## 9. COLOR GRADING Y POST-PROCESS

### 9.1 LUTs por Barrio
```cpp
// AlsasuaManifa/Public/Visuals/AlsasuaBarrioStyleSystem.h
UENUM()
enum class EBarrioStyle : uint8
{
    CascoViejo,    // LUT cálido, saturación +10%
    Ensanche,      // LUT neutro
    Industrial,    // LUT frío, contraste +15%
    Rural          // LUT natural, verde +5%
};

void ApplyBarrioLUT(EBarrioStyle Style)
{
    PostProcess->Settings.ColorGradingLUT = LUTs[Style];
}
```

### 9.2 Matched Grading a Ortofoto
```python
# Tools/GenerateMatchedLUT.py
def MatchColorToOrthophoto():
    """
    Captura screenshot in-game, compara histograma RGB con ortofoto.
    Genera LUT para que coincidan colores.
    """
    pass
```

---

## 10. CORRECCIÓN DE BUGS Y CÓDIGO OBSOLETO

### 10.1 Migrar APIs Deprecated
```cpp
// Source/AlsasuaEntities/Public/IInteractuable.h → ELIMINAR
// Migrar todos los usos a:
// Source/AlsasuaEntities/Public/IInteractableInterface.h

// AlsasuaSaveGame.h
// Eliminar UDEPRECATED_AlsasuaSaveGame
// Crear nueva clase UAlsasuaSaveGameV2 con serialización optimizada
```

### 10.2 Verificar Warnings de Compilación
```bash
# Build con warnings como errores
UnrealBuildTool.exe -WarningsAsErrors
```

---

## 11. FOTOGRAMETRÍA (Edificios Emblemáticos)

### Assets CitySample
- `Content/UnrealDrive_CitySample/` contiene assets fotogramétricos

### Edificios a Reemplazar
1. Iglesia San Juan Bautista → modelo fotogramétrico
2. Ayuntamiento → modelo fotogramétrico
3. Estación de tren → modelo fotogramétrico
4. Frontón → modelo fotogramétrico

**Proceso**:
1. Capturar 200+ fotos con drone (permisos)
2. Procesar en RealityCapture / Metashape
3. Reducir a 50k triángulos con Nanite
4. Integrar en `AlsasuaWorld/Landmarks`

---

## 12. RESUMEN DE PRIORIDADES

| Prioridad | Tarea | Impacto Visual | Coste Performance |
|-----------|-------|----------------|-------------------|
| **CRÍTICA** | Integrar assets Nanite vegetación | ⭐⭐⭐⭐⭐ | Neutral (Nanite auto-LOD) |
| **CRÍTICA** | Color matching fachadas a ortofoto | ⭐⭐⭐⭐⭐ | Cero (solo texturas) |
| **ALTA** | Materiales avanzados fachadas (paralaje) | ⭐⭐⭐⭐ | -2 FPS |
| **ALTA** | Variación desgaste carreteras | ⭐⭐⭐⭐ | Cero (procedural) |
| **ALTA** | Farolas volumétricas | ⭐⭐⭐⭐ | -3 FPS |
| **MEDIA** | Charcos dinámicos lluvia | ⭐⭐⭐ | -1 FPS |
| **MEDIA** | Detalles urbanos (clutter) | ⭐⭐⭐ | -2 FPS (HISMs) |
| **MEDIA** | Ventanas con cortinas | ⭐⭐⭐ | Cero (material) |
| **BAJA** | Ray-tracing hardware | ⭐⭐⭐⭐⭐ | **-15 FPS** ⚠️ |
| **BAJA** | Fotogrametría edificios | ⭐⭐⭐⭐ | Neutral (Nanite) |

### Estimación Performance Post-Mejoras
- **Sin RT**: 48 → 40 FPS (aceptable)
- **Con RT**: 48 → 25 FPS (no viable sin DLSS)

---

## 13. PLAN DE IMPLEMENTACIÓN (Fases)

### Fase 1: Vegetación (1 semana)
- [x] Explorar proyecto
- [ ] Integrar assets Nanite
- [ ] Crear Data Assets especies nativas
- [ ] Implementar clustering natural
- [ ] Testar performance

### Fase 2: Fachadas y Edificios (1.5 semanas)
- [ ] Materiales paralaje + weathering
- [ ] Color matching ortofoto
- [ ] Detalles instanciados (aires acondicionados, etc.)
- [ ] Ventanas realistas
- [ ] Testar performance

### Fase 3: Carreteras (1 semana)
- [ ] Sistema desgaste por zona
- [ ] Decals fluidos y manchas
- [ ] Marcas viales desgastadas
- [ ] Charcos dinámicos
- [ ] Testar performance

### Fase 4: Iluminación (1 semana)
- [ ] Farolas volumétricas
- [ ] Variación ventanas nocturnas
- [ ] Light probes densos
- [ ] Testar performance

### Fase 5: Clima y Ambiente (3 días)
- [ ] Asfalto mojado reflejos
- [ ] PP gotas en cámara
- [ ] Niebla por zona
- [ ] Audio espacializado

### Fase 6: Detalles y Polish (1 semana)
- [ ] Clutter urbano
- [ ] Carteles y señales
- [ ] LUTs por barrio
- [ ] Corrección bugs deprecated
- [ ] Optimización final

### Fase 7: Fotogrametría (Opcional, 2 semanas)
- [ ] Captura fotos emblemáticos
- [ ] Procesado RealityCapture
- [ ] Integración Nanite

**Total estimado**: 6-8 semanas (sin fotogrametría)

---

## 14. MÉTRICAS DE ÉXITO

### Objetivos Cuantitativos
- FPS ≥ 35 en GPU RTX 3060 (sin RT)
- Draw calls < 2000
- Densidad vegetación: 1.5-2.0 plantas/m² (zonas boscosas)
- Color accuracy: ΔE < 5 (comparado con ortofoto)

### Objetivos Cualitativos
- ✅ Indistinguible de foto real a 10+ metros
- ✅ Materiales responden correctamente a clima
- ✅ Iluminación nocturna creíble
- ✅ Vegetación distribuida naturalmente (no grid)

---

**Documento creado**: 15/08/2026  
**Autor**: Plan generado para AlsasuaSimulator  
**Versión**: 1.0
