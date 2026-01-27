# Govee Smart Switch - ESP32 Control

This project allows you to control Govee light bulbs using an ESP32. We start by testing the Govee API with Python to understand available options.

## Setup

1. **Create and activate virtual environment:**
   ```bash
   python3 -m venv venv
   source venv/bin/activate  # On macOS/Linux
   # or
   venv\Scripts\activate  # On Windows
   ```

2. **Install dependencies:**
   ```bash
   pip install -r requirements.txt
   ```

3. **Set up your API key:**
   ```bash
   cp .env.example .env
   # Then edit .env and add your Govee API key
   ```

## Project Structure

- `govee_config.py` - Shared API configuration and helper functions
- `test_01_list_devices.py` - First test script to list all available devices
- `.env` - Your API key (not committed to git)
- `.env.example` - Template for API key configuration

## Running Tests

Activate your virtual environment, then run:

```bash
python test_01_list_devices.py
```

This will list all your Govee devices and show what commands are available for each device.

## Next Steps

After running the first test, you can create additional test scripts (e.g., `test_02_control_device.py`, `test_03_get_state.py`) to explore different API capabilities.
