"""
ProbarMatrizActivacion.py - La matriz de activacion que exige la Fase 5.

El plan (PLAN_ARQUITECTURA_MICROSERVICIOS.md §6) la pide como verificacion de
Fase 5: todo ON, todo OFF, y cada pilar OFF individualmente. Con el cargador
runtime (UAlsasuaCargadorPlugins) cada combo es una linea de comandos distinta:

  -AlsasuaPlugins=Ninguno          -> mundo desnudo, cero pilares
  -AlsasuaPlugins=GF_A,GF_B,...    -> solo esos
  (sin parametro)                  -> lo que diga DefaultGame.ini (todo ON)

Cada combo arranca el editor en modo juego (-game -nullrhi -nosound), espera al
marcador "[Plugins] N plugins GF lanzados" del cargador, deja madurar un tramo
para que los consumidores pisen los caminos nulos, y clasifica:

  OK       lanzo los esperados y no hay Fatal/assertion/crash en el log
  FALLA    fatal, assertion o crash (se cita la linea)
  TARDÓ    no llego al marcador dentro del tiempo

Requiere la variable de entorno UE_ROOT (raiz del motor); nada de rutas fijas
de maquina, por la regla 7 de CLAUDE.md.

Uso:
  python3 Tools/ProbarMatrizActivacion.py              # matriz completa
  python3 Tools/ProbarMatrizActivacion.py --rapida     # TodoON + Ninguno + Sin_GF_Clima
  python3 Tools/ProbarMatrizActivacion.py --combo Ninguno
  python3 Tools/ProbarMatrizActivacion.py --segundos 120

Salida != 0 si algun combo falla.
"""
import argparse
import os
import re
import subprocess
import sys
import time

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROYECTO = os.path.join(RAIZ, "AlsasuaSimulator.uproject")
LOGS_DIR = os.path.join(RAIZ, "Saved", "Logs", "Matriz")

RE_LANZADOS = re.compile(r"\[Plugins\] (\d+) plugins GF lanzados de (\d+)")
RE_MAL = [
    re.compile(r"Fatal error"),
    re.compile(r"Assertion failed"),
    re.compile(r"Unhandled Exception"),
    re.compile(r"^LogWindows: Error:", re.M),
]


def plugins_del_proyecto():
    carpeta = os.path.join(RAIZ, "Plugins")
    return sorted(d for d in os.listdir(carpeta)
                  if d.startswith("GF_") and
                  os.path.isdir(os.path.join(carpeta, d)))


def combos(rapida=False):
    pilares = plugins_del_proyecto()
    yield "TodoON", None
    yield "Ninguno", []
    objetivo = ["GF_Clima"] if rapida else pilares
    for p in objetivo:
        yield "Sin_" + p, [x for x in pilares if x != p]


def escanear_log(texto):
    """Primera linea mala que encuentre, o None."""
    for patron in RE_MAL:
        m = patron.search(texto)
        if m:
            linea = texto[:m.end()].splitlines()[-1]
            return linea.strip()[:160]
    return None


def correr(nombre, lista, raiz_motor, segundos_espera, asentamiento):
    os.makedirs(LOGS_DIR, exist_ok=True)
    ruta_log = os.path.join(LOGS_DIR, "Combo_%s.log" % nombre)

    args = [
        os.path.join(raiz_motor, "Engine", "Binaries", "Win64",
                     "UnrealEditor-Cmd.exe"),
        PROYECTO,
        "-game", "-nullrhi", "-nosound", "-unattended", "-nosplash",
        "-forcelogflush", "-abslog=" + ruta_log,
    ]
    if lista is not None:
        args.append("-AlsasuaPlugins=" +
                    ("Ninguno" if not lista else ",".join(lista)))

    inicio = time.time()
    proceso = subprocess.Popen(args, stdout=subprocess.DEVNULL,
                               stderr=subprocess.DEVNULL)
    esperados = len(lista) if lista is not None else None
    lanzados = None
    mal = None

    try:
        while True:
            if time.time() - inicio > segundos_espera:
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
            m = RE_LANZADOS.search(texto)
            if m:
                lanzados = int(m.group(1))
                if esperados is not None and lanzados != esperados:
                    mal = "lanzo %d de %d esperados" % (lanzados, esperados)
                    break
                # Madurar: dar tiempo a que DirectorArranque y consumidores
                # pisen los caminos con pilares ausentes.
                if time.time() - inicio > asentamiento:
                    break
            time.sleep(2)
    finally:
        if proceso.poll() is None:
            proceso.kill()
            proceso.wait()

    if mal:
        return "FALLA", mal
    if lanzados is None:
        return "TARDO", "sin marcador del cargador en %ds" % segundos_espera
    return "OK", "%d plugins lanzados en %ds" % (
        lanzados, int(time.time() - inicio))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--rapida", action="store_true")
    parser.add_argument("--combo", help="probar un solo combo por nombre")
    parser.add_argument("--solo", help="activar SOLO estos plugins (lista separada por comas)")
    parser.add_argument("--segundos", type=int, default=180,
                        help="techo por combo (default 180)")
    parser.add_argument("--asentamiento", type=int, default=45,
                        help="segundos tras el marcador antes de dar por bueno")
    argumentos = parser.parse_args()

    raiz_motor = os.environ.get("UE_ROOT", "")
    exe = os.path.join(raiz_motor, "Engine", "Binaries", "Win64",
                       "UnrealEditor-Cmd.exe")
    if not os.path.exists(exe):
        print("Define UE_ROOT con la raiz del motor (p.ej. set UE_ROOT=C:\\Program Files\\Epic Games\\UE_5.8)")
        return 2

    resultados = []
    if argumentos.solo:
        lista = [x for x in argumentos.solo.split(",") if x]
        estado, detalle = correr("Solo_" + "_".join(lista), lista,
                                 raiz_motor, argumentos.segundos,
                                 argumentos.asentamiento)
        resultados.append((argumentos.solo, estado, detalle))
        print("  %-24s %-6s %s" % (argumentos.solo, estado, detalle), flush=True)
        return 1 if estado != "OK" else 0
    for nombre, lista in combos(argumentos.rapida):
        if argumentos.combo and nombre != argumentos.combo:
            continue
        estado, detalle = correr(nombre, lista, raiz_motor,
                                 argumentos.segundos, argumentos.asentamiento)
        resultados.append((nombre, estado, detalle))
        print("  %-24s %-6s %s" % (nombre, estado, detalle), flush=True)

    fallos = sum(1 for _, e, _ in resultados if e != "OK")
    print("\n%d/%d combos OK." % (len(resultados) - fallos, len(resultados)))
    return 1 if fallos else 0


if __name__ == "__main__":
    sys.exit(main())
