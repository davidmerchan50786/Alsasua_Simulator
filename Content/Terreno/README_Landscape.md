# Terreno — Landscape de Alsasua

Heightmap real (clipmap V3, true-UTM) convertido a un Landscape de Unreal.

## Flujo

1. **Reencodear** (una vez, o cuando cambie el clipmap V3):
   ```
   python3 Tools/PrepararLandscape.py <ruta heightmap_unificado.r16> Content/Terreno
   ```
   Genera `alsasua_landscape_4033.r16` (tamaño válido de Landscape) y `landscape_import.json`.
   Lee el `*.r16` de Unity (4097²) y lo remuestrea a 4033² preservando la codificación
   `altitudReal = 495 + q/64`. Verificado: 497.39–1155.33 m (pérdida por remuestreo ~0.27 m).

2. **Importar** en el editor, dos vías:
   - **C++**: `UImportadorLandscape::ImportarLandscape(ruta)` (Blueprint/Editor Utility, valores por defecto correctos).
   - **Manual**: Landscape Mode → Import from File, con los ajustes de abajo.

## Ajustes de import (manual)

| Parámetro            | Valor        | Por qué |
|----------------------|--------------|---------|
| Heightmap            | `alsasua_landscape_4033.r16` | 4033², uint16 LE |
| Section size         | 63×63 quads  | 4033 = 63·64 + 1 |
| Sections per comp.   | 1×1          | 64×64 componentes |
| Scale X / Y          | **178.5714** | 7200 m / 4032 quads × 100 cm |
| Scale Z              | **200**      | mapea ±512·(Z/100) → span real 1024 m, cuanto 1/64 m |
| Location Z (cm)      | **49567**    | hace WorldZ = (altitudReal − 511.33)·100 |
| Location X / Y       | centrar en Herriko Plaza | `UGeoDataAlsasua::HerrikoPlaza()` − media extensión |

## Comprobación

Con esos valores, la cota de Herriko Plaza (531.94 m) debe caer a `WorldZ ≈ 2061 cm`
(= (531.94 − 511.33)·100). Rango del terreno: −13.9 m a +643.9 m sobre el cero del mundo.

## Orientación

`PrepararLandscape.py` voltea las filas (el origen Unity es sur→norte). Si el terreno
aparece **espejado norte-sur** respecto a edificios/calles, quitar el `np.flipud` del script
y reimportar. La paridad fina XY conviene validarla contra un par de edificios OSM conocidos.
