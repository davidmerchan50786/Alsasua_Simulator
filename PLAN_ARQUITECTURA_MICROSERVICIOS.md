# Plan de Reestructuración: Núcleo Monolito + Pilares Conectables
## Alsasua Simulator — Arquitectura modular tipo "microservicios"

**Fecha:** 17/08/2026 (v2 — auditadas TODAS las ramas)
**Estado actual:** 8 módulos C++ acoplados con ciclo Manifa↔UI confesado en Build.cs

---

## 0-bis. AUDITORÍA DE RAMAS (17/08/2026)

### Ramas vivas (era UE5, agosto 2026)

| Rama | Estado vs master | Contenido |
|------|-----------------|-----------|
| `origin/main` ★TRONCO REAL | **+44 / -4** | Oleada de fixes de auditoría: coordenadas (pueblo a 857m, señales a 216km, árboles bajo tierra), escala landmarks 1:10 corregida, 19 landmarks + 30 POIs, capas instanciadas (17537 persianas, 1647 barandillas, 5038 aceras, puertas, contenedores), 29 materiales creados, túneles (`TunelAlsasua`), semáforos, vía férrea + material rodante, diálogos cargados de Content/Dialogs, save game (disfraz/misiones), capa nocturna, paradas de transporte, marcas viales en todas las vías, 2392 planos de agua, farolas/semáforos visibles |
| `origin/claude/arreglar-todo-5-8` | main +5 | **CI GitHub Actions** (`verificar.yml`), suite de verificadores Python (`VerificarVias/Dialogos/Guardado/Fuentes/Datasets/CallesNavarra`, `AuditarSistemas`, `AlturasLidarEdificios`), Cascade→Niagara, tiradas de habilidad en diálogos, fixes subsistemas auto-arranque |
| `origin/ue5-clean-integration` | +5 / -4 | Scripts de packaging release, fixes compilación 5.8, clima vía Material Parameter Collection, mannequin opcional |
| `master` (local) | +4 propios | Sistema vegetación natural + assets packs (este trabajo) — **NO está en main** |

Ya fusionadas en main (0 ahead): `claude/graphics-lighting-shadows`, `copilot/fidelidad-grafica-realidad-100`, `claude/claude-md-docs-3xqh1n`.

### Ramas legado (era Unity/HDRP, abril 2026 — behind 63)

`pamplona`, `optimizar-todo`, `revision-total-juego`, `rama-vacia`, `fix/compilacion-tests-scripts`, `claude/hardcore-matsumoto`, `claude/fix-altsasu-terrain-rendering`, `copilot/prepare-development-environment`.

Son del port Unity (C#, asmdef, HDRP). NO fusionar. Ideas aún valiosas a re-portar si hace falta:
- `optimizar-todo`: tráfico O(n)→O(1)
- `pamplona`: generador mesh de calles + texturas asfalto PBR 2K + heightmap RAW IGN/IDENA

**Acción:** archivar con tag (`legacy/unity-2026.04`) y borrar las ramas remotas.

### Sistemas maduros descubiertos (no estaban en la auditoría anterior)

- **Diálogos:** `Systems/Dialog/` completo (Subsystem, Memory, Formatter, tiradas de habilidad)
- **Misiones:** `Mision/SabotageMissionActor`, `Systems/Criminal/`, `Systems/Missions/DeepState`
- **Ferrocarril:** vía férrea generada + material rodante + `TunelAlsasua`
- **Transporte público:** paradas leídas de datos del pueblo
- **Tráfico:** `AlsasuaTrafficSystem` + `AlsasuaDynamicTrafficSystem` + `TrafficLightSystem` + `TrafficAgent`
- **Guardado:** `Core/AlsasuaSaveGame` (persiste disfraz y misiones)
- **Pipeline Python + CI:** verificadores de datos con GitHub Actions

Estos sistemas entran en el mapa de pilares (sección 2, tabla actualizada).

---

## 0. Aclaración de concepto (importante)

En Unreal, "microservicios" reales (procesos separados con red) NO tienen sentido para
sistemas de gameplay: el coste de serializar estado cada frame mata el rendimiento.

Lo que SÍ existe en UE y cumple exactamente lo pedido —**pilares que se conectan y
desconectan del motor en runtime**— son los **Game Feature Plugins** (plugin oficial
`GameFeatures` + `ModularGameplay`). Es el mismo patrón que usa Fortnite:

- Monolito núcleo: siempre cargado, no sabe nada de los pilares
- Pilares: plugins autocontenidos (código + assets + config)
- Conectar/desconectar en runtime: `LoadAndActivateGameFeature()` / `DeactivateGameFeature()`
- Comunicación por contratos (interfaces), nunca dependencias directas

Este plan usa ese modelo. La sección 7 cubre microservicios out-of-process reales para
lo único donde tienen sentido (telemetría, editor de mundo, generación offline).

---

## 1. Estado actual (problema)

```
AlsasuaSimulator (target)
 ├── AlsasuaCore        ← datos/utilidades (OK, ya es "núcleo")
 ├── AlsasuaEntities
 ├── AlsasuaWorld
 ├── AlsasuaGameplay ──┐
 ├── AlsasuaManifa ────┼── CICLO: Manifa→UI, Gameplay/UI→Manifa
 ├── AlsasuaUI ────────┘   (CircularlyReferencedDependentModules)
 └── AlsasuaEditor
```

**Problemas:**
1. `AlsasuaManifa` es un cajón de sastre: vegetación, clima, tráfico, NPCs, carreteras,
   fachadas, audio, minimapa, HUD… todo en un módulo (~60+ sistemas).
2. Ciclo de dependencias Manifa↔UI confesado en Build.cs — impide desacoplar.
3. Nada se puede desactivar: o compila todo o no arranca nada.
4. Imposible testear un pilar aislado.

---

## 2. Arquitectura objetivo

```
┌─────────────────────────────────────────────────────────┐
│  MONOLITO NÚCLEO (siempre cargado)                       │
│  ├── AlsasuaCore       datos, tipos, utilidades          │
│  ├── AlsasuaContracts  ★NUEVO: solo interfaces + structs │
│  ├── AlsasuaKernel     ★NUEVO: GameMode, service locator,│
│  │                     ciclo de vida de pilares          │
│  └── AlsasuaSimulator  target/arranque                   │
└────────────────────────┬────────────────────────────────┘
                         │  contratos (UInterface)
        ┌────────┬───────┼────────┬─────────┬──────────┐
        ▼        ▼       ▼        ▼         ▼          ▼
   GF_Vegetacion GF_Clima GF_Trafico GF_NPCs GF_Edificios GF_UI
   (plugin)      (plugin) (plugin)   (plugin) (plugin)     (plugin)
```

### Regla de oro de dependencias
- Pilar → `AlsasuaContracts` + `AlsasuaCore`: **permitido**
- Pilar → otro pilar: **PROHIBIDO** (comunicación solo vía contratos/eventos)
- Núcleo → pilar: **PROHIBIDO** (el núcleo no conoce ningún pilar)

### Pilares propuestos (Game Feature Plugins)

| Plugin | Contenido (extraído de AlsasuaManifa) | Desconectable |
|--------|----------------------------------------|---------------|
| `GF_Vegetacion` | VegetationSpawner, TreeSpawner, SeasonalFoliage, FoliagePainter | ✅ mundo sin flora |
| `GF_Clima` | WeatherSystem, AtmosphereController, TimeOfDayManager, VisualEffects | ✅ cielo estático |
| `GF_Trafico` | TrafficSystem, TrafficLightSystem, ParkingSystem, VehicleVariety | ✅ calles vacías |
| `GF_NPCs` | NPCPedestrianSystem, animaciones, diálogos | ✅ pueblo desierto |
| `GF_Edificios` | FacadeGenerator, BarrioStyleSystem, RooftopDetail, ShopFront, Emissive | ✅ cajas grises |
| `GF_Carreteras` | RoadSurface, RoadMarkings, SidewalkSystem, SignPlacer, StreetLight | ✅ terreno base |
| `GF_UI` | HUD, minimapa, menús (rompe el ciclo Manifa↔UI) | ✅ sin HUD |
| `GF_Audio` | ProceduralAudio, ambient, reverb | ✅ silencio |
| `GF_Ferrocarril` | vía férrea, material rodante, TunelAlsasua, paradas transporte | ✅ vías sin trenes |
| `GF_Dialogos` | Systems/Dialog completo (Subsystem, Memory, tiradas habilidad) | ✅ NPCs mudos |
| `GF_Misiones` | SabotageMission, Criminal, DeepState (consume GF_Dialogos vía contrato) | ✅ modo paseo |

`AlsasuaSaveGame` NO es pilar: va al núcleo (Kernel). Cada pilar aporta su fragmento
de guardado vía `ISaveFragmentProvider` — si el pilar está desconectado, su fragmento
se conserva sin tocar (round-trip seguro).

---

## 3. Contratos: módulo AlsasuaContracts

Módulo nuevo, SOLO headers: interfaces `UINTERFACE`, structs de datos, delegates.
Cero lógica, cero assets. Es la "API pública" del monolito.

```cpp
// AlsasuaContracts/Public/Services/IWeatherService.h
UINTERFACE(MinimalAPI, BlueprintType)
class UWeatherService : public UInterface { GENERATED_BODY() };

class IWeatherService
{
    GENERATED_BODY()
public:
    virtual float GetRainIntensity() const = 0;
    virtual float GetWetness() const = 0;
    virtual FOnWeatherChanged& OnWeatherChanged() = 0;
};
```

Interfaces mínimas por pilar:
- `IWeatherService` — lluvia/viento/wetness (lo consumen vegetación, carreteras, audio)
- `ITimeOfDayService` — hora, sol (lo consumen clima, edificios, NPCs)
- `IVegetationService` — spawn/clear
- `ITrafficService` — densidad, semáforos
- `IBuildingQueryService` — consulta edificios/barrios
- `IRoadQueryService` — consulta red viaria (lo consumen tráfico y NPCs)

---

## 4. Kernel: service locator + ciclo de vida

```cpp
// AlsasuaKernel/Public/AlsasuaServiceRegistry.h
UCLASS()
class UAlsasuaServiceRegistry : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    // Pilar se registra al activarse, se des-registra al desactivarse
    void RegisterService(FName ServiceName, TScriptInterface<UInterface> Service);
    void UnregisterService(FName ServiceName);

    template<typename T>
    T* GetService(FName ServiceName) const; // nullptr si pilar desconectado

    // Los consumidores DEBEN tolerar nullptr → degradación elegante
    FOnServiceRegistered OnServiceRegistered;
    FOnServiceUnregistered OnServiceUnregistered;
};
```

**Patrón de consumo obligatorio** (degradación elegante):
```cpp
if (IWeatherService* Weather = Registry->GetService<IWeatherService>("Weather"))
{
    Wetness = Weather->GetWetness();
}
// sin clima conectado → Wetness queda en default, el juego sigue
```

### Conexión/desconexión en runtime
```cpp
// Consola o menú de configuración:
UGameFeaturesSubsystem::Get().LoadAndActivateGameFeaturePlugin(TrafficPluginURL, ...);
UGameFeaturesSubsystem::Get().DeactivateGameFeaturePlugin(TrafficPluginURL);
```
Cada `UGameFeatureAction` del pilar registra/des-registra su servicio y
spawnea/destruye sus actores. Comandos de consola a añadir:
```
Alsasua.Pilar.Activar GF_Trafico
Alsasua.Pilar.Desactivar GF_Trafico
Alsasua.Pilar.Listar
```

---

## 5. Romper el ciclo Manifa↔UI

Causa actual: Manifa referencia widgets de pausa directamente.

Solución (patrón estándar):
1. Declarar eventos en `AlsasuaContracts`: `FOnPauseRequested`, `FOnMinimapDataChanged`…
2. Manifa (gameplay) **emite** eventos, no conoce widgets.
3. `GF_UI` **escucha** y muestra widgets.
4. Eliminar `CircularlyReferencedDependentModules.Add("AlsasuaUI")` de Manifa.Build.cs.

Sin UI conectada los eventos caen al vacío — correcto, el juego funciona sin HUD.

---

## 6. Fases de migración (incremental, siempre compilable)

### Fase 0 — Consolidación de ramas (2-3 días) ★OBLIGATORIA ANTES DE NADA
El refactor debe partir de UN tronco. Hoy el trabajo está repartido en 4 ramas:
- [ ] Merge `origin/main` → `master` local (trae los 44 fixes de auditoría; resolver
      conflictos en `Tools/` — main borró CreateVegetationAssets.py y GenerateMatchedLUT.py:
      conservarlos, son de este trabajo)
- [ ] Merge `origin/claude/arreglar-todo-5-8` (CI + verificadores Python)
- [ ] Merge `origin/ue5-clean-integration` (packaging + fixes 5.8)
- [ ] Commitear cambios locales pendientes de master (vegetación, LUTs)
- [ ] Push del tronco unificado a `main` (rama por defecto del repo)
- [ ] Tag `legacy/unity-2026.04` + borrar las 8 ramas Unity remotas
- **Verificación:** CI `verificar.yml` en verde sobre el tronco unificado

### Fase 1 — Fundaciones (1 semana)
- [ ] Crear módulo `AlsasuaContracts` (solo interfaces)
- [ ] Crear módulo `AlsasuaKernel` con `UAlsasuaServiceRegistry`
- [ ] Habilitar plugins `GameFeatures` + `ModularGameplay` en .uproject
- [ ] Comandos de consola de gestión de pilares
- **Verificación:** compila, juego idéntico

### Fase 2 — Piloto: GF_Clima (1 semana)
El clima es el mejor piloto: muchos consumidores, poco estado compartido.
- [ ] Crear plugin `GF_Clima`, mover WeatherSystem + TimeOfDay + Atmosphere
- [ ] Implementar `IWeatherService`/`ITimeOfDayService`, registrar en Registry
- [ ] Migrar consumidores (vegetación, carreteras, fachadas) a `GetService<>` con null-check
- **Verificación:** desactivar GF_Clima en runtime → cielo estático, cero crashes

### Fase 3 — Romper ciclo UI (1 semana)
- [ ] Eventos de UI en Contracts, extraer `GF_UI`
- [ ] Borrar `CircularlyReferencedDependentModules`
- **Verificación:** UBT sin ciclo; juego arranca sin GF_UI (sin HUD)

### Fase 4 — Pilares de mundo (2-3 semanas)
Orden por acoplamiento ascendente:
- [ ] `GF_Vegetacion` (casi autónomo, ya refactorizado)
- [ ] `GF_Audio`
- [ ] `GF_Edificios`
- [ ] `GF_Carreteras` (expone `IRoadQueryService`)
- [ ] `GF_Ferrocarril` (vía + trenes + túneles + paradas)
- [ ] `GF_Trafico` y `GF_NPCs` (consumen IRoadQueryService — los últimos)

### Fase 4-bis — Pilares de narrativa (1 semana)
- [ ] `GF_Dialogos` (Systems/Dialog + DialogWidgetBase vía eventos, no referencia directa a GF_UI)
- [ ] `GF_Misiones` (consume `IDialogService`; sin diálogos las misiones de conversación se saltan)
- [ ] `ISaveFragmentProvider` en Kernel; migrar AlsasuaSaveGame a fragmentos por pilar

### Fase 5 — Vaciar AlsasuaManifa (1 semana)
- [ ] Lo que quede en Manifa se reparte: datos→Core, arranque→Kernel
- [ ] Eliminar módulo AlsasuaManifa
- **Verificación:** build limpio, matriz de activación (2^8 combos → probar los 9 básicos:
  todo ON, todo OFF, y cada pilar OFF individualmente)

**Total: 8-9 semanas** (con Fase 0 y narrativa). Cada fase deja el tronco compilable,
jugable y con el CI `verificar.yml` en verde.

---

## 7. Microservicios out-of-process REALES (opcional, solo donde aportan)

Solo para trabajo que no necesita el frame loop:

| Servicio | Proceso | Transporte | Para qué |
|----------|---------|-----------|----------|
| `svc-worldgen` | Python | archivos JSON (ya existe: roads_unity.json) | Regenerar carreteras/edificios desde OSM/ortofoto sin abrir editor |
| `svc-lut` | Python | archivos PNG | Ya implementado: GenerateMatchedLUT.py |
| `svc-verificacion` | Python + GitHub Actions | CI | **Ya existe en `arreglar-todo-5-8`**: VerificarVias/Dialogos/Guardado/Datasets/Fuentes + AuditarSistemas — es el microservicio de calidad del proyecto |
| `svc-telemetria` | proceso aparte | UDP localhost | FPS/stats en dashboard sin coste en juego |

NO convertir en microservicio: tráfico, NPCs, clima, física — necesitan el frame loop.

---

## 8. Riesgos y mitigaciones

| Riesgo | Mitigación |
|--------|-----------|
| Hard-references a assets de un pilar desde el núcleo | Auditar con Reference Viewer antes de mover; usar `TSoftObjectPtr` |
| Estado huérfano al desconectar pilar en runtime | Cada GameFeatureAction limpia TODO lo que spawneó (actores, HISMs, delegates) |
| Blueprints que castean a clases de pilares | Sustituir casts por interfaces de Contracts |
| Orden de activación (Trafico necesita Carreteras) | `IRoadQueryService` nullable + reintento en `OnServiceRegistered` |
| Regresión de rendimiento por indirection | `GetService` cachea puntero, invalida en OnServiceUnregistered |

---

## 9. Criterios de éxito

- Cero `CircularlyReferencedDependentModules` en el proyecto
- El núcleo compila sin ningún pilar presente
- `Alsasua.Pilar.Desactivar X` en runtime: cero crashes, cero leaks (verificar con `obj gc`)
- Tiempo de compilación incremental por pilar < 30 s (hoy Manifa entero ~100 s)
- Un pilar nuevo = un plugin nuevo, cero cambios en el núcleo
