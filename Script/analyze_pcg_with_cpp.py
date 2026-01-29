import unreal
import json

def run_cpp_analysis_refined():
    graph_path = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"
    print(f"--- Refined Topology Analysis: {graph_path} ---")
    
    if not hasattr(unreal, "DungeonAssetUtils"):
        print("[Error] DungeonAssetUtils not found. Please restart editor.")
        return

    json_str = unreal.DungeonAssetUtils.analyze_pcg_topology(graph_path)
    
    try:
        data = json.loads(json_str)
        if "Error" in data:
            print(f"[Error] {data['Error']}")
            return
            
        nodes = data.get("Nodes", [])
        
        # Sort by name for easier reading
        nodes.sort(key=lambda x: x.get("Name", ""))
        
        print(f"{'Source Node (ID)':<35} | {'Type':<30} | {'Downstream Nodes'}")
        print("-" * 120)
        
        for n in nodes:
            name = n.get("Name", "Unknown")
            # Heuristic to guess type from Name if Title is same as Name
            # Or usually the previous script showed Title as Class Name.
            # Let's rely on name for ID.
            
            title = n.get("Title", "") # In C++ we set Title = Settings Class Name if available
            
            outbound = n.get("Outbound", [])
            out_str = ", ".join(outbound) if outbound else ""
            
            print(f"{name[:35]:<35} | {title[:30]:<30} | {out_str}")
            
    except Exception as e:
        print(f"JSON Parse Error: {e}")

run_cpp_analysis_refined()
