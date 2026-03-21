// Assets/Scripts/GestorAmbienteEspacial.cs
using UnityEngine;

[AddComponentMenu("Alsasua V9/Gestor de Audio 3D (Eco Urbano)")]
public class GestorAmbienteEspacial : MonoBehaviour
{
    private AudioReverbZone zonaEco;

    private void Start()
    {
        ConfigurarReverberacionGlobal();
        AplicarDopplerAVehiculos();
    }

    private void ConfigurarReverberacionGlobal()
    {
        // Las calles de Alsasua (entorno de piedra/asfalto) requieren un eco de ciudad
        zonaEco = gameObject.AddComponent<AudioReverbZone>();
        zonaEco.reverbPreset = AudioReverbPreset.City;
        zonaEco.minDistance = 50f;
        zonaEco.maxDistance = 2000f; // Cubre todo el área procedural de la ciudad
    }

    private void AplicarDopplerAVehiculos()
    {
        // Escanear todas las fuentes de audio (sirenas, cláxones) y forzar físicas 3D
        AudioSource[] todasLasFuentes = FindObjectsByType<AudioSource>(FindObjectsSortMode.None);
        foreach (AudioSource fuente in todasLasFuentes)
        {
            if (fuente.gameObject.name.Contains("Vehiculo") || fuente.gameObject.name.Contains("Police"))
            {
                fuente.spatialBlend = 1f; // 100% 3D
                fuente.dopplerLevel = 1.5f; // Efecto Doppler exagerado (Sirenas al pasar rápido)
                fuente.rolloffMode = AudioRolloffMode.Logarithmic;
                fuente.minDistance = 10f;
                fuente.maxDistance = 500f; // Se escucha a medio kilómetro de distancia
            }
        }
    }
}
