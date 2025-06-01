# WebSocket Support for Camera Control

This implementation adds WebSocket server functionality to the camera control application, allowing remote control of Sony cameras through WebSocket connections.

## Building

Make sure you have the following dependencies installed:
- libusb (via Homebrew: `brew install libusb`)
- OpenSSL (via Homebrew: `brew install openssl`)

Build the project:
```bash
make clean
make
```

## Starting the WebSocket Server

To start the WebSocket server on the default port (8080):
```bash
./out/bin/control websocket
```

To specify a custom port:
```bash
./out/bin/control websocket 9000
```

You can also specify USB bus and device numbers:
```bash
./out/bin/control websocket 8080 --bus=0 --dev=0
```

## WebSocket Protocol

The WebSocket server accepts text messages in the format: `command:parameters`

### Available Commands

#### Connection Commands
- `open:` - Connect to the camera
- `close:` - Disconnect from the camera
- `auth:` - Authenticate with the camera

#### Basic Commands
- `reset:` - Reset the device
- `clear:` - Clear halt condition
- `wait:` - Wait for camera events

#### Get Commands
- `getall:` - Get all device properties
- `get:0xPropertyCode` - Get specific property (e.g., `get:0xD6C6`)
- `getobject:0xHandle` - Get object by handle (e.g., `getobject:0xFFFFC001`)
- `getliveview:` - Get live view stream

#### PTP Transaction Commands
- `send:op=0x1001,p1=0x0,p2=0x0,data=0x1,size=2` - Send PTP command
- `recv:op=0x1008,p1=0xFFFFC001` - Receive PTP data

### Response Format

Responses are returned as JSON:

Success response:
```json
{"success": true, "result": "Device opened successfully"}
```

Error response:
```json
{"error": "Device not connected"}
```

Transaction response:
```json
{
  "code": "0x2001",
  "nparam": 2,
  "params": ["0x0", "0x0"],
  "size": 0,
  "data": "0x0"
}
```

## Testing

Open the included `websocket_test.html` file in a web browser to test the WebSocket functionality. The test client provides:
- Connection management
- Button interface for all commands
- Real-time log display
- Parameter input fields for PTP commands

## Example Usage

1. Start the server:
   ```bash
   ./out/bin/control websocket
   ```

2. Open `websocket_test.html` in a browser

3. Click "Connect" to establish WebSocket connection

4. Click "Open Device" to connect to the camera

5. Click "Authenticate" to authenticate with the camera

6. Use other commands to control the camera

## WebSocket Client Example (JavaScript)

```javascript
const ws = new WebSocket('ws://localhost:8080');

ws.onopen = () => {
    // Connect to camera
    ws.send('open:');
};

ws.onmessage = (event) => {
    const response = JSON.parse(event.data);
    console.log('Received:', response);
    
    if (response.success) {
        // Authenticate after successful connection
        ws.send('auth:');
    }
};

// Send a PTP command
ws.send('send:op=0x1001,p1=0x0,data=0x1,size=2');
```

## Notes

- The WebSocket server runs on a single thread and handles one client at a time
- Camera connection is maintained across WebSocket messages
- The server will continue running until interrupted (Ctrl+C)
- All existing control commands are available through WebSocket