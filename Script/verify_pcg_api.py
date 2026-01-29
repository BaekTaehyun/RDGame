import unreal
import sys
import inspect

def verify_pcg_api():
    print(">>> Checking PCG Python API (Force Update)...")
    
    # 2. Try Creating a Graph Asset
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    package_path = "/Game/Data/PCG_Test"
    asset_name = "PCG_PythonTest"
    
    if unreal.EditorAssetLibrary.does_asset_exist(f"{package_path}/{asset_name}"):
        unreal.EditorAssetLibrary.delete_asset(f"{package_path}/{asset_name}")
        
    print(f"Creating PCG Graph at {package_path}/{asset_name}...")
    
    try:
        factory = unreal.PCGGraphFactory()
        graph = asset_tools.create_asset(asset_name, package_path, unreal.PCGGraph, factory)
        
        if graph:
            print(f"[O] Successfully created PCG Graph: {graph.get_path_name()}")
            
            # Use StaticMeshSpawner which we know exists
            target_class = unreal.PCGStaticMeshSpawnerSettings
            
            # --- Node A ---
            print(f"Adding Node A ({target_class.__name__})...")
            res_a = graph.add_node_of_type(target_class)
            node_a = res_a[0] if isinstance(res_a, (tuple, list)) else res_a
            
            if node_a:
                settings = node_a.get_settings()
                print(f"Settings Class: {settings.get_class().get_name()}")
                print("--- Mesh Selector Inspection ---")
                try:
                    # Check selector type
                    print(f"Selector Type: {settings.mesh_selector_type}")
                    
                    # Check instance
                    selector_inst = settings.mesh_selector_instance
                    if selector_inst:
                        print(f"Selector Instance: {selector_inst}")
                        print(f"Selector Class: {selector_inst.get_class().get_name()}")
                        print("--- Selector Instance Attributes ---")
                        for d in dir(selector_inst):
                            if not d.startswith("_"):
                                print(f"  {d}")
                                
                        # Check for MeshEntries here
                        if hasattr(selector_inst, "mesh_entries"):
                            print(f"[O] Found mesh_entries in selector: {selector_inst.mesh_entries}")
                    else:
                        print("[!] No mesh_selector_instance found.")
                except Exception as e:
                    print(f"Error inspecting selector: {e}")
            
            # --- Node B ---
            print(f"Adding Node B ({target_class.__name__})...")
            res_b = graph.add_node_of_type(target_class)
            node_b = res_b[0] if isinstance(res_b, (tuple, list)) else res_b
            
            if node_a and node_b:
                print(f"[O] Created Nodes: A={node_a.get_name()}, B={node_b.get_name()}")
                
                # --- Test Edge Connection ---
                print("\n--- Testing Edge Connection (B -> A) ---")
                
                # Check method signature
                try:
                    doc = node_b.add_edge_to.__doc__
                    print(f"add_edge_to Doc: {doc}")
                except:
                    pass

                # Try connection
                try:
                    # Usually: add_edge_to(OutLabel, DownstreamNode, InLabel)
                    node_b.add_edge_to("Out", node_a, "In")
                    print("[O] add_edge_to('Out', node_a, 'In') SUCCESS")
                except Exception as e1:
                    print(f"[!] Attempt 1 failed: {e1}")
                    try:
                        node_b.add_edge_to(node_a)
                        print("[O] add_edge_to(node_a) SUCCESS")
                    except Exception as e2:
                        print(f"[!] Attempt 2 failed: {e2}")
                        
            # --- Test Property Modification ---
            print("\n--- Testing Property Modification ---")
            if node_a:
                settings = node_a.get_settings()
                print(f"Settings Object: {settings}")
                
                # Try setting 'static_mesh' property if available?
                # Actually, let's just inspect properties
                # unreal.PCGStaticMeshSpawnerSettings usually has 'mesh_entries' or 'meshes'
                pass

        else:
            print("[X] Failed to create asset.")
            
    except Exception as e:
        print(f"[X] Error: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    verify_pcg_api()
