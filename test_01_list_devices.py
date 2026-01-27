"""
Test script 01: List all available Govee devices.
This script will fetch and display all devices associated with your API key.
"""
import requests
from govee_config import get_headers, GOVEE_API_BASE_URL

def list_devices():
    """Fetch and display all Govee devices."""
    url = f"{GOVEE_API_BASE_URL}/v1/devices"
    
    try:
        headers = get_headers()
        response = requests.get(url, headers=headers)
        
        data = response.json()
        
        print("=" * 60)
        print("Govee Devices List")
        print("=" * 60)
        
        if data.get('code') == 200:
            devices = data.get('data', {}).get('devices', [])
            
            if not devices:
                print("No devices found.")
                return
            
            print(f"\nFound {len(devices)} device(s):\n")
            
            for i, device in enumerate(devices, 1):
                print(f"Device {i}:")
                print(f"  Model: {device.get('model')}")
                print(f"  Device Name: {device.get('deviceName')}")
                print(f"  Device ID: {device.get('device')}")
                print(f"  Controllable: {device.get('controllable', False)}")
                print(f"  Retrievable: {device.get('retrievable', False)}")
                print(f"  Support Commands: {', '.join(device.get('supportCmds', []))}")
                print()
            
            # Print full JSON for reference
            print("=" * 60)
            print("Full API Response (JSON):")
            print("=" * 60)
            import json
            print(json.dumps(data, indent=2))
            
        else:
            print(f"Error: {data.get('message', 'Unknown error')}")
            print(f"Code: {data.get('code')}")
            print("\nFull response:")
            import json
            print(json.dumps(data, indent=2))
            
    except requests.exceptions.RequestException as e:
        print(f"Request error: {e}")
    except ValueError as e:
        print(f"Configuration error: {e}")

if __name__ == "__main__":
    list_devices()
