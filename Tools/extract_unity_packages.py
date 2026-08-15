"""
Extract all .unitypackage files under AssetsImportados.
Each entry is a GUID directory containing:
  - pathname: text file with the original Unity path (e.g. Assets/Village/SM_House.fbx)
  - asset: the actual binary file
  - metaData: Unity metadata
  - preview.png: thumbnail
We read pathname, then copy asset to Content/AssetsImportados/<original_path>.
"""
import tarfile, os, sys, shutil, json

# Raiz derivada de donde vive este fichero. Antes fija a la maquina original
# ("F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject"), asi que solo corria alli.
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = os.path.join(RAIZ, "Content", "AssetsImportados")
DEST_ROOT = os.path.join(BASE, "ExtractedUnity")

# extensions we care about
KEEP_EXT = {'.fbx', '.obj', '.gltf', '.glb', '.png', '.jpg', '.jpeg', '.tga', '.tif', '.tiff',
            '.psd', '.mat', '.prefab', '.asset', '.wav', '.mp3', '.ogg'}

def extract_unitypackage(tgz_path, dest_root):
    """Extract one .unitypackage, returning list of (pathname, asset_size)."""
    results = []
    try:
        t = tarfile.open(tgz_path, 'r:gz')
    except Exception as e:
        print(f"  ERROR opening {tgz_path}: {e}")
        return results

    # Collect all members
    members = {m.name: m for m in t.getmembers()}

    # Find all GUID dirs (have a 'pathname' entry)
    guid_dirs = set()
    for name in members:
        parts = name.split('/')
        if len(parts) >= 2:
            guid_dirs.add(parts[0])

    for guid in guid_dirs:
        # Read pathname
        pn_member = members.get(f"{guid}/pathname")
        if not pn_member:
            continue
        try:
            pn_file = t.extractfile(pn_member)
            if not pn_file:
                continue
            pathname = pn_file.read().decode('utf-8', errors='replace').strip()
        except:
            continue

        # Read asset
        asset_member = members.get(f"{guid}/asset")
        if not asset_member:
            continue
        try:
            asset_file = t.extractfile(asset_member)
            if not asset_file:
                continue
            asset_data = asset_file.read()
        except:
            continue

        # Determine output path
        # pathname looks like: Assets/Village/SM_House.fbx or Assets/Sounds/rain.wav
        # Strip leading "Assets/" and map to dest_root
        if pathname.startswith("Assets/"):
            rel_path = pathname[7:]
        else:
            rel_path = pathname

        out_path = os.path.join(dest_root, rel_path)
        os.makedirs(os.path.dirname(out_path), exist_ok=True)

        with open(out_path, 'wb') as f:
            f.write(asset_data)

        results.append((pathname, len(asset_data)))
        t.close()
        return results  # close after first read to avoid issues

    t.close()
    return results


def extract_all():
    os.makedirs(DEST_ROOT, exist_ok=True)

    # Find all .unitypackage files
    unity_pkgs = []
    for root, dirs, files in os.walk(BASE):
        for f in files:
            if f.lower().endswith('.unitypackage'):
                unity_pkgs.append(os.path.join(root, f))

    print(f"Found {len(unity_pkgs)} .unitypackage files")
    print("=" * 60)

    total_assets = 0
    manifest = []

    for pkg_path in sorted(unity_pkgs):
        pkg_name = os.path.basename(pkg_path)
        pkg_size_mb = os.path.getsize(pkg_path) / (1024*1024)
        print(f"\n--- {pkg_name} ({pkg_size_mb:.1f} MB) ---")

        try:
            t = tarfile.open(pkg_path, 'r:gz')
        except Exception as e:
            print(f"  ERROR: {e}")
            continue

        members = {m.name: m for m in t.getmembers()}

        # Find GUID directories with pathname
        guid_dirs = set()
        for name in members:
            parts = name.split('/')
            if len(parts) >= 2:
                guid_dirs.add(parts[0])

        pkg_count = 0
        pkg_files = []

        for guid in sorted(guid_dirs):
            pn_member = members.get(f"{guid}/pathname")
            if not pn_member:
                continue
            try:
                pn_file = t.extractfile(pn_member)
                if not pn_file:
                    continue
                pathname = pn_file.read().decode('utf-8', errors='replace').strip().split('\n')[0].strip()
            except:
                continue

            asset_member = members.get(f"{guid}/asset")
            if not asset_member:
                continue
            try:
                asset_file = t.extractfile(asset_member)
                if not asset_file:
                    continue
                asset_data = asset_file.read()
            except:
                continue

            # Check extension
            ext = os.path.splitext(pathname)[1].lower()

            if pathname.startswith("Assets/"):
                rel_path = pathname[7:]
            else:
                rel_path = pathname

            out_path = os.path.join(DEST_ROOT, rel_path)
            os.makedirs(os.path.dirname(out_path), exist_ok=True)

            with open(out_path, 'wb') as f:
                f.write(asset_data)

            pkg_files.append(pathname)
            pkg_count += 1

        t.close()
        total_assets += pkg_count
        manifest.append({"package": pkg_name, "size_mb": round(pkg_size_mb, 1), "files": pkg_files})
        print(f"  Extracted {pkg_count} assets")

    # Save manifest
    manifest_path = os.path.join(DEST_ROOT, "_manifest.json")
    with open(manifest_path, 'w', encoding='utf-8') as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    print(f"\n{'='*60}")
    print(f"TOTAL: {total_assets} assets extracted from {len(unity_pkgs)} packages")
    print(f"Manifest: {manifest_path}")

    # Summary by type
    ext_counts = {}
    for pkg in manifest:
        for fp in pkg["files"]:
            ext = os.path.splitext(fp)[1].lower()
            ext_counts[ext] = ext_counts.get(ext, 0) + 1

    print(f"\nBy extension:")
    for ext, count in sorted(ext_counts.items(), key=lambda x: -x[1]):
        print(f"  {ext}: {count}")


if __name__ == "__main__":
    extract_all()
