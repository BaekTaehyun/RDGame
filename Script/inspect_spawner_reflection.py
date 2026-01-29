import unreal

def inspect_spawner_settings():
    print("--- Inspecting PCGStaticMeshSpawnerSettings ---")
    settings_cls = unreal.PCGStaticMeshSpawnerSettings
    obj = settings_cls()
    
    props = dir(obj)
    mesh_props = [p for p in props if "mesh" in p.lower() or "entry" in p.lower()]
    
    print(f"Candidates: {mesh_props}")
    
    # Try getting value of likely candidates
    for p in mesh_props:
        try:
            val = getattr(obj, p)
            print(f" {p}: {type(val)} = {val}")
        except:
            print(f" {p}: <Error>")

if __name__ == "__main__":
    inspect_spawner_settings()
