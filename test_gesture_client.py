import asyncio
import websockets
import json
import datetime

async def test_gesture_client():
    uri = "ws://localhost:9090"
    async with websockets.connect(uri) as websocket:
        # First register as a desktop client
        register_message = {
            "register": "desktop",
            "id": "test-desktop-1"
        }
        await websocket.send(json.dumps(register_message))
        response = await websocket.recv()
        print(f"Registration response: {response}")

        # Join the same room as Qt application
        join_message = {
            "type": "identify",
            "roomId": "airclass-123"
        }
        await websocket.send(json.dumps(join_message))
        response = await websocket.recv()
        print(f"Join room response: {response}")

        # Test page navigation commands
        navigation_commands = [
            {
                "type": "page_navigation",
                "action": "next",
                "timestamp": datetime.datetime.now().isoformat()
            },
            {
                "type": "page_navigation",
                "action": "previous",
                "timestamp": datetime.datetime.now().isoformat()
            }
        ]

        print("\nTesting page navigation commands...")
        for command in navigation_commands:
            print(f"\nSending navigation command: {command['action']}")
            await websocket.send(json.dumps(command))
            
            try:
                response = await asyncio.wait_for(websocket.recv(), timeout=1.0)
                print(f"Received response: {response}")
            except asyncio.TimeoutError:
                print("No response received for this navigation command")
            
            await asyncio.sleep(2)

        # Test zoom commands
        zoom_commands = [
            {
                "type": "gesture",
                "gesture_type": "ZOOM_IN",
                "client_id": "test-desktop-1",
                "timestamp": datetime.datetime.now().isoformat()
            },
            {
                "type": "gesture",
                "gesture_type": "ZOOM_OUT",
                "client_id": "test-desktop-1",
                "timestamp": datetime.datetime.now().isoformat()
            }
        ]

        print("\nTesting zoom commands...")
        for command in zoom_commands:
            print(f"\nSending zoom command: {command['gesture_type']}")
            await websocket.send(json.dumps(command))
            
            try:
                response = await asyncio.wait_for(websocket.recv(), timeout=1.0)
                print(f"Received response: {response}")
            except asyncio.TimeoutError:
                print("No response received for this zoom command")
            
            await asyncio.sleep(2)

        # Send test gestures
        test_gestures = [
            "HAND_RAISE",
            "THUMB_UP",
            "THUMB_DOWN",
            "WAVE"
        ]

        print("\nTesting gestures...")
        for gesture in test_gestures:
            gesture_message = {
                "type": "gesture",
                "gesture_type": gesture,
                "timestamp": datetime.datetime.now().isoformat()
            }
            print(f"\nSending gesture: {gesture}")
            await websocket.send(json.dumps(gesture_message))
            
            # Wait for any responses
            try:
                response = await asyncio.wait_for(websocket.recv(), timeout=1.0)
                print(f"Received response: {response}")
            except asyncio.TimeoutError:
                print("No response received for this gesture")

            # Wait a bit between gestures
            await asyncio.sleep(2)

if __name__ == "__main__":
    asyncio.run(test_gesture_client()) 