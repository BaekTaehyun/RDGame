import unreal

def check_thumbnail_api():
    print(">>> Checking Thumbnail API Availability...")
    
    # Check for our custom C++ BPL
    if hasattr(unreal, "DungeonAssetUtils"):
        print(f"[O] unreal.DungeonAssetUtils is Available!")
        
        # Test basic capture - Find a valid asset first!
        print("Finding a valid StaticMesh to test...")
        asset_reg = unreal.AssetRegistryHelpers.get_asset_registry()
        filter = unreal.ARFilter(class_names=["StaticMesh"], recursive_paths=True, package_paths=["/Game"])
        assets = asset_reg.get_assets(filter)
        
        if len(assets) > 0:
            test_asset_path = str(assets[0].package_name) # Convert FName to str
            print(f"Testing capture on: {test_asset_path}")
            
            try:
                png_data = unreal.DungeonAssetUtils.capture_thumbnail(str(test_asset_path))
                print(f"Result type: {type(png_data)}")
                print(f"Data length: {len(png_data)}")
                
                if len(png_data) > 0:
                    print("SUCCESS: Capture returned data.")
                else:
                    print("WARNING: Capture returned empty data (Asset might not have thumbnail cached).")
                    
            except Exception as e:
                print(f"Error calling capture_thumbnail: {e}")
                
        else:
            print("[X] No StaticMesh found in /Game to test.")
            
    else:
        print(f"[X] unreal.DungeonAssetUtils is NOT found. Plugin loaded? Compiled?")

if __name__ == "__main__":
    check_thumbnail_api()
