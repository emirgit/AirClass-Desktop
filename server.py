import asyncio
import websockets
import json
import logging
import sys
from enum import Enum
from typing import Dict, Set, Any
import qrcode
import base64
from io import BytesIO

# Configure logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

# Client type enum
class ClientType(Enum):
    HARDWARE = "HARDWARE"
    DESKTOP = "DESKTOP"
    MOBILE = "MOBILE"
    UNKNOWN = "UNKNOWN"

# Client info class
class ClientInfo:
    def __init__(self, websocket):
        self.websocket = websocket
        self.type = ClientType.UNKNOWN
        self.id = ""

# Classroom class to manage rooms
class Classroom:
    def __init__(self, room_id):
        self.room_id = room_id
        self.clients = set()  # Set of clients in the room

    def add_client(self, client):
        self.clients.add(client)

    def remove_client(self, client):
        self.clients.discard(client)

    def is_empty(self):
        return len(self.clients) == 0

    def get_client_ids(self):
        return [client.id for client in self.clients]

# WebSocket server class
class ClassroomWebSocketServer:
    def __init__(self):
        self.connections = {}  # Map of websocket -> ClientInfo
        self.classrooms = {}   # Map of room_id -> Classroom
        self.next_id = 1

    async def register(self, websocket):
        """Register a new client connection"""
        logging.info("New connection opened")
        self.connections[websocket] = ClientInfo(websocket)

    async def unregister(self, websocket):
        """Unregister a client connection"""
        logging.info("Connection closed")
        if websocket in self.connections:
            client = self.connections[websocket]
            # Remove client from any associated classroom
            for room_id, classroom in list(self.classrooms.items()):
                classroom.remove_client(client)
                if classroom.is_empty():
                    del self.classrooms[room_id]
                    logging.info(f"Room {room_id} deleted (empty)")
            del self.connections[websocket]

    def client_type_to_string(self, client_type):
        """Convert client type enum to string"""
        return client_type.value

    async def register_client(self, websocket, data):
        """Handle client registration"""
        client = self.connections[websocket]
        if data["register"] == "hardware":
            client.type = ClientType.HARDWARE
            client.id = data.get("id", f"hardware-{self.next_id}")
        elif data["register"] == "desktop":
            client.type = ClientType.DESKTOP
            client.id = data.get("id", f"desktop-{self.next_id}")
        elif data["register"] == "mobile":
            client.type = ClientType.MOBILE
            client.id = data.get("id", f"mobile-{self.next_id}")
        self.next_id += 1

        logging.info(f"Client registered as {self.client_type_to_string(client.type)} with ID: {client.id}")
        confirmation = {
            "type": "registration_success",
            "client_type": self.client_type_to_string(client.type),
            "client_id": client.id
        }
        await websocket.send(json.dumps(confirmation))

    async def handle_message(self, websocket, message):
        """Process incoming message"""
        try:
            client = self.connections[websocket]
            data = json.loads(message)

            if client.type == ClientType.UNKNOWN and "register" in data:
                await self.register_client(websocket, data)
                return

            if "type" in data:
                msg_type = data["type"]
                logging.info(f"Received message type: {msg_type} from {self.client_type_to_string(client.type)} ID: {client.id}")

                if msg_type == "identify" and "roomId" in data:
                    room_id = data["roomId"]
                    if room_id not in self.classrooms:
                        self.classrooms[room_id] = Classroom(room_id)
                    classroom = self.classrooms[room_id]
                    classroom.add_client(client)
                    logging.info(f"Client {client.id} joined room {room_id}")
                    
                    
                    
                    qr = qrcode.QRCode(version=1, box_size=10, border=5)
                    qr.add_data(room_id)
                    qr.make(fit=True)
                    img = qr.make_image(fill_color="black", back_color="white")
                    img.save(f"room_{room_id}.png")

                    logging.info(f"QR code for room {room_id} generated and saved as room_{room_id}.png")

                    buffered = BytesIO()
                    img.save(buffered, format="PNG")
                    base64_image = base64.b64encode(buffered.getvalue()).decode("utf-8")
        

                    # Send confirmation back to the client
                    confirmation = {
                        "type": "room_joined",
                        "room_id": room_id,
                        "client_id": client.id,
                        "qr_code": base64_image
                    }
                    await websocket.send(json.dumps(confirmation))
                else:
                    logging.warning(f"Unhandled message type: {msg_type}")
        except json.JSONDecodeError:
            logging.error("Error decoding JSON message")
            error = {"type": "error", "message": "Invalid message format"}
            await websocket.send(json.dumps(error))
        except Exception as e:
            logging.error(f"Error processing message: {str(e)}")
            error = {"type": "error", "message": "Error processing message"}
            await websocket.send(json.dumps(error))

    async def handle_connection(self, websocket):#*/, path):
        """Main connection handler"""
        await self.register(websocket)
        try:
            async for message in websocket:
                await self.handle_message(websocket, message)
        except websockets.exceptions.ConnectionClosed:
            pass
        finally:
            await self.unregister(websocket)

    async def run(self, host="0.0.0.0", port=9090):
        """Start the WebSocket server"""
        server = await websockets.serve(self.handle_connection, host, port)
        logging.info(f"WebSocket Server started on {host}:{port}")
        await server.wait_closed()

if __name__ == "__main__":
    port = 9090
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    try:
        server = ClassroomWebSocketServer()
        asyncio.run(server.run(port=port))
    except KeyboardInterrupt:
        logging.info("Server stopped")
    except Exception as e:
        logging.error(f"Exception: {str(e)}")