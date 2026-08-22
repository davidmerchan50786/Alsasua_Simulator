# QUICK START: Vegetación Realista
## Alsasua Simulator - 3 Pasos Rápidos

---

## ⚡ EJECUCIÓN RÁPIDA (5 minutos)

### 1️⃣ Abrir Unreal Editor 5.8
```
AlsasuaSimulator.uproject
```

### 2️⃣ Ejecutar Script Python

**Menú:**
```
Tools → Execute Python Script
```

**Archivo:**
```
F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Tools\CreateVegetationAssets.py
```

**O en consola Python (`~` luego `py`):**
```python
exec(open(r'F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Tools\CreateVegetationAssets.py').read())
```

**Resultado esperado en Output Log:**
```
============================================================
CREANDO DATA ASSETS DE VEGETACIÓN NATIVA
Alsasua Simulator - Especies de Navarra
============================================================
✅ Creado: /Game/Vegetation/DataAssets/VT_Aliso_Ribera
✅ Creado: /Game/Vegetation/DataAssets/VT_Sauce_Ribera
✅ Creado: /Game/Vegetation/DataAssets/VT_Haya_Norte
✅ Creado: /Game/Vegetation/DataAssets/VT_Roble_Sur
✅ Creado: /Game/Vegetation/DataAssets/VT_Abedul_Mixto
✅ Creado: /Game/Vegetation/DataAssets/VT_Arbusto_Sotobosque
✅ Creado: /Game/Vegetation/DataAssets/VT_Flores_Silvestres
✅ Creado: /Game/Vegetation/DataAssets/VT_Hierba_Alta_Ribera
============================================================
✅ COMPLETADO: 8/8 assets creados
============================================================
```

### 3️⃣ Verificar Assets

**Content Browser:**
```
Content/Vegetation/DataAssets/
```

**Deberías ver:**
- VT_Aliso_Ribera
- VT_Sauce_Ribera
- VT_Haya_Norte
- VT_Roble_Sur
- VT_Abedul_Mixto
- VT_Arbusto_Sotobosque
- VT_Flores_Silvestres
- VT_Hierba_Alta_Ribera

---

## 🎮 GENERAR VEGETACIÓN EN EL MAPA

### Método 1: Blueprint Temporal (Más Fácil)

1. **Crear Blueprint de prueba:**
   - Content Browser → Click derecho → Blueprint Class → Actor
   - Nombrar: `BP_VegetationTester`

2. **Event Graph:**
   ```blueprint
   Event BeginPlay
   ↓
   Get World Subsystem (Class: VegetationSpawnerSubsystem)
   ↓
   [Branch] Is Valid?
   ↓ True
   Set Target Landscape (Landscape: seleccionar del nivel)
   ↓
   Add Vegetation Type (x8 veces, uno por cada VT_*)
   ↓
   Spawn All Vegetation
   ```

3. **Arrastrar al nivel:**
   - Drag & Drop `BP_VegetationTester` al nivel
   - Play (Alt+P)

### Método 2: Consola de Comandos (Debug)

En el juego, presionar `~` (tilde) y ejecutar:
```
py import unreal; subsystem = unreal.get_editor_subsystem(unreal.VegetationSpawnerSubsystem); subsystem.spawn_all_vegetation()
```

---

## 📊 RESULTADOS ESPERADOS

### Ribera del Arakil
- **Alisos densos** cerca del agua (verde oscuro)
- **Sauces** dispersos
- **Hierba alta** en márgenes

### Montañas Norte
- **Hayas** en clusters (solo laderas orientadas al norte)
- **Arbustos** bajo los árboles

### Montañas Sur
- **Robles** más dispersos (laderas soleadas)

### Prados y Caminos
- **Flores silvestres** en clusters coloridos

---

## ⚙️ AJUSTES RÁPIDOS

### Si hay DEMASIADA vegetación:

Editar Data Assets en Content Browser:

```
VT_Aliso_Ribera:
  Global Probability: 80% → 50%
  Density Per M2: 1.5 → 1.0
```

### Si hay MUY POCA vegetación:

```
VT_Haya_Norte:
  Global Probability: 60% → 80%
  Cluster Strength: 0.5 → 0.7
```

### Si el FPS baja mucho:

Deshabilitar especies secundarias:
```
VT_Flores_Silvestres:
  bEnabled: true → false
  
VT_Abedul_Mixto:
  bEnabled: true → false
```

---

## 🐛 TROUBLESHOOTING

### "No se pudo crear Data Asset"

**Solución:**
- Verificar que PythonScriptPlugin está habilitado
- Crear carpeta manualmente: Content → New Folder → "Vegetation" → "DataAssets"
- Reintentar script

### "Mesh no encontrado"

**Solución:**
- Verificar que los paquetes están en Content:
  - `HighPoly_Tree_Model/`
  - `Nanite_Plants_Sample_Collection/`
  - `OWD_Flowers_Pack/`
- Si faltan, descargar desde Epic Games Launcher (gratis)

### No aparece vegetación en el juego

**Checklist:**
- ✅ Script Python ejecutado sin errores
- ✅ 8 Data Assets visibles en Content Browser
- ✅ Landscape existe en el nivel
- ✅ `SpawnAllVegetation()` fue llamado
- ✅ Output Log muestra "spawned X instances"

---

## 📚 DOCUMENTACIÓN COMPLETA

### Para más detalles:
- **Instrucciones completas:** `INSTRUCCIONES_VEGETACION.md`
- **Plan de mejoras:** `PLAN_MEJORA_REALISMO.md`
- **Estado actual:** `MEJORAS_IMPLEMENTADAS.md`

### Configuraciones avanzadas:
- Reemplazar meshes placeholder por especies específicas
- Ajustar colores y materiales
- Optimizar performance con LODs
- Añadir animación de viento

---

## 🎯 PRÓXIMO PASO

Una vez que tengas vegetación funcionando, continúa con:
- **Color Matching por Barrios** (LUTs con ortofoto)
- **Materiales de Fachada Avanzados**
- **Iluminación Volumétrica Nocturna**

Ver `PLAN_MEJORA_REALISMO.md` sección "Próximos Pasos".

---

**Tiempo estimado:** 5-10 minutos  
**Resultado:** Vegetación nativa distribuida ecológicamente  
**Performance:** Nanite auto-LOD = mínimo impacto FPS

¡Disfruta del realismo! 🌲🌳🌿
