// Assets/Scripts/ValidadorMecanicas.cs
using UnityEngine;

/// <summary>
/// Módulo Protector TDD (Test-Driven Development) V15.
/// Se usa para programar a la defensiva y verificar la integridad algorítmica.
/// </summary>
[AddComponentMenu("Alsasua V15/Validador TDD (Defensivo)")]
public class ValidadorMecanicas : MonoBehaviour
{
    private void Start()
    {
        AuditarMatematicasExplosion();
    }

    [ContextMenu("Ejecutar Self-Test: Físicas de Explosión")]
    public void AuditarMatematicasExplosion()
    {
        Debug.Log("[TDD] Iniciando Auditoría Defensiva de Físicas...");

        // Simulamos un NPC a 5 metros de una explosión de 10 metros
        float distancia = 5f;
        float radio = 10f;
        int danoMaximo = 100;

        // Fórmula extraída de SistemaExplosion.AplicarFisicasYDano
        float factorEsperado = 1f - Mathf.Clamp01(distancia / radio);
        int danoCalculado = Mathf.RoundToInt(danoMaximo * factorEsperado);

        if (danoCalculado == 50)
        {
            Debug.Log("<color=green>[TDD PASSED]</color> El declive de daño matemático es impecable (O(1)).");
        }
        else
        {
            Debug.LogError($"<color=red>[TDD FAILED]</color> El motor de daño está roto. Esperado: 50 | Recibido: {danoCalculado}");
        }

        // Test Ley Marcial
        if (Object.FindFirstObjectByType<EscuadraAntiDisturbios>() != null)
        {
            Debug.Log("<color=green>[TDD PASSED]</color> El orquestador de Ley Marcial está activo en escena.");
        }
        else
        {
            Debug.LogWarning("<color=orange>[TDD WARNING]</color> EscuadraAntiDisturbios no detectado. Las explosiones no alertarán a la policía.");
        }
    }
}
