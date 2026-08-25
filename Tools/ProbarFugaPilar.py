"""
ProbarFugaPilar.py - Criterio 9 del plan: desactivar un pilar en runtime deja
cero objetos residuales (verificacion con GC, el equivalente a `obj gc`).

Arranca el editor en modo juego con UN pilar activo y ejecuta por ExecCmds
`Alsasua.Pilar.Fuga <pilar>`; el kernel espera a que el pilar este Active,
mide objetos /Script/<pilar>, desactiva, GC x2 y vuelve a medir:

  [Fuga] GF_X: antes=N despues=0 LIMPIO   -> OK
  [Fuga] GF_X: antes=N despues=M RESIDUO  -> FALLA
  abortada / timeout                      -> TARDE

Uso:
  python3 Tools/ProbarFugaPilar.py --solo GF_Clima
  python3 Tools/ProbarFugaPilar.py            # todos los pilares, uno por uno

Salida != 0 si algun pilar falla.
"""
import argparse
import os
import re
import subprocess
import sys
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROYECTO = os.path.join(RAIZ, "AlsasuaSimulator.uproject")
LOGS_DIR = os.path.join(RAIZ, "Saved", "Logs", "Fuga")

RE_FUGA = re.compile(r"\[Fuga\] (\w+): antes=(\d+) despues=(\d+) (\w+)")
RE_MAL = [
    re.compile(r"Fatal error"),
    re.compile(r"Assertion failed"),
    re.compile(r"Unhandled Exception"),
    re.compile(r"^LogWindows: Error:", re.M),
]


def pilares_del_proyecto():
    carpeta = os.path.join(RAIZ, "Plugins")
    return sorted(d for d in os.listdir(carpeta)
                  if d.startswith("GF_") and
                  os.path.isdir(os.path.join(carpeta, d)))


def escanear_log(texto):
    for patron in RE_MAL:
        m = patron.search(texto)
        if m:
            linea = texto[:m.end()].splitlines()[-1]
            return linea.strip()[:160]
    return None


def correr(pilar, raiz_motor, segundos):
    os.makedirs(LOGS_DIR, exist_ok=True)
    ruta_log = os.path.join(LOGS_DIR, "Fuga_%s.log" % pilar)

    args = [
        os.path.join(raiz_motor, "Engine", "Binaries", "Win64",
                     "UnrealEditor-Cmd.exe"),
        PROYECTO,
        "-game", "-nullrhi", "-nosound", "-unattended", "-nosplash",
        "-forcelogflush", "-abslog=" + ruta_log,
        "-AlsasuaPlugins=" + pilar,
        "-AlsasuaFugaPilar=" + pilar,
    ]

    inicio = time.time()
    if os.path.exists(ruta_log):
        os.remove(ruta_log)  # no confundir restos de la corrida anterior
    proceso = subprocess.Popen(args, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
    resultado = None
    mal = None

    try:
        while True:
            if time.time() - inicio > segundos:
                break
            if proceso.poll() is not None and proceso.returncode != 0:
                mal = "el proceso salio con codigo %d" % proceso.returncode
                break
            try:
                texto = open(ruta_log, encoding="utf-8", errors="ignore").read()
            except OSError:
                time.sleep(2)
                continue
            mal = mal or escanear_log(texto)
            if mal:
                break
            m = RE_FUGA.search(texto)
            if m and m.group(1) == pilar:
                estado_txt = m.group(4)
                despues = int(m.group(3))
                if estado_txt == "LIMPIO":
                    resultado = ("OK", "antes=%s despues=%s LIMPIO (%ds)" %
                                 (m.group(2), despues,
                                  int(time.time() - inicio)))
                elif estado_txt == "RESIDUO":
                    resultado = ("FALLA", "%d objetos residuales tras GC" %
                                 despues)
                else:
                    resultado = ("TARDE", "prueba abortada: sin Active en 30s")
                break
            time.sleep(2)
    finally:
        if proceso.poll() is None:
            proceso.kill()
            proceso.wait()

    if mal:
        return "FALLA", mal
    return resultado or ("TARDE", "sin linea [Fuga] en %ds" % segundos)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--solo", help="verificar un solo pilar")
    parser.add_argument("--segundos", type=int, default=240,
                        help="techo por pilar (default 240)")
    argumentos = parser.parse_args()

    raiz_motor = os.environ.get("UE_ROOT", "")
    exe = os.path.join(raiz_motor, "Engine", "Binaries", "Win64",
                       "UnrealEditor-Cmd.exe")
    if not os.path.exists(exe):
        print("Define UE_ROOT con la raiz del motor")
        return 2

    objetivos = [argumentos.solo] if argumentos.solo else pilares_del_proyecto()
    fallos = 0
    for pilar in objetivos:
        estado, detalle = correr(pilar, raiz_motor, argumentos.segundos)
        print("  %-18s %-6s %s" % (pilar, estado, detalle), flush=True)
        if estado != "OK":
            fallos += 1

    print("\n%d/%d pilares sin fuga." % (len(objetivos) - fallos, len(objetivos)))
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())
