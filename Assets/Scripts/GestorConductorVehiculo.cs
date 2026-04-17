// Assets/Scripts/GestorConductorVehiculo.cs
// Sistema de entrada/salida de vehículos para el jugador.
//
// ── DESCRIPCIÓN ───────────────────────────────────────────────────────────────
//  Permite al jugador entrar y salir de coches usando la tecla E (InputSystem).
//  Integra el CarController y CarUserControl de los Standard Assets del paquete
//  importado (#Xtra/Standard Assets/Vehicles/Car) con el ControladorJugador del
//  proyecto, adaptando el input al nuevo UnityEngine.InputSystem.
//
// ── SETUP (por coche) ─────────────────────────────────────────────────────────
//  1. El prefab del coche debe tener:
//       · Rigidbody
//       · CarController   (de UnityStandardAssets.Vehicles.Car)
//       · WheelColliders configurados en el CarController
//  2. Añadir este componente GestorConductorVehiculo al mismo GO que el CarController.
//  3. Crear un GO hijo vacío "SalidaJugador" y arrastrarlo al campo puntoSalida.
//  4. Crear un GO hijo vacío "AsientoConductor" y arrastrarlo al campo asientoConductor.
//  5. Arrastra la cámara del coche (con CarCam o similar) al campo camaraCoche.
//  6. El radio de detección determina hasta qué distancia el jugador puede entrar.
//
// ── CONTROLES EN COCHE ────────────────────────────────────────────────────────
//  W/S   → aceleración / freno-reversa
//  A/D   → giro
//  Space → freno de mano
//  E     → salir del coche
//
// ── NOTAS ─────────────────────────────────────────────────────────────────────
//  · Cuando el jugador entra, el ControladorJugador se desactiva y el personaje
//    se oculta (SetActive false). Al salir se restaura junto al punto de salida.
//  · El AudioManager recibe el sonido del motor si se asignó clipMotorCoche.
//  · Compatible con Cesium: el coche debe tener CesiumGlobeAnchor si se usa
//    georreferenciación real.

using UnityEngine;
using UnityEngine.InputSystem;
using UnityStandardAssets.Vehicles.Car;

[RequireComponent(typeof(CarController))]
[AddComponentMenu("Alsasua/Gestor Conductor Vehículo")]
public class GestorConductorVehiculo : MonoBehaviour
{
    // ═══════════════════════════════════════════════════════════════════════
    //  INSPECTOR
    // ═══════════════════════════════════════════════════════════════════════

    [Header("═══ PUNTOS DE REFERENCIA ═══")]
    [Tooltip("Transform donde aparece el jugador al salir del coche.")]
    [SerializeField] private Transform puntoSalida;

    [Tooltip("Transform del asiento del conductor (posición donde 'se sienta' el jugador).")]
    [SerializeField] private Transform asientoConductor;

    [Header("═══ CÁMARA ═══")]
    [Tooltip("GameObject de la cámara del coche. Se activa al entrar y desactiva al salir.")]
    [SerializeField] private GameObject camaraCoche;

    [Header("═══ DETECCIÓN ═══")]
    [Tooltip("Radio en metros en el que el jugador puede pulsar E para entrar al coche.")]
    [SerializeField] private float radioEntrada = 3.5f;

    [Header("═══ AUDIO ═══")]
    [Tooltip("AudioSource del motor (loop). Se controla a través de AudioManager si es null.")]
    [SerializeField] private AudioSource audioMotor;

    // ═══════════════════════════════════════════════════════════════════════
    //  ESTADO INTERNO
    // ═══════════════════════════════════════════════════════════════════════

    private CarController  _carController;
    private Rigidbody      _rb;
    private ControladorJugador _jugador;
    private GameObject     _goJugador;

    private bool _jugadorDentro = false;

    // Input acumulado
    private float _h;        // horizontal  (A/D)
    private float _v;        // vertical    (W/S)
    private float _handbrake;// freno de mano (Space)

    // ═══════════════════════════════════════════════════════════════════════
    //  UNITY LIFECYCLE
    // ═══════════════════════════════════════════════════════════════════════

    private void Awake()
    {
        _carController = GetComponent<CarController>();
        _rb            = GetComponent<Rigidbody>();

        // Desactivar controles del coche hasta que el jugador entre
        _carController.enabled = false;
    }

    private void Start()
    {
        // Buscar jugador en la escena (disponible solo en Play Mode)
        _jugador = Object.FindFirstObjectByType<ControladorJugador>();
        if (_jugador != null)
            _goJugador = _jugador.gameObject;

        if (camaraCoche != null)
            camaraCoche.SetActive(false);

        if (audioMotor != null)
        {
            audioMotor.loop       = true;
            audioMotor.playOnAwake = false;
            audioMotor.Stop();
        }
    }

    private void Update()
    {
        if (_jugadorDentro)
        {
            LeerInputCoche();
            MostrarPrompSalir();

            // Salir con E
            if (Keyboard.current != null && Keyboard.current.eKey.wasPressedThisFrame)
                SalirDelCoche();
        }
        else
        {
            MostrarPromptEntrar();

            if (PuedeEntrar() && Keyboard.current != null && Keyboard.current.eKey.wasPressedThisFrame)
                EntrarAlCoche();
        }
    }

    private void FixedUpdate()
    {
        if (!_jugadorDentro) return;
        _carController.Move(_h, _v, _v, _handbrake);
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  LÓGICA PRINCIPAL
    // ═══════════════════════════════════════════════════════════════════════

    private bool PuedeEntrar()
    {
        if (_jugador == null) return false;
        float dist = Vector3.Distance(transform.position, _jugador.transform.position);
        return dist <= radioEntrada;
    }

    private void EntrarAlCoche()
    {
        _jugadorDentro = true;

        // Ocultar jugador y desactivar su controlador
        if (_goJugador != null)
            _goJugador.SetActive(false);

        // Activar cámara del coche
        if (camaraCoche != null)
            camaraCoche.SetActive(true);

        // Activar físicas del coche
        _carController.enabled = true;
        if (_rb != null) _rb.isKinematic = false;

        // Audio motor
        if (audioMotor != null)
            audioMotor.Play();
        else
            AudioManager.I?.PlayLoop(AudioManager.Clip.MotorCoche,
                GetComponent<AudioSource>());

        AlsasuaLogger.Info("GestorConductor", $"Jugador ha entrado en {name}");
    }

    private void SalirDelCoche()
    {
        _jugadorDentro = false;

        // Detener el coche suavemente
        _h = 0f; _v = 0f; _handbrake = 1f;
        _carController.Move(0f, 0f, 0f, 1f);
        _carController.enabled = false;

        // Teleportar jugador al punto de salida
        if (_goJugador != null && puntoSalida != null)
        {
            _goJugador.transform.position = puntoSalida.position;
            _goJugador.transform.rotation = puntoSalida.rotation;
            _goJugador.SetActive(true);
        }
        else if (_goJugador != null)
        {
            // Fallback: salir junto al coche
            _goJugador.transform.position = transform.position + transform.right * 2f;
            _goJugador.SetActive(true);
        }

        // Desactivar cámara del coche
        if (camaraCoche != null)
            camaraCoche.SetActive(false);

        // Parar audio motor
        if (audioMotor != null)
            audioMotor.Stop();

        AlsasuaLogger.Info("GestorConductor", $"Jugador ha salido de {name}");
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  INPUT (New InputSystem)
    // ═══════════════════════════════════════════════════════════════════════

    private void LeerInputCoche()
    {
        if (Keyboard.current == null) return;

        _h = 0f;
        if (Keyboard.current.aKey.isPressed) _h = -1f;
        if (Keyboard.current.dKey.isPressed) _h =  1f;

        _v = 0f;
        if (Keyboard.current.wKey.isPressed) _v =  1f;
        if (Keyboard.current.sKey.isPressed) _v = -1f;

        _handbrake = Keyboard.current.spaceKey.isPressed ? 1f : 0f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  HUD PROMPTS (OnGUI)
    // ═══════════════════════════════════════════════════════════════════════

    private void MostrarPromptEntrar()
    {
        if (!PuedeEntrar()) return;
        // Delegamos el dibujado al OnGUI para no depender del Canvas del HUD
        _mostrarPromptEntrar = true;
    }

    private void MostrarPrompSalir()
    {
        _mostrarPromptSalir = true;
    }

    private bool _mostrarPromptEntrar;
    private bool _mostrarPromptSalir;

    private void OnGUI()
    {
        if (_mostrarPromptEntrar)
        {
            GUI.Label(new Rect(Screen.width / 2f - 100f, Screen.height - 80f, 200f, 30f),
                      "[E] Entrar al coche");
            _mostrarPromptEntrar = false;
        }

        if (_mostrarPromptSalir)
        {
            GUI.Label(new Rect(Screen.width / 2f - 100f, Screen.height - 80f, 200f, 30f),
                      "[E] Salir del coche");
            _mostrarPromptSalir = false;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  GIZMOS (editor)
    // ═══════════════════════════════════════════════════════════════════════

#if UNITY_EDITOR
    private void OnDrawGizmosSelected()
    {
        Gizmos.color = new Color(0.2f, 0.9f, 0.2f, 0.35f);
        Gizmos.DrawWireSphere(transform.position, radioEntrada);

        if (puntoSalida != null)
        {
            Gizmos.color = Color.cyan;
            Gizmos.DrawSphere(puntoSalida.position, 0.3f);
            Gizmos.DrawLine(transform.position, puntoSalida.position);
        }
    }
#endif
}
