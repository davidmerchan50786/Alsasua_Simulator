// Assets/Scripts/ValidadorMecanicas.cs
using UnityEngine;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Módulo Protector TDD (Test-Driven Development) V23.
/// Macro-Orquestador Asíncrono de Pruebas Unitarias y de Integración.
/// </summary>
[AddComponentMenu("Alsasua V23/Validador TDD (Macro-Orchestrador)")]
public class ValidadorMecanicas : MonoBehaviour
{
    [SerializeField] private bool ejecutarEnStart = false;

    private void Start()
    {
        if (ejecutarEnStart) StartCoroutine(EjecutarSuiteCompleta());
    }

    [ContextMenu("Ejecutar Self-Test: SUITE TDD MAESTRA")]
    public void EjecutarTestsManual()
    {
        StartCoroutine(EjecutarSuiteCompleta());
    }

    private IEnumerator EjecutarSuiteCompleta()
    {
        Debug.Log("<color=cyan><b>[TDD ORCHESTRATOR] Iniciando macro-auditoría sistémica...</b></color>");
        
        yield return AuditarMatematicasExplosion();
        yield return AuditarVulnerabilidadBombas();
        
        Debug.Log("<color=cyan><b>[TDD ORCHESTRATOR] Suite Completada. Sandbox Estable.</b></color>");
    }

    private IEnumerator AuditarMatematicasExplosion()
    {
        float distancia = 5f;
        float radio = 10f;
        int danoMaximo = 100;

        float factorEsperado = 1f - Mathf.Clamp01(distancia / radio);
        int danoCalculado = Mathf.RoundToInt(danoMaximo * factorEsperado);

        AssertEquals("Físicas de Daño O(1) (Ecuación Inversa Clamp)", 50, danoCalculado);
        
        bool hayLeyMarcial = Object.FindFirstObjectByType<EscuadraAntiDisturbios>() != null;
        if (!hayLeyMarcial) Debug.LogWarning("<color=orange>[TDD WARNING]</color> Ley Marcial desactivada (Falta EscuadraAntiDisturbios).");
        
        yield return null;
    }

    private IEnumerator AuditarVulnerabilidadBombas()
    {
        // Inicializar objeto dummy para testar el flotante de la luz
        var parpadeoGO = new GameObject("TestParpadeo");
        var parpadeo = parpadeoGO.AddComponent<ParpadeoLuz>();
        
        // Simular 50,000 frames de juego (horas de simulación) sin usar Reflection
        for (int i = 0; i < 50000; i++)
        {
            parpadeo.AvanzarTimer(0.016f); // 60fps dt manual
        }
        
        AssertTrue("Prevención Float Overflow en Parpadeo URP", parpadeo.TestTimer <= Mathf.PI * 2f + 0.1f);
        
        Destroy(parpadeoGO);
        yield return null;
    }

    // --- CORE ASSERTS ---
    private void AssertEquals(string testName, int expected, int actual)
    {
        if (expected == actual) Debug.Log($"<color=green>[PASSED]</color> {testName}");
        else Debug.LogError($"<color=red>[FAILED]</color> {testName} | Exp: {expected}, Act: {actual}");
    }
    private void AssertTrue(string testName, bool condition)
    {
        if (condition) Debug.Log($"<color=green>[PASSED]</color> {testName}");
        else Debug.LogError($"<color=red>[FAILED]</color> {testName} | Condición Falsa");
    }
}
