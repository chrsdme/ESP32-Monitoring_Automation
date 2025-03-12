# Mushroom Tent Controller API Documentation

## API Overview

The Mushroom Tent Controller provides a RESTful API that allows you to interact with the system programmatically. This document outlines all available endpoints, their expected parameters, and response formats.

All API endpoints are accessible at `http://<device-ip>:<port>/api/` or `http://<hostname>.local:<port>/api/`.

## Authentication

Most API endpoints require HTTP Basic Authentication using the credentials configured during setup. Include the following header with your requests:

```
Authorization: Basic <base64-encoded-credentials>
```

Where `<base64-encoded-credentials>` is the Base64 encoding of `username:password`.

## API Endpoints

### Sensor Data

#### Get Current Sensor Readings

```
GET /api/sensors/data
```

Returns the current readings from all sensors.

**Response:**
```json
{
  "upper_dht": {
    "temperature": 22.5,
    "humidity": 75.8,
    "valid": true
  },
  "lower_dht": {
    "temperature": 22.1,
    "humidity": 76.2,
    "valid": true
  },
  "scd": {
    "temperature": 22.3,
    "humidity": 75.5,
    "co2": 950,
    "valid": true
  },
  "average": {
    "temperature": 22.3,
    "humidity": 75.83
  },
  "thresholds": {
    "humidity_low": 50.0,
    "humidity_high": 85.0,
    "temperature_low": 20.0,
    "temperature_high": 24.0,
    "co2_low": 1000.0,
    "co2_high": 1600.0
  }
}
```

#### Get Sensor Graph Data

```
GET /api/sensors/graph?type=<type>&points=<num_points>
```

Returns historical sensor data for graphing.

**Parameters:**
- `type`: Data type (0 = temperature, 1 = humidity, 2 = CO2)
- `points`: Maximum number of data points to return (default: 100)

**Response:**
```json
{
  "upper_dht": [22.1, 22.2, 22.3, 22.4, 22.5],
  "lower_dht": [21.9, 22.0, 22.1, 22.2, 22.3],
  "scd": [22.2, 22.3, 22.4, 22.5, 22.6],
  "timestamps": [1615482000, 1615482300, 1615482600, 1615482900, 1615483200]
}
```

### Relay Control

#### Get Relay Status

```
GET /api/relays/status
```

Returns the status of all relays.

**Response:**
```json
{
  "relays": [
    {
      "id": 1,
      "name": "Main PSU",
      "pin": 16,
      "visible": false,
      "is_on": true,
      "state": 2,
      "last_trigger": 1,
      "operating_time": {
        "start_hour": 0,
        "start_minute": 0,
        "end_hour": 23,
        "end_minute": 59
      }
    },
    {
      "id": 2,
      "name": "UV Light",
      "pin": 17,
      "visible": true,
      "depends_on": 1,
      "is_on": false,
      "state": 2,
      "last_trigger": 0,
      "operating_time": {
        "start_hour": 10,
        "start_minute": 0,
        "end_hour": 14,
        "end_minute": 0
      }
    },
    // Additional relays...
  ],
  "cycle_config": {
    "on_duration": 5,
    "interval": 60
  },
  "override_duration": 5
}
```

#### Set Relay State

```
POST /api/relays/set
```

Controls a specific relay.

**Request Body:**
```json
{
  "relay_id": 2,
  "state": 1
}
```

The `state` parameter accepts the following values:
- `0`: Off
- `1`: On
- `2`: Auto (follows environmental triggers and schedules)

**Response:**
```json
{
  "success": true,
  "message": "Relay state updated"
}
```

### Environmental Settings

#### Get Environmental Thresholds

```
GET /api/environment/thresholds
```

Returns the current environmental threshold settings.

**Response:**
```json
{
  "humidity_low": 50.0,
  "humidity_high": 85.0,
  "temperature_low": 20.0,
  "temperature_high": 24.0,
  "co2_low": 1000.0,
  "co2_high": 1600.0
}
```

#### Update Environmental Thresholds

```
POST /api/environment/update
```

Updates the environmental threshold settings.

**Request Body:**
```json
{
  "humidity_low": 60.0,
  "humidity_high": 90.0,
  "temperature_low": 21.0,
  "temperature_high": 25.0,
  "co2_low": 800.0,
  "co2_high": 1400.0
}
```

**Response:**
```json
{
  "success": true,
  "message": "Environmental thresholds updated"
}
```

### System Settings

#### Get System Settings

```
GET /api/settings
```

Returns the current system settings.

**Response:**
```json
{
  "logging": {
    "level": 0,
    "max_size": 50,
    "flush_interval": 60,
    "log_server": "",
    "sensor_error_count": 5
  },
  "sensors": {
    "upper_dht_pin": 13,
    "lower_dht_pin": 14,
    "scd_sda_pin": 21,
    "scd_scl_pin": 22,
    "dht_interval": 5,
    "scd_interval": 10,
    "graph_interval": 30,
    "graph_points": 100
  },
  "reboot_scheduler": {
    "enabled": false,
    "day_of_week": 0,
    "hour": 3,
    "minute": 0
  },
  "sleep_mode": {
    "mode": 0,
    "from": "00:00",
    "to": "06:00"
  }
}
```

#### Update System Settings

```
POST /api/settings/update
```

Updates the system settings.

**Request Body:**
```json
{
  "logging": {
    "level": 1,
    "max_size": 100,
    "flush_interval": 30,
    "log_server": "192.168.1.100:8080",
    "sensor_error_count": 3
  },
  "sensors": {
    "dht_interval": 10,
    "scd_interval": 20,
    "graph_interval": 60,
    "graph_points": 120
  },
  "reboot_scheduler": {
    "enabled": true,
    "day_of_week": 0,
    "hour": 4,
    "minute": 30
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Settings updated"
}
```

### Network Configuration

#### Get Network Configuration

```
GET /api/network/config
```

Returns the current network configuration.

**Response:**
```json
{
  "wifi": {
    "ssid1": "HomeWiFi",
    "ssid2": "BackupWiFi",
    "ssid3": "",
    "hostname": "mushroom",
    "current_ssid": "HomeWiFi",
    "ip_address": "192.168.1.120",
    "rssi": -58,
    "ip_config": {
      "dhcp": true,
      "ip": "",
      "gateway": "",
      "subnet": "",
      "dns1": "",
      "dns2": ""
    },
    "watchdog": {
      "min_rssi": -80,
      "check_interval": 30
    }
  },
  "mqtt": {
    "enabled": false,
    "broker": "192.168.1.100",
    "port": 1883,
    "topic": "mushroom/tent",
    "username": ""
  }
}
```

#### Update Network Configuration

```
POST /api/network/update
```

Updates the network configuration.

**Request Body:**
```json
{
  "wifi": {
    "ssid1": "NewWiFi",
    "password1": "password123",
    "hostname": "mushroom2",
    "ip_config": {
      "dhcp": false,
      "ip": "192.168.1.150",
      "gateway": "192.168.1.1",
      "subnet": "255.255.255.0",
      "dns1": "8.8.8.8",
      "dns2": "8.8.4.4"
    },
    "watchdog": {
      "min_rssi": -75,
      "check_interval": 60
    }
  },
  "mqtt": {
    "enabled": true,
    "broker": "192.168.1.100",
    "port": 1883,
    "topic": "mushroom/tent",
    "username": "mqtt_user",
    "password": "mqtt_pass"
  }
}
```

**Response:**
```json
{
  "success": true,
  "needs_reboot": true,
  "message": "Network configuration updated"
}
```

### Profile Management

#### Get Profiles

```
GET /api/profiles
```

Returns all saved profiles and the current active profile.

**Response:**
```json
{
  "profiles": {
    "Default": {
      "name": "Default",
      "environment": {
        "humidity_low": 50.0,
        "humidity_high": 85.0,
        "temperature_low": 20.0,
        "temperature_high": 24.0,
        "co2_low": 1000.0,
        "co2_high": 1600.0
      },
      "cycle": {
        "on_duration": 5,
        "interval": 60
      },
      "override_duration": 5,
      "relay_times": {
        // Relay times configuration...
      }
    },
    "Fruiting": {
      // Fruiting profile configuration...
    }
  },
  "current_profile": "Default"
}
```

#### Save Profile

```
POST /api/profiles/save
```

Saves current settings as a profile.

**Request Body:**
```json
{
  "name": "Custom",
  "settings": {
    "environment": {
      "humidity_low": 60.0,
      "humidity_high": 90.0,
      "temperature_low": 21.0,
      "temperature_high": 25.0,
      "co2_low": 800.0,
      "co2_high": 1400.0
    },
    "cycle": {
      "on_duration": 10,
      "interval": 30
    },
    "override_duration": 10,
    "relay_times": {
      // Relay times configuration...
    }
  }
}
```

**Response:**
```json
{
  "success": true,
  "message": "Profile saved"
}
```

#### Load Profile

```
POST /api/profiles/load
```

Loads a saved profile.

**Request Body:**
```json
{
  "name": "Fruiting"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Profile loaded"
}
```

### System Maintenance

#### Get System Information

```
GET /api/system/info
```

Returns system information, including version, network status, and memory usage.

**Response:**
```json
{
  "app_name": "Mushroom Tent Controller",
  "app_version": "1.0.1",
  "fs_version": "1.0.0",
  "network": {
    "hostname": "mushroom",
    "ip_address": "192.168.1.120",
    "ssid": "HomeWiFi",
    "rssi": -58,
    "mac_address": "AA:BB:CC:DD:EE:FF"
  },
  "memory": {
    "heap_size": 307200,
    "free_heap": 129540,
    "min_free_heap": 95400,
    "max_alloc_heap": 131072
  },
  "spiffs": {
    "total_bytes": 1048576,
    "used_bytes": 524288
  },
  "cpu": {
    "chip_model": "ESP32",
    "chip_revision": 1,
    "cpu_freq_mhz": 240,
    "sdk_version": "4.4.0"
  },
  "uptime_seconds": 3600
}
```

#### Reboot Device

```
POST /api/system/reboot
```

Reboots the device.

**Response:**
```json
{
  "success": true,
  "message": "Device will reboot now"
}
```

#### Factory Reset

```
POST /api/system/factory-reset
```

Performs a factory reset, clearing all settings and profiles.

**Response:**
```json
{
  "success": true,
  "message": "Device will perform factory reset and reboot"
}
```

#### WiFi Scanning

```
GET /api/wifi/scan
```

Scans for available WiFi networks.

**Response:**
```json
{
  "networks": [
    {
      "ssid": "HomeWiFi",
      "rssi": -58,
      "bssid": "AA:BB:CC:DD:EE:FF",
      "channel": 6,
      "encrypted": true
    },
    {
      "ssid": "Neighbor",
      "rssi": -72,
      "bssid": "11:22:33:44:55:66",
      "channel": 11,
      "encrypted": true
    }
  ]
}
```

#### Test WiFi Connection

```
POST /api/wifi/test
```

Tests a connection to a WiFi network.

**Request Body:**
```json
{
  "ssid": "TestNetwork",
  "password": "testpassword"
}
```

**Response:**
```json
{
  "success": true,
  "message": "Connection successful"
}
```

### File Management

#### List Files

```
GET /api/system/files
```

Lists all files in the filesystem.

**Response:**
```json
{
  "files": [
    {
      "name": "/config/profiles.json",
      "size": 2048,
      "url": "/config/profiles.json"
    },
    {
      "name": "/logs/system.log",
      "size": 4096,
      "url": "/logs/system.log"
    }
  ],
  "total_bytes": 1048576,
  "used_bytes": 524288,
  "free_bytes": 524288
}
```

#### Delete File

```
DELETE /api/system/delete?path=/path/to/file.txt
```

Deletes a file from the filesystem.

**Parameters:**
- `path`: Path to the file to delete

**Response:**
```json
{
  "success": true,
  "message": "File deleted"
}
```

#### Upload File

```
POST /api/upload
```

Uploads a file to the filesystem. This endpoint expects a multipart/form-data request with a file field named "file".

**Response:**
```json
{
  "success": true,
  "message": "File uploaded successfully",
  "path": "/uploaded/file.txt",
  "size": 1024
}
```

## Error Responses

All API endpoints will return standard HTTP status codes:

- `200 OK`: Request successful
- `400 Bad Request`: Invalid parameters
- `401 Unauthorized`: Authentication required
- `403 Forbidden`: Authentication failed
- `404 Not Found`: Resource not found
- `500 Internal Server Error`: Server error

Error responses will include a JSON body with details:

```json
{
  "success": false,
  "message": "Error description"
}
```