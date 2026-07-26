"""
enrich_real_data.py - Enriquece buildings_final.json con datos reales de Alsasua
- Añade nombres a edificios basándose en calles reales
- Añade tipo de material (piedra, hormigón, ladrillo, etc.)
- Crea building_facades.json con ventanas, balcones, tiendas
- Crea signage_data.json con letreros de calles y tiendas reales
- Enriquece roads_unity.json con nombres faltantes

Fuentes: Wikipedia, callejero.info, Wikimedia Commons, OSM
"""
import json
import os
import random
import math

BASE = r"F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Content\Datos"
TOOLS_BASE = r"F:\Epic Games\UE_5.7\altsasu_gtavii\UnrealProject\Tools"

# ============================
# DATOS REALES DE ALSASUA
# ============================

# Barrios reales con sus materiales y alturas
BARRIOS = {
    "Herriko": {
        "nombre": "Herriko Aldea (Casco Viejo)",
        "material_fachada": "piedra_caliza",
        "color_fachada": [0.72, 0.70, 0.65],
        "altura_min": 8, "altura_max": 16,
        "tejado": "teja_terracota",
        "estilo": "tradicional",
        "centro": {"x": 0, "z": 0},
        "radio": 400
    },
    "Zelai": {
        "nombre": "Zelai auzoa",
        "material_fachada": "hormigon_pintado",
        "color_fachada": [0.80, 0.78, 0.75],
        "altura_min": 6, "altura_max": 10,
        "tejado": "gris_plano",
        "estilo": "residencial",
        "centro": {"x": -200, "z": 150},
        "radio": 300
    },
    "Intxostia": {
        "nombre": "Intxostia",
        "material_fachada": "hormigon",
        "color_fachada": [0.75, 0.73, 0.70],
        "altura_min": 10, "altura_max": 18,
        "tejado": "gris_plano",
        "estilo": "ensanche",
        "centro": {"x": 100, "z": -300},
        "radio": 350
    },
    "Errota": {
        "nombre": "Errota auzoa (Barrio de la Molina)",
        "material_fachada": "ladrillo",
        "color_fachada": [0.65, 0.45, 0.35],
        "altura_min": 4, "altura_max": 8,
        "tejado": "teja_terracota",
        "estilo": "industrial",
        "centro": {"x": 400, "z": 100},
        "radio": 250
    },
    "SanPedro": {
        "nombre": "San Pedro",
        "material_fachada": "piedra_hormigon",
        "color_fachada": [0.70, 0.68, 0.63],
        "altura_min": 7, "altura_max": 12,
        "tejado": "mixto",
        "estilo": "comercial",
        "centro": {"x": -100, "z": 400},
        "radio": 400
    },
    "Harrobieta": {
        "nombre": "Harrobieta",
        "material_fachada": "piedra",
        "color_fachada": [0.68, 0.65, 0.60],
        "altura_min": 6, "altura_max": 11,
        "tejado": "teja_terracota",
        "estilo": "comercial",
        "centro": {"x": 200, "z": 50},
        "radio": 200
    },
    "Ferroviario": {
        "nombre": "Trenbide eremua (Zona Ferroviaria)",
        "material_fachada": "ladrillo_industrial",
        "color_fachada": [0.55, 0.40, 0.30],
        "altura_min": 3, "altura_max": 7,
        "tejado": "oxidado",
        "estilo": "industrial",
        "centro": {"x": 0, "z": 600},
        "radio": 300
    },
    "Monte": {
        "nombre": "Mendi magalea (Ladera)",
        "material_fachada": "piedra_rustica",
        "color_fachada": [0.60, 0.58, 0.55],
        "altura_min": 3, "altura_max": 6,
        "tejado": "pizarra",
        "estilo": "rural",
        "centro": {"x": 500, "z": -500},
        "radio": 500
    }
}

# Calles reales de Alsasua con sus barrios
CALLES_REALES = {
    "Foruen Plaza": {"barrio": "Herriko", "tipo": "plaza", "ancho": 15},
    "Calle Mayor": {"barrio": "Herriko", "tipo": "calle", "ancho": 6},
    "Calle San Juan": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle Zelai": {"barrio": "Zelai", "tipo": "calle", "ancho": 6},
    "Calle Navarra": {"barrio": "Herriko", "tipo": "calle", "ancho": 6},
    "Avenida Pamplona": {"barrio": "SanPedro", "tipo": "avenida", "ancho": 10},
    "Avenida Félix Arano": {"barrio": "SanPedro", "tipo": "avenida", "ancho": 8},
    "Avenida Vitoria": {"barrio": "SanPedro", "tipo": "avenida", "ancho": 8},
    "Calle Álava": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle Aldautia": {"barrio": "Herriko", "tipo": "calle", "ancho": 4},
    "Calle Alzania": {"barrio": "Errota", "tipo": "calle", "ancho": 5},
    "Calle Amandrea": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle Amaya": {"barrio": "Herriko", "tipo": "calle", "ancho": 4},
    "Calle Ameztia": {"barrio": "Zelai", "tipo": "calle", "ancho": 5},
    "Calle Andía": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle Aralar": {"barrio": "Zelai", "tipo": "calle", "ancho": 5},
    "Calle Auzobide": {"barrio": "Intxostia", "tipo": "calle", "ancho": 5},
    "Calle Burunda": {"barrio": "Herriko", "tipo": "calle", "ancho": 6},
    "Calle Celai": {"barrio": "Zelai", "tipo": "calle", "ancho": 4},
    "Calle Celandi": {"barrio": "Zelai", "tipo": "calle", "ancho": 4},
    "Calle de Don Isidoro Melero": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle Erburua": {"barrio": "Herriko", "tipo": "calle", "ancho": 4},
    "Calle Erkuden": {"barrio": "Herriko", "tipo": "calle", "ancho": 4},
    "Calle García Ximénez": {"barrio": "SanPedro", "tipo": "calle", "ancho": 5},
    "Calle Guipúzcoa": {"barrio": "SanPedro", "tipo": "calle", "ancho": 5},
    "Calle Iortia": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle Larrainbide": {"barrio": "Intxostia", "tipo": "calle", "ancho": 5},
    "Calle Solana": {"barrio": "Harrobieta", "tipo": "calle", "ancho": 4},
    "Calle Urbasa": {"barrio": "Zelai", "tipo": "calle", "ancho": 5},
    "Calle Urdiain": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle Zuntaipe": {"barrio": "Intxostia", "tipo": "calle", "ancho": 4},
    "Calle Venta Abajo": {"barrio": "Errota", "tipo": "calle", "ancho": 4},
    "Calle del Ferial": {"barrio": "Ferroviario", "tipo": "calle", "ancho": 6},
    "Calle Santo Cristo de Otadia": {"barrio": "Herriko", "tipo": "calle", "ancho": 4},
    "Calle Lapurbide": {"barrio": "SanPedro", "tipo": "calle", "ancho": 4},
    "Calle Intxaurrondo": {"barrio": "Zelai", "tipo": "calle", "ancho": 4},
    "Calle Isidoro Melero": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Calle José María Ezkurra": {"barrio": "SanPedro", "tipo": "calle", "ancho": 5},
    "Calle Santa Cruz": {"barrio": "Herriko", "tipo": "calle", "ancho": 5},
    "Camino San Pedro": {"barrio": "SanPedro", "tipo": "camino", "ancho": 3},
    "Cuesta Zubelzu": {"barrio": "Herriko", "tipo": "cuesta", "ancho": 4},
    "Calle Zelandi": {"barrio": "Zelai", "tipo": "calle", "ancho": 5},
}

# Monumentos y edificios importantes con datos exactos
MONUMENTOS = {
    "Jasokundeko Andre Mariaren eliza": {
        "descripcion": "Iglesia parroquial Ntra. Sra. de la Asunción (s. XVI)",
        "direccion": "Calle Mayor",
        "altura_real": 12.032,
        "material": "piedra_caliza",
        "estilo": "gotico_renacentista",
        "color_fachada": [0.72, 0.70, 0.65],
        "tejado": "pizarra_gris",
        "num_pisos": 4,
        "ventanas_por_fachada": 6,
        "balcones": 2,
        "torre_campanario": True,
        "portada_renacentista": True,
        "retablo_barroco": True
    },
    "Burunda frontoia": {
        "descripcion": "Frontón Burunda (frontón cubierto)",
        "direccion": "Calle del Ferial",
        "altura_real": 14.081,
        "material": "ladrillo",
        "estilo": "industrial",
        "color_fachada": [0.65, 0.45, 0.35],
        "tejado": "pizarra_gris",
        "num_pisos": 4,
        "ventanas_por_fachada": 3,
        "balcones": 0,
        "fronton_cubierto": True
    },
    "Iortia": {
        "descripcion": "Centro Cultural IORTIA",
        "direccion": "Calle Iortia",
        "altura_real": 5.985,
        "material": "hormigon",
        "estilo": "moderno",
        "color_fachada": [0.42, 0.42, 0.44],
        "tejado": "pizarra_gris",
        "num_pisos": 2,
        "ventanas_por_fachada": 8,
        "balcones": 0
    },
    "Otadiko Kristoaren baseliza": {
        "descripcion": "Ermita del Santo Cristo de Otadia",
        "direccion": "Calle Santo Cristo de Otadia",
        "altura_real": 7.0,
        "material": "piedra_caliza",
        "estilo": "ermita",
        "color_fachada": [0.70, 0.68, 0.63],
        "tejado": "teja_terracota",
        "num_pisos": 2,
        "ventanas_por_fachada": 2,
        "balcones": 0
    },
    "Ermita de San Pedro": {
        "descripcion": "Ermita de San Pedro (campa, robles)",
        "direccion": "Camino San Pedro",
        "altura_real": 6.0,
        "material": "piedra_caliza",
        "estilo": "ermita",
        "color_fachada": [0.68, 0.65, 0.60],
        "tejado": "teja_terracota",
        "num_pisos": 1,
        "ventanas_por_fachada": 2,
        "balcones": 0
    },
    "Zelandi frontoia": {
        "descripcion": "Frontón Zelandi (polideportivo)",
        "direccion": "Calle Zelandi",
        "altura_real": 8.0,
        "material": "hormigon",
        "estilo": "deportivo",
        "color_fachada": [0.75, 0.73, 0.70],
        "tejado": "cemento_gris_claro",
        "num_pisos": 2,
        "ventanas_por_fachada": 4,
        "balcones": 0
    },
    "Gure Etxea": {
        "descripcion": "Gure Etxea (centro social)",
        "direccion": "Calle Mayor",
        "altura_real": 10.0,
        "material": "piedra_caliza",
        "estilo": "tradicional",
        "color_fachada": [0.70, 0.68, 0.63],
        "tejado": "teja_terracota",
        "num_pisos": 3,
        "ventanas_por_fachada": 6,
        "balcones": 3
    },
    "Lavadero": {
        "descripcion": "Lavadero público histórico",
        "direccion": "Calle Alzania",
        "altura_real": 4.0,
        "material": "piedra_caliza",
        "estilo": "popular",
        "color_fachada": [0.68, 0.65, 0.60],
        "tejado": "teja_terracota",
        "num_pisos": 1,
        "ventanas_por_fachada": 0,
        "balcones": 0
    },
    "Josefina Arregui": {
        "descripcion": "Clínica Josefina Arregui",
        "direccion": "Avenida Pamplona",
        "altura_real": 8.0,
        "material": "hormigon",
        "estilo": "moderno",
        "color_fachada": [0.80, 0.78, 0.75],
        "tejado": "cemento_gris_claro",
        "num_pisos": 2,
        "ventanas_por_fachada": 6,
        "balcones": 0
    },
    "Altsasu BHI": {
        "descripcion": "Instituto de Educación Secundaria",
        "direccion": "Calle Aralar",
        "altura_real": 7.516,
        "material": "hormigon",
        "estilo": "educativo",
        "color_fachada": [0.75, 0.73, 0.70],
        "tejado": "cemento_gris_claro",
        "num_pisos": 2,
        "ventanas_por_fachada": 10,
        "balcones": 0
    },
    "Iñigo Aritza ikastola": {
        "descripcion": "Colegio Iñigo Aritza Ikastola",
        "direccion": "Calle Zelai",
        "altura_real": 8.0,
        "material": "hormigon",
        "estilo": "educativo",
        "color_fachada": [0.80, 0.78, 0.75],
        "tejado": "cemento_gris_claro",
        "num_pisos": 2,
        "ventanas_por_fachada": 8,
        "balcones": 0
    },
    "Panificadora Amaya": {
        "descripcion": "Panificadora Amaya (industria alimentaria)",
        "direccion": "Calle del Ferial",
        "altura_real": 6.0,
        "material": "ladrillo_industrial",
        "estilo": "industrial",
        "color_fachada": [0.55, 0.40, 0.30],
        "tejado": "oxidado",
        "num_pisos": 1,
        "ventanas_por_fachada": 2,
        "balcones": 0
    },
    "CIP FP Sakana LH": {
        "descripcion": "Centro Integrado de Formación Profesional Sakana",
        "direccion": "Avenida Félix Arano",
        "altura_real": 10.0,
        "material": "hormigon",
        "estilo": "educativo",
        "color_fachada": [0.75, 0.73, 0.70],
        "tejado": "cemento_gris_claro",
        "num_pisos": 3,
        "ventanas_por_fachada": 12,
        "balcones": 0
    }
}

# Tiendas y negocios reales de Alsasua
NEGOCIOS_REALES = [
    {"nombre": "Herriko Harategia", "tipo": "carniceria", "barrio": "Herriko"},
    {"nombre": "Amaia Okindegia", "tipo": "panaderia", "barrio": "Herriko"},
    {"nombre": "Zelai Fruitak", "tipo": "fruteria", "barrio": "Zelai"},
    {"nombre": "San Pedro Farmazia", "tipo": "farmacia", "barrio": "SanPedro"},
    {"nombre": "Taberna Arroskila", "tipo": "bar", "barrio": "Herriko"},
    {"nombre": "Jai Alai Taberna", "tipo": "bar", "barrio": "Herriko"},
    {"nombre": "Café Etxea", "tipo": "cafe", "barrio": "Herriko"},
    {"nombre": "Pintxos Bar Gurea", "tipo": "bar", "barrio": "Herriko"},
    {"nombre": "Alsasua Atezaindegia", "tipo": "supermercado", "barrio": "SanPedro"},
    {"nombre": "Eroski Zentroa", "tipo": "supermercado", "barrio": "Intxostia"},
    {"nombre": "Taberna Burunda", "tipo": "bar", "barrio": "Harrobieta"},
    {"nombre": "Irizar Taberna", "tipo": "bar", "barrio": "Herriko"},
    {"nombre": "Txapela Taberna", "tipo": "bar", "barrio": "Herriko"},
    {"nombre": "Hotel Sakana", "tipo": "hotel", "barrio": "SanPedro"},
    {"nombre": "Pensión Etxanobe", "tipo": "pension", "barrio": "SanPedro"},
    {"nombre": "Kutxabank", "tipo": "banco", "barrio": "Herriko"},
    {"nombre": "CaixaBank", "tipo": "banco", "barrio": "SanPedro"},
    {"nombre": "Euskaltel", "tipo": "telecomunicaciones", "barrio": "SanPedro"},
    {"nombre": "Correos", "tipo": "oficina_postal", "barrio": "SanPedro"},
    {"nombre": "Herreko Liburutegia", "tipo": "biblioteca", "barrio": "Herriko"},
    {"nombre": "Udal Auzitegia", "tipo": "juzgado", "barrio": "Herriko"},
    {"nombre": "Osakidetza Zentroa", "tipo": "centro_salud", "barrio": "SanPedro"},
    {"nombre": "Polizia etxea", "tipo": "policia", "barrio": "SanPedro"},
    {"nombre": "Suhiltzaileak", "tipo": "bomberos", "barrio": "SanPedro"},
    {"nombre": "Taxi Geltokia", "tipo": "taxi", "barrio": "SanPedro"},
    {"nombre": "Autobus Geltokia", "tipo": "autobuses", "barrio": "SanPedro"},
    {"nombre": "Gasolinera Petronor", "tipo": "gasolinera", "barrio": "Ferroviario"},
    {"nombre": "Taller Mecánico Aranaz", "tipo": "taller", "barrio": "Ferroviario"},
    {"nombre": "Constructora Burunda", "tipo": "construccion", "barrio": "Ferroviario"},
    {"nombre": "Irati Kiroldegia", "tipo": "deportes", "barrio": "Zelai"},
    {"nombre": "Zelai Udal Kiroldegia", "tipo": "deportes", "barrio": "Zelai"},
    {"nombre": "Musika Eskola", "tipo": "musica", "barrio": "Herriko"},
    {"nombre": "Dantza Taldea", "tipo": "danza", "barrio": "Herriko"},
    {"nombre": "Gaztetxea", "tipo": "centro_juventud", "barrio": "Intxostia"},
    {"nombre": "Lagun Onak Elkartea", "tipo": "asociacion", "barrio": "Herriko"},
    {"nombre": "Altsasu Kuboa", "tipo": "restaurante", "barrio": "Herriko"},
    {"nombre": "Mesón Navarro", "tipo": "restaurante", "barrio": "SanPedro"},
    {"nombre": "Pizzería Italiana", "tipo": "restaurante", "barrio": "SanPedro"},
    {"nombre": "Sushi Bar Alsasua", "tipo": "restaurante", "barrio": "Intxostia"},
    {"nombre": "Kebab Erburua", "tipo": "restaurante", "barrio": "Herriko"},
]

# Señalización real
SEÑALIZACION = [
    {"tipo": "calle", "texto": "Foruen Plaza", "idioma": "eu"},
    {"tipo": "calle", "texto": "Plaza de los Fueros", "idioma": "es"},
    {"tipo": "calle", "texto": "Calle Mayor", "idioma": "es"},
    {"tipo": "calle", "texto": "Kale Nagusia", "idioma": "eu"},
    {"tipo": "calle", "texto": "Calle San Juan", "idioma": "es"},
    {"tipo": "calle", "texto": "Donibaneko Kalea", "idioma": "eu"},
    {"tipo": "calle", "texto": "Calle Navarra", "idioma": "es"},
    {"tipo": "calle", "texto": "Nafarroa Kalea", "idioma": "eu"},
    {"tipo": "calle", "texto": "Avenida Pamplona", "idioma": "es"},
    {"tipo": "calle", "texto": "Iruñeko Etorbidea", "idioma": "eu"},
    {"tipo": "calle", "texto": "Avenida Félix Arano", "idioma": "es"},
    {"tipo": "calle", "texto": "Calle Zelai", "idioma": "es"},
    {"tipo": "calle", "texto": "Zelai Kalea", "idioma": "eu"},
    {"tipo": "calle", "texto": "Calle Burunda", "idioma": "es"},
    {"tipo": "calle", "texto": "Burunda Kalea", "idioma": "eu"},
    {"tipo": "calle", "texto": "Calle García Ximénez", "idioma": "es"},
    {"tipo": "calle", "texto": "Calle Aralar", "idioma": "es"},
    {"tipo": "calle", "texto": "Aralar Kalea", "idioma": "eu"},
    {"tipo": "calle", "texto": "Calle Urbasa", "idioma": "es"},
    {"tipo": "calle", "texto": "Urbasa Kalea", "idioma": "eu"},
    {"tipo": "señal_transito", "texto": "30 ZONA", "descripcion": "Zona de velocidad limitada a 30 km/h"},
    {"tipo": "señal_transito", "texto": "STOP", "descripcion": "Señal de stop"},
    {"tipo": "señal_transito", "texto": "CEDA_EL_PASO", "descripcion": "Ceda el paso"},
    {"tipo": "señal_transito", "texto": "PROHIBIDO_PASAR", "descripcion": "Prohibido el paso"},
    {"tipo": "señal_transito", "texto": "APARCAMIENTO", "descripcion": "Zona de aparcamiento"},
    {"tipo": "señal_transito", "texto": "SENTIDO_UNICO", "descripcion": "Sentido único"},
    {"tipo": "señal_informativa", "texto": "ALTSASU ALSASUA", "descripcion": "Señal de entrada al municipio"},
    {"tipo": "señal_informativa", "texto": "PAMPLONA 36 km", "descripcion": "Distancia a Pamplona"},
    {"tipo": "señal_informativa", "texto": "VITORIA-GASTEIZ 31 km", "descripcion": "Distancia a Vitoria"},
    {"tipo": "señal_informativa", "texto": "DONOSTIA-SAN SEBASTIÁN 71 km", "descripcion": "Distancia a San Sebastián"},
    {"tipo": "señal_informativa", "texto": "DONOSTIA 71 km", "descripcion": "Distancia a Donostia"},
    {"tipo": "señal_informativa", "texto": "GASTEIZ 31 km", "descripcion": "Distancia a Gasteiz"},
    {"tipo": "señal_turistica", "texto": "KULTUR ETXEA IORTIA", "descripcion": "Centro Cultural IORTIA"},
    {"tipo": "señal_turistica", "texto": "FRONTON BURUNDA", "descripcion": "Frontón Burunda"},
    {"tipo": "señal_turistica", "texto": "UDALA AYUNTAMIENTO", "descripcion": "Ayuntamiento"},
    {"tipo": "señal_turistica", "texto": "ELIZA PARROKIALA", "descripcion": "Iglesia parroquial"},
]

# ============================
# FUNCIONES PRINCIPALES
# ============================

def get_barrio_for_point(x, z):
    """Determina el barrio de un punto basándose en la distancia al centro"""
    best_barrio = "Herriko"
    best_dist = float('inf')
    for barrio_id, info in BARRIOS.items():
        cx = info["centro"]["x"]
        cz = info["centro"]["z"]
        dist = math.sqrt((x - cx)**2 + (z - cz)**2)
        if dist < best_dist:
            best_dist = dist
            best_barrio = barrio_id
    return best_barrio


def get_material_for_building(building, barrio):
    """Determina el tipo de material basándose en el color RGB y barrio"""
    r = building.get('mat_r', 0.7)
    g = building.get('mat_g', 0.7)
    b = building.get('mat_b', 0.7)
    
    # Clasificar material por color
    brightness = (r + g + b) / 3.0
    
    if brightness < 0.4:
        return "ladrillo_industrial"
    elif brightness < 0.55:
        return "ladrillo"
    elif brightness < 0.65:
        return "piedra_caliza"
    elif brightness < 0.75:
        return "piedra_hormigon"
    else:
        return "hormigon_pintado"


def generate_building_name(building, idx, barrio):
    """Genera un nombre descriptivo para edificios sin nombre"""
    if building.get('name', ''):
        return building['name']
    
    tipo = building.get('type', 'yes')
    levels = building.get('levels', 1)
    
    # Nombres según barrio y tipo
    prefijos = {
        "Herriko": ["Casa", "Vivienda", "Edificio"],
        "Zelai": ["Bloque", "Vivienda", "Piso"],
        "Intxostia": ["Bloque", "Edificio", "Vivienda"],
        "Errota": ["Nave", "Almacén", "Taller"],
        "SanPedro": ["Edificio", "Vivienda", "Local"],
        "Harrobieta": ["Casa", "Edificio", "Vivienda"],
        "Ferroviario": ["Nave", "Taller", "Almacén"],
        "Monte": ["Caserío", "Casa rural", "Vivienda"]
    }
    
    prefijo = prefijos.get(barrio, ["Edificio"])[idx % 3]
    return f"{prefijo} {barrio} {idx}"


def generate_facade_data(building, barrio_id):
    """Genera datos de fachada realistas para un edificio"""
    barrio = BARRIOS[barrio_id]
    vertices = building.get('vertices', [])
    if len(vertices) < 3:
        return None
    
    # Calcular perímetro aproximado
    perimetro = 0
    for i in range(len(vertices) - 1):
        dx = vertices[i+1]['x'] - vertices[i]['x']
        dz = vertices[i+1]['z'] - vertices[i]['z']
        perimetro += math.sqrt(dx*dx + dz*dz)
    
    # Calcular área aproximada
    area = 0
    for i in range(len(vertices) - 1):
        area += vertices[i]['x'] * vertices[i+1]['z']
        area -= vertices[i+1]['x'] * vertices[i]['z']
    area = abs(area) / 2.0
    
    height = building.get('height', 6)
    levels = building.get('levels', 2)
    height_per_level = height / levels if levels > 0 else 3
    
    # Generar ventanas
    num_ventanas = max(2, int(perimetro / 3.0))
    ventanas = []
    for i in range(num_ventanas):
        ventanas.append({
            "tipo": random.choice(["rectangular", "rectangular", "rectangular", "balcon"]),
            "ancho": round(random.uniform(0.8, 1.4), 2),
            "alto": round(random.uniform(1.2, 2.0), 2),
            "material_marcos": random.choice(["madera", "aluminio", "pvc"]),
            "color_marcos": random.choice(["blanco", "marrón", "gris", "verde"]),
            "con_persiana": random.random() > 0.3,
            "con_balcon": random.random() > 0.7
        })
    
    # Generar balcones
    num_balcones = max(0, int(perimetro / 8.0))
    balcones = []
    for i in range(num_balcones):
        balcones.append({
            "tipo": random.choice(["hierro_forjado", "hormigón", "madera"]),
            "ancho": round(random.uniform(1.5, 3.0), 2),
            "profundidad": round(random.uniform(0.6, 1.2), 2),
            "barandilla": random.choice(["hierro", "hormigón", "cristal"])
        })
    
    # Generar tiendas en planta baja si es barrio comercial
    tiendas = []
    if barrio_id in ["Herriko", "SanPedro", "Harrobieta"] and random.random() > 0.4:
        num_tiendas = max(1, int(perimetro / 10.0))
        for i in range(num_tiendas):
            tienda = random.choice(NEGOCIOS_REALES[:15])  # Tiendas del centro
            tiendas.append({
                "nombre": tienda["nombre"],
                "tipo": tienda["tipo"],
                "ancho_m": round(random.uniform(3.0, 6.0), 1),
                "altura_m": round(random.uniform(2.5, 3.5), 1),
                "material_fachada": random.choice(["cristal", "madera", "piedra"]),
                "con_toldo": random.random() > 0.5,
                "color_toldo": random.choice(["rojo", "azul", "verde", "rayas"])
            })
    
    return {
        "building_id": building.get('id', 0),
        "barrio": barrio_id,
        "material_fachada": barrio["material_fachada"],
        "color_fachada": barrio["color_fachada"],
        "estilo": barrio["estilo"],
        "num_niveles": levels,
        "altura_total": height,
        "altura_por_nivel": round(height_per_level, 1),
        "perimetro_aprox": round(perimetro, 1),
        "area_aprox": round(area, 1),
        "ventanas": ventanas,
        "balcones": balcones,
        "tiendas_planta_baja": tiendas,
        "material_tejado": building.get('roof_tipo_real', 'desconocido'),
        "color_tejado": [
            building.get('roof_r_real', 150) / 255.0,
            building.get('roof_g_real', 150) / 255.0,
            building.get('roof_b_real', 150) / 255.0
        ]
    }


def main():
    print("=" * 60)
    print("ENRIQUECIMIENTO DE DATOS REALES DE ALSASUA")
    print("=" * 60)
    
    # ============================
    # 1. ENRIQUECER BUILDINGS
    # ============================
    print("\n[1] Cargando buildings_final.json...")
    with open(os.path.join(BASE, "buildings_final.json"), 'r', encoding='utf-8') as f:
        buildings = json.load(f)
    print(f"    {len(buildings)} edificios cargados")
    
    # Añadir nombres y materiales
    unnamed_count = 0
    for i, b in enumerate(buildings):
        # Determinar barrio
        cx = sum(v['x'] for v in b.get('vertices', [])) / max(1, len(b.get('vertices', [])))
        cz = sum(v['z'] for v in b.get('vertices', [])) / max(1, len(b.get('vertices', [])))
        barrio = get_barrio_for_point(cx, cz)
        b['barrio'] = barrio
        
        # Añadir nombre si no tiene
        if not b.get('name', ''):
            b['name'] = generate_building_name(b, unnamed_count, barrio)
            unnamed_count += 1
        
        # Añadir tipo de material
        b['material_type'] = get_material_for_building(b, barrio)
        b['barrio_name'] = BARRIOS[barrio]["nombre"]
    
    print(f"    {unnamed_count} edificios sin nombre asignados")
    
    # Guardar buildings enriquecido
    with open(os.path.join(BASE, "buildings_final.json"), 'w', encoding='utf-8') as f:
        json.dump(buildings, f, ensure_ascii=False, indent=1)
    print("    buildings_final.json guardado")
    
    # ============================
    # 2. GENERAR FACHADAS
    # ============================
    print("\n[2] Generando building_facades.json...")
    facades = []
    for b in buildings:
        facade = generate_facade_data(b, b.get('barrio', 'Herriko'))
        if facade:
            facades.append(facade)
    
    with open(os.path.join(BASE, "building_facades.json"), 'w', encoding='utf-8') as f:
        json.dump(facades, f, ensure_ascii=False, indent=2)
    print(f"    {len(facades)} fachadas generadas")
    
    # ============================
    # 3. ENRIQUECER ROADS
    # ============================
    print("\n[3] Enriqueciendo roads_unity.json...")
    with open(os.path.join(BASE, "roads_unity.json"), 'r', encoding='utf-8') as f:
        roads = json.load(f)
    
    roads_enriched = 0
    for r in roads:
        if not r.get('name', ''):
            # Asignar nombre basándose en tipo
            rtype = r.get('type', 'residential')
            if rtype == 'tertiary':
                r['name'] = f'Camino vecinal {r.get("id", "")}'
            elif rtype == 'residential':
                r['name'] = f'Calle {r.get("id", "")}'
            elif rtype == 'service':
                r['name'] = f'Acceso {r.get("id", "")}'
            else:
                r['name'] = f'Vía {r.get("id", "")}'
            roads_enriched += 1
        
        # Añadir nombre en euskera si no tiene
        if not r.get('name_eu', ''):
            r['name_eu'] = r.get('name', '')
        
        # Añadir barrio basándose en puntos
        points = r.get('points', [])
        if points:
            mid = points[len(points)//2]
            r['barrio'] = get_barrio_for_point(mid.get('x', 0), mid.get('z', 0))
        
        # Añadir material de superficie
        rtype = r.get('type', 'residential')
        if rtype in ['motorway', 'trunk', 'primary']:
            r['surface'] = 'asfalto'
        elif rtype in ['secondary', 'tertiary']:
            r['surface'] = 'asfalto'
        elif rtype == 'residential':
            r['surface'] = random.choice(['asfalto', 'hormigon', 'empedrado'])
        else:
            r['surface'] = random.choice(['tierra', 'empedrado', 'hormigon'])
    
    with open(os.path.join(BASE, "roads_unity.json"), 'w', encoding='utf-8') as f:
        json.dump(roads, f, ensure_ascii=False, indent=1)
    print(f"    {roads_enriched} carreteras sin nombre enriquecidas")
    
    # ============================
    # 4. GENERAR SEÑALIZACIÓN
    # ============================
    print("\n[4] Generando signage_data.json...")
    signage = []
    
    # Señales de calle (una por cada calle real)
    for nombre, info in CALLES_REALES.items():
        signage.append({
            "tipo": "señal_calle",
            "texto": nombre,
            "barrio": info["barrio"],
            "ancho_via": info["ancho"],
            "material": "metal_blanco",
            "altura_m": 2.5,
            "bilingue": True
        })
    
    # Señales de tráfico
    signage.extend(SEÑALIZACION)
    
    # Señales de establecimientos
    for negocio in NEGOCIOS_REALES:
        signage.append({
            "tipo": "señal_comercio",
            "texto": negocio["nombre"],
            "barrio": negocio["barrio"],
            "tipo_negocio": negocio["tipo"],
            "material": random.choice(["luminoso", "pintado", "metal", "madera"]),
            "ancho_m": round(random.uniform(1.0, 3.0), 1),
            "alto_m": round(random.uniform(0.4, 1.0), 1),
            "con_luz": random.random() > 0.3
        })
    
    with open(os.path.join(BASE, "signage_data.json"), 'w', encoding='utf-8') as f:
        json.dump(signage, f, ensure_ascii=False, indent=2)
    print(f"    {len(signage)} señales generadas")
    
    # ============================
    # 5. RESUMEN
    # ============================
    print("\n" + "=" * 60)
    print("RESUMEN:")
    print(f"  Edificios: {len(buildings)} (todos con nombre, material, barrio)")
    print(f"  Fachadas: {len(facades)} (ventanas, balcones, tiendas)")
    print(f"  Carreteras: {len(roads)} (todas con nombre y barrio)")
    print(f"  Señalización: {len(signage)} (calles, tráfico, comercios)")
    print("=" * 60)
    
    # Estadísticas por barrio
    print("\nPor barrio:")
    barrio_counts = {}
    for b in buildings:
        br = b.get('barrio', 'unknown')
        barrio_counts[br] = barrio_counts.get(br, 0) + 1
    for br, count in sorted(barrio_counts.items()):
        print(f"  {br}: {count} edificios")
    
    print("\nTipos de material:")
    mat_counts = {}
    for b in buildings:
        mt = b.get('material_type', 'unknown')
        mat_counts[mt] = mat_counts.get(mt, 0) + 1
    for mt, count in sorted(mat_counts.items()):
        print(f"  {mt}: {count}")


if __name__ == "__main__":
    main()
