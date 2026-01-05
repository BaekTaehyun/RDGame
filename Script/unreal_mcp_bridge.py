import sys
import json
import socket
import logging
import base64

# Configure logging to stderr (stdout is for MCP communication)
logging.basicConfig(level=logging.DEBUG, stream=sys.stderr, format='[MCP-Bridge] %(message)s')

HOST = '127.0.0.1'
PORT = 3001

class UnrealMCPBridge:
    def __init__(self):
        self.running = True

    def connect_and_send(self, command_data):
        """Sends command to Unreal Socket and returns response."""
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.settimeout(20.0) # 20s timeout
                s.connect((HOST, PORT))
                
                # Send Request
                msg = json.dumps(command_data)
                s.sendall(msg.encode('utf-8'))
                
                # Receive Response (Length-Prefixed)
                header = s.recv(4)
                if not header:
                    return {"error": "No response from Unreal Server"}
                    
                msg_len = int.from_bytes(header, 'big')
                
                chunks = []
                bytes_recd = 0
                while bytes_recd < msg_len:
                    chunk = s.recv(min(msg_len - bytes_recd, 1024 * 64))
                    if not chunk:
                        break
                    chunks.append(chunk)
                    bytes_recd += len(chunk)
                
                data = b"".join(chunks)
                response = data.decode('utf-8')
                
                # Parse Wrapper
                wrapper = json.loads(response)
                if wrapper.get("status") == "error":
                     return {"error": wrapper.get("message")}
                     
                return wrapper.get("result", {})
                
        except Exception as e:
            logging.error(f"Socket Error: {e}")
            return {"error": f"Socket connection failed: {str(e)}"}

    def run(self):
        """Main Loop: Read Stdin, Proccess JSON-RPC, Write Stdout"""
        logging.info("Unreal MCP Bridge Started")
        
        while self.running:
            try:
                line = sys.stdin.readline()
                if not line:
                    break
                
                # logging.debug(f"Raw Input: {line.strip()}")
                
                request = json.loads(line)
                response = self.handle_request(request)
                
                if response:
                    out_str = json.dumps(response)
                    sys.stdout.write(out_str + "\n")
                    sys.stdout.flush()
                    # logging.debug(f"Sent: {out_str[:50]}...")
                    
            except json.JSONDecodeError:
                logging.error("Invalid JSON received")
            except Exception as e:
                logging.error(f"Loop Error: {e}")
                # Important: Do not print to stdout on error, it breaks the pipe
                pass
                
    def handle_request(self, request):
        method = request.get("method")
        msg_id = request.get("id")
        params = request.get("params", {})
        
        logging.debug(f"Received Method: {method}")

        # 1. Initialize
        if method == "initialize":
            return {
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "protocolVersion": "2024-11-05",
                    "capabilities": {
                        "tools": {}
                    },
                    "serverInfo": {
                        "name": "unreal-mcp-bridge",
                        "version": "1.0.0"
                    }
                }
            }
        
        # 2. Initialized Notification
        elif method == "notifications/initialized":
            return None # No response needed
            
        # 3. List Tools
        elif method == "tools/list":
            return {
                "jsonrpc": "2.0",
                "id": msg_id,
                "result": {
                    "tools": [
            {
                "name": "scan_assets",
                "description": "Scans Unreal Engine assets in a specific path.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "path": {
                            "type": "string",
                            "description": "Content path to scan (e.g. /Game/Characters)"
                        }
                    },
                    "required": ["path"]
                }
            },
            {
                "name": "get_thumbnail",
                "description": "Retrieves the thumbnail of an asset as a base64 encoded PNG.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "asset_path": {
                            "type": "string",
                            "description": "Full path to the asset (e.g. /Game/Textures/MyTex.MyTex)"
                        }
                    },
                    "required": ["asset_path"]
                }
            },
            {
                "name": "create_pcg_graph",
                "description": "Creates a new PCG Graph asset.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "package_path": {"type": "string", "description": "Folder path (e.g. /Game/PCG)"},
                        "asset_name": {"type": "string", "description": "Name of the new asset"}
                    },
                    "required": ["package_path", "asset_name"]
                }
            },
            {
                "name": "add_pcg_node",
                "description": "Adds a node to a PCG Graph.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "graph_path": {"type": "string", "description": "Path to the PCG Graph asset"},
                        "node_class": {"type": "string", "description": "Unreal Python class name (e.g. PCGStaticMeshSpawnerSettings)"}
                    },
                    "required": ["graph_path", "node_class"]
                }
            },
            {
                "name": "connect_pcg_nodes",
                "description": "Connects two nodes in a PCG Graph.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "graph_path": {"type": "string", "description": "Path to the PCG Graph asset"},
                        "upstream_node": {"type": "string", "description": "Name of the source node"},
                        "downstream_node": {"type": "string", "description": "Name of the destination node"},
                        "upstream_pin": {"type": "string", "description": "Output pin name (default: Out)"},
                        "downstream_pin": {"type": "string", "description": "Input pin name (default: In)"}
                    },
                    "required": ["graph_path", "upstream_node", "downstream_node"]
                }
            },
            {
                "name": "set_pcg_node_properties",
                "description": "Sets properties on a PCG Node's settings object.",
                "inputSchema": {
                    "type": "object",
                    "properties": {
                        "graph_path": {"type": "string", "description": "Path to the PCG Graph asset"},
                        "node_name": {"type": "string", "description": "Name of the node to modify"},
                        "properties": {
                            "type": "object",
                            "description": "Dictionary of property names and values (e.g. {'debug': True})"
                        }
                    },
                    "required": ["graph_path", "node_name", "properties"]
                }
            }
        ]
                }
            }
            
        # 4. Call Tool
        elif method == "tools/call":
            tool_name = params.get("name")
            tool_args = params.get("arguments", {})
            
            if tool_name == "scan_assets":
                path = tool_args.get("path", "/Game")
                res_data = self.connect_and_send({"command": "scan_assets", "path": path})
                
                # Format text result
                content = [{"type": "text", "text": json.dumps(res_data, indent=2)}]
                return {"jsonrpc": "2.0", "id": msg_id, "result": {"content": content}}
                
            elif tool_name == "get_thumbnail":
                asset_path = tool_args.get("asset_path")
                res_data = self.connect_and_send({"command": "get_thumbnail", "asset_path": asset_path})
                
                content = []
                if "error" in res_data:
                     content = [{"type": "text", "text": f"Error: {res_data['error']}"}]
                elif "image_data" in res_data:
                     # MCP Image Format
                     content = [
                         {
                             "type": "image",
                             "data": res_data["image_data"],
                             "mimeType": "image/png"
                         }
                     ]
                
                return {"jsonrpc": "2.0", "id": msg_id, "result": {"content": content}}
                
            elif tool_name in ["create_pcg_graph", "add_pcg_node", "connect_pcg_nodes", "set_pcg_node_properties"]:
                # Pass through directly mapping arguments
                payload = {"command": tool_name}
                payload.update(tool_args)
                res_data = self.connect_and_send(payload)
                
                content = []
                if "error" in res_data:
                    content = [{"type": "text", "text": f"Error: {res_data['error']}"}]
                else:
                    content = [{"type": "text", "text": json.dumps(res_data, indent=2)}]
                
                return {"jsonrpc": "2.0", "id": msg_id, "result": {"content": content}}
                
            else:
                 return {
                    "jsonrpc": "2.0",
                    "id": msg_id,
                    "error": {"code": -32601, "message": "Method not found"}
                }
                
        # Ping
        elif method == "ping":
             return {"jsonrpc": "2.0", "id": msg_id, "result": {}}

        return None

if __name__ == "__main__":
    bridge = UnrealMCPBridge()
    bridge.run()
