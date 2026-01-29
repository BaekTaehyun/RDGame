import unreal
import time

graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def force_save():
    print("--- Force Saving PCG Asset ---")
    graph = unreal.load_asset(graph_path)
    if graph:
        # Mark dirty
        graph.modify()
        
        # Save
        success = unreal.EditorAssetLibrary.save_loaded_asset(graph)
        print(f"Asset Saved: {success}")
        
        # Try to Notify Editor (Not always exposed to Python)
        # But saving usually triggers a re-read if the editor checks file stamps, 
        # or at least ensures it's there when reopened.
        
    else:
        print("Graph not found")

if __name__ == "__main__":
    force_save()
