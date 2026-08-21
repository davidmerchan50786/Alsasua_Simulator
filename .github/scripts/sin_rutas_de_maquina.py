#!/usr/bin/env python3
"""
Gate de CLAUDE.md §7: ningún script de Tools/ puede llevar una ruta absoluta de
una máquina concreta.

Estaba escrito como un grep -E con '[A-Z]:\\\\', y en expresión regular
extendida eso son DOS barras literales: cazaba la forma escapada de una cadena
normal ("F:\\Epic Games\\...") y dejaba pasar el raw string, r"C:\Epic Games",
que lleva una sola barra y es la forma más cómoda de escribir una ruta de
Windows. Un gate con un falso negativo en el caso más probable no es un gate.

En Python el patrón se lee sin capas de comillas de por medio, y de paso se
distingue el comentario del código de verdad usando el tokenizador en vez de
adivinar con otro grep.

Salida != 0 si encuentra algo.
"""
import io
import os
import sys
import tokenize

RAIZ = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HERRAMIENTAS = os.path.join(RAIZ, "Tools")

# Letra de unidad + dos puntos + una barra invertida. Una, no dos: aquí no hay
# escapado de shell ni de ERE que valga.
def tiene_ruta(texto):
    for i, c in enumerate(texto):
        if c == ":" and i > 0 and texto[i - 1].isalpha() and texto[i - 1].isupper():
            if i + 1 < len(texto) and texto[i + 1] == "\\":
                # Descartar "C:\" en medio de una palabra (p.ej. "ABC:\\").
                if i >= 2 and (texto[i - 2].isalnum() or texto[i - 2] == "_"):
                    continue
                return True
    return False


# CLAUDE.md §7 admite una excepción: la ruta a una instalación externa al repo
# (Blender, CitySample), y sólo como valor por defecto de un parámetro. Se marca
# en la línea, igual que el "// ejes ok" de VerificarFuentes.py: incómodo a
# propósito, para que nadie lo ponga sin mirarlo.
MARCA_EXCEPCION = "# ruta externa ok"


def cadenas_de(ruta):
    """Sólo literales de cadena y su línea. Mencionar una ruta en un comentario
    de migración es legítimo; ponerla en el código no."""
    with open(ruta, "rb") as fh:
        try:
            for tok in tokenize.tokenize(fh.readline):
                if tok.type == tokenize.STRING:
                    yield tok.start[0], tok.string
        except (tokenize.TokenError, IndentationError, SyntaxError):
            return


def main():
    hallazgos = []
    for nombre in sorted(os.listdir(HERRAMIENTAS)):
        if not nombre.endswith(".py"):
            continue
        ruta = os.path.join(HERRAMIENTAS, nombre)
        with open(ruta, encoding="utf-8", errors="ignore") as fh:
            lineas = fh.read().splitlines()
        # Un docstring que EXPLICA la regla cita la ruta a propósito; se salta el
        # que es la cabecera del módulo o de una función.
        for linea, literal in cadenas_de(ruta):
            if 0 < linea <= len(lineas) and MARCA_EXCEPCION in lineas[linea - 1]:
                continue
            # Quitar el prefijo (r, b, f, u y combinaciones) antes de mirar si es
            # un docstring: ue5_materials_barrio.py abre con r""" y se colaba por
            # delante de esta guarda.
            desnudo = literal.lstrip("rRbBfFuU")
            if desnudo.startswith(('"""', "'''")):
                continue
            if tiene_ruta(literal):
                hallazgos.append("Tools/%s:%d  %s" % (nombre, linea, literal.strip()))

    if hallazgos:
        print("Rutas absolutas de máquina en Tools/ (CLAUDE.md §7):")
        for h in hallazgos:
            print("  " + h)
        print("\nDeriva la raíz de la ubicación del propio script")
        print("(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))) o, en")
        print("los de editor, de unreal.Paths.project_dir().")
        return 1

    print("Ninguna ruta absoluta de máquina en Tools/.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
