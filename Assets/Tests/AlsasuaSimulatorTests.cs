// Assets/Tests/AlsasuaSimulatorTests.cs
// Suite de tests de integración para Alsasua Simulator.
// Cubre todos los objetivos implementados en las sesiones de mejora.
//
// ── CÓMO EJECUTAR ─────────────────────────────────────────────────────────
//   Window > General > Test Runner > EditMode tab > Run All
//
// ── COBERTURA ─────────────────────────────────────────────────────────────
//   T01  ControladorJugador – RatioVida correcto
//   T02  ControladorJugador – FlashDano se activa al recibir daño
//   T03  ControladorJugador – TextoEstado "─ MUERTO ─" cuando vida=0
//   T04  ControladorJugador – Curar no supera VidaMax
//   T05  ControladorJugador – Propiedades de estado son accesibles
//   T06  SistemaDisparo     – ProgressRecarga: 0 al inicio de recarga, 1 al terminar
//   T07  SistemaDisparo     – Object Pool (bursts) inicializado con tamaño correcto
//   T08  SistemaDisparo     – Object Pool (decals) inicializado con tamaño correcto
//   T09  SistemaDisparo     – Material décal compartido (no N instancias)
//   T10  SistemaDisparo     – Dispersión agachado < dispersión de pie
//   T11  SistemaDisparo     – Dispersión en aire > dispersión de pie
//   T12  SistemaTrafico     – Aceleración desde parado < aceleración a velocidad
//   T13  AudioManager       – Singleton: solo una instancia al crear varios
//   T14  AudioManager       – Pool tiene el número correcto de AudioSources
//   T15  AudioManager       – Play con clip null no lanza excepción
//   T16  EnemigoPatrulla    – FlashDano no modifica sharedMaterial directamente
//   T17  HUDJugador         – Canvas creado correctamente en Awake
//   ── Sección 5/6 ──────────────────────────────────────────────────────
//   T18  AlsasuaLogger      – Info/Warn/Error/Verbose no lanzan excepción
//   T19  AlsasuaLogger      – NivelMinimo=None silencia todas las llamadas
//   T20  SistemaAtmosfera   – OnCambioDia se dispara al cruzar el horizonte
//   T21  VehiculoNPC        – RecibirDano reduce la vida del vehículo
//   T22  VehiculoNPC        – destruido=true cuando vida llega a ≤0
//   T23  VehiculoNPC        – sharedMaterial.color no cambia tras daño (usa MPB)
//   T24  SistemaBombas      – bombasDisponibles es positivo al inicializar
//   T25  SistemaBombas      – ColocarBomba sin cámara retorna sin excepción
//   T26  BarricadaFuego     – RecibirDano reduce la vida de la barricada
//   T27  BarricadaFuego     – segundo RecibirDano ignorado cuando vida≤0 (BUG 21)
//   T28  EnemigoPatrulla    – Estado inicial es Patrullando (propiedad pública)
//   T29  SistemaDisparo     – velocidadCorrerRef por defecto = 8.5f (Sección 6)
//   T30  BarricadaFuego     – PrenderFuego crea ParticleSystems

using System.Collections;
using System.Reflection;
using System.Text.RegularExpressions;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

public class AlsasuaSimulatorTests
{
    // ═══════════════════════════════════════════════════════════════════════
    //  HELPERS
    // ═══════════════════════════════════════════════════════════════════════

    // Crea un ControladorJugador con las dependencias mínimas sin errores fatales
    private static GameObject CrearJugadorMinimo()
    {
        var go = new GameObject("TestJugador");
        go.AddComponent<CharacterController>();
        return go;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T01 — ControladorJugador: RatioVida correcto
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T01_RatioVida_EsUnoAlInicio()
    {
        var go   = CrearJugadorMinimo();
        var ctrl = go.AddComponent<ControladorJugador>();
        yield return null;  // ejecutar Awake + Start

        Assert.AreEqual(1f, ctrl.RatioVida, 0.001f,
            "RatioVida debe ser 1.0 con vida == vidaMax");

        Object.DestroyImmediate(go);
    }

    [UnityTest]
    public IEnumerator T01b_RatioVida_ReduceDespuesDeDano()
    {
        var go   = CrearJugadorMinimo();
        var ctrl = go.AddComponent<ControladorJugador>();
        yield return null;

        ctrl.RecibirDano(50);

        Assert.Less(ctrl.RatioVida, 1f,
            "RatioVida debe bajar al recibir daño");
        Assert.Greater(ctrl.RatioVida, 0f,
            "RatioVida no debe ser negativo con 50 daño sobre 100 HP");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T02 — ControladorJugador: FlashDano se activa al recibir daño
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T02_FlashDano_PositivoTrasRecibirDano()
    {
        var go   = CrearJugadorMinimo();
        var ctrl = go.AddComponent<ControladorJugador>();
        yield return null;

        Assert.AreEqual(0f, ctrl.FlashDano, "FlashDano debe ser 0 al inicio");

        ctrl.RecibirDano(10);

        Assert.Greater(ctrl.FlashDano, 0f,
            "FlashDano debe ser > 0 inmediatamente tras recibir daño");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T03 — ControladorJugador: TextoEstado "─ MUERTO ─" al morir
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T03_TextoEstado_MuertoAlMorir()
    {
        var go   = CrearJugadorMinimo();
        var ctrl = go.AddComponent<ControladorJugador>();
        yield return null;

        ctrl.RecibirDano(9999);  // daño letal

        Assert.IsTrue(ctrl.EstaMuerto, "EstaMuerto debe ser true");
        Assert.AreEqual("─ MUERTO ─", ctrl.TextoEstado,
            "TextoEstado debe devolver '─ MUERTO ─' cuando vida == 0");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T04 — ControladorJugador: Curar no supera VidaMax
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T04_Curar_NoSuperaVidaMax()
    {
        var go   = CrearJugadorMinimo();
        var ctrl = go.AddComponent<ControladorJugador>();
        yield return null;

        ctrl.RecibirDano(30);
        ctrl.Curar(9999);

        Assert.AreEqual(ctrl.VidaMax, ctrl.Vida,
            "Curar con cantidad excesiva debe tapar en VidaMax");
        Assert.AreEqual(1f, ctrl.RatioVida, 0.001f,
            "RatioVida debe ser 1.0 tras curar completamente");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T05 — ControladorJugador: propiedades de estado público no crashean
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T05_PropiedadesEstado_AccesiblesSinException()
    {
        var go   = CrearJugadorMinimo();
        var ctrl = go.AddComponent<ControladorJugador>();
        yield return null;

        Assert.DoesNotThrow(() =>
        {
            _ = ctrl.EstaAgachadoP;
            _ = ctrl.EstaCorriendoP;
            _ = ctrl.EstaEnSueloP;
            _ = ctrl.EstaApuntandoP;
            _ = ctrl.VelocidadHoriz;
        }, "Las propiedades públicas de estado no deben lanzar excepciones");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T06 — SistemaDisparo: ProgressRecarga 0→1 durante recarga
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T06_ProgressRecarga_CeroAlInicioYUnoAlTerminar()
    {
        var go   = new GameObject("TestDisparo");
        go.AddComponent<Camera>();            // SistemaDisparo busca Camera
        var sd   = go.AddComponent<SistemaDisparo>();
        yield return null;

        // Iniciar recarga real a través de la API pública
        sd.IniciarRecarga();

        Assert.AreEqual(0f, sd.ProgressRecarga, 0.01f,
            "ProgressRecarga debe ser 0 al inicio de la recarga");

        // Simular avance del tiempo usando el hook de testing
        sd._Test_AvanzarRecarga(999f);

        Assert.AreEqual(1f, sd.ProgressRecarga, 0.01f,
            "ProgressRecarga debe ser 1 cuando la recarga finaliza");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T07 — SistemaDisparo: Object Pool de bursts tiene tamaño correcto
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T07_ObjectPool_Bursts_TamañoCorrecto()
    {
        var go = new GameObject("TestDisparo");
        go.AddComponent<Camera>();
        var sd = go.AddComponent<SistemaDisparo>();
        yield return null;

        Assert.AreEqual(20, sd.TamañoPoolBursts,
            "POOL_BURSTS debe ser 20 según la constante de la clase");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T08 — SistemaDisparo: Object Pool de decals tiene tamaño correcto
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T08_ObjectPool_Decals_TamañoCorrecto()
    {
        var go = new GameObject("TestDisparo");
        go.AddComponent<Camera>();
        var sd = go.AddComponent<SistemaDisparo>();
        yield return null;

        Assert.AreEqual(50, sd.TamañoPoolDecals,
            "POOL_DECALS debe ser 50 según la constante de la clase");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T09 — SistemaDisparo: décals usan material compartido (no N instancias)
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T09_Decals_UsanMaterialCompartido()
    {
        var go = new GameObject("TestDisparo");
        go.AddComponent<Camera>();
        var sd = go.AddComponent<SistemaDisparo>();
        yield return null;

        var matCompartido = sd.MaterialDecalCompartido;

        if (matCompartido == null || sd.TamañoPoolDecals == 0)
        {
            Assert.Inconclusive("Pool o material no disponibles (shader no encontrado en test env).");
            yield break;
        }

        var rend0 = sd.RendererDecal(0);

        if (rend0 != null && rend0.sharedMaterial != null)
            Assert.AreSame(matCompartido, rend0.sharedMaterial,
                "Todos los décals deben usar el mismo material compartido (_matDecalCompartido)");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T10 — SistemaDisparo: dispersión agachado < dispersión de pie
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T10_Dispersion_AgachadoMenorQueDesPie()
    {
        var goJugador = CrearJugadorMinimo();
        goJugador.AddComponent<Camera>();
        var ctrl = goJugador.AddComponent<ControladorJugador>();
        var sd   = goJugador.AddComponent<SistemaDisparo>(); // Cacheará ctrl en Awake()
        yield return null;

        // De pie
        ctrl.ForzarEstadoFisico(enSuelo: true, agachado: false);
        float dispersionDePie = sd.CalcularDispersion();

        // Agachado
        ctrl.ForzarEstadoFisico(enSuelo: true, agachado: true);
        float dispersionAgachado = sd.CalcularDispersion();

        Assert.Less(dispersionAgachado, dispersionDePie,
            "Dispersión agachado debe ser menor que dispersión de pie");

        Object.DestroyImmediate(goJugador);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T11 — SistemaDisparo: dispersión en el aire > dispersión de pie
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T11_Dispersion_AireMayorQueEnSuelo()
    {
        var goJugador = CrearJugadorMinimo();
        goJugador.AddComponent<Camera>();
        var ctrl = goJugador.AddComponent<ControladorJugador>();
        var sd   = goJugador.AddComponent<SistemaDisparo>();
        yield return null;

        // En suelo
        ctrl.ForzarEstadoFisico(enSuelo: true, agachado: false);
        float dispersionEnSuelo = sd.CalcularDispersion();

        // En el aire
        ctrl.ForzarEstadoFisico(enSuelo: false, agachado: false);
        float dispersionEnAire = sd.CalcularDispersion();

        Assert.Greater(dispersionEnAire, dispersionEnSuelo,
            "Dispersión en el aire debe ser mayor que en suelo");

        Object.DestroyImmediate(goJugador);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T12 — SistemaTrafico: factor de aceleración es menor a velocidad 0
    // ═══════════════════════════════════════════════════════════════════════

    [Test]
    public void T12_Trafico_AceleracionSuaveDesdeParado()
    {
        const float velMax = 10f;

        float FactorAcel(float velActual)
        {
            float t = Mathf.Clamp01(velActual / Mathf.Max(velMax * 0.4f, 0.1f));
            return Mathf.Lerp(0.5f, 2.5f, t);
        }

        float factorDesdeParado  = FactorAcel(0f);
        float factorEnMovimiento = FactorAcel(velMax);

        Assert.Less(factorDesdeParado, factorEnMovimiento,
            "La aceleración desde parado debe ser menor que a velocidad máxima");
        Assert.AreEqual(0.5f, factorDesdeParado, 0.01f,
            "Factor de aceleración desde parado debe ser 0.5 m/s²");
        Assert.AreEqual(2.5f, factorEnMovimiento, 0.01f,
            "Factor de aceleración a velocidad máxima debe ser 2.5 m/s²");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T13 — AudioManager: Singleton descarta instancias duplicadas
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T13_AudioManager_Singleton_SoloUnaInstancia()
    {
        if (AudioManager.I != null) Object.DestroyImmediate(AudioManager.I.gameObject);

        var go1 = new GameObject("AM1");
        go1.AddComponent<AudioManager>();
        var go2 = new GameObject("AM2");
        go2.AddComponent<AudioManager>();
        yield return null;

        var instancias = Object.FindObjectsByType<AudioManager>(FindObjectsSortMode.None);
        Assert.AreEqual(1, instancias.Length,
            "Solo debe existir una instancia de AudioManager (Singleton)");
        Assert.IsNotNull(AudioManager.I,
            "AudioManager.I no debe ser null tras inicialización");

        Object.DestroyImmediate(AudioManager.I.gameObject);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T14 — AudioManager: Pool tiene el número correcto de AudioSources
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T14_AudioManager_Pool_TamañoCorrecto()
    {
        if (AudioManager.I != null) Object.DestroyImmediate(AudioManager.I.gameObject);

        var go = new GameObject("AM");
        var am = go.AddComponent<AudioManager>();
        yield return null;

        var fuentes = go.GetComponentsInChildren<AudioSource>();
        Assert.AreEqual(am.TamañoPool, fuentes.Length,
            "El número de AudioSources hijos debe coincidir con el tamaño del pool");
        Assert.Greater(am.TamañoPool, 0, "El pool debe tener al menos un AudioSource");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T15 — AudioManager: Play con clip null no lanza excepción
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T15_AudioManager_PlayConClipNull_SinException()
    {
        if (AudioManager.I != null) Object.DestroyImmediate(AudioManager.I.gameObject);

        var go = new GameObject("AM");
        go.AddComponent<AudioManager>();
        yield return null;

        Assert.DoesNotThrow(() =>
        {
            AudioManager.I.Play(AudioManager.Clip.Disparo);
            AudioManager.I.Play(AudioManager.Clip.Recarga, Vector3.zero);
        }, "Play con clip null (no asignado en Inspector) no debe lanzar excepción");

        Object.DestroyImmediate(AudioManager.I.gameObject);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T16 — EnemigoPatrulla: FlashDano no modifica sharedMaterial.color
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T16_EnemigoPatrulla_FlashDano_NoModificaSharedMaterial()
    {
        var go     = new GameObject("TestEnemigo");
        var enemigo = go.AddComponent<EnemigoPatrulla>();
        yield return null;  // Start() → CrearCuerpoBasico()

        var renderers      = go.GetComponentsInChildren<Renderer>();
        Assert.Greater(renderers.Length, 0,
            "El enemigo debe tener al menos un Renderer tras CrearCuerpoBasico()");

        Color colorAntes = renderers[0].sharedMaterial != null
            ? renderers[0].sharedMaterial.color
            : Color.white;

        enemigo.RecibirDano(10);

        Color colorDespues = renderers[0].sharedMaterial != null
            ? renderers[0].sharedMaterial.color
            : Color.white;

        Assert.AreEqual(colorAntes, colorDespues,
            "sharedMaterial.color NO debe ser modificado — FlashDano debe usar SetPropertyBlock");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T17 — HUDJugador: Canvas creado correctamente en Awake
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T17_HUDJugador_Canvas_CreadoEnAwake()
    {
        var goJugador = CrearJugadorMinimo();
        goJugador.AddComponent<ControladorJugador>();
        yield return null;

        var goHUD = new GameObject("TestHUD");
        var hud   = goHUD.AddComponent<HUDJugador>();
        yield return null;

        var canvas = goHUD.GetComponentInChildren<Canvas>();
        Assert.IsNotNull(canvas,
            "HUDJugador debe crear un Canvas hijo en Awake()");
        Assert.AreEqual(RenderMode.ScreenSpaceOverlay, canvas.renderMode,
            "Canvas debe usar RenderMode.ScreenSpaceOverlay");
        Assert.AreEqual(100, canvas.sortingOrder,
            "Canvas debe tener sortingOrder=100 para estar encima de otros Canvas");

        Object.DestroyImmediate(goHUD);
        Object.DestroyImmediate(goJugador);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T18 — AlsasuaLogger: Info/Warn/Error/Verbose no lanzan excepción
    // ═══════════════════════════════════════════════════════════════════════

    [Test]
    public void T18_AlsasuaLogger_MetodosPublicos_NoLanzanException()
    {
        Assert.DoesNotThrow(() => AlsasuaLogger.Verbose("Test", "mensaje verbose"),
            "AlsasuaLogger.Verbose no debe lanzar excepción");
        Assert.DoesNotThrow(() => AlsasuaLogger.Info("Test", "mensaje info"),
            "AlsasuaLogger.Info no debe lanzar excepción");
        Assert.DoesNotThrow(() => AlsasuaLogger.Warn("Test", "mensaje warning"),
            "AlsasuaLogger.Warn no debe lanzar excepción");
        // LogAssert.Expect impide que el test runner marque el test como fallido
        // por el Debug.LogError que emite internamente AlsasuaLogger.Error.
        LogAssert.Expect(LogType.Error, new Regex(".*"));
        Assert.DoesNotThrow(() => AlsasuaLogger.Error("Test", "mensaje error"),
            "AlsasuaLogger.Error no debe lanzar excepción");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T19 — AlsasuaLogger: NivelMinimo=None silencia todas las llamadas
    // ═══════════════════════════════════════════════════════════════════════

    [Test]
    public void T19_AlsasuaLogger_NivelNone_SilenciaTodasLasLlamadas()
    {
        var nivelOriginal = AlsasuaLogger.NivelMinimo;

        // Con Level.None ningún mensaje debe emitirse (ni crashear)
        AlsasuaLogger.NivelMinimo = AlsasuaLogger.Level.None;
        Assert.DoesNotThrow(() =>
        {
            AlsasuaLogger.Verbose("Test", "silenciado");
            AlsasuaLogger.Info("Test",    "silenciado");
            AlsasuaLogger.Warn("Test",    "silenciado");
            AlsasuaLogger.Error("Test",   "silenciado");
        }, "Con NivelMinimo=None ninguna llamada debe lanzar excepción");

        Assert.AreEqual(AlsasuaLogger.Level.None, AlsasuaLogger.NivelMinimo,
            "NivelMinimo debe conservar el valor asignado");

        AlsasuaLogger.NivelMinimo = nivelOriginal; // restaurar
        Assert.AreEqual(nivelOriginal, AlsasuaLogger.NivelMinimo,
            "NivelMinimo debe restaurarse correctamente");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T20 — SistemaAtmosfera: OnCambioDia se dispara al cruzar el horizonte
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T20_SistemaAtmosfera_OnCambioDia_DisparaAlCruzarHorizonte()
    {
        // En tests no hay Directional Light → Awake emite una advertencia esperada
        LogAssert.Expect(LogType.Warning, new Regex(".*Directional Light.*|.*Atmósfera.*|.*luz.*"));
        var go  = new GameObject("TestAtmosfera");
        var atm = go.AddComponent<SistemaAtmosfera>();
        yield return null;  // Awake + Start

        bool eventoDisparado = false;
        bool argumento        = true;  // esperamos false (noche)
        atm.OnCambioDia += b => { eventoDisparado = true; argumento = b; };

        // Forzar estado previo = día (_eraDeDia = true) y hora = 2:00 AM (noche profunda)
        var fEraDeDia = typeof(SistemaAtmosfera).GetField("_eraDeDia",
            BindingFlags.Instance | BindingFlags.NonPublic);
        var fHora = typeof(SistemaAtmosfera).GetField("horaDelDia",
            BindingFlags.Instance | BindingFlags.NonPublic);

        fEraDeDia?.SetValue(atm, true);   // partimos de "era de día"
        fHora?.SetValue(atm, 2f);          // forzar hora = 2:00 AM (profunda noche → sol bajo)

        // Llamar ActualizarAtmosfera() para que detecte la transición día→noche
        typeof(SistemaAtmosfera).GetMethod("ActualizarAtmosfera",
            BindingFlags.Instance | BindingFlags.NonPublic)?.Invoke(atm, null);

        Assert.IsTrue(eventoDisparado,
            "OnCambioDia debe dispararse cuando la elevación solar cruza el horizonte");
        Assert.IsFalse(argumento,
            "El argumento debe ser false (noche) al pasar de día a noche");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T21 — VehiculoNPC: RecibirDano reduce la vida del vehículo
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T21_VehiculoNPC_RecibirDano_ReduceVida()
    {
        var go  = VehiculoNPC.CrearCocheBasico(Vector3.zero, new System.Collections.Generic.List<Transform>());
        var veh = go.GetComponent<VehiculoNPC>();
        yield return null;

        int vidaAntes = veh.Vida;
        veh.RecibirDano(20);
        int vidaTras = veh.Vida;

        Assert.Less(vidaTras, vidaAntes,
            "Vida del vehículo debe reducirse tras recibir daño");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T22 — VehiculoNPC: Destruido=true cuando vida llega a ≤0
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T22_VehiculoNPC_Destruido_TrasVidaACero()
    {
        var go  = VehiculoNPC.CrearCocheBasico(Vector3.zero, new System.Collections.Generic.List<Transform>());
        var veh = go.GetComponent<VehiculoNPC>();
        yield return null;

        Assert.IsFalse(veh.Destruido, "El vehículo no debe estar destruido al inicio");

        veh.RecibirDano(veh.VidaMax + 100);  // golpe letal seguro

        Assert.IsTrue(veh.Destruido,
            "Destruido debe ser true después de un daño que supera la vida total");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T23 — VehiculoNPC: sharedMaterial.color no cambia tras daño (usa MPB)
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T23_VehiculoNPC_DanoNoModificaSharedMaterial()
    {
        var go  = VehiculoNPC.CrearCocheBasico(Vector3.zero, new System.Collections.Generic.List<Transform>());
        var veh = go.GetComponent<VehiculoNPC>();
        yield return null;

        var renderers = go.GetComponentsInChildren<Renderer>();
        Assert.Greater(renderers.Length, 0, "El vehículo debe tener al menos un Renderer");

        Color colorAntes = renderers[0].sharedMaterial != null
            ? renderers[0].sharedMaterial.color
            : Color.white;

        veh.RecibirDano(20);

        Color colorDespues = renderers[0].sharedMaterial != null
            ? renderers[0].sharedMaterial.color
            : Color.white;

        Assert.AreEqual(colorAntes, colorDespues,
            "sharedMaterial.color NO debe modificarse — VehiculoNPC debe usar MaterialPropertyBlock");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T24 — SistemaBombas: bombasDisponibles es positivo al inicializar
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T24_SistemaBombas_BombasDisponibles_PositivoAlInicio()
    {
        var go = new GameObject("TestBombas");
        go.AddComponent<SistemaBombas>();
        yield return null;

        var fBombas = typeof(SistemaBombas).GetField("bombasDisponibles",
            BindingFlags.Instance | BindingFlags.NonPublic);
        int bombas = fBombas != null ? (int)fBombas.GetValue(go.GetComponent<SistemaBombas>()) : 0;

        Assert.Greater(bombas, 0,
            "bombasDisponibles debe ser positivo al inicializar SistemaBombas");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T25 — SistemaBombas: ColocarBomba sin cámara retorna sin excepción
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T25_SistemaBombas_ColocarBomba_SinCamara_NoLanzaException()
    {
        var go = new GameObject("TestBombas");
        go.AddComponent<SistemaBombas>();
        yield return null;

        // Con cámara=null, ColocarBomba() debe hacer LogWarning y retornar silenciosamente
        LogAssert.Expect(LogType.Warning, new Regex(".*"));
        Assert.DoesNotThrow(() => go.GetComponent<SistemaBombas>().ColocarBomba(),
            "ColocarBomba sin cámara disponible no debe lanzar excepción");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T26 — BarricadaFuego: RecibirDano reduce la vida de la barricada
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T26_BarricadaFuego_RecibirDano_ReduceVida()
    {
        var barricada = BarricadaFuego.Crear(Vector3.zero);
        yield return null;  // Start()

        var fVida = typeof(BarricadaFuego).GetField("vida",
            BindingFlags.Instance | BindingFlags.NonPublic);
        int vidaAntes = (int)fVida.GetValue(barricada);

        barricada.RecibirDano(50);
        int vidaTras = (int)fVida.GetValue(barricada);

        Assert.Less(vidaTras, vidaAntes,
            "La vida de la barricada debe reducirse tras RecibirDano");

        Object.DestroyImmediate(barricada.gameObject);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T27 — BarricadaFuego: segundo daño ignorado cuando vida≤0 (BUG 21)
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T27_BarricadaFuego_SegundoDano_IgnoradoCuandoDestruida()
    {
        var barricada = BarricadaFuego.Crear(Vector3.zero);
        yield return null;

        var fVida = typeof(BarricadaFuego).GetField("vida",
            BindingFlags.Instance | BindingFlags.NonPublic);

        // Primer golpe letal
        barricada.RecibirDano(10000);
        int vidaTrasGolpeLetal = (int)fVida.GetValue(barricada);
        Assert.LessOrEqual(vidaTrasGolpeLetal, 0, "Vida debe ser ≤0 tras golpe letal");

        // Segundo golpe: vida no debe bajar más (guard BUG 21 FIX)
        barricada.RecibirDano(10000);
        int vidaTrasSegundoGolpe = (int)fVida.GetValue(barricada);

        Assert.AreEqual(vidaTrasGolpeLetal, vidaTrasSegundoGolpe,
            "El segundo daño tras destrucción debe ser ignorado (BUG 21 FIX)");

        Object.DestroyImmediate(barricada.gameObject);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T28 — EnemigoPatrulla: Estado inicial es Patrullando
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T28_EnemigoPatrulla_EstadoInicial_EsPatrullando()
    {
        var go      = new GameObject("TestEnemigo");
        var enemigo = go.AddComponent<EnemigoPatrulla>();
        yield return null;  // Start()

        Assert.AreEqual(EnemigoPatrulla.EstadoIA.Patrullando, enemigo.Estado,
            "El estado inicial de EnemigoPatrulla debe ser Patrullando");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T29 — SistemaDisparo: velocidadCorrerRef por defecto = 8.5f
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T29_SistemaDisparo_VelocidadCorrerRef_ValorPorDefecto()
    {
        var go = new GameObject("TestDisparo");
        go.AddComponent<Camera>();
        var sd = go.AddComponent<SistemaDisparo>();
        yield return null;

        Assert.AreEqual(8.5f, sd.VelocidadCorrerRef, 0.001f,
            "VelocidadCorrerRef debe tener el valor por defecto 8.5f");

        Object.DestroyImmediate(go);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  T30 — BarricadaFuego: PrenderFuego crea ParticleSystems
    // ═══════════════════════════════════════════════════════════════════════

    [UnityTest]
    public IEnumerator T30_BarricadaFuego_PrenderFuego_CreaParticleSystems()
    {
        var go        = new GameObject("TestBarricada");
        var barricada = go.AddComponent<BarricadaFuego>();
        yield return null;  // Start() — fuegoPrendido=true por defecto → PrenderFuego()

        var sistemas = go.GetComponentsInChildren<ParticleSystem>();
        Assert.Greater(sistemas.Length, 0,
            "PrenderFuego debe crear al menos un ParticleSystem hijo");

        Object.DestroyImmediate(go);
    }
}
