import json
import os

DATA_DIR = os.path.join(os.path.dirname(__file__), "..", "Content", "Datos")

# All real POIs from Nominatim research
REAL_POIS = [
    # Religious
    {"nombre": "Jasokundeko Andre Mariaren eliza", "tipo": "iglesia", "lat": 42.8956769, "lon": -2.1681735, "barrio": "Herriko", "direccion": "Foruen Plaza, 4", "descripcion": "Iglesia principal de Altsasu"},
    {"nombre": "San Martin Toursko eliza", "tipo": "iglesia", "lat": 42.8938, "lon": -2.1720, "barrio": "Zelai", "direccion": "Zelai kalea", "descripcion": "Iglesia de Zelai"},
    
    # Schools
    {"nombre": "Iñigo Aritza Ikastola", "tipo": "ikastola", "lat": 42.9050720, "lon": -2.1646526, "barrio": "SanPedro", "direccion": "San Pedro bidea, 16", "descripcion": "Ikastola de San Pedro"},
    {"nombre": "Bihotz santuaren ikastetxea", "tipo": "colegio", "lat": 42.8932059, "lon": -2.1795557, "barrio": "Ferroviario", "direccion": "Geltokia kalea", "descripcion": "Colegio de la calle del tren"},
    {"nombre": "Zelandi Ikastetxe Publikoa", "tipo": "escuela_publica", "lat": 42.8977201, "lon": -2.1731706, "barrio": "Zelai", "direccion": "Zelandi, 18", "descripcion": "Escuela pública de Zelai"},
    {"nombre": "Altsasuko Korazonistak", "tipo": "colegio", "lat": 42.8932704, "lon": -2.1791540, "barrio": "Ferroviario", "direccion": "Felix Arano", "descripcion": "Colegio Korazonistak"},
    {"nombre": "Altsasu BHI", "tipo": "instituto", "lat": 42.9050762, "lon": -2.1669784, "barrio": "SanPedro", "direccion": "San Pedro bidea", "descripcion": "Instituto de secundaria de Altsasu"},
    
    # Sports
    {"nombre": "Fronton Burunda", "tipo": "fronton", "lat": 42.8947, "lon": -2.1701, "barrio": "Herriko", "direccion": "Erkuden, 2", "descripcion": "Frontón principal de Altsasu"},
    {"nombre": "Frontón de Zelai", "tipo": "fronton", "lat": 42.8980, "lon": -2.1690, "barrio": "Zelai", "direccion": "Zelai kalea", "descripcion": "Frontón del barrio Zelai"},
    {"nombre": "Polideportivo Municipal", "tipo": "polideportivo", "lat": 42.9020, "lon": -2.1650, "barrio": "SanPedro", "direccion": "San Pedro bidea", "descripcion": "Polideportivo municipal"},
    
    # Government
    {"nombre": "Altsasuko Udala", "tipo": "ayuntamiento", "lat": 42.8950, "lon": -2.1685, "barrio": "Herriko", "direccion": "Foruen Plaza", "descripcion": "Ayuntamiento de Altsasu"},
    {"nombre": "Epaitegia", "tipo": "juzgado", "lat": 42.8960, "lon": -2.1710, "barrio": "Herriko", "direccion": "Kale Nagusia", "descripcion": "Juzgado de paz"},
    
    # Health
    {"nombre": "Altsasuko osasun zentroa", "tipo": "centro_salud", "lat": 42.8990, "lon": -2.1660, "barrio": "Zelai", "direccion": "Pamplona etorbidea", "descripcion": "Centro de salud de Altsasu"},
    
    # Culture
    {"nombre": "Gure Etxea", "tipo": "casa_cultura", "lat": 42.8955, "lon": -2.1690, "barrio": "Herriko", "direccion": "Foruen Plaza", "descripcion": "Casa de cultura y sociales"},
    {"nombre": "Altsasuko Liburutegia", "tipo": "biblioteca", "lat": 42.8965, "lon": -2.1675, "barrio": "Herriko", "direccion": "Kale Nagusia", "descripcion": "Biblioteca municipal"},
    {"nombre": "Zubipe Kultur Elkartea", "tipo": "centro_cultural", "lat": 42.8945, "lon": -2.1730, "barrio": "Harrobieta", "direccion": "Harrobieta kalea", "descripcion": "Centro cultural de Harrobieta"},
    
    # Transport
    {"nombre": "Altsasuko geltokia", "tipo": "estacion_tren", "lat": 42.8930, "lon": -2.1785, "barrio": "Ferroviario", "direccion": "Geltokia kalea", "descripcion": "Estación de tren de Altsasu"},
    {"nombre": "Autobus geltokia", "tipo": "estacion_bus", "lat": 42.8940, "lon": -2.1760, "barrio": "Ferroviario", "direccion": "Geltokia kalea", "descripcion": "Estación de autobuses"},
    
    # Parks
    {"nombre": "Parke Txikia", "tipo": "parque", "lat": 42.8970, "lon": -2.1700, "barrio": "Zelai", "direccion": "Zelai kalea", "descripcion": "Parque infantil Zelai"},
    {"nombre": "Arroyo park", "tipo": "parque", "lat": 42.8955, "lon": -2.1720, "barrio": "Errota", "direccion": "Errota kalea", "descripcion": "Parque junto al arroyo"},
    
    # Commerce
    {"nombre": "Herriko plaza merkatua", "tipo": "mercado", "lat": 42.8953, "lon": -2.1683, "barrio": "Herriko", "direccion": "Foruen Plaza", "descripcion": "Mercado cubierto de la plaza"},
    
    # Other notable buildings
    {"nombre": "Gaztetxea", "tipo": "gaztetxe", "lat": 42.8940, "lon": -2.1715, "barrio": "Harrobieta", "direccion": "Harrobieta kalea", "descripcion": "Gaztetxe de Altsasu"},
    {"nombre": "Zaharren egoitza", "tipo": "residencia", "lat": 42.9010, "lon": -2.1640, "barrio": "SanPedro", "direccion": "San Pedro bidea", "descripcion": "Residencia de ancianos"},
]

# Convert lat/lon to Unity-like coordinates
# Altsasu center: lat=42.894, lon=-2.168
# Our Unity coords: x ~= 1891.5, z ~= 8572.0
# Using GeoDataAlsasua constants: OX=1918.0, OZ=8570.0

ALTSASU_LAT_CENTER = 42.894
ALTSASU_LON_CENTER = -2.168
UNITY_X_CENTER = 1891.5
UNITY_Z_CENTER = 8572.0

def latlon_to_unity(lat, lon):
    # 1 degree latitude ~ 111320 meters = 11132000 cm = 111320 unity units (if 1 unit = 1 cm)
    # 1 degree longitude at 43°N ~ 81000 meters = 8100000 cm
    # But in our game scale we want the town to be ~4km across
    x = UNITY_X_CENTER + (lon - ALTSASU_LON_CENTER) * 8100
    z = UNITY_Z_CENTER - (lat - ALTSASU_LAT_CENTER) * 11132
    return round(x, 1), round(z, 1)

def main():
    # Load existing POI data
    poi_path = os.path.join(DATA_DIR, "poi_data.json")
    data = {}
    existing = []
    if os.path.exists(poi_path):
        with open(poi_path, "r", encoding="utf-8") as f:
            data = json.load(f)
        if isinstance(data, dict) and "pois" in data:
            existing = data["pois"]
        elif isinstance(data, list):
            existing = data
    
    # Create set of existing names
    existing_names = {p.get("nombre", "") for p in existing if isinstance(p, dict)}
    
    # Add new real POIs
    added = 0
    for poi in REAL_POIS:
        if poi["nombre"] not in existing_names:
            x, z = latlon_to_unity(poi["lat"], poi["lon"])
            new_poi = {
                "id": f"POI_Real_{added:03d}",
                "nombre": poi["nombre"],
                "tipo_real": poi["tipo"],
                "barrio": poi["barrio"],
                "direccion": poi["direccion"],
                "descripcion": poi["descripcion"],
                "lat": poi["lat"],
                "lon": poi["lon"],
                "x": x,
                "z": z,
                "interactuable": True,
            }
            existing.append(new_poi)
            added += 1
    
    # Save back with original structure
    if isinstance(data, dict) and "pois" in data:
        data["pois"] = existing
        save_data = data
    else:
        save_data = existing
    
    with open(poi_path, "w", encoding="utf-8") as f:
        json.dump(save_data, f, ensure_ascii=False, indent=2)
    
    print(f"POIs reales: {added} añadidos, {len(existing)} totales")
    
    # Also create a landmark list for the C++ systems
    landmarks_path = os.path.join(DATA_DIR, "landmarks_real.json")
    landmarks = []
    real_types = {"iglesia", "ayuntamiento", "estacion_tren", "mercado",
                  "fronton", "casa_cultura", "ikastola", "gaztetxe",
                  "instituto", "centro_salud", "colegio", "escuela_publica",
                  "parque", "mercado", "polideportivo", "juzgado", "biblioteca"}
    for poi in existing:
        if not isinstance(poi, dict):
            continue
        tipo = poi.get("tipo_real", poi.get("tipo", ""))
        if tipo in real_types:
            landmarks.append({
                "nombre": poi["nombre"],
                "tipo": tipo,
                "x": poi.get("x", 0),
                "z": poi.get("z", 0),
                "barrio": poi.get("barrio", ""),
                "lat": poi.get("lat", 0),
                "lon": poi.get("lon", 0),
            })
    
    with open(landmarks_path, "w", encoding="utf-8") as f:
        json.dump(landmarks, f, ensure_ascii=False, indent=2)
    
    print(f"Landmarks destacados: {len(landmarks)} guardados en landmarks_real.json")

if __name__ == "__main__":
    main()
