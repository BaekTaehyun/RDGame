import unreal

def inspect_struct():
    print(">>> Inspecting PCGMeshSelectorWeightedEntry Properties")
    
    try:
        cls = unreal.PCGMeshSelectorWeightedEntry
        inst = cls()
        print(f"Instance: {inst}")
        
        print("--- Attributes (dir) ---")
        for d in dir(inst):
            if not d.startswith("_"):
                print(f"  {d}")
                
        # Try finding the mesh property
        mesh_path = "/Engine/BasicShapes/Cube.Cube"
        mesh_asset = unreal.load_asset(mesh_path)
        
        # Test 1: Direct 'mesh'
        if hasattr(inst, "mesh"):
            print("[?] Found 'mesh' attribute")
            try:
                inst.mesh = mesh_asset
                print("[O] Set 'mesh' SUCCESS")
            except Exception as e:
                print(f"[X] Set 'mesh' FAILED: {e}")

        # Test 2: 'descriptor' (Likely the correct path)
        if hasattr(inst, "descriptor"):
            print("[?] Found 'descriptor' attribute")
            desc = inst.descriptor
            print(f"    Descriptor Type: {type(desc)}")
            print("    --- Descriptor Attributes ---")
            for d in dir(desc):
                if not d.startswith("_"):
                    print(f"      {d}")
            
            # Try setting static_mesh on descriptor
            if hasattr(desc, "static_mesh"):
                try:
                    desc.static_mesh = mesh_asset
                    inst.descriptor = desc # Structs might need reassignment
                    print("[O] Set 'descriptor.static_mesh' SUCCESS")
                except Exception as e:
                    print(f"[X] Set 'descriptor.static_mesh' FAILED: {e}")
            elif hasattr(desc, "static_mesh_path"): # Maybe it is a path?
                 pass 
            else:
                 print("    [!] 'static_mesh' not found in descriptor. Checking if it is a SoftObjectPath...")


    except Exception as e:
        print(f"Error: {e}")

inspect_struct()
