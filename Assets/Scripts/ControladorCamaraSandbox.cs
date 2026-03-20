// Assets/Scripts/ControladorCamaraSandbox.cs
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.Rendering.Universal;

[RequireComponent(typeof(Camera))]
public class ControladorCamaraSandbox : MonoBehaviour
{
    public enum ModoVista { Libre, Cenital, TerceraPersona }
    public enum ModoVision { Normal, Nocturna, Termica }

    [Header("Estado")]
    public ModoVista modoActual = ModoVista.Libre;
    public ModoVision visionActual = ModoVision.Normal;

    [Header("Configuración Vuelo")]
    public float velocidadBase = 35f;
    public float velocidadRapida = 120f;
    public float sensibilidadRaton = 3f;

    // Control FPS
    private float pitch = 0f;
    private float yaw = 0f;
    private Camera cam;

    // Volumen de Post-Procesado autogenerado para Milsim
    private Volume volume;
    private VolumeProfile profile;
    private ColorAdjustments colorAdj;
    private FilmGrain grain;
    private Bloom bloom;
    private Vignette vignette;
    private LensDistortion lensDistortion;

    void Start()
    {
        cam = GetComponent<Camera>();
        cam.farClipPlane = 5000f; // Necesario para abarcar todo Alsasua desde el aire
        
        ConfigurarPostProcesadoV7();
        CambiarVision(ModoVision.Normal);
    }

    void Update()
    {
        ManejarInputModos();
        ManejarMovimiento();
    }

    private void ManejarInputModos()
    {
        // Swapping térmico
        if (Input.GetKeyDown(KeyCode.Alpha1)) CambiarVision(ModoVision.Normal);
        if (Input.GetKeyDown(KeyCode.Alpha2)) CambiarVision(ModoVision.Nocturna);
        if (Input.GetKeyDown(KeyCode.Alpha3)) CambiarVision(ModoVision.Termica);

        // Control de Vistas Sandbox
        if (Input.GetKeyDown(KeyCode.F1)) CambiarVista(ModoVista.Libre);
        if (Input.GetKeyDown(KeyCode.F2)) CambiarVista(ModoVista.Cenital);
        // F3 -> Tercera Persona requeriría enganchar a un Target. Se mantendrá como Free-Look temporal.
    }

    private void ManejarMovimiento()
    {
        float speed = Input.GetKey(KeyCode.LeftShift) ? velocidadRapida : velocidadBase;

        if (modoActual == ModoVista.Libre)
        {
            // Ocultar y loquear cursor al girar (Cámara Dron típica)
            if (Input.GetMouseButton(1))
            {
                Cursor.lockState = CursorLockMode.Locked;
                Cursor.visible = false;
                yaw += Input.GetAxis("Mouse X") * sensibilidadRaton;
                pitch -= Input.GetAxis("Mouse Y") * sensibilidadRaton;
                pitch = Mathf.Clamp(pitch, -89f, 89f);
                transform.eulerAngles = new Vector3(pitch, yaw, 0f);
            }
            else
            {
                Cursor.lockState = CursorLockMode.None;
                Cursor.visible = true;
            }

            Vector3 move = new Vector3(Input.GetAxis("Horizontal"), 0, Input.GetAxis("Vertical"));
            if (Input.GetKey(KeyCode.E)) move.y = 1f;  // Elevador
            if (Input.GetKey(KeyCode.Q)) move.y = -1f; // Descenso

            transform.Translate(move * speed * Time.deltaTime, Space.Self);
        }
        else if (modoActual == ModoVista.Cenital)
        {
            // RTS / Estrategia Top-Down View
            Vector3 move = new Vector3(Input.GetAxis("Horizontal"), 0, Input.GetAxis("Vertical"));
            transform.Translate(move * speed * Time.deltaTime, Space.World);

            float zoom = Input.GetAxis("Mouse ScrollWheel");
            if (Mathf.Abs(zoom) > 0.01f)
            {
                transform.position += Vector3.down * zoom * speed * 5f;
            }
        }
    }

    private void CambiarVista(ModoVista nuevoModo)
    {
        modoActual = nuevoModo;
        if (modoActual == ModoVista.Cenital)
        {
            // Forzar plano cenital
            transform.rotation = Quaternion.Euler(90f, 0f, 0f);
            transform.position = new Vector3(transform.position.x, 250f, transform.position.z);
        }
    }

    private void ConfigurarPostProcesadoV7()
    {
        // Crear un entorno de Volumen URP completamente por código sin tocar el Editor
        volume = gameObject.AddComponent<Volume>();
        profile = ScriptableObject.CreateInstance<VolumeProfile>();
        volume.sharedProfile = profile;
        volume.isGlobal = true;

        colorAdj = profile.Add<ColorAdjustments>(false);
        grain = profile.Add<FilmGrain>(false);
        bloom = profile.Add<Bloom>(false);
        vignette = profile.Add<Vignette>(false);
        lensDistortion = profile.Add<LensDistortion>(false);
    }

    private void CambiarVision(ModoVision vision)
    {
        visionActual = vision;
        
        colorAdj.active = true;
        grain.active = true;
        bloom.active = true;
        vignette.active = true;
        lensDistortion.active = true;

        switch (vision)
        {
            case ModoVision.Normal:
                colorAdj.postExposure.Override(0f);
                colorAdj.colorFilter.Override(Color.white);
                colorAdj.contrast.Override(20f);
                colorAdj.saturation.Override(0f);
                colorAdj.hueShift.Override(0f);
                grain.intensity.Override(0f);
                bloom.intensity.Override(0f);
                vignette.intensity.Override(0.2f);
                lensDistortion.intensity.Override(0f);
                break;

            case ModoVision.Nocturna:
                // Amplificador de exposición, tinte militar NVG, grano masivo
                colorAdj.postExposure.Override(3.5f);
                colorAdj.colorFilter.Override(new Color(0.1f, 1f, 0.15f)); 
                colorAdj.contrast.Override(35f);
                colorAdj.saturation.Override(-30f);
                colorAdj.hueShift.Override(0f);
                grain.type.Override(FilmGrainLookup.Large02);
                grain.intensity.Override(0.85f);
                bloom.intensity.Override(3.5f);
                bloom.tint.Override(new Color(0.6f, 1f, 0.6f));
                vignette.intensity.Override(0.48f);
                lensDistortion.intensity.Override(-0.1f);
                break;

            case ModoVision.Termica:
                // Shader Fake Infrarrojo: Inversión de colores, saturación abrasiva y destellos
                colorAdj.postExposure.Override(2f);
                colorAdj.colorFilter.Override(new Color(0.2f, 0.2f, 1f)); 
                colorAdj.contrast.Override(70f);
                colorAdj.saturation.Override(80f);
                colorAdj.hueShift.Override(140f); 
                grain.type.Override(FilmGrainLookup.Medium1);
                grain.intensity.Override(0.5f);
                bloom.intensity.Override(8f);
                bloom.threshold.Override(0.2f); // Todo lo brillante explota en Bloom (Calor)
                bloom.tint.Override(Color.white);
                vignette.intensity.Override(0.35f);
                lensDistortion.intensity.Override(0f);
                break;
        }
    }
}
