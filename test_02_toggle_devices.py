"""
Test script 02: Toggle both Govee devices on/off.
Press Enter to toggle the state of both devices. Press Ctrl+C to exit.
"""
import requests
import json
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from govee_config import get_headers, GOVEE_API_BASE_URL

def get_devices():
    """Fetch all Govee devices."""
    url = f"{GOVEE_API_BASE_URL}/v1/devices"
    
    try:
        response = requests.get(url, headers=get_headers())
        data = response.json()
        
        if data.get('code') == 200:
            devices = data.get('data', {}).get('devices', [])
            # Filter only controllable devices
            controllable_devices = [d for d in devices if d.get('controllable', False)]
            return controllable_devices
        else:
            print(f"Error fetching devices: {data.get('message', 'Unknown error')}")
            return []
    except Exception as e:
        print(f"Error: {e}")
        return []

def control_device(device_id, model, command_name, value, retry_count=3):
    """Control a single device with retry logic."""
    url = f"{GOVEE_API_BASE_URL}/v1/devices/control"
    
    payload = {
        "device": device_id,
        "model": model,
        "cmd": {
            "name": command_name,
            "value": value
        }
    }
    
    for attempt in range(retry_count):
        try:
            # Try PUT first (most common for control endpoints)
            response = requests.put(url, headers=get_headers(), json=payload, timeout=10)
            
            # If PUT fails, try POST
            if response.status_code == 405:
                response = requests.post(url, headers=get_headers(), json=payload, timeout=10)
            
            # Check if response has content
            if not response.text or response.text.strip() == '':
                if attempt < retry_count - 1:
                    wait_time = (attempt + 1) * 0.5  # Exponential backoff
                    time.sleep(wait_time)
                    continue
                return False, "Empty response from API (possible rate limit)"
            
            # Check status code
            if response.status_code == 429:
                # Rate limited - wait longer
                wait_time = (attempt + 1) * 2
                print(f"    Rate limited, waiting {wait_time}s...")
                time.sleep(wait_time)
                continue
            
            # Try to parse JSON
            try:
                data = response.json()
            except json.JSONDecodeError:
                if attempt < retry_count - 1:
                    wait_time = (attempt + 1) * 0.5
                    time.sleep(wait_time)
                    continue
                return False, f"Invalid JSON response (status {response.status_code}): {response.text[:100]}"
            
            if data.get('code') == 200:
                return True, None
            else:
                error_msg = data.get('message', 'Unknown error')
                error_code = data.get('code', 'N/A')
                
                # If rate limited, retry
                if error_code == 429 or 'rate limit' in error_msg.lower():
                    if attempt < retry_count - 1:
                        wait_time = (attempt + 1) * 2
                        print(f"    Rate limited, waiting {wait_time}s...")
                        time.sleep(wait_time)
                        continue
                
                return False, f"{error_msg} (code: {error_code})"
                
        except requests.exceptions.Timeout:
            if attempt < retry_count - 1:
                wait_time = (attempt + 1) * 0.5
                time.sleep(wait_time)
                continue
            return False, "Request timeout"
        except requests.exceptions.RequestException as e:
            if attempt < retry_count - 1:
                wait_time = (attempt + 1) * 0.5
                time.sleep(wait_time)
                continue
            return False, f"Request error: {str(e)}"
        except Exception as e:
            return False, f"Unexpected error: {str(e)}"
    
    return False, "Max retries exceeded"

def toggle_devices(devices, turn_on):
    """Toggle all devices to the specified state in parallel."""
    state = "on" if turn_on else "off"
    state_text = "ON" if turn_on else "OFF"
    
    print(f"\nTurning devices {state_text} (parallel)...")
    
    # Use ThreadPoolExecutor to send requests in parallel
    results = {}
    with ThreadPoolExecutor(max_workers=len(devices)) as executor:
        # Submit all device control tasks
        future_to_device = {
            executor.submit(
                control_device,
                device.get('device'),
                device.get('model'),
                "turn",
                state
            ): device for device in devices
        }
        
        # Collect results as they complete
        for future in as_completed(future_to_device):
            device = future_to_device[future]
            device_name = device.get('deviceName', 'Unknown')
            
            try:
                success, error = future.result()
                results[device_name] = (success, error)
                
                if success:
                    print(f"  ✓ {device_name}: {state_text}")
                else:
                    print(f"  ✗ {device_name}: Failed - {error}")
            except Exception as e:
                results[device_name] = (False, str(e))
                print(f"  ✗ {device_name}: Exception - {e}")
    
    return all(success for success, _ in results.values())

def main():
    """Main loop to toggle devices."""
    print("=" * 60)
    print("Govee Device Toggle Control")
    print("=" * 60)
    
    # Get devices
    print("\nFetching devices...")
    devices = get_devices()
    
    if not devices:
        print("No controllable devices found!")
        return
    
    if len(devices) < 2:
        print(f"Warning: Only {len(devices)} device(s) found. Expected 2.")
    
    print(f"\nFound {len(devices)} controllable device(s):")
    for i, device in enumerate(devices, 1):
        print(f"  {i}. {device.get('deviceName')} ({device.get('model')})")
    
    # Get initial state (assume off to start)
    current_state = False  # False = off, True = on
    
    print("\n" + "=" * 60)
    print("Press Enter to toggle devices (Ctrl+C to exit)")
    print("=" * 60)
    
    try:
        while True:
            # Wait for Enter key
            input()
            
            # Toggle state
            current_state = not current_state
            
            # Control devices
            success = toggle_devices(devices, current_state)
            
            if success:
                state_text = "ON" if current_state else "OFF"
                print(f"\n✓ All devices are now {state_text}")
            else:
                print(f"\n⚠ Some devices may have failed to toggle")
            
            print("\nPress Enter again to toggle, or Ctrl+C to exit...")
            
    except KeyboardInterrupt:
        print("\n\nExiting...")
    except Exception as e:
        print(f"\nError: {e}")

if __name__ == "__main__":
    main()
