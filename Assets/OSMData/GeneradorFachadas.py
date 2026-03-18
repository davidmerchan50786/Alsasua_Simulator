"""
GeneradorFachadas.py
====================
Descarga footprints de edificios de Alsasua (Overpass API) y obtiene
una imagen de Google Street View por cada fachada visible.

Salida:
  Assets/OSMData/alsasua_edificios.json   -- metadatos de edificios + paredes
  Assets/OSMData/fachadas/*.jpg           -- texturas de fachada
  Assets/OSMData/_progreso.json           -- progreso reanudable

Uso:
  export GOOGLE_API_KEY="tu_clave_aqui"
  python GeneradorFachadas.py              # descarga completa
  python GeneradorFachadas.py --dry-run   # estima el coste sin descargar
  python GeneradorFachadas.py --reset-progreso  # fuerza re-descarga total

Requisitos:
  pip install requests
"""

import argparse
import json
import math
import os
import sys
import time
from pathlib import Path
from typing import Optional

try:
    import requests
except ImportError:
    sys.exit("ERROR: instala la librería requests:  pip install requests")

# ---------------------------------------------------------------------------
# Argumentos de línea de comandos
# ---------------------------------------------------------------------------
parser = argparse.ArgumentParser(description="Generador de fachadas Street View para Alsasua")
parser.add_argument(
    "--dry-run", action="store_true",
    help="Solo cuenta paredes y estima el coste sin descargar imágenes")
parser.add_argument(
    "--reset-progreso", action="store_true",
    help="Ignora el progreso guardado y procesa todo de nuevo")
ARGS = parser.parse_args()

# ---------------------------------------------------------------------------
# Configuración
# ---------------------------------------------------------------------------
API_KEY = os.environ.get("GOOGLE_API_KEY", "")
if not API_KEY and not ARGS.dry_run:
    sys.exit(
        "ERROR: variable de entorno GOOGLE_API_KEY no definida.\n"
        "  export GOOGLE_API_KEY='tu_clave'   (Linux/Mac)\n"
        "  $env:GOOGLE_API_KEY='tu_clave'     (PowerShell)\n"
        "  (O usa --dry-run para solo contar sin descargar)"
    )

# Bounding box de Alsasua (Altsasu)
BBOX_SUR   = 42.880
BBOX_NORTE = 42.910
BBOX_OESTE = -2.190
BBOX_ESTE  = -2.150

STEP_BACK_M          = 15.0   # metros de retroceso desde la pared para colocar la cámara SV
SV_IMG_W             = 640
SV_IMG_H             = 480
SV_FOV               = 90
SV_PITCH             = 15     # grados hacia arriba para capturar más fachada
DELAY_ENTRE_LLAMADAS = 0.1    # segundos entre peticiones HTTP (throttle)
MAX_REINTENTOS       = 3      # reintentos por petición con backoff exponencial
PARED_MIN_M          = 1.0    # longitud mínima de pared en metros para procesar
GUARDAR_PROGRESO_CADA = 10    # OPT: era 50; más granular → menos pérdida si crashea

ALTURA_POR_PISO = 3.5  # metros por planta
ALTURA_DEFAULT  = 9.0  # si no hay tag building:levels

# Rutas de salida (relativas al directorio del script)
DIR_SCRIPT    = Path(__file__).parent
DIR_FACHADAS  = DIR_SCRIPT / "fachadas"
JSON_SALIDA   = DIR_SCRIPT / "alsasua_edificios.json"
JSON_PROGRESO = DIR_SCRIPT / "_progreso.json"

# OPT: Session reutiliza conexiones TCP/TLS — evita handshake por cada petición.
# Con ~12.000 llamadas a maps.googleapis.com el ahorro es significativo.
_session = requests.Session()
# FIX: registrar cierre explícito de la sesión HTTP al salir del proceso.
# Python cierra sockets en GC, pero atexit garantiza el CLOSE_WAIT correcto incluso
# si el proceso termina por excepción, Ctrl+C o sys.exit().
import atexit
atexit.register(_session.close)

# ---------------------------------------------------------------------------
# Geometría en coordenadas geográficas
# ---------------------------------------------------------------------------
LAT_SCALE = 111_320.0  # metros por grado de latitud (aprox. constante)


def _lon_scale(lat_deg: float) -> float:
    """Metros por grado de longitud a una latitud dada."""
    return LAT_SCALE * math.cos(math.radians(lat_deg))


def _rumbo(lat1: float, lon1: float, lat2: float, lon2: float) -> float:
    """Ángulo en grados (0=N, 90=E) desde (lat1,lon1) hacia (lat2,lon2)."""
    mid_lat = (lat1 + lat2) / 2
    ls = _lon_scale(mid_lat)
    dx = (lon2 - lon1) * ls
    dy = (lat2 - lat1) * LAT_SCALE
    return (math.degrees(math.atan2(dx, dy)) + 360) % 360


def _area_signada(nodos: list) -> float:
    """
    Área signada del polígono (fórmula del calzador / shoelace).
    Coordenadas: lon como x (este), lat como y (norte).

    Retorna:
      +  -> polígono CCW: el normal IZQUIERDO de cada arista apunta al exterior.
      -  -> polígono CW:  el normal DERECHO  de cada arista apunta al exterior.

    Esta detección es crítica: OSM debería usar CCW para outer ways pero en la
    práctica muchos edificios tienen nodos en orden CW. Sin corrección, la cámara
    se colocaría dentro del edificio y no fotografiaría la fachada.
    """
    n = len(nodos)
    area = 0.0
    for i in range(n):
        lat1, lon1 = nodos[i]
        lat2, lon2 = nodos[(i + 1) % n]
        area += lon1 * lat2 - lon2 * lat1  # lon=x, lat=y
    return area / 2.0


def _posicion_sv(
    lat1: float, lon1: float,
    lat2: float, lon2: float,
    exterior_izquierdo: bool,
) -> Optional[tuple]:
    """
    Calcula la posición de la cámara Street View y el heading para fotografiar
    la pared que va de (lat1,lon1) a (lat2,lon2).

    exterior_izquierdo=True  -> polígono CCW -> normal izquierdo = exterior.
    exterior_izquierdo=False -> polígono CW  -> normal derecho  = exterior.

    Retorna (cam_lat, cam_lon, heading) o None si el segmento es demasiado corto.
    """
    mid_lat = (lat1 + lat2) / 2
    mid_lon = (lon1 + lon2) / 2
    ls = _lon_scale(mid_lat)

    dx = (lon2 - lon1) * ls
    dy = (lat2 - lat1) * LAT_SCALE

    long_pared = math.hypot(dx, dy)
    if long_pared < PARED_MIN_M:
        return None

    # Normal izquierdo (CCW exterior): rotar 90° a la izquierda -> (-dy, dx)
    nx_izq = -dy / long_pared
    ny_izq =  dx / long_pared

    signo = 1.0 if exterior_izquierdo else -1.0
    nx = nx_izq * signo
    ny = ny_izq * signo

    cam_lat = mid_lat + (ny * STEP_BACK_M) / LAT_SCALE
    cam_lon = mid_lon + (nx * STEP_BACK_M) / ls

    heading = _rumbo(cam_lat, cam_lon, mid_lat, mid_lon)
    return cam_lat, cam_lon, heading


# ---------------------------------------------------------------------------
# HTTP — peticiones con reintentos y backoff exponencial
# ---------------------------------------------------------------------------
def _get_con_reintento(
    url: str,
    params: dict,
    timeout: int = 15,
) -> Optional[requests.Response]:
    """
    GET con reintentos exponenciales usando la Session compartida.
    Retorna el Response si el status es 2xx, o None tras MAX_REINTENTOS fallos.
    """
    for intento in range(MAX_REINTENTOS):
        try:
            # OPT: _session reutiliza la conexión TCP/TLS con maps.googleapis.com
            r = _session.get(url, params=params, timeout=timeout)
            r.raise_for_status()
            return r
        except requests.exceptions.HTTPError as e:
            codigo = e.response.status_code if e.response is not None else 0
            if codigo == 429:  # Too Many Requests
                espera = 2 ** (intento + 1)
                print(f"  [WARN] Rate limit (429). Esperando {espera}s...")
                time.sleep(espera)
            elif codigo in (500, 502, 503, 504):
                espera = 2 ** intento
                print(f"  [WARN] Error servidor ({codigo}). "
                      f"Reintento {intento + 1}/{MAX_REINTENTOS} en {espera}s...")
                time.sleep(espera)
            else:
                print(f"  [WARN] HTTP {codigo}: {e}")
                return None
        except requests.exceptions.Timeout:
            espera = 2 ** intento
            print(f"  [WARN] Timeout. Reintento {intento + 1}/{MAX_REINTENTOS} en {espera}s...")
            time.sleep(espera)
        except requests.exceptions.RequestException as e:
            print(f"  [WARN] Error de red: {e}")
            return None
    print(f"  [ERROR] Se agotaron los {MAX_REINTENTOS} reintentos para {url}")
    return None


# ---------------------------------------------------------------------------
# Google Street View — metadata + descarga
# ---------------------------------------------------------------------------
SV_BASE      = "https://maps.googleapis.com/maps/api/streetview"
SV_META_BASE = "https://maps.googleapis.com/maps/api/streetview/metadata"


def _sv_metadata(lat: float, lon: float, heading: float) -> Optional[dict]:
    """Consulta el endpoint de metadatos (gratuito). Retorna el JSON o None."""
    params = {
        "location": f"{lat:.7f},{lon:.7f}",
        "heading":  f"{heading:.1f}",
        "key":      API_KEY,
    }
    r = _get_con_reintento(SV_META_BASE, params, timeout=10)
    if r is None:
        return None
    try:
        return r.json()
    except ValueError:
        print("  [WARN] Respuesta de metadata no es JSON válido")
        return None


def _sv_descargar(lat: float, lon: float, heading: float, ruta_destino: Path) -> bool:
    """
    Descarga la imagen Street View y la guarda en ruta_destino.
    Retorna True si OK y el fichero es un JPEG válido.
    Usa escritura atómica (.tmp -> replace) para evitar ficheros parciales.
    """
    params = {
        "size":     f"{SV_IMG_W}x{SV_IMG_H}",
        "location": f"{lat:.7f},{lon:.7f}",
        "heading":  f"{heading:.1f}",
        "pitch":    str(SV_PITCH),
        "fov":      str(SV_FOV),
        "key":      API_KEY,
    }
    r = _get_con_reintento(SV_BASE, params, timeout=20)
    if r is None:
        return False

    contenido = r.content

    # Verificar que es un JPEG válido (magic bytes: FF D8 FF)
    if len(contenido) < 3 or contenido[:3] != b'\xff\xd8\xff':
        print(f"  [WARN] Respuesta no es un JPEG válido ({len(contenido)} bytes)")
        return False

    ruta_destino.parent.mkdir(parents=True, exist_ok=True)

    # Escritura atómica: guardar en .tmp y reemplazar el destino final.
    # BUG FIX Windows (WinError 183): Path.replace() sobreescribe; rename() falla si existe.
    # BUG FIX Windows (WinError 5 / Errno 13): si un proceso anterior dejó un .tmp bloqueado,
    #   write_bytes falla con PermissionError → fallback a escritura directa al destino.
    # BUG FIX race condition: el .tmp puede desaparecer entre exists() y unlink()
    #   → unlink(missing_ok=True) evita FileNotFoundError.
    tmp = ruta_destino.with_suffix(".tmp")
    try:
        tmp.write_bytes(contenido)
        tmp.replace(ruta_destino)
    except PermissionError:
        try:
            ruta_destino.write_bytes(contenido)
        except OSError as e2:
            print(f"  [WARN] Error al guardar imagen (fallback directo): {e2}")
            return False
    except OSError as e:
        print(f"  [WARN] Error al guardar imagen: {e}")
        try:
            tmp.unlink(missing_ok=True)
        except OSError:
            pass
        return False

    return True


# ---------------------------------------------------------------------------
# Overpass API — footprints de edificios
# ---------------------------------------------------------------------------
OVERPASS_URL = "https://overpass-api.de/api/interpreter"


def _consulta_overpass() -> dict:
    """Descarga los ways con tag building dentro del bbox de Alsasua."""
    query = f"""
    [out:json][timeout:90];
    (
      way["building"]({BBOX_SUR},{BBOX_OESTE},{BBOX_NORTE},{BBOX_ESTE});
    );
    out body geom;
    """
    print("Consultando Overpass API...")
    for intento in range(MAX_REINTENTOS):
        try:
            # Overpass no es una API de Google → usar requests directo (no _session)
            r = requests.post(OVERPASS_URL, data={"data": query}, timeout=120)
            r.raise_for_status()
            datos = r.json()
            total = len(datos.get("elements", []))
            print(f"  {total} elementos recibidos.")
            return datos
        except requests.RequestException as e:
            espera = 2 ** intento
            if intento < MAX_REINTENTOS - 1:
                print(f"  [WARN] Error Overpass: {e}. Reintento en {espera}s...")
                time.sleep(espera)
            else:
                sys.exit(f"ERROR fatal al consultar Overpass: {e}")
    return {"elements": []}  # inalcanzable


def _altura_edificio(tags: dict) -> float:
    """Estima la altura en metros a partir de los tags OSM."""
    if "height" in tags:
        try:
            return float(str(tags["height"]).replace("m", "").strip())
        except ValueError:
            pass
    if "building:levels" in tags:
        try:
            return float(tags["building:levels"]) * ALTURA_POR_PISO
        except ValueError:
            pass
    return ALTURA_DEFAULT


# ---------------------------------------------------------------------------
# Progreso reanudable
# ---------------------------------------------------------------------------
def _cargar_progreso() -> set:
    """Carga el conjunto de IDs de paredes ya procesadas."""
    if ARGS.reset_progreso or not JSON_PROGRESO.exists():
        return set()
    try:
        return set(json.loads(JSON_PROGRESO.read_text(encoding="utf-8")))
    except (ValueError, OSError):
        return set()


def _guardar_progreso(procesadas: set) -> None:
    """Guarda el conjunto de IDs procesados (escritura atómica)."""
    tmp = JSON_PROGRESO.with_suffix(".tmp")
    try:
        # OPT: separators compactos → archivo más pequeño (sin espacios tras , y :)
        tmp.write_text(
            json.dumps(list(procesadas), separators=(',', ':')),
            encoding="utf-8")
        tmp.replace(JSON_PROGRESO)
    except OSError as e:
        print(f"  [WARN] No se pudo guardar progreso: {e}")


# ---------------------------------------------------------------------------
# Pipeline principal
# ---------------------------------------------------------------------------
def procesar_edificios() -> None:
    datos_osm = _consulta_overpass()
    paredes_procesadas = _cargar_progreso()
    edificios_json = []
    total_paredes  = 0
    paredes_con_sv = 0
    nuevas_sesion  = 0

    elementos  = datos_osm.get("elements", [])
    total_ways = sum(1 for e in elementos if e.get("type") == "way")

    if ARGS.dry_run:
        print("\n[DRY-RUN] MODO DRY-RUN: no se haran llamadas a la API de Google.\n")

    edif_idx = 0
    for elem in elementos:
        if elem.get("type") != "way":
            continue
        if "geometry" not in elem:
            continue

        edif_idx += 1
        way_id = elem["id"]
        tags   = elem.get("tags", {})
        nodos  = [(n["lat"], n["lon"]) for n in elem["geometry"]]
        altura = _altura_edificio(tags)

        # OSM cierra el anillo: último nodo == primero. Eliminar duplicado.
        if len(nodos) > 1 and nodos[0] == nodos[-1]:
            nodos = nodos[:-1]

        if len(nodos) < 3:
            continue

        area = _area_signada(nodos)
        exterior_izquierdo = (area >= 0)
        winding_str = "CCW" if exterior_izquierdo else "CW"

        print(f"[{edif_idx}/{total_ways}] way/{way_id} — "
              f"{len(nodos)} nodos, {altura:.1f}m [{winding_str}]")

        paredes = []
        n = len(nodos)

        for i in range(n):
            lat1, lon1 = nodos[i]
            lat2, lon2 = nodos[(i + 1) % n]
            total_paredes += 1
            pared_id = f"{way_id}_{i}"

            resultado = _posicion_sv(lat1, lon1, lat2, lon2, exterior_izquierdo)
            if resultado is None:
                paredes.append({
                    "indice":   i,
                    "p1":       [lon1, lat1],
                    "p2":       [lon2, lat2],
                    "tiene_sv": False,
                    "imagen":   None,
                })
                continue

            cam_lat, cam_lon, heading = resultado
            nombre_img = f"way_{way_id}_pared_{i}.jpg"
            ruta_img   = DIR_FACHADAS / nombre_img

            # -- Dry-run ------------------------------------------------
            if ARGS.dry_run:
                paredes.append({
                    "indice":     i,
                    "p1":         [lon1, lat1],
                    "p2":         [lon2, lat2],
                    "sv_lat":     cam_lat,
                    "sv_lon":     cam_lon,
                    "sv_heading": heading,
                    "tiene_sv":   None,
                    "imagen":     None,
                })
                continue

            # -- Imagen ya en disco y progreso guardado ------------------
            if ruta_img.exists() and pared_id in paredes_procesadas:
                paredes_con_sv += 1
                paredes.append({
                    "indice":     i,
                    "p1":         [lon1, lat1],
                    "p2":         [lon2, lat2],
                    "sv_lat":     cam_lat,
                    "sv_lon":     cam_lon,
                    "sv_heading": heading,
                    "tiene_sv":   True,
                    "imagen":     f"fachadas/{nombre_img}",
                })
                continue

            time.sleep(DELAY_ENTRE_LLAMADAS)

            meta     = _sv_metadata(cam_lat, cam_lon, heading)
            tiene_sv = meta is not None and meta.get("status") == "OK"

            if tiene_sv and not ruta_img.exists():
                time.sleep(DELAY_ENTRE_LLAMADAS)
                ok = _sv_descargar(cam_lat, cam_lon, heading, ruta_img)
                if ok:
                    paredes_con_sv += 1
                    nuevas_sesion  += 1
                    print(f"  [OK] Pared {i}: SV descargado -> {nombre_img}")
                else:
                    tiene_sv   = False
                    nombre_img = None
            elif tiene_sv:
                paredes_con_sv += 1
                print(f"  [OK] Pared {i}: SV ya existe -> {nombre_img}")
            else:
                nombre_img = None

            paredes_procesadas.add(pared_id)

            # OPT: guardar cada GUARDAR_PROGRESO_CADA paredes (era 50 → ahora 10)
            if len(paredes_procesadas) % GUARDAR_PROGRESO_CADA == 0:
                _guardar_progreso(paredes_procesadas)

            paredes.append({
                "indice":     i,
                "p1":         [lon1, lat1],
                "p2":         [lon2, lat2],
                "sv_lat":     cam_lat      if tiene_sv else None,
                "sv_lon":     cam_lon      if tiene_sv else None,
                "sv_heading": heading      if tiene_sv else None,
                "tiene_sv":   tiene_sv,
                "imagen":     f"fachadas/{nombre_img}" if nombre_img else None,
            })

        calle   = tags.get("addr:street", "")
        numero  = tags.get("addr:housenumber", "")
        dir_str = f"{calle} {numero}".strip() if (calle or numero) else ""
        nombre_edificio = (tags.get("name") or dir_str or f"way_{way_id}").strip()

        edificios_json.append({
            "id":      f"way/{way_id}",
            "nombre":  nombre_edificio,
            "altura":  altura,
            "winding": winding_str,
            "tags":    {k: v for k, v in tags.items() if k in (
                            "building", "building:levels", "height",
                            "name", "addr:street", "addr:housenumber")},
            "nodos":   [[lon, lat] for lat, lon in nodos],
            "paredes": paredes,
        })

    # -- Dry-run -----------------------------------------------------------
    if ARGS.dry_run:
        print(f"\n=== DRY-RUN COMPLETADO ===")
        print(f"  Edificios: {len(edificios_json)}")
        print(f"  Paredes totales: {total_paredes}")
        paredes_procesables = sum(
            1 for e in edificios_json
            for p in e["paredes"] if p.get("tiene_sv") is None)
        print(f"  Paredes con posición SV calculada: {paredes_procesables}")
        print(f"  Coste estimado (metadata): $0.00 (endpoint gratuito)")
        print(f"  Coste estimado (imágenes): ~${paredes_procesables * 0.007:.2f}")
        return

    # -- Guardar progreso final --------------------------------------------
    _guardar_progreso(paredes_procesadas)

    # -- Guardar JSON (escritura atómica) ----------------------------------
    salida = {
        "generado":        time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "bbox":            {"sur": BBOX_SUR, "norte": BBOX_NORTE,
                            "oeste": BBOX_OESTE, "este": BBOX_ESTE},
        "total_edificios": len(edificios_json),
        "total_paredes":   total_paredes,
        "paredes_con_sv":  paredes_con_sv,
        "nuevas_sesion":   nuevas_sesion,
        "edificios":       edificios_json,
    }
    tmp_json = JSON_SALIDA.with_suffix(".tmp")
    try:
        tmp_json.write_text(
            json.dumps(salida, ensure_ascii=False, indent=2),
            encoding="utf-8")
        tmp_json.replace(JSON_SALIDA)
    except OSError as e:
        sys.exit(f"ERROR al guardar JSON: {e}")

    print(f"\n=== COMPLETADO ===")
    print(f"  Edificios: {len(edificios_json)}")
    print(f"  Paredes totales: {total_paredes}")
    print(f"  Paredes con Street View: {paredes_con_sv}")
    print(f"  Nuevas esta sesión: {nuevas_sesion}")
    print(f"  JSON -> {JSON_SALIDA}")
    print(f"  Imágenes -> {DIR_FACHADAS}/")


if __name__ == "__main__":
    # FIX: validación fail-fast de la configuración geográfica.
    # Si alguien edita las constantes BBOX y comete un error (ej. sur > norte),
    # el script fallaría silenciosamente con 0 edificios en vez de dar un error claro.
    assert BBOX_SUR  < BBOX_NORTE, f"BBOX inválido: SUR ({BBOX_SUR}) debe ser < NORTE ({BBOX_NORTE})"
    assert BBOX_OESTE < BBOX_ESTE, f"BBOX inválido: OESTE ({BBOX_OESTE}) debe ser < ESTE ({BBOX_ESTE})"

    DIR_FACHADAS.mkdir(parents=True, exist_ok=True)
    procesar_edificios()
