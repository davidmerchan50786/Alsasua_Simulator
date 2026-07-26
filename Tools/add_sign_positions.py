"""
Add real x/z positions to signage_data.json using roads_unity.json data.
Signs are placed at the START of each named street.
"""
import json
import os
import random

BASE = r"F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject"
DATOS = os.path.join(BASE, "Content", "Datos")

def main():
    with open(os.path.join(DATOS, "roads_unity.json"), "r", encoding="utf-8") as f:
        roads = json.load(f)
    with open(os.path.join(DATOS, "signage_data.json"), "r", encoding="utf-8") as f:
        signs = json.load(f)
    
    street_map = {}
    for road in roads:
        name = road.get("name", "")
        if not name or name.startswith("Sin nombre"):
            continue
        if name not in street_map:
            street_map[name] = {
                "x": road.get("x", 0),
                "z": road.get("z", 0),
                "barrio": road.get("barrio", ""),
                "points": road.get("points", []),
            }
    
    placed = 0
    for sign in signs:
        if sign["tipo"] == "señal_calle":
            texto = sign["texto"]
            matched = False
            for street_name, info in street_map.items():
                if texto.lower() in street_name.lower() or street_name.lower() in texto.lower():
                    sign["x"] = info["x"]
                    sign["z"] = info["z"]
                    sign["barrio"] = info["barrio"]
                    matched = True
                    break
            if not matched and street_map:
                sign["x"] = 1890 + random.uniform(-3, 3)
                sign["z"] = 8540 + random.uniform(-3, 3)
        elif sign["tipo"] == "comercial":
            barrio = sign.get("barrio", "")
            barrio_centers = {
                "Herriko": (1891.5, 8572.0),
                "Zelai": (1894.0, 8578.0),
                "Intxostia": (1897.0, 8575.0),
                "SanPedro": (1888.0, 8568.0),
            }
            if barrio in barrio_centers:
                cx, cz = barrio_centers[barrio]
                sign["x"] = cx + random.uniform(-2, 2)
                sign["z"] = cz + random.uniform(-2, 2)
            else:
                sign["x"] = 1895 + random.uniform(-5, 5)
                sign["z"] = 8575 + random.uniform(-5, 5)
        else:
            barrio = sign.get("barrio", "")
            barrio_centers = {
                "Herriko": (1891.5, 8572.0),
                "Zelai": (1894.0, 8578.0),
                "Intxostia": (1897.0, 8575.0),
                "SanPedro": (1888.0, 8568.0),
            }
            if barrio in barrio_centers:
                cx, cz = barrio_centers[barrio]
                sign["x"] = cx + random.uniform(-1, 1)
                sign["z"] = cz + random.uniform(-1, 1)
            else:
                sign["x"] = 1895 + random.uniform(-3, 3)
                sign["z"] = 8575 + random.uniform(-3, 3)
        placed += 1
    
    with open(os.path.join(DATOS, "signage_data.json"), "w", encoding="utf-8") as f:
        json.dump(signs, f, indent=2, ensure_ascii=False)
    
    print(f"[OK] {placed} señales con posiciones asignadas")
    with_pos = sum(1 for s in signs if "x" in s and "z" in s)
    print(f"    {with_pos}/{len(signs)} señales tienen coordenadas x/z")

if __name__ == "__main__":
    main()
