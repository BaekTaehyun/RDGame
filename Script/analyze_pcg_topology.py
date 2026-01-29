import unreal
import os
import glob
import re

def analyze_topology():
    # 1. Setup Paths
    project_dir = unreal.SystemLibrary.get_project_directory()
    analysis_dir = os.path.join(project_dir, "Saved", "PCG_Analysis")
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    
    if not os.path.exists(analysis_dir):
        os.makedirs(analysis_dir)
        
    print(f"--- Toplogy Analysis: {graph_path} ---")
    
    # 2. Export T3D
    # Use export_assets which is confirmed to exist
    graph_asset = unreal.load_asset(graph_path)
    if not graph_asset:
        print("[Error] Graph not found.")
        return

    # Clear previous
    for f in glob.glob(os.path.join(analysis_dir, "**", "*.T3D"), recursive=True):
        try: os.remove(f)
        except: pass
        
    print("Exporting...")
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.export_assets([graph_asset.get_path_name()], analysis_dir)
    
    # 3. Find Exported File
    t3d_files = glob.glob(os.path.join(analysis_dir, "**", "*.T3D"), recursive=True)
    if not t3d_files:
        print("[Error] T3D file not found after export.")
        return
        
    t3d_path = t3d_files[0]
    print(f"Parsing: {t3d_path}")
    
    # 4. Parse T3D
    with open(t3d_path, "r", encoding="utf-8") as f:
        content = f.read()
        
    # Regex Strategy:
    # 1. Find Nodes (Begin Object ... Name="NodeName")
    # 2. Inside Node, find InputPins/OutputPins
    # 3. Inside Pins, find Edges (InboundNode/OutboundNode)
    
    # Look for Edge definitions
    # Edges often appear like: Edges(0)=(InboundNode=...,InboundPin=...,OutboundNode=...,OutboundPin=...)
    # Or simplified.
    
    print("\n--- Topology Map ---\n")
    
    # Fallback: Just look for literal edge connections in the text
    # Pattern: InboundNode=PCGNode'"..."'
    
    # Let's try to map edges by scanning for "Edges" blocks
    # This is a heuristic parser
    
    lines = content.split('\n')
    current_node = None
    edge_count = 0
    
    for line in lines:
        line = line.strip()
        
        # Detect Node Start
        # Begin Object Class=/Script/PCG.PCGStaticMeshSpawnerSettings Name="StaticMeshSpawner_0"
        if line.startswith("Begin Object") and "Name=" in line:
            # Extract Name
            m = re.search(r'Name="([^"]+)"', line)
            if m:
                current_node = m.group(1)
                
        # Detect Edge in Node
        # We assume Edges are listed under Pins inside the Node or Settings
        # Actually in PCG, Edges might be on the Graph object linking Nodes?
        # Let's check for standard PCG edge syntax.
        
        # If we see lines mentioning "InboundNode" or "OutboundNode", print them
        if "InboundNode" in line or "OutboundNode" in line:
            print(f"[{current_node}] Found Connection Data: {line}")
            edge_count += 1
            
    if edge_count == 0:
        print("No explicit edge connections found in T3D.")
        print("Note: PCG connections might be stored in a binary blob 'NodeData' or similar if not text-serialized.")
        print("Total Bytes Read:", len(content))
        # Dump a small snippet to see structure
        print("\n--- Snippet (First 50 lines) ---")
        print("\n".join(lines[:50]))

analyze_topology()
