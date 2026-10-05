"""
VerificarRelieve.py — Contrasta los dos heightmaps del proyecto con el MDT25 real
del IGN (WCS servicios.idee.es) y busca huecos.

  1. alsasua_landscape_4033.r16 (terreno jugable, 7,2 km, altitud = 495 + q/64,
     fila 0 = norte): huecos/picos locales, orientación (prueba espejos y giros),
     sesgo, RMSE y peores puntos frente al MDT25 a 25 m.
  2. alsasua_relieve_lejano_2048.r16 (anillo de 60 km, altitud = q/32, fila 0 =
     sur): huecos/picos y 24 parches de 1 km comparados con el MDT25.
  3. Costura: borde del terreno jugable contra el anillo en ese mismo punto.

Primera pasada (2026-10-04): jugable corr 0.9999, RMSE 2,1 m, orientación
correcta; anillo RMSE 3,0 m en 24 parches; ningún hueco en los datos. La única
diferencia grande (~60 m más bajo que el MDT25, ~3 km al oeste, un parche de
unos 100 m) tiene pinta de ser la cantera de Olazti —el LiDAR sería posterior al
MDT25 y la cantera habría seguido bajando—; sin verificar sobre el terreno. Si en pantalla hay agujeros, no salen de estos datos.

Requiere: numpy, tifffile y red hacia servicios.idee.es.
Uso: python3 Tools/VerificarRelieve.py      (salida != 0 si encuentra huecos)
"""
import io
import os
import sys
import urllib.request

import numpy as np
import tifffile
from numpy.lib.stride_tricks import sliding_window_view

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TERRENO = os.path.join(RAIZ, "Content", "Terreno")
E0, N0 = 567951.0, 4749902.0
WCS = ("https://servicios.idee.es/wcs-inspire/mdt?service=WCS&version=2.0.1"
       "&request=GetCoverage&coverageId=Elevacion25830_25&format=image/tiff")


def mdt(x0, y0, x1, y1):
    """MDT25 del IGN, norte arriba, en metros."""
    url = WCS + "&subset=x(%f,%f)&subset=y(%f,%f)" % (x0, x1, y0, y1)
    return tifffile.imread(io.BytesIO(urllib.request.urlopen(url, timeout=120).read())).astype(float)


def desviacion_local(h):
    med = np.median(sliding_window_view(np.pad(h, 1, mode="edge"), (3, 3)), axis=(-1, -2))
    return h - med


def huecos(nombre, q, h, umbral):
    d = desviacion_local(h)
    n_huecos = int((d < -umbral).sum())
    print("%s: %.1f-%.1f m, q=0: %d, q=65535: %d, huecos >%gm: %d, picos >%gm: %d"
          % (nombre, h.min(), h.max(), (q == 0).sum(), (q == 65535).sum(),
             umbral, n_huecos, umbral, int((d > umbral).sum())))
    return int((q == 0).sum() + (q == 65535).sum()) + n_huecos


def main():
    fallos = 0
    lado = 7200.0
    q = np.fromfile(os.path.join(TERRENO, "alsasua_landscape_4033.r16"), "<u2").reshape(4033, 4033)
    h = 495 + q / 64.0
    fallos += huecos("jugable", q, h, 15)

    m = mdt(E0 - lado / 2, N0 - lado / 2, E0 + lado / 2, N0 + lado / 2)
    n = m.shape[0]
    idx = np.linspace(0, 4033, n + 1).astype(int)
    hb = np.array([[h[idx[i]:idx[i + 1], idx[j]:idx[j + 1]].mean() for j in range(n)] for i in range(n)])
    orientaciones = [("tal cual", hb), ("espejo N-S", np.flipud(hb)), ("espejo E-O", np.fliplr(hb)),
                     ("girado 180", hb[::-1, ::-1]), ("traspuesto", hb.T)]
    mejor = max(orientaciones, key=lambda o: np.corrcoef(o[1].ravel(), m.ravel())[0, 1])
    for nom, a in orientaciones:
        dd = a - m
        print("  %-11s corr=%.4f sesgo=%+.2f RMSE=%.2f max=%.1f"
              % (nom, np.corrcoef(a.ravel(), m.ravel())[0, 1], dd.mean(), np.sqrt((dd ** 2).mean()), abs(dd).max()))
    if mejor[0] != "tal cual":
        print("  OJO: el terreno casa mejor con '%s' que tal cual." % mejor[0])
        fallos += 1
    dd = hb - m
    for k in np.argsort(abs(dd).ravel())[::-1][:5]:
        r, c = divmod(k, n)
        print("  peor: UTM %.0f,%.0f modelo %.1f real %.1f dif %+.1f"
              % (E0 - lado / 2 + (c + .5) * lado / n, N0 + lado / 2 - (r + .5) * lado / n, hb[r, c], m[r, c], dd[r, c]))

    ql = np.fromfile(os.path.join(TERRENO, "alsasua_relieve_lejano_2048.r16"), "<u2").reshape(2048, 2048)
    g = ql * 0.03125
    fallos += huecos("anillo", ql, g, 50)
    gn = np.flipud(g)
    mp = 60000 / 2048
    rng = np.random.default_rng(1)
    difs = []
    while len(difs) < 24:
        e = E0 + rng.uniform(-28000, 28000)
        no = N0 + rng.uniform(-28000, 28000)
        if abs(e - E0) < 4000 and abs(no - N0) < 4000:
            continue
        real = mdt(e - 500, no - 500, e + 500, no + 500).mean()
        c, r = int((e - (E0 - 30000)) / mp), int(((N0 + 30000) - no) / mp)
        difs.append(gn[r - 17:r + 17, c - 17:c + 17].mean() - real)
    difs = np.array(difs)
    print("  anillo vs MDT25 (24 parches de 1 km): sesgo %+.1f RMSE %.1f max %.1f"
          % (difs.mean(), np.sqrt((difs ** 2).mean()), abs(difs).max()))

    print("\n%s" % ("Sin huecos en los datos." if not fallos else "%d hallazgo(s)." % fallos))
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())
