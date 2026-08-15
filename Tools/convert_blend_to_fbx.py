"""
Convert all .blend files to FBX in-place (same directory).
Uses Blender in --background mode with --python script.
"""
import subprocess
import os
import sys
import json

BLENDER = r"C:\Program Files\Blender Foundation\Blender 5.2\blender.exe"
# Raiz derivada de donde vive este fichero. Antes fija a la maquina original
# ("F:/Epic Games/UE_5.7/altsasu_gtavii/UnrealProject"), asi que solo corria alli.
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASE = os.path.join(RAIZ, "Content", "AssetsImportados")

# Python script to run inside Blender
BLENDER_SCRIPT = r'''
import bpy
import sys
import os

def convert_blend_to_fbx(blend_path):
    """Open .blend, export all meshes as FBX."""
    fbx_out = os.path.splitext(blend_path)[0] + ".fbx"

    # Clear default scene
    bpy.ops.wm.read_factory_settings(use_empty=False)

    # Open the .blend file
    try:
        bpy.ops.wm.open_mainfile(filepath=blend_path)
    except Exception as e:
        print(f"ERROR opening {blend_path}: {e}")
        return False

    # Deselect all
    bpy.ops.object.select_all(action='DESELECT')

    # Find all mesh objects
    mesh_objects = [obj for obj in bpy.data.objects if obj.type == 'MESH']

    if not mesh_objects:
        print(f"WARNING: No mesh objects in {blend_path}")
        # Try to export anyway (might have curves/lights etc)
        mesh_objects = list(bpy.data.objects)

    if not mesh_objects:
        print(f"SKIP: No objects in {blend_path}")
        return False

    # Select all mesh objects
    for obj in mesh_objects:
        obj.select_set(True)

    # Set active
    bpy.context.view_layer.objects.active = mesh_objects[0]

    # Export as FBX
    try:
        bpy.ops.export_scene.fbx(
            filepath=fbx_out,
            use_selection=False,
            global_scale=1.0,
            apply_scale_options='FBX_SCALE_ALL',
            axis_forward='-Z',
            axis_up='Y',
            use_mesh_modifiers=True,
            mesh_smooth_type='OFF',
            path_mode='COPY',
            embed_textures=True,
            bake_anim=True,
            bake_anim_use_all_bones=True,
            bake_anim_use_nla_strips=False,
            bake_anim_use_all_actions=True,
        )
        fbx_size = os.path.getsize(fbx_out)
        print(f"OK: {fbx_out} ({fbx_size/1024:.0f} KB)")
        return True
    except Exception as e:
        print(f"ERROR exporting {blend_path}: {e}")
        return False

if __name__ == "__main__":
    # Get the blend path from command line args (after --)
    argv = sys.argv
    if "--" not in argv:
        print("No blend file specified")
        sys.exit(1)
    args = argv[argv.index("--") + 1:]
    blend_path = args[0]

    # Workaround: open_mainfile already opens the file
    # We need to use open instead
    success = convert_blend_to_fbx(blend_path)
    sys.exit(0 if success else 1)
'''

def find_blend_files():
    """Find all .blend files."""
    blend_files = []
    for root, dirs, files in os.walk(BASE):
        # Skip already-extracted
        if "ExtractedUnity" in root:
            continue
        for f in files:
            if f.lower().endswith('.blend'):
                blend_files.append(os.path.join(root, f))
    return blend_files

def convert_with_blender(blend_path):
    """Run Blender to convert a single .blend to FBX."""
    fbx_out = os.path.splitext(blend_path)[0] + ".fbx"
    if os.path.exists(fbx_out):
        return "SKIP"

    # Create temp script for this file
    script = f'''
import bpy
import sys

blend_path = r"{blend_path}"
fbx_out = r"{fbx_out}"

# Open file
bpy.ops.wm.open_mainfile(filepath=blend_path)

# Find meshes
mesh_objects = [obj for obj in bpy.data.objects if obj.type == 'MESH']
if not mesh_objects:
    mesh_objects = list(bpy.data.objects)

if not mesh_objects:
    print(f"SKIP: No objects in {{blend_path}}")
    sys.exit(0)

# Select all
bpy.ops.object.select_all(action='DESELECT')
for obj in mesh_objects:
    obj.select_set(True)
bpy.context.view_layer.objects.active = mesh_objects[0]

# Export FBX
try:
    bpy.ops.export_scene.fbx(
        filepath=fbx_out,
        use_selection=False,
        global_scale=1.0,
        apply_scale_options='FBX_SCALE_ALL',
        axis_forward='-Z',
        axis_up='Y',
        use_mesh_modifiers=True,
        path_mode='COPY',
        embed_textures=True,
        bake_anim=True,
    )
    fbx_size = __import__("os").path.getsize(fbx_out)
    print(f"OK: {{fbx_out}} ({{fbx_size//1024}} KB)")
except Exception as e:
    print(f"ERROR: {{e}}")
    sys.exit(1)
'''
    script_path = os.path.join(os.environ.get('TEMP', r'C:\Temp'), f'_blend_convert_{hash(blend_path) & 0xFFFF:04x}.py')
    with open(script_path, 'w', encoding='utf-8') as sf:
        sf.write(script)

    try:
        result = subprocess.run(
            [BLENDER, "--background", "--python", script_path],
            capture_output=True, text=True, timeout=120, encoding='utf-8', errors='replace'
        )
        output = result.stdout + result.stderr
        # Check for success
        if f"OK: {fbx_out}" in output:
            return "OK"
        elif "SKIP: No objects" in output:
            return "SKIP"
        else:
            # Extract useful error
            for line in output.split('\n'):
                if 'Error' in line or 'error' in line or 'ERROR' in line:
                    return f"ERROR: {line.strip()}"
            return f"ERROR: Blender exited with code {result.returncode}"
    except subprocess.TimeoutExpired:
        return "TIMEOUT"
    except Exception as e:
        return f"ERROR: {e}"
    finally:
        try:
            os.remove(script_path)
        except:
            pass

def main():
    blend_files = find_blend_files()
    print(f"Found {len(blend_files)} .blend files to convert")

    results = {"ok": [], "skip": [], "error": [], "timeout": []}

    for i, bp in enumerate(sorted(blend_files)):
        rel = os.path.relpath(bp, BASE)
        print(f"[{i+1}/{len(blend_files)}] {rel}...", end=" ", flush=True)

        status = convert_with_blender(bp)
        print(status)

        if status == "OK":
            results["ok"].append(rel)
        elif status == "SKIP":
            results["skip"].append(rel)
        elif status == "TIMEOUT":
            results["timeout"].append(rel)
        else:
            results["error"].append({"file": rel, "error": status})

    print(f"\n{'='*60}")
    print(f"Converted: {len(results['ok'])}")
    print(f"Skipped: {len(results['skip'])}")
    print(f"Errors: {len(results['error'])}")
    print(f"Timeouts: {len(results['timeout'])}")

    if results["error"]:
        print("\nErrors:")
        for e in results["error"]:
            print(f"  {e['file']}: {e['error']}")

if __name__ == "__main__":
    main()
