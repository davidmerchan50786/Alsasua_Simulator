"""
VerificarGuardado.py — Campos del save que se guardan y no se cargan, o al revés.

UAlsasuaLegacySaveGame declara los campos y UGuardadoSubsystem los escribe en
GuardarEnSlot y los lee en CargarDeSlot. Que las dos mitades cuadren no lo
comprueba nada: un campo que se escribe y no se lee compila igual, se guarda
igual y se pierde igual, y no hay forma de notarlo salvo jugando, guardando,
cargando y fijándose en ese detalle concreto.

Pasó con dos:

  DisfrazType / DisfrazDurability   se guardaban y no se cargaban. Quien salvara
                                    encubierto reaparecía a cara descubierta.
  CompletedMissionIDs               ni se guardaba ni se aplicaba, aunque el log
                                    de carga lo contaba. El progreso de campaña
                                    se perdía entero en cada carga.

Uso:  python3 Tools/VerificarGuardado.py      (salida != 0 si hay descuadre)
"""
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CABECERA = os.path.join(RAIZ, "Source", "AlsasuaGameplay", "Public", "AlsasuaLegacySaveGame.h")
CUERPO = os.path.join(RAIZ, "Source", "AlsasuaGameplay", "Private", "GuardadoSubsystem.cpp")

# UPROPERTY() <tipo> <Nombre> [= valor];  — el tipo puede llevar plantilla.
RE_CAMPO = re.compile(r'UPROPERTY\([^)]*\)\s*[\w:<>,\s\*]+?\s(\w+)\s*(?:=[^;]*)?;')

# Campos de fontanería del propio save: no los mueve GuardarEnSlot/CargarDeSlot
# como estado de partida, y esperarlos en las dos mitades sería ruido.
FONTANERIA = {"SaveVersion", "bValido", "Fecha", "SaveSlotName", "UserIndex"}

# Un campo cuyo comentario en el header diga SIN IMPLEMENTAR es un hueco
# reservado a propósito, no un descuadre. Se conserva para no romper los saves
# que ya lo llevan escrito. Marcarlo cuesta lo justo para que nadie lo use como
# alfombra.
RE_RESERVADO = re.compile(r'SIN IMPLEMENTAR')


def main():
    with open(CABECERA, encoding="utf-8") as fh:
        campos = RE_CAMPO.findall(fh.read())
    with open(CUERPO, encoding="utf-8") as fh:
        cuerpo = fh.read()

    # GuardarEnSlot escribe (S->Campo = ...), CargarDeSlot lee (usa S->Campo).
    corte = cuerpo.find("CargarDeSlot")
    guardar, cargar = cuerpo[:corte], cuerpo[corte:]

    escritos = {c for c in campos if re.search(r'S->%s\s*=' % c, guardar)}
    leidos = {c for c in campos if re.search(r'S->%s\b' % c, cargar)}

    with open(CABECERA, encoding="utf-8") as fh:
        lineas = fh.read().splitlines()
    reservados = set()
    for i, l in enumerate(lineas):
        m = RE_CAMPO.search(l)
        if m and any(RE_RESERVADO.search(x) for x in lineas[max(0, i - 8):i]):
            reservados.add(m.group(1))

    utiles = [c for c in campos if c not in FONTANERIA and c not in reservados]
    if reservados:
        print("  reservados sin implementar: %s\n" % ", ".join(sorted(reservados)))
    print("%d campos en el save (%d de fontanería, %d de partida).\n"
          % (len(campos), len(FONTANERIA & set(campos)), len(utiles)))

    solo_esc = sorted(c for c in utiles if c in escritos and c not in leidos)
    solo_lee = sorted(c for c in utiles if c in leidos and c not in escritos)
    ninguno = sorted(c for c in utiles if c not in escritos and c not in leidos)

    print("  %-28s %-9s %s" % ("campo", "se guarda", "se carga"))
    for c in utiles:
        print("  %-28s %-9s %s" % (c, "sí" if c in escritos else "NO",
                                   "sí" if c in leidos else "NO"))

    fallos = []
    if solo_esc:
        fallos.append("se guardan y no se cargan: %s" % ", ".join(solo_esc))
    if solo_lee:
        fallos.append("se cargan y no se guardan: %s" % ", ".join(solo_lee))
    if ninguno:
        fallos.append("ni se guardan ni se cargan: %s" % ", ".join(ninguno))

    if not fallos:
        print("\n  El round-trip cuadra.")
        return 0

    print()
    for f in fallos:
        print("  %s" % f)
    print("\n  Un campo descuadrado no rompe nada y no avisa: se guarda igual y se")
    print("  pierde igual. Míralo antes de darlo por intencionado.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
