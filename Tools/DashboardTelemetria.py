"""
DashboardTelemetria.py - Receptor del svc-telemetria del plan §7.

Escucha UDP en localhost:7777 las lineas JSON que UAlsasuaTelemetria envia
cada segundo ({t, fps, ms, mb}) y pinta una linea de estado en vivo. Sin
dependencias externas.

Uso:
  python3 Tools/DashboardTelemetria.py [--puerto 7777]

El juego solo envia si se lanza con -AlsasuaTelemetria (o TelemetriaActivada
en DefaultGame.ini); sin eso, cero coste.
"""
import argparse
import json
import socket


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--puerto", type=int, default=7777)
    argumentos = parser.parse_args()

    oido = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    oido.bind(("127.0.0.1", argumentos.puerto))
    oido.settimeout(1.0)
    print("svc-telemetria escuchando en 127.0.0.1:%d (Ctrl+C para salir)"
          % argumentos.puerto)

    peor_fps = None
    try:
        while True:
            try:
                datos, _ = oido.recvfrom(2048)
            except socket.timeout:
                continue
            try:
                m = json.loads(datos.decode("utf-8").strip())
            except (ValueError, UnicodeDecodeError):
                continue
            if peor_fps is None or m.get("fps", 999) < peor_fps:
                peor_fps = m.get("fps")
            print("\r t=%6.1fs  fps=%5.1f  ms=%6.2f  mem=%5d MB   peor fps=%.1f "
                  % (m.get("t", 0), m.get("fps", 0), m.get("ms", 0),
                     m.get("mb", 0), peor_fps or 0), end="", flush=True)
    except KeyboardInterrupt:
        print("\nfin.")


if __name__ == "__main__":
    main()
