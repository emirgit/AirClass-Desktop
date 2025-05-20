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
    level=logging.DEBUG,
    format='%(asctime)s - %(levelname)s - %(message)s'
)

# Client type enum
class ClientType(Enum):
    HARDWARE = "HARDWARE"
    DESKTOP = "DESKTOP"
    MOBILE = "MOBILE"
    UNKNOWN = "UNKNOWN"

# Gesture types enum
class GestureType(Enum):
    HAND_RAISE = "HAND_RAISE"
    THUMB_UP = "THUMB_UP"
    THUMB_DOWN = "THUMB_DOWN"
    WAVE = "WAVE"
    ZOOM_IN = "ZOOM_IN"
    ZOOM_OUT = "ZOOM_OUT"
    UNKNOWN = "UNKNOWN"

# Client info class
class ClientInfo:
    def __init__(self, websocket):
        self.websocket = websocket
        self.type = ClientType.UNKNOWN
        self.id = ""
        self.current_gesture = GestureType.UNKNOWN
        self.last_gesture_time = None

# Classroom class to manage rooms
class Classroom:
    def __init__(self, room_id):
        self.room_id = room_id
        self.clients = set()  # Set of clients in the room
        self.gesture_history = []  # List to store gesture events

    def add_client(self, client):
        self.clients.add(client)

    def remove_client(self, client):
        self.clients.discard(client)

    def is_empty(self):
        return len(self.clients) == 0

    def get_client_ids(self):
        return [client.id for client in self.clients]

    def add_gesture_event(self, client_id, gesture_type, timestamp):
        self.gesture_history.append({
            "client_id": client_id,
            "gesture": gesture_type,
            "timestamp": timestamp
        })
        logging.info(f"Gesture recorded in room {self.room_id}: {client_id} performed {gesture_type}")

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

    async def handle_gesture(self, websocket, data):
        """Handle incoming gesture data"""
        client = self.connections[websocket]
        try:
            gesture_type = data.get("gesture_type")
            timestamp = data.get("timestamp")
            
            if not gesture_type or not timestamp:
                raise ValueError("Missing gesture_type or timestamp")

            # Update client's current gesture
            client.current_gesture = GestureType(gesture_type)
            client.last_gesture_time = timestamp

            # Find the classroom this client belongs to
            for room_id, classroom in self.classrooms.items():
                if client in classroom.clients:
                    classroom.add_gesture_event(client.id, gesture_type, timestamp)
                    
                    # Broadcast gesture to all clients in the room
                    gesture_message = {
                        "type": "gesture",
                        "client_id": client.id,
                        "gesture_type": gesture_type,
                        "timestamp": timestamp
                    }
                    
                    for room_client in classroom.clients:
                        if room_client.websocket != websocket:  # Don't send back to sender
                            await room_client.websocket.send(json.dumps(gesture_message))
                    
                    logging.info(f"Gesture {gesture_type} from {client.id} processed and broadcasted")
                    break

        except ValueError as e:
            logging.error(f"Invalid gesture data: {str(e)}")
            error = {"type": "error", "message": "Invalid gesture data format"}
            await websocket.send(json.dumps(error))

    async def handle_page_navigation(self, websocket, data):
        """Handle page navigation commands"""
        client = self.connections[websocket]
        try:
            action = data.get("action")
            timestamp = data.get("timestamp")
            
            if not action or not timestamp:
                raise ValueError("Missing action or timestamp")

            # Find the classroom this client belongs to
            for room_id, classroom in self.classrooms.items():
                if client in classroom.clients:
                    # Broadcast navigation command to all clients in the room
                    navigation_message = {
                        "type": "page_navigation",
                        "client_id": client.id,
                        "action": action,
                        "timestamp": timestamp
                    }
                    
                    for room_client in classroom.clients:
                        if room_client.websocket != websocket:  # Don't send back to sender
                            await room_client.websocket.send(json.dumps(navigation_message))
                    
                    logging.info(f"Page navigation {action} from {client.id} processed and broadcasted")
                    break

        except ValueError as e:
            logging.error(f"Invalid navigation data: {str(e)}")
            error = {"type": "error", "message": "Invalid navigation data format"}
            await websocket.send(json.dumps(error))

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

                    confirmation = {
                        "type": "room_joined",
                        "room_id": room_id,
                        "client_id": client.id,
                        "qr_code": base64_image
                    }
                    await websocket.send(json.dumps(confirmation))
                elif msg_type == "gesture":
                    await self.handle_gesture(websocket, data)
                elif msg_type == "page_navigation":
                    await self.handle_page_navigation(websocket, data)
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

    async def handle_connection(self, websocket):
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
        async with websockets.serve(self.handle_connection, host, port):
            logging.info(f"WebSocket server started on {host}:{port}")
            await asyncio.Future()  # run forever

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