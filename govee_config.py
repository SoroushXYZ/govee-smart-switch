"""
Shared configuration for Govee API interactions.
"""
import os
from dotenv import load_dotenv

# Load environment variables from .env file
load_dotenv()

# Govee API Configuration
GOVEE_API_KEY = os.getenv('GOVEE_API_KEY')
GOVEE_API_BASE_URL = 'https://developer-api.govee.com'

# API Headers
def get_headers():
    """Get the standard headers for Govee API requests."""
    if not GOVEE_API_KEY:
        raise ValueError("GOVEE_API_KEY not found in environment variables. Please create a .env file with your API key.")
    
    return {
        'Govee-API-Key': GOVEE_API_KEY,
        'Content-Type': 'application/json'
    }
