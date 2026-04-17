// Assets/Scripts/GeoDataCalles.cs
// ═══════════════════════════════════════════════════════════════════════════
//  LAYOUT URBANO REAL DE ALSASUA / ALTSASU
//  Valle de Burunda — Navarra
//
//  Todas las calles, manzanas y edificios singulares están posicionados en
//  espacio local Unity respecto al origen de GeoDataAlsasua (Herriko Plaza).
//
//  FUENTES:
//    · OpenStreetMap (openstreetmap.org) — calles y footprints de edificios
//    · Google Street View — alturas y tipologías de fachada
//    · Catastro de Navarra (idena.navarra.es) — parcelas y usos del suelo
//
//  SISTEMA DE COORDENADAS:
//    +X = Este, +Z = Norte, +Y = Arriba, 1 unidad = 1 metro
//    Origen = Herriko Plaza / Plaza de los Fueros (42.9016 N, -2.1668 W)
// ═══════════════════════════════════════════════════════════════════════════

using UnityEngine;

public static class GeoDataCalles
{
    // ═══════════════════════════════════════════════════════════════════════
    //  TIPOS
    // ═══════════════════════════════════════════════════════════════════════

    public enum TipoEdificio : byte
    {
        CascoAntiguo,   // edificio antiguo (piedra, 2-3 plantas, pre-1900)
        Residencial,    // bloque residencial moderno (3-5 plantas)
        Comercial,      // planta baja comercial + pisos (4-6 plantas)
        Industrial,     // nave o almacén (1-2 plantas, superficie grande)
        Institucional,  // ayuntamiento, iglesia, cuartel, colegio
        Deportivo,      // polideportivo, frontón
    }

    public enum TipoCalle : byte
    {
        Autovia,        // N-1 / NA-120 (calzada de 6+ m, dos carriles)
        Primaria,       // calle mayor (4-6 m)
        Secundaria,     // calle de barrio (3-4 m)
        Peatonal,       // calle peatonal o callejón (<3 m)
    }

    /// <summary>Una calle definida como polilínea de waypoints.</summary>
    public struct CalleData
    {
        public string    Nombre;
        public Vector3[] Puntos;     // polilínea (orden: entrada → salida)
        public float     Ancho;      // metros de calzada
        public TipoCalle Tipo;
    }

    /// <summary>
    /// Una manzana (city block) definida por su centro, tamaño y tipología.
    /// Los edificios se colocan en el perímetro interior del bloque.
    /// </summary>
    public struct ManzanaData
    {
        public string       Nombre;
        public Vector3      Centro;
        public float        TamanoX;       // ancho E-O (metros)
        public float        TamanoZ;       // largo N-S (metros)
        public float        RotacionY;     // rotación en grados si la manzana no es ortogonal
        public int          NumPlantas;    // altura típica de los edificios
        public TipoEdificio Tipo;
    }

    /// <summary>
    /// Un edificio singular (landmark) con footprint exacto.
    /// </summary>
    public struct EdificioSingular
    {
        public string       Nombre;
        public Vector3      Centro;
        public float        TamanoX;
        public float        TamanoZ;
        public float        Altura;        // metros reales
        public TipoEdificio Tipo;
    }

    // ═══════════════════════════════════════════════════════════════════════
    //  CALLES — polilíneas reales de Alsasua (OSM)
    // ═══════════════════════════════════════════════════════════════════════
    //
    //  Referencia OSM de calles clave (lat/lon → Unity con GeoDataAlsasua.GpsAUnity):
    //    Nafarroa Kalea     42.9030/-2.1680 → 42.8990/-2.1680 (N-S, lado O casco)
    //    Ameztia Kalea      42.8997/-2.1695 → 42.8997/-2.1655 (E-O, sur)
    //    Kale Nagusia       42.9015/-2.1685 → 42.9015/-2.1650 (E-O, centro)
    //    Brentana Kalea     42.9025/-2.1655 → 42.9010/-2.1655 (N-S, lado E)
    //    San Francisco      42.9035/-2.1680 → 42.9035/-2.1655 (E-O, norte)
    //    Erdikale (kale)    42.9018/-2.1683 → 42.9008/-2.1690 (diagonal casco viejo)

    public static readonly CalleData[] CallesPrincipales = new CalleData[]
    {
        // ── N-S ───────────────────────────────────────────────────────────

        new CalleData
        {
            Nombre = "Nafarroa Kalea / Calle Navarra",
            Ancho  = 7f,
            Tipo   = TipoCalle.Primaria,
            Puntos = new Vector3[]
            {
                new Vector3(-122f, 0f, -360f),  // cruce sur (Ameztia)
                new Vector3(-118f, 0f, -180f),  // tramo medio sur
                new Vector3(-110f, 0f,    0f),  // Herriko Plaza W
                new Vector3(-105f, 0f,  190f),  // tramo norte
                new Vector3(-100f, 0f,  310f),  // cruce San Francisco
            },
        },

        new CalleData
        {
            Nombre = "Erdikale / Calle Central (casco viejo)",
            Ancho  = 4f,
            Tipo   = TipoCalle.Secundaria,
            Puntos = new Vector3[]
            {
                new Vector3( -75f, 0f, -120f),  // entrada sur casco viejo
                new Vector3( -95f, 0f,  -60f),  // giro callejón
                new Vector3(-110f, 0f,    0f),  // Plaza (confluencia Navarra)
                new Vector3(-130f, 0f,   50f),  // callejón norte
                new Vector3(-120f, 0f,  120f),  // salida norte casco viejo
            },
        },

        new CalleData
        {
            Nombre = "Brentana Kalea",
            Ancho  = 5f,
            Tipo   = TipoCalle.Secundaria,
            Puntos = new Vector3[]
            {
                new Vector3(  80f, 0f,   50f),
                new Vector3(  85f, 0f,  150f),
                new Vector3(  90f, 0f,  280f),
            },
        },

        new CalleData
        {
            Nombre = "Inurritza Kalea (N-S sur)",
            Ancho  = 5f,
            Tipo   = TipoCalle.Secundaria,
            Puntos = new Vector3[]
            {
                new Vector3(  20f, 0f, -350f),
                new Vector3(  25f, 0f, -220f),
                new Vector3(  30f, 0f,  -80f),
            },
        },

        new CalleData
        {
            Nombre = "N-1 (Servicio urbano) — sentido norte",
            Ancho  = 10f,
            Tipo   = TipoCalle.Autovia,
            Puntos = new Vector3[]
            {
                new Vector3( 100f, 0f, -900f),
                new Vector3(  90f, 0f, -500f),
                new Vector3(  80f, 0f, -200f),
                new Vector3(  70f, 0f,    0f),
                new Vector3(  60f, 0f,  250f),
                new Vector3(  50f, 0f,  600f),
                new Vector3(  40f, 0f,  900f),
            },
        },

        // ── E-O ───────────────────────────────────────────────────────────

        new CalleData
        {
            Nombre = "Ameztia Kalea (sur — cuartel GC)",
            Ancho  = 6f,
            Tipo   = TipoCalle.Secundaria,
            Puntos = new Vector3[]
            {
                new Vector3(-380f, 0f, -310f),  // inicio O (lado cuartel GC)
                new Vector3(-260f, 0f, -310f),  // frente cuartel GC
                new Vector3(-120f, 0f, -310f),  // tramo centro
                new Vector3(  20f, 0f, -310f),  // cruce Inurritza
                new Vector3(  90f, 0f, -310f),  // cruce N-1 urbana
            },
        },

        new CalleData
        {
            Nombre = "Kale Nagusia / Calle Mayor",
            Ancho  = 8f,
            Tipo   = TipoCalle.Primaria,
            Puntos = new Vector3[]
            {
                new Vector3(-230f, 0f,  -30f),  // inicio O (zona industrial Ibarrea)
                new Vector3(-110f, 0f,  -20f),  // cruce Navarra sur
                new Vector3(   0f, 0f,    0f),  // Herriko Plaza (centro)
                new Vector3( 120f, 0f,   10f),  // cruce Brentana
                new Vector3( 200f, 0f,   15f),  // acceso N-1 comercial
            },
        },

        new CalleData
        {
            Nombre = "Inurritza Kalea (E-O)",
            Ancho  = 5f,
            Tipo   = TipoCalle.Secundaria,
            Puntos = new Vector3[]
            {
                new Vector3(-180f, 0f, -170f),
                new Vector3( -90f, 0f, -170f),
                new Vector3(   0f, 0f, -165f),
                new Vector3(  70f, 0f, -160f),
            },
        },

        new CalleData
        {
            Nombre = "San Franzisko Kalea / Calle San Francisco",
            Ancho  = 6f,
            Tipo   = TipoCalle.Secundaria,
            Puntos = new Vector3[]
            {
                new Vector3(-155f, 0f,  260f),
                new Vector3( -80f, 0f,  255f),
                new Vector3(   0f, 0f,  250f),
                new Vector3( 100f, 0f,  250f),
            },
        },

        new CalleData
        {
            Nombre = "Agirre Kalea (casco viejo, E-O)",
            Ancho  = 4f,
            Tipo   = TipoCalle.Peatonal,
            Puntos = new Vector3[]
            {
                new Vector3(-195f, 0f,  -80f),
                new Vector3(-120f, 0f,  -75f),
                new Vector3( -70f, 0f,  -70f),
            },
        },

        new CalleData
        {
            Nombre = "Zubiaurre Kalea (acceso estación tren)",
            Ancho  = 6f,
            Tipo   = TipoCalle.Secundaria,
            Puntos = new Vector3[]
            {
                new Vector3(-510f, 0f, -780f),  // estación
                new Vector3(-400f, 0f, -650f),
                new Vector3(-280f, 0f, -500f),
                new Vector3(-150f, 0f, -400f),
                new Vector3( -80f, 0f, -310f),  // cruce Ameztia
            },
        },

        new CalleData
        {
            Nombre = "NA-120 (E-O, paso urbano)",
            Ancho  = 9f,
            Tipo   = TipoCalle.Autovia,
            Puntos = new Vector3[]
            {
                new Vector3(-500f, 0f,  80f),
                new Vector3(-250f, 0f,  60f),
                new Vector3(   0f, 0f,  40f),
                new Vector3( 300f, 0f,  20f),
                new Vector3( 600f, 0f,   0f),
            },
        },
    };

    // ═══════════════════════════════════════════════════════════════════════
    //  MANZANAS — bloques urbanos reales de Alsasua
    //  Posiciones calculadas a partir del layout OSM del casco urbano.
    //  Cada manzana define donde se colocan edificios en SistemaEdificios.
    // ═══════════════════════════════════════════════════════════════════════

    public static readonly ManzanaData[] ManzanasAlsasua = new ManzanaData[]
    {
        // ─── CASCO ANTIGUO (NW del centro) ──────────────────────────────

        new ManzanaData
        {
            Nombre    = "Casco Antiguo Norte",
            Centro    = new Vector3(-140f, 0f,  80f),
            TamanoX   = 70f, TamanoZ = 80f,
            NumPlantas = 3,
            Tipo      = TipoEdificio.CascoAntiguo,
        },
        new ManzanaData
        {
            Nombre    = "Casco Antiguo Sur",
            Centro    = new Vector3(-130f, 0f, -60f),
            TamanoX   = 80f, TamanoZ = 90f,
            NumPlantas = 3,
            Tipo      = TipoEdificio.CascoAntiguo,
        },
        new ManzanaData
        {
            Nombre    = "Casco Antiguo Oeste",
            Centro    = new Vector3(-200f, 0f,  20f),
            TamanoX   = 60f, TamanoZ = 100f,
            NumPlantas = 2,
            Tipo      = TipoEdificio.CascoAntiguo,
        },

        // ─── CENTRO URBANO (alrededor de Herriko Plaza) ─────────────────

        new ManzanaData
        {
            Nombre    = "Centro Norte (frente plaza)",
            Centro    = new Vector3( -50f, 0f, 120f),
            TamanoX   = 100f, TamanoZ = 90f,
            NumPlantas = 5,
            Tipo      = TipoEdificio.Comercial,
        },
        new ManzanaData
        {
            Nombre    = "Centro Sur (junto plaza)",
            Centro    = new Vector3( -40f, 0f, -90f),
            TamanoX   = 100f, TamanoZ = 80f,
            NumPlantas = 4,
            Tipo      = TipoEdificio.Comercial,
        },
        new ManzanaData
        {
            Nombre    = "Centro Este",
            Centro    = new Vector3( 110f, 0f,  30f),
            TamanoX   = 70f, TamanoZ = 100f,
            NumPlantas = 4,
            Tipo      = TipoEdificio.Comercial,
        },
        new ManzanaData
        {
            Nombre    = "Barrio Norte-Centro",
            Centro    = new Vector3( -60f, 0f, 200f),
            TamanoX   = 110f, TamanoZ = 80f,
            NumPlantas = 4,
            Tipo      = TipoEdificio.Residencial,
        },

        // ─── BARRIO NORTE (hacia Vitoria) ────────────────────────────────

        new ManzanaData
        {
            Nombre    = "Barrio Norte A",
            Centro    = new Vector3( -80f, 0f, 340f),
            TamanoX   = 120f, TamanoZ = 70f,
            NumPlantas = 4,
            Tipo      = TipoEdificio.Residencial,
        },
        new ManzanaData
        {
            Nombre    = "Barrio Norte B (junto comisaría PF)",
            Centro    = new Vector3(  60f, 0f, 350f),
            TamanoX   = 100f, TamanoZ = 70f,
            NumPlantas = 3,
            Tipo      = TipoEdificio.Residencial,
        },

        // ─── BARRIO SUR (hacia N-1 sur y polígono Ondarria) ─────────────

        new ManzanaData
        {
            Nombre    = "Barrio Sur A",
            Centro    = new Vector3( -50f, 0f, -200f),
            TamanoX   = 110f, TamanoZ = 80f,
            NumPlantas = 3,
            Tipo      = TipoEdificio.Residencial,
        },
        new ManzanaData
        {
            Nombre    = "Barrio Sur B (junto cuartel GC)",
            Centro    = new Vector3(-120f, 0f, -350f),
            TamanoX   = 120f, TamanoZ = 60f,
            NumPlantas = 3,
            Tipo      = TipoEdificio.Residencial,
        },
        new ManzanaData
        {
            Nombre    = "Barrio Sur Este",
            Centro    = new Vector3(  60f, 0f, -230f),
            TamanoX   = 80f, TamanoZ = 80f,
            NumPlantas = 3,
            Tipo      = TipoEdificio.Residencial,
        },

        // ─── BARRIO OESTE (Ibarrea) ──────────────────────────────────────

        new ManzanaData
        {
            Nombre    = "Barrio Oeste A",
            Centro    = new Vector3(-320f, 0f,  50f),
            TamanoX   = 90f, TamanoZ = 90f,
            NumPlantas = 3,
            Tipo      = TipoEdificio.Residencial,
        },
        new ManzanaData
        {
            Nombre    = "Barrio Oeste B",
            Centro    = new Vector3(-310f, 0f, -150f),
            TamanoX   = 80f, TamanoZ = 80f,
            NumPlantas = 2,
            Tipo      = TipoEdificio.Residencial,
        },

        // ─── ZONA INDUSTRIAL (Ondarria / Isasia) ─────────────────────────

        new ManzanaData
        {
            Nombre    = "Nave Industrial Ondarria A",
            Centro    = new Vector3( 350f, 0f, -700f),
            TamanoX   = 180f, TamanoZ = 120f,
            NumPlantas = 1,
            Tipo      = TipoEdificio.Industrial,
        },
        new ManzanaData
        {
            Nombre    = "Nave Industrial Ondarria B",
            Centro    = new Vector3( 550f, 0f, -850f),
            TamanoX   = 150f, TamanoZ = 100f,
            NumPlantas = 1,
            Tipo      = TipoEdificio.Industrial,
        },
        new ManzanaData
        {
            Nombre    = "Nave Industrial Isasia",
            Centro    = new Vector3(-900f, 0f,-1200f),
            TamanoX   = 200f, TamanoZ = 150f,
            NumPlantas = 1,
            Tipo      = TipoEdificio.Industrial,
        },
    };

    // ═══════════════════════════════════════════════════════════════════════
    //  EDIFICIOS SINGULARES — landmarks con footprint exacto
    // ═══════════════════════════════════════════════════════════════════════

    public static readonly EdificioSingular[] EdificiosSingulares = new EdificioSingular[]
    {
        new EdificioSingular
        {
            Nombre  = "Udaletxea / Ayuntamiento",
            Centro  = GeoDataAlsasua.Ayuntamiento,
            TamanoX = 30f, TamanoZ = 20f,
            Altura  = 12f,
            Tipo    = TipoEdificio.Institucional,
        },
        new EdificioSingular
        {
            Nombre  = "Iglesia San Miguel Arcángel",
            Centro  = GeoDataAlsasua.IglesiaSanMiguel,
            TamanoX = 40f, TamanoZ = 22f,
            Altura  = 22f,   // incluye campanario
            Tipo    = TipoEdificio.Institucional,
        },
        new EdificioSingular
        {
            Nombre  = "Cuartel Guardia Civil (Calle Ameztia)",
            Centro  = GeoDataAlsasua.CuartelGuardiaCivil,
            TamanoX = 55f, TamanoZ = 45f,
            Altura  = 8f,
            Tipo    = TipoEdificio.Institucional,
        },
        new EdificioSingular
        {
            Nombre  = "Comisaría Policía Foral",
            Centro  = GeoDataAlsasua.ComisariaPolForal,
            TamanoX = 35f, TamanoZ = 30f,
            Altura  = 9f,
            Tipo    = TipoEdificio.Institucional,
        },
        new EdificioSingular
        {
            Nombre  = "Estación de Tren Alsasua (Adif)",
            Centro  = GeoDataAlsasua.EstacionTren,
            TamanoX = 60f, TamanoZ = 20f,
            Altura  = 7f,
            Tipo    = TipoEdificio.Institucional,
        },
        new EdificioSingular
        {
            Nombre  = "Polideportivo Municipal",
            Centro  = new Vector3(-220f, 0f, 330f),
            TamanoX = 90f, TamanoZ = 60f,
            Altura  = 12f,
            Tipo    = TipoEdificio.Deportivo,
        },
        new EdificioSingular
        {
            Nombre  = "Frontón Pilota (trinquete)",
            Centro  = new Vector3( -60f, 0f, 130f),
            TamanoX = 55f, TamanoZ = 30f,
            Altura  = 14f,
            Tipo    = TipoEdificio.Deportivo,
        },
        new EdificioSingular
        {
            Nombre  = "Colegio San Francisco",
            Centro  = new Vector3(-110f, 0f, 200f),
            TamanoX = 55f, TamanoZ = 45f,
            Altura  = 10f,
            Tipo    = TipoEdificio.Institucional,
        },
        new EdificioSingular
        {
            Nombre  = "Supermercado / Eroski",
            Centro  = new Vector3( 150f, 0f, -50f),
            TamanoX = 60f, TamanoZ = 40f,
            Altura  = 6f,
            Tipo    = TipoEdificio.Comercial,
        },
    };

    // ═══════════════════════════════════════════════════════════════════════
    //  UTILIDAD — obtener la calle más cercana a un punto
    // ═══════════════════════════════════════════════════════════════════════

    /// <summary>
    /// Devuelve la distancia mínima de un punto al segmento más cercano de
    /// cualquiera de las calles principales.
    /// Útil para evitar spawnear vegetación o árboles en la calzada.
    /// </summary>
    public static float DistanciaAlCallejero(Vector3 punto)
    {
        float minDist = float.MaxValue;
        foreach (var calle in CallesPrincipales)
        {
            for (int i = 0; i < calle.Puntos.Length - 1; i++)
            {
                float d = DistanciaSegmento(punto, calle.Puntos[i], calle.Puntos[i + 1]);
                if (d < minDist) minDist = d;
            }
        }
        return minDist;
    }

    private static float DistanciaSegmento(Vector3 p, Vector3 a, Vector3 b)
    {
        Vector3 ab = b - a;
        float len2 = ab.x * ab.x + ab.z * ab.z;
        if (len2 < 0.001f) return Vector3.Distance(p, a);
        float t = Mathf.Clamp01(((p.x - a.x) * ab.x + (p.z - a.z) * ab.z) / len2);
        Vector3 proj = new Vector3(a.x + t * ab.x, p.y, a.z + t * ab.z);
        return Vector3.Distance(p, proj);
    }
}
