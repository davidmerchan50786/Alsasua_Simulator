# Instrucciones: Integración de Assets de Vegetación
## Alsasua Simulator - Especies Nativas de Navarra

---

## 📦 Assets Descargados

Ya tienes estos paquetes en `Content/`:
- ✅ `HighPoly_Tree_Model/` - Árbol de alta calidad
- ✅ `Nanite_Plants_Sample_Collection/` - Plantas Nanite (arbustos, hierbas)
- ✅ `OWD_Flowers_Pack/` - 8 tipos de flores
- ✅ `GV_FreeShrubsPack/` - Arbustos adicionales

---

## 🚀 PASO 1: Crear Data Assets de Vegetación

### Método A: Ejecutar Script Python (RECOMENDADO)

1. **Abrir Alsasua Simulator en Unreal Editor 5.8**

2. **Ejecutar el script de Python:**
   - Menú: `Tools` → `Execute Python Script`
   - Navegar a: `F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Tools\CreateVegetationAssets.py`
   - Click `Execute`

   **O desde la consola de Python en Unreal:**
   ```python
   exec(open(r'F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Tools\CreateVegetationAssets.py').read())
   ```

3. **Resultado:**
   Se crearán 8 Data Assets en `/Game/Vegetation/DataAssets/`:
   - `VT_Aliso_Ribera` - Alisos junto al río Arakil
   - `VT_Sauce_Ribera` - Sauces ribereños
   - `VT_Haya_Norte` - Hayas en laderas norte
   - `VT_Roble_Sur` - Robles en laderas sur
   - `VT_Abedul_Mixto` - Abedules dispersos
   - `VT_Arbusto_Sotobosque` - Arbustos bajo árboles
   - `VT_Flores_Silvestres` - Flores de prado
   - `VT_Hierba_Alta_Ribera` - Hierbas altas en ribera

### Método B: Crear Manualmente (Si el script falla)

1. Content Browser → Click derecho en `/Game/Vegetation/`
2. `Miscellaneous` → `Data Asset`
3. Seleccionar clase: `VegetationType`
4. Nombrar: `VT_Aliso_Ribera`
5. Configurar propiedades según tabla abajo

---

## ⚙️ PASO 2: Configurar Data Assets

Cada Data Asset tiene esta configuración:

### VT_Aliso_Ribera (Alnus glutinosa)
```
Vegetation:
  ✓ bEnabled = true
  Type Name = "Aliso (Alnus glutinosa) - Ribera"
  Seed = 1001

Prefabs:
  [0] Mesh = /Game/HighPoly_Tree_Model/Meshes/SM_HP_Tree
      Probability = 100.0

Spawning:
  Global Probability = 80%
  Density Per M2 = 1.5
  Min Distance = 3.0 m
  Height Range = (500, 550) m  <- altura sobre nivel del mar
  Slope Range = (0, 15) grados
  Scale Range = (0.9, 1.3)
  ✓ bRandomRotation = true
  Sink Amount = 0.0

Nanite:
  ✓ bEnableNanite = true

Clustering:
  ✓ bUseNaturalClustering = true
  Cluster Size = 30.0 m
  Cluster Strength = 0.7

Ecology:
  ✓ bRiverAffinity = true
  Water Distance Range = (0, 15) m  <- 100% densidad 0-15m del río
  ☐ bNorthFacing = false
  ☐ bSouthFacing = false
```

### VT_Haya_Norte (Fagus sylvatica)
```
Vegetation:
  ✓ bEnabled = true
  Type Name = "Haya (Fagus sylvatica) - Ladera Norte"
  Seed = 2001

Prefabs:
  [0] Mesh = /Game/HighPoly_Tree_Model/Meshes/SM_HP_Tree
      Probability = 100.0

Spawning:
  Global Probability = 60%
  Density Per M2 = 0.8
  Min Distance = 6.0 m
  Height Range = (600, 1200) m
  Slope Range = (10, 50) grados
  Scale Range = (1.0, 1.4)

Nanite:
  ✓ bEnableNanite = true

Clustering:
  ✓ bUseNaturalClustering = true
  Cluster Size = 50.0 m
  Cluster Strength = 0.5

Ecology:
  ☐ bRiverAffinity = false
  ✓ bNorthFacing = true  <- solo laderas orientadas al norte
  ☐ bSouthFacing = false
```

### VT_Flores_Silvestres
```
Vegetation:
  Type Name = "Flores Silvestres - Prados"
  Seed = 4001

Prefabs (8 variantes):
  [0] SM_Flower_01_1, Probability = 15%
  [1] SM_Flower_01_2, Probability = 15%
  [2] SM_Flower_02_1, Probability = 15%
  [3] SM_Flower_03_1, Probability = 15%
  [4] SM_Flower_04_1, Probability = 10%
  [5] SM_Flower_05_1, Probability = 10%
  [6] SM_Flower_06_1, Probability = 10%
  [7] SM_Flower_07_1, Probability = 10%

Spawning:
  Global Probability = 40%
  Density Per M2 = 3.0  <- denso
  Min Distance = 1.0 m
  Height Range = (520, 800) m
  Slope Range = (0, 30) grados
  Scale Range = (0.5, 0.9)

Nanite:
  ☐ bEnableNanite = false  <- flores pequeñas, no necesario

Clustering:
  ✓ bUseNaturalClustering = true
  Cluster Size = 15.0 m
  Cluster Strength = 0.9  <- clusters muy marcados
```

---

## 🌍 PASO 3: Añadir al VegetationSpawnerSubsystem

### Opción A: En el GameMode Blueprint

1. Abrir `Content/Blueprints/BP_AlsasuaGameMode`
2. En el `Event BeginPlay`:
   - Buscar nodo `Get Subsystem` → `VegetationSpawnerSubsystem`
   - Añadir nodo `Add Vegetation Type`
   - Conectar cada Data Asset creado

### Opción B: Crear Actor de Configuración

1. Crear nuevo Blueprint: `BP_VegetationConfig`
2. Añadir componente `VegetationSpawnerSubsystem` reference
3. En `Construction Script`:
   ```blueprint
   VegetationSpawner = GetWorldSubsystem(VegetationSpawnerSubsystem)
   
   VegetationTypes.Add(VT_Aliso_Ribera)
   VegetationTypes.Add(VT_Sauce_Ribera)
   VegetationTypes.Add(VT_Haya_Norte)
   VegetationTypes.Add(VT_Roble_Sur)
   VegetationTypes.Add(VT_Abedul_Mixto)
   VegetationTypes.Add(VT_Arbusto_Sotobosque)
   VegetationTypes.Add(VT_Flores_Silvestres)
   VegetationTypes.Add(VT_Hierba_Alta_Ribera)
   
   VegetationSpawner->VegetationTypes = VegetationTypes
   ```

### Opción C: Modificar Código C++ (Permanente)

Editar `Source/AlsasuaGameplay/Private/AlsasuaGameplayGameMode.cpp`:

```cpp
void AAlsasuaGameplayGameMode::BeginPlay()
{
    Super::BeginPlay();
    
    UVegetationSpawnerSubsystem* VegSpawner = GetWorld()->GetSubsystem<UVegetationSpawnerSubsystem>();
    
    if (VegSpawner)
    {
        TArray<FString> VegetationAssetPaths = {
            "/Game/Vegetation/DataAssets/VT_Aliso_Ribera",
            "/Game/Vegetation/DataAssets/VT_Sauce_Ribera",
            "/Game/Vegetation/DataAssets/VT_Haya_Norte",
            "/Game/Vegetation/DataAssets/VT_Roble_Sur",
            "/Game/Vegetation/DataAssets/VT_Abedul_Mixto",
            "/Game/Vegetation/DataAssets/VT_Arbusto_Sotobosque",
            "/Game/Vegetation/DataAssets/VT_Flores_Silvestres",
            "/Game/Vegetation/DataAssets/VT_Hierba_Alta_Ribera"
        };
        
        for (const FString& AssetPath : VegetationAssetPaths)
        {
            UVegetationType* VegType = Cast<UVegetationType>(
                StaticLoadObject(UVegetationType::StaticClass(), nullptr, *AssetPath)
            );
            
            if (VegType)
            {
                VegSpawner->VegetationTypes.Add(VegType);
            }
        }
        
        VegSpawner->SetTargetLandscape(GetWorld()->GetLandscape());
    }
}
```

---

## 🌱 PASO 4: Generar Vegetación

### En Editor (Testing)

1. Abrir el mapa principal: `Content/Maps/Alsasua_Main`

2. Encontrar `VegetationSpawnerSubsystem`:
   - Window → Developer Tools → Output Log
   - Consola de comandos: `~` (tilde)

3. Ejecutar comando:
   ```
   py VegetationSpawnerSubsystem.SpawnAllVegetation()
   ```
   
   **O en Blueprint:**
   - Crear botón temporal en el nivel
   - `On Clicked` → `Get VegetationSpawnerSubsystem` → `Spawn All Vegetation`

### En Runtime (Juego)

El sistema debería auto-ejecutarse en `BeginPlay` si configuraste el GameMode.

---

## 🎨 PASO 5: Ajustes Visuales

### Reemplazar Meshes Placeholder

Algunos Data Assets usan `SM_HP_Tree` como placeholder. Reemplázalos por meshes específicos:

**Alisos y Sauces** (ribera):
- Buscar en Marketplace: "Wetland Trees" o "Riparian Vegetation"
- O usar `CitySample` trees con tint verde oscuro

**Hayas** (sombra, ladera norte):
- Buscar "Beech Tree" o usar HighPoly con material oscuro
- Follaje denso, copa redondeada

**Robles** (sol, ladera sur):
- Buscar "Oak Tree"
- Follaje menos denso, copa irregular

### Configurar Materiales

Editar materiales de cada mesh:
- Habilitar `Two Sided Foliage = true`
- Subsurface scattering para hojas
- Wind animation (conectar a `AlsasuaVisualEffectsManager` → Wind)

---

## 📊 PASO 6: Optimización

### Verificar Performance

Después de generar vegetación:

1. Consola: `stat fps`
2. Consola: `stat unit`
3. Consola: `stat scenerendering`

**Objetivo:** Mantener ≥ 35 FPS

### Si FPS < 35:

#### Reducir Densidad Global
```
VT_Aliso_Ribera:
  Global Probability: 80% → 60%
  Density Per M2: 1.5 → 1.0
```

#### Aumentar Min Distance
```
VT_Flores_Silvestres:
  Min Distance: 1.0 → 2.0 m
```

#### Reducir Altura Máxima (menos vegetación en montes)
```
VT_Haya_Norte:
  Height Range: (600, 1200) → (600, 900)
```

#### Deshabilitar Especies Secundarias
```
VT_Abedul_Mixto:
  bEnabled = false  <- temporalmente
```

### Verificar Nanite

Consola:
```
r.Nanite.ShowStats 1
```

Deberías ver triángulos virtualizados bajando automáticamente con la distancia.

---

## 🐛 TROUBLESHOOTING

### Error: "No se pudo crear Data Asset"

**Solución:**
1. Verificar que `PythonScriptPlugin` está habilitado en `.uproject`
2. Crear carpeta manualmente: Content Browser → New Folder → "Vegetation/DataAssets"
3. Reintentar script

### Error: "Mesh no encontrado"

**Solución:**
1. Verificar que los paquetes están en Content:
   ```
   Content/HighPoly_Tree_Model/Meshes/SM_HP_Tree.uasset
   Content/OWD_Flowers_Pack/Meshes/SM_Flower_01_1.uasset
   ```

2. Si faltan, reimportar paquetes desde Marketplace

3. Actualizar rutas en el script Python

### Vegetación no aparece en el juego

**Checklist:**
- ☐ `VegetationTypes` array tiene los 8 assets
- ☐ `TargetLandscape` está asignado
- ☐ `bEnabled = true` en cada Data Asset
- ☐ `SpawnAllVegetation()` fue ejecutado
- ☐ Altura del terreno está en rango (`HeightRange`)

**Debug:**
```cpp
// Output Log mostrará:
Vegetation Spawner: VT_Aliso_Ribera spawned 1523 instances
Vegetation Spawner: VT_Haya_Norte spawned 847 instances
...
```

### FPS muy bajo después de spawning

**Solución inmediata:**
```
VegetationSpawner->ClearAllVegetation()
```

Luego reducir densidades (ver Optimización arriba).

---

## 📈 Resultados Esperados

### Ribera del Arakil (500-550m)
- **Alisos densos** (0-15m del río)
- **Sauces dispersos** (0-20m)
- **Hierba alta** (5-30m)
- **Flores silvestres** en márgenes

### Laderas Norte (600-1200m)
- **Hayas dominantes** (clusters de 50m)
- **Arbustos sotobosque** bajo hayas
- Orientación: Norte/Noroeste

### Laderas Sur (580-1000m)
- **Robles dispersos** (clusters de 60m)
- Menos densidad que norte (sol más fuerte)
- Orientación: Sur/Sureste

### Prados y Zonas Abiertas (520-800m)
- **Flores silvestres** (alta densidad, clusters pequeños)
- **Abedules aislados**

---

## 📝 Notas Finales

### Especies Nativas Reales de Navarra

Este sistema usa distribución ecológica real:

| Especie | Nombre Científico | Hábitat Real |
|---------|------------------|--------------|
| Aliso | *Alnus glutinosa* | Riberas, 0-15m del agua |
| Sauce | *Salix alba* | Riberas, suelos húmedos |
| Haya | *Fagus sylvatica* | Laderas norte, 600-1400m |
| Roble | *Quercus robur* | Laderas sur y valles |
| Abedul | *Betula pendula* | Terrenos pobres, mixtos |

### Referencias

- **Vegetación de Navarra:** Gobierno de Navarra - Mapa Forestal
- **Río Arakil:** Confederación Hidrográfica del Ebro
- **Alsasua (Altsasu):** 42.9°N, 2.17°W, altitud 520-1200m

---

**Autor:** Sistema de Vegetación Alsasua Simulator  
**Fecha:** 15/08/2026  
**Versión:** 1.0
