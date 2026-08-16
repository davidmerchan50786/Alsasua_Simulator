"""
VerificarVias.py — Comprueba que UCargadorVias puede leer sus cinco datasets.

Existe porque el ferrocarril llevaba tiempo sin construirse y nadie se enteró:
CargadorVias::Encolar deserializaba la raíz de cada JSON como array, y
railways_unity.json es un objeto {"rails": [...], "stations": [...]}. Contra una
raíz de objeto la deserialización devuelve false, Encolar salía sin encolar nada
y el director seguía registrando "Vías férreas cargadas". 86 trazados y 38,7 km
de vía desaparecidos sin un solo aviso en el log.

Sin compilador en Linux no se puede verificar el arreglo ejecutando el juego, así
que este script replica lo que hace Encolar — misma forma de raíz, mismo mínimo
de puntos, mismo cálculo de ancho — y dice cuántos trazados saldrían de cada
fichero. Si un dataset se regenera con otra forma, esto lo caza antes de que se
vuelva a perder en silencio.

Uso:  python3 Tools/VerificarVias.py
"""
import json
import math
import os

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATOS = os.path.join(RAIZ, "Content", "Datos")

# Mismo orden y mismos parámetros que UCargadorVias::PrepararCarga.
#   (fichero, tag, epsilon_cm, ancho_defecto_m, ancho_por_tracks, campo_array)
COLA = [
    ("footways_unity.json",  "Acera",   8.0, 3.0, False, None),
    ("railways_unity.json",  "Via",    14.0, 2.5, True,  "rails"),
    ("waterways_unity.json", "Agua",  -20.0, 6.0, False, None),
    ("caminos_unity.json",   "Camino",  6.0, 3.0, False, None),
    ("tunnels_unity.json",   "Tunel",   8.0, 4.0, False, None),
]


def leer(ruta, campo):
    """Lo mismo que hace Encolar: array en la raíz, o el campo del objeto."""
    with open(ruta, encoding="utf-8") as f:
        doc = json.load(f)
    if isinstance(doc, list):
        return doc, "array"
    if campo and isinstance(doc, dict) and isinstance(doc.get(campo), list):
        return doc[campo], "objeto.%s" % campo
    return None, "objeto claves=%s" % (list(doc) if isinstance(doc, dict) else type(doc))


def main():
    total, fallos = 0, 0
    print("  %-24s %-14s %6s %8s %9s %9s"
          % ("fichero", "raíz", "trazos", "puntos", "long km", "ancho m"))

    for fichero, tag, _eps, ancho_def, por_tracks, campo in COLA:
        ruta = os.path.join(DATOS, fichero)
        if not os.path.exists(ruta):
            print("  %-24s AUSENTE (Encolar lo omite con warning)" % fichero)
            continue

        arr, forma = leer(ruta, campo)
        if arr is None:
            print("  %-24s %-14s  <-- Encolar no sabe leer esta raíz" % (fichero, forma))
            fallos += 1
            continue

        trazos = puntos = 0
        largo = 0.0
        anchos = []
        for o in arr:
            pts = o.get("pts")
            # Encolar exige >=6 números en el array plano, o sea >=2 puntos.
            if not isinstance(pts, list) or len(pts) < 6:
                continue
            xz = [(pts[i], pts[i + 2]) for i in range(0, len(pts) - 2, 3)]
            if len(xz) < 2:
                continue
            trazos += 1
            puntos += len(xz)
            largo += sum(math.dist(a, b) for a, b in zip(xz, xz[1:]))

            if por_tracks:
                if o.get("type") == "platform":
                    anchos.append(4.0)
                else:
                    anchos.append(1.6 * max(1, o.get("tracks", 1)) + 2.8)
            else:
                anchos.append(o.get("width", ancho_def))

        total += trazos
        print("  %-24s %-14s %6d %8d %8.1f %9.1f"
              % (fichero, forma, trazos, puntos, largo / 1000.0,
                 sum(anchos) / len(anchos) if anchos else 0.0))

        if trazos == 0:
            fallos += 1

    print("\n  %d trazados en total." % total)
    print("  Los túneles se encolan pero PasoPresupuesto los cuenta sin construir")
    print("  malla, a la espera de ATunelAlsasua.")
    if fallos:
        raise SystemExit("\n  %d dataset(s) no producen nada. Eso es el bug." % fallos)
    print("  Ningún dataset se pierde por la forma de su raíz.")

    rodante()


# Mismos valores que AlsasuaFerrocarrilSystem.cpp.
LOCO_M, VAGON_M, SEPARACION_M = 18.0, 12.2, 1.8
RADIO_ESTACION_M, VAGONES = 450.0, 6


def rodante():
    """Réplica de la selección de vía de UAlsasuaFerrocarrilSystem.

    Sin poder lanzar el juego, esto es lo que dice si la estación tiene
    apartaderos donde quepa una composición parada, o si el sistema se va a
    quedar sin candidatas y no colocará nada.
    """
    with open(os.path.join(DATOS, "railways_unity.json"), encoding="utf-8") as f:
        doc = json.load(f)

    est = next((s for s in doc.get("stations", []) if s.get("type") == "station"), None)
    if not est:
        print("\n  Material rodante: sin estación en el dataset.")
        return
    ex, ey = est["x"] * 100.0, est["z"] * 100.0

    largo_comp = (LOCO_M + VAGONES * (VAGON_M + SEPARACION_M) + SEPARACION_M) * 100.0
    radio = RADIO_ESTACION_M * 100.0

    cands = []
    for r in doc.get("rails", []):
        if r.get("type") != "rail":
            continue
        p = r.get("pts", [])
        pts = [(p[i] * 100.0, p[i + 2] * 100.0) for i in range(0, len(p) - 2, 3)]
        if len(pts) < 2:
            continue
        largo = sum(math.dist(a, b) for a, b in zip(pts, pts[1:]))
        dmin = min(math.dist(q, (ex, ey)) for q in pts)
        if dmin > radio or largo < largo_comp * 1.2:
            continue
        cands.append((r.get("electrified", "no") != "no", dmin, largo, r.get("name", "")))

    # Apartadero sin catenaria primero, y a igualdad el más cercano: una rastra
    # de mercancías parada en la vía general cortaría la Madril-Hendaia.
    cands.sort(key=lambda c: (c[0], c[1]))

    print("\n  Material rodante junto a la estación de %s" % est.get("name", "?"))
    print("  composición de 1+%d = %.1f m, radio de búsqueda %.0f m"
          % (VAGONES, largo_comp / 100.0, RADIO_ESTACION_M))
    print("  %d vías candidatas; se usan las 3 primeras:" % len(cands))
    for elec, dist, largo, nombre in cands[:3]:
        print("    %-12s a %5.0f m de la estación, %6.0f m de largo  %s"
              % ("vía general" if elec else "apartadero", dist / 100.0,
                 largo / 100.0, nombre or "(sin nombre)"))
    if not cands:
        print("    ninguna: el sistema saldrá sin colocar nada.")


if __name__ == "__main__":
    main()
