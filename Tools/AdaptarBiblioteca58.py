"""
Enable Nanite on all imported static meshes (UE 5.8).
Run in UE5 Output Log: exec(open(r'H:\Temp\opencode\AlsasuaUE5Clean\Tools\AdaptarBiblioteca58.py').read())

This iterates all UStaticMesh assets under /Game/ImportedAssets and enables
Nanite via the Interchange pipeline. Static meshes with Nanite enabled render
at full quality with automatic LOD — no manual impostors needed.
"""
import unreal

def enable_nanite_on_mesh(asset_path):
    """Enable Nanite on a single StaticMesh asset."""
    mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not mesh or not isinstance(mesh, unreal.StaticMesh):
        return False

    # Enable Nanite via the static mesh's build settings
    static_mesh = unreal.StaticMesh.cast(mesh)
    if not static_mesh:
        return False

    # Check if Nanite is already enabled
    if static_mesh.get_editor_property('nanite_settings').get_editor_property('enabled'):
        return True

    # Enable Nanite
    nanite_settings = static_mesh.get_editor_property('nanite_settings')
    nanite_settings.set_editor_property('enabled', True)
    static_mesh.set_editor_property('nanite_settings', nanite_settings)

    # Save
    unreal.EditorAssetLibrary.save_asset(asset_path)
    return True

def main():
    # Find all static meshes under ImportedAssets
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    all_assets = asset_registry.get_assets_by_path('/Game/ImportedAssets', True, False)

    static_meshes = [
        a for a in all_assets
        if a.asset_class == 'StaticMesh'
    ]

    unreal.log(f'Found {len(static_meshes)} static meshes under /Game/ImportedAssets')

    enabled = 0
    skipped = 0
    failed = 0

    for i, asset_data in enumerate(static_meshes):
        asset_path = str(asset_data.package_name)

        try:
            if enable_nanite_on_mesh(asset_path):
                enabled += 1
            else:
                skipped += 1
        except Exception as e:
            failed += 1
            unreal.log_warning(f'FAIL: {asset_path}: {e}')

        if (i + 1) % 25 == 0:
            unreal.log(f'[{i+1}/{len(static_meshes)}] Nanite enabled={enabled} skip={skipped} fail={failed}')

    unreal.log(f'=== Done: {enabled} meshes got Nanite, {skipped} skipped, {failed} failed ===')

if __name__ == "__main__":
    main()
