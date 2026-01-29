import sys
import json
import subprocess
import time

BRIDGE_SCRIPT = r"c:\Users\COM2US\Documents\Unreal Projects\RdGame\Script\unreal_mcp_bridge.py"
GRAPH_PATH = "/Game/LevelPrototyping/PCG_Nature_Wood.PCG_Nature_Wood"

def restore_ruins():
    print(f"--- [Restore] Rebuilding Ruins Chain via MCP ---", flush=True)
    
    proc = subprocess.Popen(
        [sys.executable, BRIDGE_SCRIPT],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=sys.stderr,
        text=True,
        bufsize=0
    )
    
    def rpc(method, params, expect_response=True):
        req_id = int(time.time()*1000)
        req = {"jsonrpc": "2.0", "method": method, "params": params}
        if expect_response:
            req["id"] = req_id
        
        try:
            proc.stdin.write(json.dumps(req) + "\n")
            proc.stdin.flush()
        except: return None
        
        if expect_response:
            return json.loads(proc.stdout.readline())
        return None

    try:
        rpc("initialize", {}, True)
        rpc("notifications/initialized", {}, False)

        # 1. Create Nodes
        print("1. Creating Nodes...")
        
        # Filter (X=1000, Y=800)
        res = rpc("tools/call", {
            "name": "add_pcg_node", 
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_class": "PCGDensityFilterSettings",
                "node_name": "Ruins_Filter",
                "position_x": 1000,
                "position_y": 800
            }
        })
        filter_node = res['result']['content'][0]['text']
        filter_node = json.loads(filter_node)['node_name']
        print(f"   Created Filter: {filter_node}")

        # Transform (X=1300, Y=800)
        res = rpc("tools/call", {
            "name": "add_pcg_node", 
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_class": "PCGTransformPointsSettings",
                "node_name": "Ruins_Transform",
                "position_x": 1300,
                "position_y": 800
            }
        })
        trans_node = res['result']['content'][0]['text']
        trans_node = json.loads(trans_node)['node_name']
        print(f"   Created Transform: {trans_node}")

        # Spawner (X=1600, Y=800)
        res = rpc("tools/call", {
            "name": "add_pcg_node", 
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_class": "PCGStaticMeshSpawnerSettings",
                "node_name": "Ruins_Spawner",
                "position_x": 1600,
                "position_y": 800
            }
        })
        spawner_node = res['result']['content'][0]['text']
        spawner_node = json.loads(spawner_node)['node_name']
        print(f"   Created Spawner: {spawner_node}")

        # 2. Connect
        print("\n2. Connecting Chain...")
        # SelfPruning_0 -> Filter
        rpc("tools/call", {"name": "connect_pcg_nodes", "arguments": {
            "graph_path": GRAPH_PATH,
            "upstream_node": "SelfPruning_0",
            "downstream_node": filter_node
        }})
        
        # Filter -> Transform
        rpc("tools/call", {"name": "connect_pcg_nodes", "arguments": {
            "graph_path": GRAPH_PATH,
            "upstream_node": filter_node,
            "downstream_node": trans_node
        }})

        # Transform -> Spawner
        rpc("tools/call", {"name": "connect_pcg_nodes", "arguments": {
            "graph_path": GRAPH_PATH,
            "upstream_node": trans_node,
            "downstream_node": spawner_node
        }})
        
        # 3. Configure Properties
        print("\n3. Configuring Properties...")
        
        # Filter Range (0.05 ~ 0.3) - Pattern Break
        rpc("tools/call", {
            "name": "set_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": filter_node,
                "properties": {"LowerBound": 0.05, "UpperBound": 0.3}
            }
        })

        # Transform Scale/Rot
        # Need to use 'execute_unreal_script' for complex structs if 'set_pcg_node_properties' is basic.
        # But let's try basic first or use the 'fix' script logic.
        # For now, just Scale/Rot via existing logic?
        # Re-using 'apply_pcg_fix_direct.py' logic might be safer for Rotator.
        
        # Just Set Meshes for Spawner
        paths = [
            "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar_Damaged.SM_Stone_Pillar_Damaged",
            "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Pillar.SM_Stone_Pillar",
            "/Game/Stylised_Dungeon_Pack/Meshes/Columns_And_Pillars/SM_Stone_Column_Destroyed.SM_Stone_Column_Destroyed"
        ]
        rpc("tools/call", {
            "name": "set_pcg_node_properties",
            "arguments": {
                "graph_path": GRAPH_PATH,
                "node_name": spawner_node,
                "properties": {"MeshEntries": paths}
            }
        })

        print("\n[Done] Ruins Restored.")

    except Exception as e:
        print(f"[Error] {e}")
    finally:
        proc.terminate()

if __name__ == "__main__":
    restore_ruins()
