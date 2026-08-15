"""
Herramienta para generar LUTs (Look-Up Tables) que hacen color matching
entre capturas in-game y la ortofoto real de Alsasua.

Objetivo: Conseguir que los colores del juego coincidan exactamente con
los colores de la realidad (ortofoto PNOA).
"""

import numpy as np
from PIL import Image
import json
import os

def load_image(path):
    img = Image.open(path).convert('RGB')
    return np.array(img, dtype=np.float32) / 255.0

def compute_histogram_stats(img):
    r_hist, _ = np.histogram(img[:,:,0], bins=256, range=(0,1))
    g_hist, _ = np.histogram(img[:,:,1], bins=256, range=(0,1))
    b_hist, _ = np.histogram(img[:,:,2], bins=256, range=(0,1))
    
    stats = {
        'r_mean': float(np.mean(img[:,:,0])),
        'g_mean': float(np.mean(img[:,:,1])),
        'b_mean': float(np.mean(img[:,:,2])),
        'r_std': float(np.std(img[:,:,0])),
        'g_std': float(np.std(img[:,:,1])),
        'b_std': float(np.std(img[:,:,2]))
    }
    
    return stats, (r_hist, g_hist, b_hist)

def histogram_match_channel(source_channel, target_hist):
    source_values, bin_idx, source_counts = np.unique(
        (source_channel * 255).astype(np.uint8),
        return_inverse=True,
        return_counts=True
    )
    
    source_quantiles = np.cumsum(source_counts).astype(np.float64)
    source_quantiles /= source_quantiles[-1]
    
    target_quantiles = np.cumsum(target_hist).astype(np.float64)
    target_quantiles /= target_quantiles[-1]
    
    interp_values = np.interp(source_quantiles, target_quantiles, np.arange(256))
    
    return interp_values[bin_idx].reshape(source_channel.shape) / 255.0

def generate_matched_lut(game_screenshot_path, ortophoto_path, output_path, barrio_name="Default"):
    print(f"Generando LUT para {barrio_name}...")
    print(f"  Screenshot: {game_screenshot_path}")
    print(f"  Ortofoto: {ortophoto_path}")
    
    game_img = load_image(game_screenshot_path)
    ortho_img = load_image(ortophoto_path)
    
    game_stats, _ = compute_histogram_stats(game_img)
    ortho_stats, ortho_hists = compute_histogram_stats(ortho_img)
    
    print(f"\nEstadísticas GAME:")
    print(f"  RGB Mean: ({game_stats['r_mean']:.3f}, {game_stats['g_mean']:.3f}, {game_stats['b_mean']:.3f})")
    print(f"  RGB Std:  ({game_stats['r_std']:.3f}, {game_stats['g_std']:.3f}, {game_stats['b_std']:.3f})")
    
    print(f"\nEstadísticas ORTOFOTO:")
    print(f"  RGB Mean: ({ortho_stats['r_mean']:.3f}, {ortho_stats['g_mean']:.3f}, {ortho_stats['b_mean']:.3f})")
    print(f"  RGB Std:  ({ortho_stats['r_std']:.3f}, {ortho_stats['g_std']:.3f}, {ortho_stats['b_std']:.3f})")
    
    lut_size = 32
    
    lut_3d = np.zeros((lut_size, lut_size, lut_size, 3), dtype=np.float32)
    
    for r in range(lut_size):
        for g in range(lut_size):
            for b in range(lut_size):
                input_color = np.array([r, g, b], dtype=np.float32) / (lut_size - 1)
                
                output_color = input_color.copy()
                
                for c in range(3):
                    mean_diff = ortho_stats[['r_mean', 'g_mean', 'b_mean'][c]] - game_stats[['r_mean', 'g_mean', 'b_mean'][c]]
                    std_ratio = ortho_stats[['r_std', 'g_std', 'b_std'][c]] / max(game_stats[['r_std', 'g_std', 'b_std'][c]], 0.01)
                    
                    output_color[c] = (output_color[c] - game_stats[['r_mean', 'g_mean', 'b_mean'][c]]) * std_ratio + ortho_stats[['r_mean', 'g_mean', 'b_mean'][c]]
                
                output_color = np.clip(output_color, 0.0, 1.0)
                
                lut_3d[b, g, r] = output_color
    
    lut_height = lut_size
    lut_width = lut_size * lut_size
    lut_2d = np.zeros((lut_height, lut_width, 3), dtype=np.uint8)
    
    for slice_idx in range(lut_size):
        x_offset = slice_idx * lut_size
        lut_2d[:, x_offset:x_offset+lut_size, :] = (lut_3d[slice_idx, :, :, :] * 255).astype(np.uint8)
    
    lut_img = Image.fromarray(lut_2d, 'RGB')
    lut_img.save(output_path)
    
    print(f"\n✅ LUT generada: {output_path}")
    print(f"   Tamaño: {lut_width}x{lut_height} (32³ LUT)")
    
    metadata = {
        'barrio': barrio_name,
        'game_screenshot': os.path.basename(game_screenshot_path),
        'ortophoto_reference': os.path.basename(ortophoto_path),
        'lut_size': lut_size,
        'game_stats': game_stats,
        'ortho_stats': ortho_stats
    }
    
    meta_path = output_path.replace('.png', '_meta.json')
    with open(meta_path, 'w', encoding='utf-8') as f:
        json.dump(metadata, f, indent=2, ensure_ascii=False)
    
    print(f"   Metadata: {meta_path}")
    
    return lut_img, metadata

def generate_neutral_lut(output_path):
    lut_size = 32
    lut_3d = np.zeros((lut_size, lut_size, lut_size, 3), dtype=np.float32)
    
    for r in range(lut_size):
        for g in range(lut_size):
            for b in range(lut_size):
                lut_3d[b, g, r] = np.array([r, g, b], dtype=np.float32) / (lut_size - 1)
    
    lut_height = lut_size
    lut_width = lut_size * lut_size
    lut_2d = np.zeros((lut_height, lut_width, 3), dtype=np.uint8)
    
    for slice_idx in range(lut_size):
        x_offset = slice_idx * lut_size
        lut_2d[:, x_offset:x_offset+lut_size, :] = (lut_3d[slice_idx, :, :, :] * 255).astype(np.uint8)
    
    lut_img = Image.fromarray(lut_2d, 'RGB')
    lut_img.save(output_path)
    
    print(f"✅ LUT neutral generada: {output_path}")

if __name__ == "__main__":
    print("=" * 60)
    print("GENERADOR DE LUTs PARA COLOR MATCHING")
    print("Alsasua Simulator - Realismo Fotográfico")
    print("=" * 60)
    
    base_dir = os.path.dirname(os.path.abspath(__file__))
    project_dir = os.path.dirname(base_dir)
    
    screenshots_dir = os.path.join(project_dir, "Saved", "Screenshots", "ColorMatching")
    output_dir = os.path.join(project_dir, "Content", "LUTs")
    
    os.makedirs(screenshots_dir, exist_ok=True)
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"\nDirectorios:")
    print(f"  Screenshots: {screenshots_dir}")
    print(f"  Output LUTs: {output_dir}")
    
    neutral_lut_path = os.path.join(output_dir, "LUT_Neutral.png")
    generate_neutral_lut(neutral_lut_path)
    
    print("\n" + "=" * 60)
    print("INSTRUCCIONES DE USO:")
    print("=" * 60)
    print("""
1. Toma screenshots en el juego de cada barrio (vista aérea):
   - Guarda en: Saved/Screenshots/ColorMatching/
   - Nombre: CascoViejo_Game.png, Ensanche_Game.png, etc.

2. Extrae la misma zona de la ortofoto real:
   - Guarda en: Saved/Screenshots/ColorMatching/
   - Nombre: CascoViejo_Ortho.png, Ensanche_Ortho.png, etc.

3. Ejecuta este script de nuevo (detectará automáticamente los pares)

4. Las LUTs generadas se guardarán en Content/LUTs/

5. En el editor de Unreal, importa las LUTs y asígnalas al
   AlsasuaBarrioStyleSystem para cada barrio.

Ejemplo de comando (cuando tengas los pares de imágenes):
  python GenerateMatchedLUT.py

El script detectará automáticamente pares como:
  - CascoViejo_Game.png + CascoViejo_Ortho.png → LUT_CascoViejo.png
  - Ensanche_Game.png + Ensanche_Ortho.png → LUT_Ensanche.png
""")
    
    game_screenshots = [f for f in os.listdir(screenshots_dir) if f.endswith('_Game.png')]
    
    if not game_screenshots:
        print("\n⚠️  No se encontraron screenshots del juego.")
        print(f"   Coloca imágenes con formato '*_Game.png' en:")
        print(f"   {screenshots_dir}")
    else:
        print(f"\n📸 Encontrados {len(game_screenshots)} screenshots del juego:")
        for game_file in game_screenshots:
            barrio = game_file.replace('_Game.png', '')
            ortho_file = f"{barrio}_Ortho.png"
            
            game_path = os.path.join(screenshots_dir, game_file)
            ortho_path = os.path.join(screenshots_dir, ortho_file)
            
            if os.path.exists(ortho_path):
                output_path = os.path.join(output_dir, f"LUT_{barrio}.png")
                generate_matched_lut(game_path, ortho_path, output_path, barrio)
                print()
            else:
                print(f"\n⚠️  Falta ortofoto para {barrio}:")
                print(f"   Esperado: {ortho_path}")
    
    print("\n" + "=" * 60)
    print("Proceso completado.")
    print("=" * 60)
