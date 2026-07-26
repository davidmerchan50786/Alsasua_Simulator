"""
Meshy.ai Asset Generation Prompts for Altsasu/Alsasua Project
Run this script to generate all prompts needed for the project.
Copy each prompt into meshy.ai text-to-3d or image-to-3d.

Usage:
    python meshy_prompts.py          # Print all prompts
    python meshy_prompts.py --json   # Export as JSON
    python meshy_prompts.py --api    # Generate via Meshy API (needs API key)
"""

import json
import os
import sys
import time

# ============================================================
# MESHY API CONFIG (set your API key here or via env var)
# ============================================================
MESHY_API_KEY = os.environ.get("MESHY_API_KEY", "")
MESHY_API_URL = "https://api.meshy.ai/openapi/v2"

# ============================================================
# ASSET PROMPTS - Organized by category
# ============================================================

ASSETS = {
    # ============================================================
    # BUILDINGS - Basque Country traditional architecture
    # ============================================================
    "buildings": [
        {
            "name": "Edificio_Herriko_Tradicional",
            "prompt": "Traditional Basque Country stone townhouse, 3 stories, red clay tile roof, wooden balcony with iron railing, white plaster walls with exposed stone corners, aged weathered texture, Navarra Spain architecture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
        {
            "name": "Edificio_Moderno_Intxostia",
            "prompt": "Modern Spanish apartment building, 4 stories, flat roof, concrete and brick facade, large windows with aluminum frames, balconies with glass railing, residential block in Basque Country town, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
        {
            "name": "Casa_Chorro_Errota",
            "prompt": "Small traditional Basque farmhouse, 2 stories, stone walls, wooden beam structure, terracotta tile roof, rural Navarra architecture, weathered wood door, flower boxes on windows, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
        {
            "name": "Nave_Industrial_Ferroviario",
            "prompt": "Industrial warehouse building, corrugated metal siding, large sliding doors, concrete floor, flat roof with skylights, Spanish industrial estate architecture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
        {
            "name": "Iglesia_Jasokundeko",
            "prompt": "Basque Country church, Romanesque-Gothic style, stone walls, bell tower with arched openings, rose window, red tile roof, medieval Navarra architecture, weathered stone texture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
        {
            "name": "Ayuntamiento_Altsasu",
            "prompt": "Small Spanish town hall, 18th century Baroque style, stone facade with arched portico, three arches on ground floor, coat of arms above entrance, red tile roof, formal civic architecture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
        {
            "name": "Estacion_Tren_Altsasu",
            "prompt": "Spanish regional train station, stone and brick building, arched entrance, clock on facade, platform canopy, Renfe style architecture, small town railway station Navarra, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
        {
            "name": "Fronton_Pelota",
            "prompt": "Basque pelota fronton court, tall concrete wall with painted lines, open-air court, wooden gallery seating, traditional Basque sports architecture, stone and concrete construction, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "building"
        },
    ],

    # ============================================================
    # STREET FURNITURE - Farolas, bancos, fuentes
    # ============================================================
    "street_furniture": [
        {
            "name": "Farola_Clasica",
            "prompt": "Traditional Spanish cast iron street lamp, ornate Victorian style, black metal, single glass lantern on curved arm, decorative base with floral details, classic European street lighting, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Farola_Moderna",
            "prompt": "Modern minimalist street lamp, slim black metal pole, LED light fixture, simple geometric design, contemporary urban lighting, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Banco_Parque",
            "prompt": "Traditional park bench, wooden slats on cast iron frame, dark green painted metal, classic European public seating, weathered wood texture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Fuente_Agua",
            "prompt": "Spanish stone water fountain, circular basin on pedestal, carved stone with moss, water spout from wall, traditional Navarra village fountain, weathered limestone texture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Papelera_Urbana",
            "prompt": "Urban waste bin, dark green metal cylinder, domed top with opening, public trash can, European street furniture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Buzon_Correos",
            "prompt": "Spanish red mailbox, cylindrical metal post box, Correos brand style, red painted metal with yellow details, traditional Spanish postal service, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Parada_Autobus",
            "prompt": "Bus stop shelter, glass and metal frame, advertising panel, bench seating, modern urban transit stop, European bus shelter design, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Fuente_Bebida",
            "prompt": "Public drinking fountain, stainless steel pedestal, push-button water dispenser, modern urban water fountain, park drinking fountain, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # TRAFFIC ELEMENTS
    # ============================================================
    "traffic": [
        {
            "name": "Semaforo_Urbano",
            "prompt": "Traffic light, black metal pole, three LED lights red yellow green, pedestrian signal, standard European traffic signal, urban intersection, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Senal_Calle",
            "prompt": "Blue rectangular street sign, white text, mounted on metal pole, European bilingual street sign, Navarra Spain style, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Stop_Senal",
            "prompt": "Octagonal red STOP sign on metal pole, standard traffic stop sign, reflective surface, European road sign, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Barandilla_Puente",
            "prompt": "Metal bridge guardrail, galvanized steel railing, vertical bars with horizontal top rail, safety barrier for bridge, industrial metal construction, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Contenedor_Reciclaje",
            "prompt": "Colorful recycling waste container, three-compartment bin, blue yellow green sections, large wheeled dumpster, municipal recycling station, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # VEHICLES
    # ============================================================
    "vehicles": [
        {
            "name": "Coche_Sedan",
            "prompt": "Generic European sedan car, compact size, modern design, four doors, silver color, parked car, low detail distant vehicle, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vehicle"
        },
        {
            "name": "Furgoneta_Secundaria",
            "prompt": "White delivery van, compact commercial vehicle, box shape, European style utility van, parked vehicle, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vehicle"
        },
        {
            "name": "Autobus_Urbano",
            "prompt": "Spanish city bus, blue and white livery, articulated single-deck, modern urban transit bus, regional Navarra bus, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vehicle"
        },
        {
            "name": "Coche_Policia",
            "prompt": "Spanish police car, white with blue stripe, Ertzaintza Basque police vehicle, emergency lights on roof, patrol car, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vehicle"
        },
    ],

    # ============================================================
    # VEGETATION - Basque Country native species
    # ============================================================
    "vegetation": [
        {
            "name": "Arbol_Haya",
            "prompt": "European beech tree, tall deciduous, smooth gray bark, green summer foliage, broad canopy, mature specimen, Basque Country forest tree, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vegetation"
        },
        {
            "name": "Arbol_Roble",
            "prompt": "English oak tree, massive deciduous, rough dark bark, spreading canopy, lobed green leaves, ancient old-growth, Basque Country woodland, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vegetation"
        },
        {
            "name": "Arbol_Abedul",
            "prompt": "Silver birch tree, slender deciduous, white peeling bark, delicate green leaves, graceful form, common in Navarra, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vegetation"
        },
        {
            "name": "Seto_Verde",
            "prompt": "Green hedge, dense trimmed bushes, dark green leaves, rectangular shaped, garden boundary hedge, well-maintained, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vegetation"
        },
        {
            "name": "Hierba_Larga",
            "prompt": "Tall wild grass, green meadow grass, swaying blades, natural ground cover, Basque Country field vegetation, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "vegetation"
        },
    ],

    # ============================================================
    # NATURE - Rocks, river elements
    # ============================================================
    "nature": [
        {
            "name": "Roca_Grande",
            "prompt": "Large natural boulder, gray limestone, rough surface with moss patches, Basque Country mountain rock, weathered stone, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Piedra_Muro",
            "prompt": "Stacked stone wall segment, dry stone construction, irregular shapes, traditional Basque Country stone wall, moss covered, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Tronco_Caido",
            "prompt": "Fallen tree log, decaying bark, moss growing on surface, woodland floor debris, old growth forest fallen trunk, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # SHOP FRONTS - Real businesses of Altsasu
    # ============================================================
    "shops": [
        {
            "name": "Taberna_Tradicional",
            "prompt": "Traditional Basque tavern facade, wooden door with glass panels, red and white striped awning, rustic stone surround, warm interior glow through windows, old-world charm, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Okindegia_Front",
            "prompt": "Basque bakery storefront, large display window, wooden door, bread and pastry display, warm golden lighting inside, traditional village bakery, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Harategia_Front",
            "prompt": "Butcher shop facade, white tile walls visible through window, glass display case, red and white checkered awning, traditional Spanish butcher, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # INTERIORS - Window/balcony details
    # ============================================================
    "interiors": [
        {
            "name": "Ventana_Basca",
            "prompt": "Traditional Basque window frame, white wooden shutters, small flower box with red geraniums, glass panes reflecting light, weathered paint, Mediterranean style window, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Balcon_Hierro",
            "prompt": "Wrought iron balcony, ornate railing with scrollwork, attached to stone building wall, small potted plants on floor, traditional Spanish ironwork, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Persiana_Española",
            "prompt": "Spanish rolling shutter, green painted aluminum slats, half-open position, mounted on building facade, traditional Spanish window covering, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # ROOFTOP DETAILS
    # ============================================================
    "rooftop": [
        {
            "name": "Antena_TV",
            "prompt": "Television antenna, metal Yagi antenna on rooftop mount, old-style TV aerial, attached to chimney, residential building antenna, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Chimenea_Piedra",
            "prompt": "Stone chimney stack, traditional Basque roof chimney, rough stone construction, clay flue liner on top, terracotta chimney pot, weathered mortar, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Deposito_Agua",
            "prompt": "Black plastic water storage tank, cylindrical rooftop water deposit, Iberian style water tank, mounted on concrete base, residential water supply, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Placa_Solar",
            "prompt": "Solar panel array, photovoltaic panels on tilted metal frame, residential rooftop solar installation, blue-black silicon cells, aluminum frame, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # INTERIOR FURNITURE - Basque village interiors
    # ============================================================
    "interior_furniture": [
        {
            "name": "Mesa_Taberna",
            "prompt": "Rustic Basque tavern wooden table, thick oak tabletop, sturdy turned legs, beer stains, warm wood finish, traditional Spanish bar furniture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Silla_Taberna",
            "prompt": "Traditional Basque tavern chair, dark wood frame, woven rush seat, slightly worn, rustic Spanish bar chair, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Barra_Taberna",
            "prompt": "Basque tavern bar counter, dark polished wood, brass foot rail, bottle shelves behind, traditional Spanish bar, copper taps, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Estanteria_Libros",
            "prompt": "Wooden bookshelf, old library shelving, filled with books, dark oak wood, traditional Spanish public library furniture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Cama_Simple",
            "prompt": "Simple single bed, iron frame, white cotton sheets, thin mattress, basic Spanish village bedroom furniture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Escritorio_Oficina",
            "prompt": "Office desk, simple wooden desk with drawers, paper stacks, pen holder, Spanish municipal office furniture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # SHOP INTERIORS
    # ============================================================
    "shop_interiors": [
        {
            "name": "Estante_Supermercado",
            "prompt": "Supermarket shelving unit, metal frame with adjustable shelves, product display rack, fluorescent lit, Spanish grocery store interior, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Caja_Cobro",
            "prompt": "Checkout counter, supermarket cash register, conveyor belt, plastic bags dispenser, Spanish retail checkout, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Nevera_Cerveza",
            "prompt": "Glass door beverage refrigerator, commercial beer fridge, illuminated interior, bottles visible, Spanish bar cooler, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Horno_Panaderia",
            "prompt": "Commercial bakery oven, large stainless steel bread oven, industrial baking equipment, Spanish panaderia interior, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Mostrador_Carniceria",
            "prompt": "Butcher shop display counter, glass refrigerated display case, meat hooks above, white tile backsplash, Spanish carniceria, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],

    # ============================================================
    # VILLAGE PROPS
    # ============================================================
    "village_props": [
        {
            "name": "Lavadero_Callejero",
            "prompt": "Traditional Basque public washhouse, stone basin with water taps, covered laundry area, arched stone structure, rural Navarra architecture, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Fuente_Vieja",
            "prompt": "Old stone drinking fountain, carved stone basin, iron spout, moss-covered stone base, traditional Spanish village fountain, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Cruce_Camino",
            "prompt": "Stone cross at crossroads, traditional Basque roadside cross, carved stone pillar with crucifix, mossy base, rural Navarra, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Hito_Fronterizo",
            "prompt": "Stone boundary marker, carved milestone, traditional Basque border stone, moss-covered, engraved text, rural pathway marker, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Bancal_Terraza",
            "prompt": "Terraced garden wall, dry stone retaining wall, small agricultural terrace, traditional Basque hillside farming, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
        {
            "name": "Portal_Madera",
            "prompt": "Traditional Basque wooden door portal, carved wooden lintel, stone doorframe, aged oak wood door, traditional Navarra entrance, game-ready low-poly, PBR materials",
            "style": "realistic",
            "category": "prop"
        },
    ],
}


def print_prompts():
    """Print all prompts organized by category."""
    total = 0
    for category, items in ASSETS.items():
        print(f"\n{'='*60}")
        print(f"  {category.upper().replace('_', ' ')}")
        print(f"{'='*60}")
        for item in items:
            total += 1
            print(f"\n  [{total}] {item['name']}")
            print(f"  Style: {item['style']}")
            print(f"  Prompt: {item['prompt']}")
    print(f"\n{'='*60}")
    print(f"  TOTAL: {total} assets to generate")
    print(f"{'='*60}")


def export_json():
    """Export all prompts as JSON for batch processing."""
    output = []
    idx = 0
    for category, items in ASSETS.items():
        for item in items:
            idx += 1
            output.append({
                "id": idx,
                "name": item["name"],
                "category": category,
                "prompt": item["prompt"],
                "style": item["style"],
                "model_type": "standard",
                "topology": "quad",
                "target_polycount": 10000,
            })

    out_path = os.path.join(os.path.dirname(__file__), "..", "Content", "Datos", "meshy_prompts.json")
    os.makedirs(os.path.dirname(out_path), exist_ok=True)
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(output, f, ensure_ascii=False, indent=2)
    print(f"Exported {len(output)} prompts to {out_path}")
    return output


def wait_for_task(task_id, headers, max_wait=600):
    """Poll task status until completed or failed."""
    for _ in range(max_wait // 10):
        try:
            resp = requests.get(f"{MESHY_API_URL}/text-to-3d/{task_id}", headers=headers, timeout=30)
            if resp.status_code == 200:
                data = resp.json()
                status = data.get("status", "UNKNOWN")
                progress = data.get("progress", 0)
                if status == "SUCCEEDED":
                    return data
                elif status in ("FAILED", "EXPIRED"):
                    return data
                print(f"    Status: {status} ({progress}%)", end="\r")
        except Exception as e:
            print(f"    Poll error: {e}")
        time.sleep(10)
    return None


def generate_via_api():
    """Generate assets via Meshy API v2 (preview -> refine workflow)."""
    if not MESHY_API_KEY:
        print("ERROR: Set MESHY_API_KEY environment variable or edit this script.")
        return

    try:
        import requests
    except ImportError:
        print("Install requests: pip install requests")
        return

    headers = {
        "Authorization": f"Bearer {MESHY_API_KEY}",
        "Content-Type": "application/json"
    }

    # Check balance first
    try:
        resp = requests.get(f"{MESHY_API_URL}/balance", headers=headers, timeout=15)
        if resp.status_code == 200:
            bal = resp.json()
            print(f"Balance: {bal.get('total_credits', '?')} credits")
    except:
        pass

    download_dir = os.path.join(os.path.dirname(__file__), "..", "Content", "AssetsImportados", "MeshyAI")
    os.makedirs(download_dir, exist_ok=True)

    results = []
    idx = 0
    for category, items in ASSETS.items():
        for item in items:
            idx += 1
            name = item["name"]
            print(f"\n[{idx}/{len(ASSETS.get(category, []))}] {name}...")

            # Step 1: Preview
            preview_payload = {
                "mode": "preview",
                "prompt": item["prompt"],
                "negative_prompt": "low quality, blurry, distorted, deformed",
                "ai_model": "latest",
                "should_remesh": True,
                "target_polycount": 10000,
                "target_formats": ["glb", "fbx"],
            }

            try:
                resp = requests.post(f"{MESHY_API_URL}/text-to-3d",
                    headers=headers, json=preview_payload, timeout=30)
                if resp.status_code not in (200, 201, 202):
                    print(f"  Preview error {resp.status_code}: {resp.text[:200]}")
                    results.append({"name": name, "status": "preview_error", "code": resp.status_code})
                    continue
                preview_id = resp.json().get("result")
                print(f"  Preview task: {preview_id}")
            except Exception as e:
                print(f"  Preview exception: {e}")
                results.append({"name": name, "status": "exception", "error": str(e)})
                continue

            # Wait for preview
            preview_result = wait_for_task(preview_id, headers)
            if not preview_result or preview_result.get("status") != "SUCCEEDED":
                err = preview_result.get("task_error", {}).get("message", "timeout") if preview_result else "timeout"
                print(f"  Preview failed: {err}")
                results.append({"name": name, "status": "preview_failed", "error": err})
                continue

            print(f"  Preview done. Credits: {preview_result.get('consumed_credits', '?')}")

            # Step 2: Refine (add texture)
            refine_payload = {
                "mode": "refine",
                "preview_task_id": preview_id,
                "enable_pbr": True,
                "target_formats": ["glb", "fbx"],
            }

            try:
                resp = requests.post(f"{MESHY_API_URL}/text-to-3d",
                    headers=headers, json=refine_payload, timeout=30)
                if resp.status_code not in (200, 201, 202):
                    print(f"  Refine error {resp.status_code}: {resp.text[:200]}")
                    # Still save preview
                    results.append({"name": name, "status": "preview_only", "preview_id": preview_id})
                    continue
                refine_id = resp.json().get("result")
                print(f"  Refine task: {refine_id}")
            except Exception as e:
                print(f"  Refine exception: {e}")
                results.append({"name": name, "status": "preview_only", "preview_id": preview_id})
                continue

            # Wait for refine
            refine_result = wait_for_task(refine_id, headers)
            if not refine_result or refine_result.get("status") != "SUCCEEDED":
                err = refine_result.get("task_error", {}).get("message", "timeout") if refine_result else "timeout"
                print(f"  Refine failed: {err}")
                results.append({"name": name, "status": "refine_failed", "preview_id": preview_id})
                continue

            # Download models
            model_urls = refine_result.get("model_urls", {})
            downloaded = []
            for fmt, url in model_urls.items():
                if url:
                    fpath = os.path.join(download_dir, f"{name}.{fmt}")
                    try:
                        r = requests.get(url, timeout=120)
                        with open(fpath, "wb") as f:
                            f.write(r.content)
                        downloaded.append(fmt)
                        print(f"  Downloaded: {name}.{fmt} ({len(r.content)//1024}KB)")
                    except Exception as e:
                        print(f"  Download error ({fmt}): {e}")

            results.append({
                "name": name,
                "status": "success",
                "refine_id": refine_id,
                "downloaded": downloaded,
                "credits": refine_result.get("consumed_credits", 0),
            })
            time.sleep(2)

    out_path = os.path.join(os.path.dirname(__file__), "..", "Content", "Datos", "meshy_results.json")
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(results, f, ensure_ascii=False, indent=2)

    success = sum(1 for r in results if r["status"] == "success")
    print(f"\nDone: {success}/{len(results)} assets generated")
    print(f"Results saved to {out_path}")


if __name__ == "__main__":
    if "--json" in sys.argv:
        export_json()
    elif "--api" in sys.argv:
        generate_via_api()
    else:
        print_prompts()
