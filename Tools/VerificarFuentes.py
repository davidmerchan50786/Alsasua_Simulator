"""
VerificarFuentes.py — Errores de sintaxis que en este repo no los caza nadie.

Esto es un proyecto UE de Windows sin CI: no hay forma de compilar desde Linux ni
desde un contenedor, así que un error de sintaxis puede entrar en main y quedarse
ahí hasta que alguien abra Visual Studio. Pasó: dos puntos y coma acabaron dentro
de un comentario en AlsasuaFoliagePainter.cpp

    Grass02.AssetPath = Base + TEXT("/ruta")   // no hay grass_02; era la textura;

y AlsasuaManifa —214 cpp, el módulo gordo— dejó de compilar entero.

Esto no es un compilador ni lo pretende. Son dos comprobaciones baratas que
cazan justo lo que se cuela cuando se edita a ciegas:

  1. Sentencia cuyo punto y coma se lo ha tragado un comentario de línea.
  2. Llaves, paréntesis o corchetes descuadrados en un fichero.

Uso:  python3 Tools/VerificarFuentes.py      (salida != 0 si encuentra algo)
"""
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUENTE = os.path.join(RAIZ, "Source")

def cegar_literales(linea):
    """Devuelve la línea con el contenido de los literales sustituido por
    espacios, conservando las posiciones. Hace falta para localizar el '//' de
    verdad: la línea del fallo original llevaba TEXT("/multi_stylized_grass/…"),
    y buscar la primera barra a pelo se paraba dentro de la cadena."""
    fuera = list(linea)
    i, n, comilla = 0, len(linea), None
    while i < n:
        c = linea[i]
        if comilla:
            if c == '\\':
                fuera[i] = ' '
                if i + 1 < n:
                    fuera[i + 1] = ' '
                i += 2
                continue
            if c == comilla:
                comilla = None
            else:
                fuera[i] = ' '
        elif c in '"\'':
            comilla = c
        i += 1
    return ''.join(fuera)


def sin_comentarios_ni_cadenas(texto):
    """Quita comentarios y literales para que no descuadren el recuento."""
    fuera = []
    i, n = 0, len(texto)
    while i < n:
        c = texto[i]
        if c == '/' and i + 1 < n and texto[i + 1] == '/':
            while i < n and texto[i] != '\n':
                i += 1
        elif c == '/' and i + 1 < n and texto[i + 1] == '*':
            i += 2
            while i + 1 < n and not (texto[i] == '*' and texto[i + 1] == '/'):
                i += 1
            i += 2
        elif c in '"\'':
            comilla = c
            i += 1
            while i < n and texto[i] != comilla:
                i += 2 if texto[i] == '\\' else 1
            i += 1
        else:
            fuera.append(c)
            i += 1
    return ''.join(fuera)


def main():
    fallos = []
    revisados = 0

    for base, _, ficheros in os.walk(FUENTE):
        for nombre in sorted(ficheros):
            if not nombre.endswith((".cpp", ".h")):
                continue
            ruta = os.path.join(base, nombre)
            rel = os.path.relpath(ruta, RAIZ)
            with open(ruta, encoding="utf-8", errors="ignore") as fh:
                texto = fh.read()
            revisados += 1

            # 1. Punto y coma comido por el comentario.
            for num, linea in enumerate(texto.splitlines(), 1):
                cegada = cegar_literales(linea)
                pos = cegada.find('//')
                if pos < 0:
                    continue
                comentario = linea[pos + 2:].rstrip()
                if not comentario.endswith(';'):
                    continue
                codigo = linea[:pos].rstrip()
                if not codigo or codigo.endswith((';', '{', '}', ',', ':', '\\')):
                    continue
                # Una sentencia de verdad lleva asignación o llamada.
                if '=' not in codigo and '(' not in codigo:
                    continue
                fallos.append("%s:%d  falta ';' — se lo ha llevado el comentario\n"
                              "      %s" % (rel, num, linea.strip()))

            # 2. Delimitadores descuadrados.
            limpio = sin_comentarios_ni_cadenas(texto)
            for abre, cierra, que in (('{', '}', 'llaves'),
                                      ('(', ')', 'paréntesis'),
                                      ('[', ']', 'corchetes')):
                d = limpio.count(abre) - limpio.count(cierra)
                if d != 0:
                    fallos.append("%s  %s descuadrados: %+d" % (rel, que, d))

    print("%d ficheros .cpp/.h revisados.\n" % revisados)
    if not fallos:
        print("Sin hallazgos. No garantiza que compile — sólo que no tiene")
        print("estos dos fallos, que son los que se cuelan al editar a ciegas.")
        return 0

    for f in fallos:
        print("  " + f)
    print("\n%d hallazgo(s)." % len(fallos))
    return 1


if __name__ == "__main__":
    sys.exit(main())
