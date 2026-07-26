"""
enrich_furniture.py - Add comprehensive street furniture based on real Alsasua data
bancos, fuentes, papeleras, paradas de autobús, bancos, faroles decorativos
"""
import json
import os
import random

BASE = r"F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject"
DATOS = os.path.join(BASE, "Content", "Datos")

def main():
    with open(os.path.join(DATOS, "roads_unity.json"), "r", encoding="utf-8") as f:
        roads = json.load(f)
    
    roads_arr = roads if isinstance(roads, list) else roads.get("roads", [])
    
    items = []
    rng = random.Random(42)
    
    # Collect named streets
    named_streets = []
    for r in roads_arr:
        name = r.get("name", "")
        if not name or name.startswith("Sin nombre"):
            continue
        pts = r.get("points", [])
        if not pts:
            continue
        px = pts[0].get("x", 0)
        pz = pts[0].get("z", 0)
        named_streets.append({**r, "x": px, "z": pz})
    
    # Bancos: 1 por cada 3 calles principales
    for r in named_streets:
        ancho = r.get("ancho_via", r.get("width", 8))
        if ancho >= 8 and rng.random() < 0.33:
            items.append({
                "type": "banco",
                "x": r["x"] + rng.uniform(-1, 1),
                "z": r["z"] + rng.uniform(-1, 1),
                "calle": r["name"],
                "barrio": r.get("barrio", ""),
                "material": "madera_piedra",
                "rotacion": rng.uniform(0, 360)
            })
    
    # Fuentes: en Foruen Plaza y calles principales del casco
    fuentes_candidatas = [
        {"x": 1891.5, "z": 8572.0, "nombre": "Fuente Foruen Plaza"},
        {"x": 1890.0, "z": 8573.5, "nombre": "Fuente Kale Nagusia"},
        {"x": 1892.5, "z": 8571.0, "nombre": "Fuente Zelai Kalea"},
        {"x": 1889.0, "z": 8570.0, "nombre": "Fuente Harrobieta"},
        {"x": 1893.0, "z": 8576.0, "nombre": "Fuente Intxostia"},
    ]
    for f in fuentes_candidatas:
        items.append({
            "type": "fuente",
            "x": f["x"],
            "z": f["z"],
            "nombre": f["nombre"],
            "material": "piedra_caliza",
            "activa": True,
            "rotacion": 0
        })
    
    # Papeleras: 1 por cada 5 calles
    for r in named_streets:
        if rng.random() < 0.2:
            items.append({
                "type": "papelera",
                "x": r["x"] + rng.uniform(-0.5, 0.5),
                "z": r["z"] + rng.uniform(-0.5, 0.5),
                "calle": r["name"],
                "barrio": r.get("barrio", ""),
                "material": "metal_verde",
                "rotacion": 0
            })
    
    # Paradas de autobús (Línea Sakana)
    paradas = [
        {"x": 1891.0, "z": 8572.5, "nombre": "Parada Foruen Plaza", "linea": "Sakana"},
        {"x": 1893.0, "z": 8574.0, "nombre": "Parada Zelai", "linea": "Sakana"},
        {"x": 1895.0, "z": 8575.0, "nombre": "Parada Intxostia", "linea": "Sakana"},
        {"x": 1888.0, "z": 8568.0, "nombre": "Parada San Pedro", "linea": "Sakana"},
        {"x": 1889.5, "z": 8570.0, "nombre": "Parada Harrobieta", "linea": "Sakana"},
        {"x": 1896.0, "z": 8577.0, "nombre": "Parada Zelandi", "linea": "Sakana"},
        {"x": 1890.0, "z": 8573.0, "nombre": "Parada Kale Nagusia", "linea": "Sakana"},
    ]
    for p in paradas:
        items.append({
            "type": "parada_bus",
            "x": p["x"],
            "z": p["z"],
            "nombre": p["nombre"],
            "linea": p["linea"],
            "con_techo": True,
            "rotacion": rng.uniform(0, 360)
        })
    
    # Señales de stop en intersecciones principales
    stops = [
        {"x": 1891.0, "z": 8572.0, "rotacion": 90},
        {"x": 1893.0, "z": 8574.0, "rotacion": 180},
        {"x": 1889.5, "z": 8570.5, "rotacion": 270},
    ]
    for s in stops:
        items.append({
            "type": "señal_stop",
            "x": s["x"],
            "z": s["z"],
            "rotacion": s["rotacion"],
            "altura_m": 2.2,
            "material": "metal_blanco"
        })
    
    # Señales de velocidad 30 (zona urbana)
    vel30 = [
        {"x": 1892.0, "z": 8573.0},
        {"x": 1894.0, "z": 8575.0},
        {"x": 1890.5, "z": 8571.5},
        {"x": 1889.0, "z": 8569.0},
        {"x": 1896.0, "z": 8576.0},
    ]
    for v in vel30:
        items.append({
            "type": "señal_velocidad",
            "x": v["x"],
            "z": v["z"],
            "velocidad": 30,
            "rotacion": rng.uniform(0, 360),
            "altura_m": 2.0
        })
    
    # Cruces peatonales
    cruces = [
        {"x": 1891.5, "z": 8572.0, "rotacion": 0},
        {"x": 1893.0, "z": 8574.5, "rotacion": 45},
        {"x": 1890.0, "z": 8571.0, "rotacion": 90},
        {"x": 1895.0, "z": 8576.0, "rotacion": 0},
    ]
    for c in cruces:
        items.append({
            "type": "cruce_peatonal",
            "x": c["x"],
            "z": c["z"],
            "rotacion": c["rotacion"],
            "ancho_m": 3.0,
            "material": "pintura_blanca"
        })
    
    # Protectores de acera (bollards)
    for r in named_streets[:15]:
        items.append({
            "type": "bollard",
            "x": r["x"] + rng.uniform(-0.3, 0.3),
            "z": r["z"] + rng.uniform(-0.3, 0.3),
            "calle": r["name"],
            "material": "metal_gris",
            "altura_m": 0.7,
            "rotacion": 0
        })
    
    # Guarda-barandas en puentes
    puentes = [
        {"x": 1891.0, "z": 8571.5},
        {"x": 1893.5, "z": 8575.0},
    ]
    for p in puentes:
        items.append({
            "type": "guarda_barandas",
            "x": p["x"],
            "z": p["z"],
            "material": "hierro_forjado",
            "altura_m": 1.0,
            "rotacion": 0
        })
    
    # Papeleras de reciclaje
    reciclaje = [
        {"x": 1891.0, "z": 8572.0, "tipo": "organico"},
        {"x": 1893.0, "z": 8574.0, "tipo": "envases"},
        {"x": 1895.0, "z": 8576.0, "tipo": "papel"},
    ]
    for r in reciclaje:
        items.append({
            "type": "papelera_reciclaje",
            "x": r["x"],
            "z": r["z"],
            "tipo": r["tipo"],
            "material": "metal_color",
            "rotacion": 0
        })
    
    # faroles decorativos del casco antiguo
    for r in named_streets:
        barrio = r.get("barrio", "")
        if barrio in ["Herriko", "Harrobieta"] and rng.random() < 0.5:
            items.append({
                "type": "farola_decorativa",
                "x": r["x"],
                "z": r["z"],
                "calle": r["name"],
                "barrio": barrio,
                "estilo": "forjado_tradicional",
                "altura_m": 3.5,
                "rotacion": 0
            })
    
    # Save
    output_path = os.path.join(DATOS, "street_furniture.json")
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(items, f, indent=2, ensure_ascii=False)
    
    # Count by type
    types = {}
    for item in items:
        t = item["type"]
        types[t] = types.get(t, 0) + 1
    
    print(f"[OK] {len(items)} elementos de mobiliario urbano")
    for t, c in sorted(types.items(), key=lambda x: -x[1]):
        print(f"    {t}: {c}")

if __name__ == "__main__":
    main()
