#!/bin/bash

# Script to demonstrate controlling multiple Sony FX30 cameras
# This script shows how to use the enhanced control program with multiple cameras

echo "=== Sony FX30 Multi-Camera Control Script ==="
echo

# First, list all available Sony cameras
echo "1. Listing all Sony cameras:"
./out/bin/control listsony
echo

# Function to control a specific camera by index
control_camera() {
    local camera_index=$1
    local command=$2
    local params=$3
    
    echo "Controlling FX30 camera #${camera_index}: ${command}"
    ./out/bin/control $command --fx30 --camera-index=$camera_index $params
    echo
}

# Function to start WebSocket server for a specific camera
start_websocket_for_camera() {
    local camera_index=$1
    local port=$2
    
    echo "Starting WebSocket server for FX30 camera #${camera_index} on port ${port}"
    echo "Use the following URL in your WebSocket client: ws://localhost:${port}"
    echo "Press Ctrl+C to stop the server"
    ./out/bin/control websocket $port --fx30 --camera-index=$camera_index
}

# Check if we have any arguments
if [ $# -eq 0 ]; then
    echo "Usage examples:"
    echo
    echo "List cameras:"
    echo "  $0 list"
    echo
    echo "Open specific camera:"
    echo "  $0 open 0        # Open camera at index 0"
    echo "  $0 open 1        # Open camera at index 1"
    echo
    echo "Start WebSocket server for specific camera:"
    echo "  $0 websocket 0 8080   # Camera 0 on port 8080"
    echo "  $0 websocket 1 8081   # Camera 1 on port 8081"
    echo
    echo "Send command to specific camera:"
    echo "  $0 auth 0        # Authenticate with camera 0"
    echo "  $0 getall 1      # Get all properties from camera 1"
    echo
    echo "Control all cameras simultaneously:"
    echo "  $0 openall       # Open all FX30 cameras"
    echo "  $0 authall       # Authenticate all FX30 cameras"
    exit 1
fi

case $1 in
    "list")
        echo "Listing all Sony cameras:"
        ./out/bin/control listsony
        ;;
    
    "open")
        if [ -z "$2" ]; then
            echo "Error: Camera index required. Usage: $0 open <camera_index>"
            exit 1
        fi
        control_camera $2 "open"
        ;;
    
    "close")
        if [ -z "$2" ]; then
            echo "Error: Camera index required. Usage: $0 close <camera_index>"
            exit 1
        fi
        control_camera $2 "close"
        ;;
    
    "auth")
        if [ -z "$2" ]; then
            echo "Error: Camera index required. Usage: $0 auth <camera_index>"
            exit 1
        fi
        control_camera $2 "auth"
        ;;
    
    "getall")
        if [ -z "$2" ]; then
            echo "Error: Camera index required. Usage: $0 getall <camera_index>"
            exit 1
        fi
        control_camera $2 "getall"
        ;;
    
    "websocket")
        if [ -z "$2" ] || [ -z "$3" ]; then
            echo "Error: Camera index and port required. Usage: $0 websocket <camera_index> <port>"
            exit 1
        fi
        start_websocket_for_camera $2 $3
        ;;
    
    "openall")
        echo "Opening all FX30 cameras..."
        # Get the number of FX30 cameras
        camera_count=$(./out/bin/control listsony | grep "FX30:" | sed 's/.*FX30: \([0-9]*\).*/\1/')
        if [ -z "$camera_count" ] || [ "$camera_count" -eq 0 ]; then
            echo "No FX30 cameras found."
            exit 1
        fi
        
        for ((i=0; i<camera_count; i++)); do
            echo "Opening FX30 camera #$i..."
            control_camera $i "open"
        done
        ;;
    
    "authall")
        echo "Authenticating all FX30 cameras..."
        # Get the number of FX30 cameras
        camera_count=$(./out/bin/control listsony | grep "FX30:" | sed 's/.*FX30: \([0-9]*\).*/\1/')
        if [ -z "$camera_count" ] || [ "$camera_count" -eq 0 ]; then
            echo "No FX30 cameras found."
            exit 1
        fi
        
        for ((i=0; i<camera_count; i++)); do
            echo "Authenticating FX30 camera #$i..."
            control_camera $i "auth"
        done
        ;;
    
    *)
        echo "Unknown command: $1"
        echo "Run '$0' without arguments to see usage examples."
        exit 1
        ;;
esac