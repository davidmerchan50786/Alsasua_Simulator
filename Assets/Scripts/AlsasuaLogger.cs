// Assets/Scripts/AlsasuaLogger.cs
// ─────────────────────────────────────────────────────────────────────────────
// Sistema de logging centralizado para Alsasua Simulator.
//
// Sustituye los Debug.Log dispersos con un sistema de niveles configurable:
//   · Verbose — datos de frame (solo en desarrollo con muchas entidades)
//   · Info    — eventos normales: spawn, cambio de estado, recarga
//   · Warning — situaciones anómalas recuperables
//   · Error   — fallos no recuperables
//
// USO DESDE CUALQUIER SCRIPT:
//   AlsasuaLogger.Info("Disparo",   "Recargado: 30/30 Reserva: 90");
//   AlsasuaLogger.Warn("IA",        "Sin waypoints asignados — fallback a Caminando");
//   AlsasuaLogger.Verbose("Trafico","PrepararOrdenCarriles: 3 carriles, 12 vehículos activos");
//
// SILENCIAR EN PRODUCCIÓN:
//   AlsasuaLogger.NivelMinimo = AlsasuaLogger.Level.Warning;
//
// ACTIVAR VERBOSE PARA DEPURAR UN SISTEMA:
//   AlsasuaLogger.NivelMinimo = AlsasuaLogger.Level.Verbose;
// ─────────────────────────────────────────────────────────────────────────────

using UnityEngine;

public static class AlsasuaLogger
{
    // ── Enumeración de niveles ─────────────────────────────────────────────

    /// <summary>Nivel de verbosidad del log. Los mensajes por debajo del mínimo se ignoran.</summary>
    public enum Level : byte
    {
        Verbose = 0,   // datos de frame: solo útil con profiler o depuración intensiva
        Info    = 1,   // eventos de juego normales (spawn, recarga, cambio de estado)
        Warning = 2,   // situaciones anómalas pero recuperables
        Error   = 3,   // fallos no recuperables (shader null, asset perdido)
        None    = 4,   // silencio total (builds de producción)
    }

    // ── Configuración global ───────────────────────────────────────────────

    /// <summary>
    /// Nivel mínimo de log que se envía a la consola de Unity.
    /// Cambiar en runtime para activar/desactivar verbosidad sin recompilar.
    /// Por defecto: Info (no muestra Verbose, no satura la consola).
    /// </summary>
    public static Level NivelMinimo =
#if UNITY_EDITOR || DEVELOPMENT_BUILD
        Level.Info;
#else
        Level.Warning;   // producción: solo errores y advertencias
#endif

    // ── API pública ────────────────────────────────────────────────────────

    /// <summary>Log de depuración intensivo (datos de frame, conteos, etc.).</summary>
    public static void Verbose(string tag, string mensaje) =>
        Registrar(Level.Verbose, tag, mensaje);

    /// <summary>Evento de juego normal (spawn, cambio de estado, acción).</summary>
    public static void Info(string tag, string mensaje) =>
        Registrar(Level.Info, tag, mensaje);

    /// <summary>Situación anómala recuperable (fallback activado, recurso no encontrado).</summary>
    public static void Warn(string tag, string mensaje) =>
        Registrar(Level.Warning, tag, mensaje);

    /// <summary>Fallo no recuperable (shader null, NRE, estado incoherente).</summary>
    public static void Error(string tag, string mensaje) =>
        Registrar(Level.Error, tag, mensaje);

    // ── Implementación ────────────────────────────────────────────────────

    private static void Registrar(Level nivel, string tag, string mensaje)
    {
        if (nivel < NivelMinimo) return;

        string texto = $"[{tag}] {mensaje}";
        switch (nivel)
        {
            case Level.Error:   Debug.LogError(texto);   break;
            case Level.Warning: Debug.LogWarning(texto); break;
            default:            Debug.Log(texto);        break;
        }
    }
}
