// Assets/Tests/AutoRunTests.cs
// Auto-ejecuta los EditMode tests al abrir el proyecto.
// Resultados impresos en la Consola de Unity y guardados en:
//   Assets/Tests/TestResults_AutoRun.txt
//
// Se ejecuta UNA SOLA VEZ por sesión del Editor.

#if UNITY_EDITOR
using System.Collections.Generic;
using System.IO;
using UnityEditor;
using UnityEditor.TestTools.TestRunner.Api;
using UnityEngine;

[InitializeOnLoad]
public static class AutoRunTests
{
    private const string SESSION_KEY  = "AlsasuaAutoTestRan";
    private const string RESULTS_FILE = "Assets/Tests/TestResults_AutoRun.txt";

    static AutoRunTests()
    {
        if (SessionState.GetBool(SESSION_KEY, false)) return;
        SessionState.SetBool(SESSION_KEY, true);
        EditorApplication.delayCall += LanzarTests;
    }

    private static void LanzarTests()
    {
        var api = ScriptableObject.CreateInstance<TestRunnerApi>();
        api.RegisterCallbacks(new Listener(api));
        api.Execute(new ExecutionSettings(new Filter
        {
            testMode      = TestMode.EditMode,
            assemblyNames = new[] { "AlsasuaTests" }
        }));
    }

    // ── Listener ─────────────────────────────────────────────────────────

    private class Listener : ICallbacks
    {
        private readonly TestRunnerApi        _api;
        private readonly List<string>         _lines  = new List<string>();
        private          int                  _pass, _fail, _skip;

        public Listener(TestRunnerApi api) => _api = api;

        public void RunStarted(ITestAdaptor root)
        {
            _lines.Add("═══ ALSASUA SIMULATOR — TEST RUN ═══");
            Debug.Log("[AutoRunTests] Ejecutando tests EditMode...");
        }

        public void TestStarted(ITestAdaptor test) { }

        public void TestFinished(ITestResultAdaptor result)
        {
            // Saltar nodos intermedios (suites/clases)
            if (!result.Test.IsSuite)
            {
                string icono = result.TestStatus == TestStatus.Passed  ? "✓" :
                               result.TestStatus == TestStatus.Failed   ? "✗" : "○";
                string linea = $"  {icono} {result.Test.Name} ({result.Duration * 1000:F0}ms)";
                _lines.Add(linea);

                if      (result.TestStatus == TestStatus.Passed)
                    _pass++;
                else if (result.TestStatus == TestStatus.Failed)
                {
                    _fail++;
                    _lines.Add($"    FALLO: {result.Message?.Trim()}");
                    Debug.LogError($"[FAIL] {result.Test.Name}\n{result.Message}");
                }
                else
                    _skip++;
            }
        }

        public void RunFinished(ITestResultAdaptor result)
        {
            int total = _pass + _fail + _skip;
            bool ok   = _fail == 0;

            _lines.Add("");
            _lines.Add($"RESULTADO: {(ok ? "✓ TODOS PASARON" : $"✗ {_fail} FALLARON")}");
            _lines.Add($"Pasados : {_pass}/{total}");
            if (_fail  > 0) _lines.Add($"Fallados: {_fail}/{total}");
            if (_skip  > 0) _lines.Add($"Omitidos: {_skip}/{total}");
            _lines.Add($"Duración: {result.Duration:F2}s");

            // Imprimir en Consola
            string resumen = string.Join("\n", _lines);
            if (ok) Debug.Log($"[AutoRunTests]\n{resumen}");
            else    Debug.LogWarning($"[AutoRunTests]\n{resumen}");

            // Guardar fichero de texto
            try { File.WriteAllText(RESULTS_FILE, resumen); }
            catch { /* si no puede escribir, no pasa nada */ }

            Object.DestroyImmediate(_api);
        }
    }
}
#endif
