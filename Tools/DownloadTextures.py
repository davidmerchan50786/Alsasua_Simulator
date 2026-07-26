#!/usr/bin/env python3
"""
DownloadTextures.py — Batch download CC0 PBR textures from ambientCG v3 API.
Downloads ZIPs, extracts, and moves textures to Content/Textures/.
Run from project root: python Tools/DownloadTextures.py
"""
import os
import sys
import json
import urllib.request
import urllib.error
import zipfile
import shutil
import tempfile

CONTENT_DIR = os.path.join(os.path.dirname(__file__), "..", "Content", "Textures")
TEMP_DIR = os.path.join(tempfile.gettempdir(), "ambientcg_downloads")

# Category -> best asset ID mapping (popular CC0 materials)
ASSETS = {
    "Asphalt":      "Asphalt033",
    "Cobblestone":  "PavingStones137",
    "StoneWall":    "Rock064",
    "Brick":        "Bricks085",
    "RoofTiles":    "RoofingTiles013A",
    "Ground":       "Ground103",
    "Concrete":     "Concrete034",
    "Wood":         "Wood095",
    "Grass":        "Grass005",
    "MetalPlate":   "Metal063",
}

# Maps to extract from each ZIP
MAP_NAMES = {
    "Color":     ["Color.jpg", "color.jpg"],
    "NormalGL":  ["NormalGL.jpg", "normalgl.jpg", "Normal.jpg", "normal.jpg"],
    "Roughness": ["Roughness.jpg", "roughness.jpg"],
    "AO":        ["AmbientOcclusion.jpg", "ambientocclusion.jpg", "AO.jpg", "ao.jpg"],
}


def download_and_extract_zip(asset_id, asset_name):
    url = f"https://ambientcg.com/get?file={asset_id}_2K-PNG.zip"
    zip_path = os.path.join(TEMP_DIR, f"{asset_name}.zip")

    if os.path.exists(zip_path):
        print(f"  [SKIP] {asset_name} ZIP already downloaded")
    else:
        print(f"  [DL] {asset_name} ({asset_id}) ... ", end="", flush=True)
        try:
            req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
            with urllib.request.urlopen(req, timeout=120) as resp:
                data = resp.read()
                with open(zip_path, "wb") as f:
                    f.write(data)
            print(f"OK ({len(data) // (1024*1024)} MB)")
        except Exception as e:
            print(f"FAIL: {e}")
            return False

    # Extract
    extract_dir = os.path.join(TEMP_DIR, asset_name)
    if os.path.exists(extract_dir):
        shutil.rmtree(extract_dir)
    try:
        with zipfile.ZipFile(zip_path, "r") as zf:
            zf.extractall(extract_dir)
    except zipfile.BadZipFile:
        print(f"  [ERR] Bad ZIP for {asset_name}")
        return False

    # Find and move texture files
    os.makedirs(CONTENT_DIR, exist_ok=True)
    moved = 0
    for root, dirs, files in os.walk(extract_dir):
        for f in files:
            f_lower = f.lower()
            if not f_lower.endswith(".png"):
                continue

            ue_name = None
            if "_color" in f_lower:
                ue_name = f"T_{asset_name}_Color.png"
            elif "_normalgl" in f_lower or "_normal_gl" in f_lower:
                ue_name = f"T_{asset_name}_NormalGL.png"
            elif "_normal" in f_lower:
                ue_name = f"T_{asset_name}_Normal.png"
            elif "_roughness" in f_lower:
                ue_name = f"T_{asset_name}_Roughness.png"
            elif "_ambientocclusion" in f_lower or "_ao" in f_lower:
                ue_name = f"T_{asset_name}_AO.png"

            if ue_name:
                src = os.path.join(root, f)
                dst = os.path.join(CONTENT_DIR, ue_name)
                if not os.path.exists(dst):
                    shutil.copy2(src, dst)
                    print(f"    -> {ue_name}")
                    moved += 1

    return moved > 0


def main():
    os.makedirs(CONTENT_DIR, exist_ok=True)
    os.makedirs(TEMP_DIR, exist_ok=True)

    total_ok = 0
    for asset_name, asset_id in ASSETS.items():
        print(f"\n[{asset_name}]")
        if download_and_extract_zip(asset_id, asset_name):
            total_ok += 1

    print(f"\n{'='*50}")
    print(f"Successfully downloaded: {total_ok}/{len(ASSETS)}")
    print(f"Textures saved to: {os.path.abspath(CONTENT_DIR)}")
    print(f"UE5 will auto-import .png/.jpg files on next editor open.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
