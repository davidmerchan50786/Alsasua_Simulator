"""
RepararIluminacion.py — Deja el nivel ABIERTO en el editor con una iluminación
sana, para que el visor en modo Iluminación (Lit) no salga negro.

Lo mismo que hace UGuardianIluminacion al darle a Play, pero sobre el mapa
guardado, que es lo que se ve en el editor:
  - un solo sol (DirectionalLight): pitch -45 si estaba bajo el horizonte,
    10 lux si estaba apagado, Movable;
  - una sola luz de cielo (SkyLight) con captura en tiempo real;
  - una sola atmósfera (SkyAtmosphere); si falta, se crea;
  - nieblas duplicadas fuera (no crea niebla nueva);
  - post-process con exposición automática por histograma, compensación 0 y
    sin rango min/max forzado.
Antes de tocar nada escribe en el log lo que encuentra; al final guarda.

Ejecutar con el mapa abierto: Tools > Execute Python Script > Tools/RepararIluminacion.py
"""
import unreal

actores = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
niveles = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


def de_clase(cls):
    return [a for a in actores.get_all_level_actors() if isinstance(a, cls)]


def dejar_uno(cls, preferido):
    lista = de_clase(cls)
    if len(lista) <= 1:
        return lista[0] if lista else None
    queda = next((a for a in lista if a.get_name() == preferido), None) \
        or next((a for a in lista if a.get_name().startswith(preferido)), None) \
        or lista[0]
    for a in lista:
        if a != queda:
            actores.destroy_actor(a)
    unreal.log("RepararIluminacion: %s -> quitados %d duplicados, queda %s"
               % (cls.__name__, len(lista) - 1, queda.get_name()))
    return queda


# ── Sol ──────────────────────────────────────────────────────────────────────
sol = dejar_uno(unreal.DirectionalLight, "Atmosphere_Sun")
if not sol:
    sol = actores.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 0),
                                         unreal.Rotator(roll=0, pitch=-45, yaw=30))
    unreal.log_warning("RepararIluminacion: no había sol; creado uno a -45.")
luz = sol.get_editor_property("light_component")
rot = sol.get_actor_rotation()
unreal.log("RepararIluminacion: sol %s pitch=%.1f intensidad=%.2f visible=%s"
           % (sol.get_name(), rot.pitch, luz.get_editor_property("intensity"), luz.is_visible()))
luz.set_mobility(unreal.ComponentMobility.MOVABLE)
if rot.pitch > -5:
    sol.set_actor_rotation(unreal.Rotator(roll=0, pitch=-45, yaw=rot.yaw), False)
    unreal.log_warning("RepararIluminacion: sol bajo el horizonte -> pitch -45.")
if luz.get_editor_property("intensity") <= 0.01 or not luz.is_visible():
    luz.set_visibility(True)
    luz.set_editor_property("intensity", 10.0)
    unreal.log_warning("RepararIluminacion: sol apagado -> 10 lux.")
luz.set_editor_property("atmosphere_sun_light", True)

# ── Cielo ────────────────────────────────────────────────────────────────────
if not dejar_uno(unreal.SkyAtmosphere, "Atmosphere_SkyAtmosphere"):
    actores.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator())
    unreal.log_warning("RepararIluminacion: no había SkyAtmosphere; creada.")

cielo = dejar_uno(unreal.SkyLight, "Atmosphere_SkyLight")
if not cielo:
    cielo = actores.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 0), unreal.Rotator())
    unreal.log_warning("RepararIluminacion: no había SkyLight; creada.")
sl = cielo.get_editor_property("light_component")
sl.set_mobility(unreal.ComponentMobility.MOVABLE)
sl.set_editor_property("real_time_capture", True)
if sl.get_editor_property("intensity") <= 0.01:
    sl.set_editor_property("intensity", 1.0)
sl.set_visibility(True)
sl.recapture_sky()

dejar_uno(unreal.ExponentialHeightFog, "Atmosphere_Fog")

# ── Exposición ───────────────────────────────────────────────────────────────
for ppv in de_clase(unreal.PostProcessVolume):
    s = ppv.get_editor_property("settings")
    unreal.log("RepararIluminacion: post-process %s metodo=%s bias=%.2f"
               % (ppv.get_name(), s.get_editor_property("auto_exposure_method"),
                  s.get_editor_property("auto_exposure_bias")))
    s.set_editor_property("override_auto_exposure_method", True)
    s.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
    s.set_editor_property("override_auto_exposure_bias", True)
    s.set_editor_property("auto_exposure_bias", 0.0)
    s.set_editor_property("override_auto_exposure_min_brightness", False)
    s.set_editor_property("override_auto_exposure_max_brightness", False)
    ppv.set_editor_property("settings", s)

niveles.save_current_level()
unreal.log("RepararIluminacion: listo y guardado. Visor en modo Iluminación.")
