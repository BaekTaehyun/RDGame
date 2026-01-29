import unreal
import os

def export_graph_to_text():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    export_dir = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script"
    export_filename = "PCG_Export.t3d"
    full_path = os.path.join(export_dir, export_filename)
    
    print(f"--- Exporting Graph to: {full_path} ---")
    
    # helper to get asset
    graph_asset = unreal.load_asset(graph_path)
    if not graph_asset:
        print("Graph not found.")
        return

    # Create Export Task
    task = unreal.AssetExportTask()
    task.object = graph_asset
    task.filename = full_path
    task.automated = True
    task.replace_identical = True
    task.prompt = False
    
    # Use T3D exporter
    exporter = unreal.ObjectExporterT3D()
    task.exporter = exporter
    
    # Execute
    # Correct API is export_assets(assets, export_path)
    success = unreal.AssetToolsHelpers.get_asset_tools().export_assets([full_path], export_dir) # Try passing string path if object fails, or list of strings
    # Actually, export_assets usually takes a list of strings (paths) or objects. 
    # And the second arg is the directory.
    # The task wrapper might be for export_asset_tasks.
    # Let's try the simple version: 
    # unreal.AssetToolsHelpers.get_asset_tools().export_assets([graph_asset.get_path_name()], export_dir)
    
    # RETHINK: The user wants me to be careful.
    # The error said `export_assets() required argument 'export_path' (pos 2)`.
    # It takes (Assets, ExportPath).
    success = unreal.AssetToolsHelpers.get_asset_tools().export_assets([graph_asset.get_path_name()], export_dir)
    
    if success:
        print("[SUCCESS] Exported T3D file.")
        
        # Verify file exists
        if os.path.exists(full_path):
             print(f"[SUCCESS] File exists at {full_path}")
        else:
             print("[FAIL] File not found after success report.")
    else:
        print("[FAIL] Export Task failed.")

export_graph_to_text()
