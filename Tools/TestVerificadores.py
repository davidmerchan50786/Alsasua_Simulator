"""
TestVerificadores.py — Pruebas de los verificadores. La red necesita su red.

VerificarFuentes.py lleva once comprobaciones y VerificarModulos.py dos, y las
trece se validaron a mano: metiendo el fallo, viendo saltar el aviso y quitándolo
otra vez. Esa validación se pierde en cuanto termina la sesión, y una
comprobación que ha dejado de saltar es peor que no tenerla, porque el informe
sigue diciendo "sin hallazgos".

No es hipotético. La comprobación 10 —método declarado y no definido— no saltaba
en su primera versión: aceptaba una función libre homónima del mismo módulo como
si fuera la definición del miembro, que era exactamente el caso que la motivaba.
Sólo se vio porque se probó a propósito.

Cada prueba monta un árbol de mentira con el fallo, comprueba que el verificador
lo canta, y comprueba con la versión sana que se calla. Lo segundo importa tanto
como lo primero: un verificador que avisa siempre se ignora en una semana.

Uso:  python3 Tools/TestVerificadores.py      (salida != 0 si alguna falla)
"""
import os
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import VerificarFuentes as VF          # noqa: E402
import VerificarModulos as VM          # noqa: E402
import VerificarPlugins as VP          # noqa: E402


# ── andamiaje ───────────────────────────────────────────────────────────────

class Arbol:
    """Un Source/ de mentira, con la forma que UBT espera."""

    def __init__(self):
        self.raiz = tempfile.mkdtemp(prefix="alsasua_test_")

    def escribir(self, rel, texto):
        ruta = os.path.join(self.raiz, rel)
        os.makedirs(os.path.dirname(ruta), exist_ok=True)
        with open(ruta, "w", encoding="utf-8") as fh:
            fh.write(texto)
        return ruta

    def borrar(self):
        shutil.rmtree(self.raiz, ignore_errors=True)


class MentiraRaiz:
    """Apunta RAIZ/FUENTE de los verificadores al arbol de mentira.

    Sin esto los relpath comparan rutas del arbol (en %TEMP%, otra unidad) con
    la constante RAIZ del repo real y ValueError: path is on mount...
    """

    def __init__(self, arbol):
        self.arbol = arbol
        self.antes = []

    def __enter__(self):
        for mod in (VF, VM):
            if hasattr(mod, "RAIZ"):
                self.antes.append((mod, "RAIZ", getattr(mod, "RAIZ")))
                setattr(mod, "RAIZ", self.arbol.raiz)
            if hasattr(mod, "FUENTE"):
                self.antes.append((mod, "FUENTE", getattr(mod, "FUENTE")))
                setattr(mod, "FUENTE", self.arbol.raiz)
        return self.arbol

    def __exit__(self, *excepcion):
        for mod, nombre, valor in reversed(self.antes):
            setattr(mod, nombre, valor)
        return False


RESULTADOS = []


def comprobar(nombre, malo, bueno):
    """malo() tiene que devolver hallazgos; bueno() tiene que devolver ninguno."""
    try:
        h_malo = malo()
        h_bueno = bueno()
    except Exception as e:                       # noqa: BLE001
        RESULTADOS.append((nombre, "EXCEPCIÓN", "%s: %s" % (type(e).__name__, e)))
        return
    if not h_malo:
        RESULTADOS.append((nombre, "NO SALTA", "el fallo está y no lo canta"))
    elif h_bueno:
        RESULTADOS.append((nombre, "FALSO POSITIVO",
                           "la versión sana también avisa: %s" % h_bueno[0][:90]))
    else:
        RESULTADOS.append((nombre, "ok", ""))


# ── comprobación 4: CVars g.* sin registrar ─────────────────────────────────

def test_cvars():
    def caso(registrar):
        with MentiraRaiz(Arbol()) as a:
            try:
                reg = ('static TAutoConsoleVariable<int32> CVarX(\n'
                       '    TEXT("g.Prueba"), 0, TEXT(""));\n') if registrar else ''
                a.escribir("M/Private/A.cpp",
                           reg + 'void F() { IConsoleManager::Get().FindConsoleVariable(TEXT("g.Prueba")); }\n')
                return list(VF.cvars_sin_registrar(a.raiz))
            finally:
                a.borrar()
    comprobar("4  CVar g.* sin registrar",
              lambda: caso(False), lambda: caso(True))


# ── comprobación 8: include a cabecera que no existe ────────────────────────

def test_include_roto():
    def caso(existe):
        a = Arbol()
        try:
            if existe:
                a.escribir("M/Public/AlsasuaCosa.h", "#pragma once\n")
            a.escribir("M/Private/A.cpp", '#include "AlsasuaCosa.h"\n')
            validas = VF.cabeceras_del_proyecto(a.raiz)
            generadas = {h.replace(".h", ".generated.h") for h in validas}
            texto = open(os.path.join(a.raiz, "M/Private/A.cpp"), encoding="utf-8").read()
            return VF.includes_rotos("A.cpp", texto, validas, generadas)
        finally:
            a.borrar()
    comprobar("8  include a cabecera retirada",
              lambda: caso(False), lambda: caso(True))


# ── comprobación 10: declarado y no definido ───────────────────────────────

CAB_10 = '''#pragma once
class MI_API UCosa
{
    GENERATED_BODY()
public:
    bool Hacer(int32 X);
};
'''

def test_declarada_sin_definir():
    def caso(como):
        with MentiraRaiz(Arbol()) as a:
            try:
                a.escribir("M/Public/Cosa.h", CAB_10)
                cuerpos = {
                    # el fallo que motivó la comprobación: definida sólo como
                    # función libre, en el namespace anónimo
                    "libre": 'namespace\n{\n    bool Hacer(int32 X) { return true; }\n}\n',
                    "miembro": 'bool UCosa::Hacer(int32 X) { return true; }\n',
                }
                a.escribir("M/Private/Cosa.cpp", cuerpos[como])
                return VF.declaradas_sin_definir(a.raiz)
            finally:
                a.borrar()
    comprobar("10 declarado y no definido",
              lambda: caso("libre"), lambda: caso("miembro"))


# �� comprobaci�n 11: UObject* sin UPROPERTY ��������������������������������

def test_uproperty():
    def caso(con):
        with MentiraRaiz(Arbol()) as a:
            try:
                marca = "    UPROPERTY()\n" if con else ""
                a.escribir("M/Public/Cosa.h",
                           "#pragma once\n"
                           "class MI_API UCosa\n{\n"
                           "    GENERATED_BODY()\n"
                           "private:\n"
                           + marca +
                           "    AActor* Objetivo = nullptr;\n};\n")
                return VF.punteros_sin_uproperty(a.raiz)
            finally:
                a.borrar()
    comprobar("11 UObject* sin UPROPERTY",
              lambda: caso(False), lambda: caso(True))


# ── VerificarModulos: dependencia sin declarar, y ciclo ────────────────────

BUILD = 'public class %s : ModuleRules {\n  public %s(x) { PublicDependencyModuleNames.AddRange(new string[] { %s }); }\n}\n'


def _modulos(deps_a, deps_b):
    a = Arbol()
    a.escribir("A/A.Build.cs", BUILD % ("A", "A", deps_a))
    a.escribir("B/B.Build.cs", BUILD % ("B", "B", deps_b))
    a.escribir("A/Public/Ah.h", "#pragma once\n")
    a.escribir("B/Public/Bh.h", "#pragma once\n")
    a.escribir("B/Private/B.cpp", '#include "Ah.h"\n')   # B usa A
    return a


def test_modulos():
    def correr(arbol):
        with MentiraRaiz(arbol):
            import io
            import contextlib
            buf = io.StringIO()
            with contextlib.redirect_stdout(buf):
                rc = VM.main()
            return [buf.getvalue()] if rc else []

    def dep(declarada):
        a = _modulos('"Core"', '"Core", "A"' if declarada else '"Core"')
        try:
            return correr(a)
        finally:
            a.borrar()

    def ciclo(hay):
        a = _modulos('"Core", "B"' if hay else '"Core"', '"Core", "A"')
        try:
            return correr(a)
        finally:
            a.borrar()

    comprobar("M1 dependencia usada sin declarar",
              lambda: dep(False), lambda: dep(True))
    comprobar("M2 ciclo entre módulos",
              lambda: ciclo(True), lambda: ciclo(False))


# �� VerificarPlugins: activable fantasma y cargador vaciado ������������������

UPLUGIN = '{ "FileVersion": 3, "FriendlyName": "Mentira" }\n'

CARGADOR = ('void UAlsasuaCargadorPlugins::ActivarTodos()\n{\n'
            '\tGFS.LoadAndActivateGameFeaturePlugin(URL, Completa);\n}\n'
            'void UAlsasuaCargadorPlugins::DesactivarTodos()\n{\n'
            '\tGFS.DeactivateGameFeaturePlugin(URL);\n'
            '\tGFS.UnloadGameFeaturePlugin(URL);\n}\n')


def test_plugins():
    def caso(con_plugin, con_llamadas):
        with MentiraRaiz(Arbol()) as a:
            try:
                if con_plugin:
                    a.escribir("Plugins/GF_Mentira/GF_Mentira.uplugin", UPLUGIN)
                a.escribir("Config/DefaultGame.ini",
                           "[/Script/AlsasuaKernel.AlsasuaCargadorPlugins]\n"
                           "+PluginsActivables=GF_Mentira\n")
                if con_llamadas:
                    a.escribir("Source/AlsasuaKernel/Private/Plugins/"
                               "AlsasuaCargadorPlugins.cpp", CARGADOR)
                return VP.activables_inexistentes(a.raiz) + VP.cargador_roto(a.raiz)
            finally:
                a.borrar()

    comprobar("P1 plugin activable inexistente",
              lambda: caso(False, True), lambda: caso(True, True))
    comprobar("P2 cargador sin llamadas",
              lambda: caso(True, False), lambda: caso(True, True))


def main():
    test_cvars()
    test_include_roto()
    test_declarada_sin_definir()
    test_uproperty()
    test_modulos()
    test_plugins()

    print("Pruebas de los verificadores\n")
    fallos = 0
    for nombre, estado, detalle in RESULTADOS:
        print("  %-34s %s%s" % (nombre, estado, ("  — " + detalle) if detalle else ""))
        if estado != "ok":
            fallos += 1

    print()
    if fallos:
        print("%d de %d comprobaciones no hacen lo que dicen." % (fallos, len(RESULTADOS)))
        print("Una que ha dejado de saltar es peor que no tenerla: el informe")
        print("sigue diciendo 'sin hallazgos'.")
        return 1
    print("Las %d saltan con el fallo y se callan sin él." % len(RESULTADOS))
    return 0


if __name__ == "__main__":
    sys.exit(main())
