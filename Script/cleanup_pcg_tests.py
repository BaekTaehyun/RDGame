import unreal

def cleanup_tests():
    print("--- Cleaning up MCP Test Assets ---")
    
    # Path used in verification script
    package_path = "/Game/Data/PCG_Test"
    
    # List assets
    asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = asset_registry.get_assets_by_path(package_path, recursive=True)
    
    deleted_count = 0
    
    for asset_data in assets:
        asset_name = str(asset_data.asset_name)
        # Check pattern used in verification: MCP_PCG_<timestamp>
        if asset_name.startswith("MCP_PCG_"):
            full_path = asset_data.package_name
            print(f"Deleting: {full_path}")
            if unreal.EditorAssetLibrary.delete_asset(str(full_path)):
                deleted_count += 1
            else:
                print(f"  [Error] Failed to delete {full_path}")
                
    print(f"--- Cleanup Finished. Deleted {deleted_count} assets. ---")

cleanup_tests()
