from bleak import BleakClient

class BLETransmitter:
    def __init__(self, address, char_uuid):
        self.address = address
        self.char_uuid = char_uuid
        self.client = None

    async def connect(self):
        self.client = BleakClient(self.address)
        await self.client.connect()
        print("BLE connected.")

    async def send(self, offset, direction, timestamp):
        if not self.client or not self.client.is_connected:
            return  # Prevent crash if not connected
        
        message = f"{offset}, {direction}, {timestamp}"

        try:
            await self.client.write_gatt_char(self.char_uuid, message.encode())
        except Exception as e:
            print(f"[BLE] Write error: {e}")

    async def disconnect(self):
        if self.client:
            await self.client.disconnect()
            print("BLE disconnected.")
