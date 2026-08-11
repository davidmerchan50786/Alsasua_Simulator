"""
asset_manifest.py - Comprehensive asset manifest mapping all real data
Creates a single JSON that maps every asset to its UE5 path, category, and usage.
"""
import json
import os
import glob

# Raiz derivada de donde vive este fichero. Antes fija a la maquina original
# ("F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject"), asi que solo corria alli.
BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTENT = os.path.join(BASE, "Content")
ASSETS = os.path.join(CONTENT, "AssetsImportados")
DATOS = os.path.join(CONTENT, "Datos")
MATERIALS = os.path.join(CONTENT, "Materiales")

def find_files(root, extensions):
    result = []
    for ext in extensions:
        for f in glob.glob(os.path.join(root, "**", f"*.{ext}"), recursive=True):
            size = os.path.getsize(f)
            rel = os.path.relpath(f, CONTENT).replace("\\", "/")
            ue5_path = "/Game/" + rel.replace(".uasset", "")
            result.append({
                "disk_path": f,
                "ue5_path": ue5_path,
                "size_bytes": size,
                "filename": os.path.basename(f),
            })
    return result

def main():
    manifest = {
        "version": "2.0",
        "project": "Alsasua/Altsasu - Copia 1:1",
        "real_data_files": {},
        "assets_by_category": {},
        "barrio_materials": {
            "Herriko": {
                "wall": "piedra_caliza",
                "roof": "teja_terracota",
                "style": "casco_vasco",
                "description": "Casco antiguo - muros de piedra caliza, tejados de teja"
            },
            "Zelai": {
                "wall": "ladrillo",
                "roof": "teja_terracota",
                "style": "ensanche",
                "description": "Ensanche - bloques de ladrillo, tejados planos o a dos aguas"
            },
            "Intxostia": {
                "wall": "piedra_hormigon",
                "roof": "pizarra",
                "style": "industrial_residencial",
                "description": "Zona industrial/residencial - hormigón y piedra"
            },
            "SanPedro": {
                "wall": "ladrillo_industrial",
                "roof": "plano",
                "style": "industrial",
                "description": "Polígono industrial"
            },
            "Errota": {
                "wall": "piedra",
                "roof": "teja_terracota",
                "style": "rural",
                "description": "Zona rural - edificios históricos"
            },
            "Harrobieta": {
                "wall": "piedra",
                "roof": "pizarra",
                "style": "rural_antiguo",
                "description": "Barrio más antiguo - cantería"
            },
            "Ferroviario": {
                "wall": "ladrillo",
                "roof": "plano",
                "style": "ferroviario",
                "description": "Zona del tren - ladrillo y chapa"
            },
            "Monte": {
                "wall": "piedra",
                "roof": "teja_terracota",
                "style": "diseminado",
                "description": "Diseminado - casas sueltas en ladera"
            }
        },
        "street_categories": {
            "principal": {"width_m": 12, "surface": "asphalt", "sidewalks": True, "farolas": "tradicional"},
            "secundaria": {"width_m": 8, "surface": "asphalt", "sidewalks": True, "farolas": "tradicional"},
            "residencial": {"width_m": 6, "surface": "asphalt", "sidewalks": False, "farolas": "moderna"},
            "peatonal": {"width_m": 4, "surface": "cobblestone", "sidewalks": False, "farolas": "tradicional"},
            "rural": {"width_m": 5, "surface": "gravel", "sidewalks": False, "farolas": None}
        },
        "tree_species_navarra": {
            "haltza": {"scientific": "Alnus glutinosa", "es": "Aliso negro", "height_range": [8, 20], "habitat": "ribera"},
            "haya": {"scientific": "Fagus sylvatica", "es": "Haya europea", "height_range": [20, 35], "habitat": "monte_bosque"},
            "urki": {"scientific": "Betula pendula", "es": "Abedul", "height_range": [10, 25], "habitat": "monte_bosque"},
            "sahats": {"scientific": "Salix alba", "es": "Sauce", "height_range": [8, 15], "habitat": "ribera"},
            "pinu_horizin": {"scientific": "Pinus halepensis", "es": "Pino carrasco", "height_range": [8, 15], "habitat": "secano"},
            "pinu_larrein": {"scientific": "Pinus sylvestris", "es": "Pino baltico", "height_range": [15, 30], "habitat": "monte"},
            "haritz": {"scientific": "Quercus robur", "es": "Roble", "height_range": [15, 30], "habitat": "monte_bosque"},
            "laranondo": {"scientific": "Populus tremula", "es": "Alamo temblon", "height_range": [15, 25], "habitat": "ribera"},
            "nespera": {"scientific": "Eriobotrya japonica", "es": "Nispero", "height_range": [4, 8], "habitat": "urbano"}
        },
        "real_landmarks": [
            {"name": "Jasokundeko Andre Mariaren eliza", "barrio": "Herriko", "lat": 42.8956769, "lon": -2.1681735, "type": "church"},
            {"name": "Gure Etxea", "barrio": "Herriko", "lat": 42.8950901, "lon": -2.1679397, "type": "civic_center"},
            {"name": "Foruen Plaza", "barrio": "Herriko", "lat": 42.8954211, "lon": -2.1678526, "type": "square"},
            {"name": "Fronton Burunda", "barrio": "Herriko", "lat": 42.8947099, "lon": -2.1700584, "type": "fronton"},
            {"name": "Bihotz santuaren ikastetxea", "barrio": "Zelai", "lat": 42.8932059, "lon": -2.1795557, "type": "school"},
            {"name": "Zelandi Ikastetxe Publikoa", "barrio": "Zelai", "lat": 42.8977201, "lon": -2.1731706, "type": "school"},
            {"name": "Altsasuko Korazonistak", "barrio": "Zelai", "lat": 42.8932704, "lon": -2.1791540, "type": "school"},
            {"name": "Altsasu BHI", "barrio": "SanPedro", "lat": 42.9050762, "lon": -2.1669784, "type": "school"},
        ]
    }

    # Scan imported assets
    mesh_extensions = ["fbx", "obj", "usd", "USD"]
    texture_extensions = ["png", "jpg", "tga", "exr"]
    audio_extensions = ["wav", "mp3", "ogg"]

    for category in os.listdir(ASSETS):
        cat_path = os.path.join(ASSETS, category)
        if not os.path.isdir(cat_path):
            continue

        meshes = find_files(cat_path, mesh_extensions)
        textures = find_files(cat_path, texture_extensions)
        audio = find_files(cat_path, audio_extensions)

        if meshes or textures or audio:
            manifest["assets_by_category"][category] = {
                "meshes": meshes,
                "textures": textures,
                "audio": audio,
                "total_files": len(meshes) + len(textures) + len(audio),
            }

    # Scan real data files
    for f in os.listdir(DATOS):
        if f.endswith(".json"):
            fpath = os.path.join(DATOS, f)
            size = os.path.getsize(fpath)
            try:
                with open(fpath, "r", encoding="utf-8") as fh:
                    data = json.load(fh)
                if isinstance(data, list):
                    count = len(data)
                    keys = list(data[0].keys()) if data else []
                elif isinstance(data, dict):
                    count = len(data)
                    keys = list(data.keys())[:10]
                else:
                    count = 0
                    keys = []
            except:
                count = 0
                keys = []

            manifest["real_data_files"][f] = {
                "size_bytes": size,
                "items": count,
                "fields": keys,
            }

    # Save manifest
    out_path = os.path.join(DATOS, "asset_manifest.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"[OK] Asset manifest guardado en: {out_path}")
    print(f"    Datos reales: {len(manifest['real_data_files'])} archivos")
    print(f"    Assets por categoria: {len(manifest['assets_by_category'])}")
    print(f"    Barrios: {len(manifest['barrio_materials'])}")
    print(f"    Especies arboreas: {len(manifest['tree_species_navarra'])}")
    print(f"    Puntos de interes: {len(manifest['real_landmarks'])}")

    # Count total files
    total_meshes = sum(c.get("total_files", 0) for c in manifest["assets_by_category"].values())
    print(f"    Total assets: {total_meshes}")

if __name__ == "__main__":
    main()
