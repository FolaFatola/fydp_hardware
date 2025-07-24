from picamera2 import Picamera2
import time

import video_feed as v
import ble_transmission as b

# replace, obviously
ESP32_BLE_ADDRESS = "AA:BB:CC:DD:EE:FF"
# I just generated this online
CHARACTERISTIC_UUID = "33e1f35a-6fe6-481c-8ca1-30b84bb4eab1"

async def main():
    # Connect to ESP32 over BLE
    ble = b.BLETransmitter(ESP32_BLE_ADDRESS, CHARACTERISTIC_UUID)
    await ble.connect()
    # Initialize the camera
    camera = Picamera2()
    config = camera.create_preview_configuration(main={"size": (1280, 720), "format": "BGR888"})
    camera.configure(config)
    camera.start()
    # Allow camera to warm up
    time.sleep(2)

    try:
        while True:
            # Capture frame
            frame = camera.capture_array()
            offset, direction, timestamp = v.video_pipeline(frame)
            # Send msg
            await ble.send(offset, direction, timestamp)
    finally:
        await ble.disconnect()
        camera.stop()

if __name__ == "__main__":
    main()