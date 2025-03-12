// Create relay controls for visible relays
data.relays.forEach(relay => {
    if (relay.visible) {
        // Create relay card
        const relayCard = document.createElement('div');
        relayCard.className = 'relay-card';
        
        // Relay header
        const relayHeader = document.createElement('div');
        relayHeader.className = 'relay-header';
        
        const relayName = document.createElement('h3');
        relayName.className = 'relay-name';
        relayName.textContent = relay.name;
        
        // Determine relay status text and class
        let statusText;
        switch (relay.last_trigger) {
            case 0: statusText = 'Manual'; break;
            case 1: statusText = 'Schedule'; break;
            case 2: statusText = 'Environmental'; break;
            case 3: statusText = 'Dependency'; break;
            default: statusText = 'Unknown';
        }
        
        const relayStatus = document.createElement('span');
        relayStatus.className = 'relay-status';
        relayStatus.textContent = statusText;
        
        relayHeader.appendChild(relayName);
        relayHeader.appendChild(relayStatus);
        
        // Relay toggle
        const relayToggle = document.createElement('label');
        relayToggle.className = 'relay-toggle';
        
        const relayInput = document.createElement('input');
        relayInput.type = 'checkbox';
        relayInput.checked = relay.is_on;
        relayInput.dataset.relayId = relay.id;
        
        const relaySlider = document.createElement('span');
        relaySlider.className = 'relay-slider';
        
        relayToggle.appendChild(relayInput);
        relayToggle.appendChild(relaySlider);
        
        // Add event listener for toggle
        relayInput.addEventListener('change', function() {
            toggleRelay(relay.id, this.checked);
        });
        
        // Relay info (additional details if needed)
        const relayInfo = document.createElement('div');
        relayInfo.className = 'relay-info';
        
        // Time range
        const timeRange = document.createElement('span');
        timeRange.textContent = `${relay.operating_time.start_hour}:${relay.operating_time.start_minute.toString().padStart(2, '0')} - ${relay.operating_time.end_hour}:${relay.operating_time.end_minute.toString().padStart(2, '0')}`;
        
        // State
        const stateText = document.createElement('span');
        switch (relay.state) {
            case 0: stateText.textContent = 'OFF'; break;
            case 1: stateText.textContent = 'ON'; break;
            case 2: stateText.textContent = 'AUTO'; break;
            default: stateText.textContent = 'Unknown';
        }
        
        relayInfo.appendChild(timeRange);
        relayInfo.appendChild(stateText);
        
        // Assemble relay card
        relayCard.appendChild(relayHeader);
        relayCard.appendChild(relayToggle);
        relayCard.appendChild(relayInfo);
        
        // Add to container
        relayContainer.appendChild(relayCard);
    }
});
} catch (error) {
console.error('Failed to load relay status:', error);
showAlert('Failed to load relay status', 'danger');
}
}

/**
* Toggle a relay's state
* @param {number} relayId - ID of the relay to toggle
* @param {boolean} turnOn - Whether to turn the relay on or off
*/
async function toggleRelay(relayId, turnOn) {
try {
const state = turnOn ? 1 : 0; // 0 = OFF, 1 = ON, 2 = AUTO
const result = await apiRequest(API.RELAY_SET, 'POST', {
    relay_id: relayId,
    state: state
});

if (result.success) {
    showAlert(`Relay ${relayId} ${turnOn ? 'turned ON' : 'turned OFF'}`, 'success', 3000);
    
    // Update the relay status after a short delay
    setTimeout(() => {
        loadRelayStatus();
    }, 2000);
} else {
    showAlert(`Failed to toggle relay: ${result.message}`, 'danger');
    // Reset the checkbox to its previous state
    loadRelayStatus();
}
} catch (error) {
console.error('Error toggling relay:', error);
showAlert('Error toggling relay', 'danger');
// Reset the checkbox to its previous state
loadRelayStatus();
}
}

/**
* Load environmental thresholds from the server
*/
async function loadEnvironmentalThresholds() {
try {
const data = await apiRequest(API.ENVIRONMENT);
environmentalThresholds = data;

// Update displays
updateSensors();
} catch (error) {
console.error('Failed to load environmental thresholds:', error);
}
}

/**
* Load settings for the settings modal
*/
async function loadSettings() {
try {
const data = await apiRequest(API.SETTINGS);

// Populate logging settings
document.getElementById('logLevel').value = data.logging ? data.logging.level : 0;
document.getElementById('maxLogSize').value = data.logging ? data.logging.max_size : 50;
document.getElementById('logFlushInterval').value = data.logging ? data.logging.flush_interval : 60;
document.getElementById('remoteLogServer').value = data.logging ? data.logging.log_server : '';
document.getElementById('sensorErrorCount').value = data.logging ? data.logging.sensor_error_count : 5;

// Populate sensor settings
document.getElementById('dhtInterval').value = data.sensors ? data.sensors.dht_interval : 5;
document.getElementById('scdInterval').value = data.sensors ? data.sensors.scd_interval : 10;
document.getElementById('graphInterval').value = data.sensors ? data.sensors.graph_interval : 30;
document.getElementById('graphPoints').value = data.sensors ? data.sensors.graph_points : 100;

// Populate sensor PIN configuration
document.getElementById('upperDhtPin').value = data.sensors ? data.sensors.upper_dht_pin : 13;
document.getElementById('lowerDhtPin').value = data.sensors ? data.sensors.lower_dht_pin : 14;
document.getElementById('scdSdaPin').value = data.sensors ? data.sensors.scd_sda_pin : 21;
document.getElementById('scdSclPin').value = data.sensors ? data.sensors.scd_scl_pin : 22;

// Populate reboot scheduler settings
document.getElementById('rebootDay').value = data.reboot_scheduler ? data.reboot_scheduler.day_of_week : 0;

// Populate reboot scheduler hours and minutes if not already done
const rebootHour = document.getElementById('rebootHour');
if (rebootHour.children.length === 0) {
    for (let h = 0; h < 24; h++) {
        const option = document.createElement('option');
        option.value = h;
        option.textContent = h.toString().padStart(2, '0');
        rebootHour.appendChild(option);
    }
}
rebootHour.value = data.reboot_scheduler ? data.reboot_scheduler.hour : 3;

const rebootMinute = document.getElementById('rebootMinute');
if (rebootMinute.children.length === 0) {
    for (let m = 0; m < 60; m += 5) {
        const option = document.createElement('option');
        option.value = m;
        option.textContent = m.toString().padStart(2, '0');
        rebootMinute.appendChild(option);
    }
}
rebootMinute.value = data.reboot_scheduler ? data.reboot_scheduler.minute : 0;

document.getElementById('enableReboot').checked = data.reboot_scheduler ? data.reboot_scheduler.enabled : false;

// Populate sleep mode settings
document.getElementById('sleepMode').value = data.sleep_mode ? data.sleep_mode.mode : 0;
document.getElementById('sleepTimeFrom').value = data.sleep_mode ? data.sleep_mode.from : '00:00';
document.getElementById('sleepTimeTo').value = data.sleep_mode ? data.sleep_mode.to : '06:00';

} catch (error) {
console.error('Failed to load settings:', error);
showAlert('Failed to load settings', 'danger');
}
}

/**
* Save settings from the settings modal
*/
async function saveSettings() {
// Collect settings from form
const settings = {
logging: {
    level: parseInt(document.getElementById('logLevel').value),
    max_size: parseInt(document.getElementById('maxLogSize').value),
    flush_interval: parseInt(document.getElementById('logFlushInterval').value),
    log_server: document.getElementById('remoteLogServer').value,
    sensor_error_count: parseInt(document.getElementById('sensorErrorCount').value)
},
sensors: {
    dht_interval: parseInt(document.getElementById('dhtInterval').value),
    scd_interval: parseInt(document.getElementById('scdInterval').value),
    graph_interval: parseInt(document.getElementById('graphInterval').value),
    graph_points: parseInt(document.getElementById('graphPoints').value),
    upper_dht_pin: parseInt(document.getElementById('upperDhtPin').value),
    lower_dht_pin: parseInt(document.getElementById('lowerDhtPin').value),
    scd_sda_pin: parseInt(document.getElementById('scdSdaPin').value),
    scd_scl_pin: parseInt(document.getElementById('scdSclPin').value)
},
reboot_scheduler: {
    enabled: document.getElementById('enableReboot').checked,
    day_of_week: parseInt(document.getElementById('rebootDay').value),
    hour: parseInt(document.getElementById('rebootHour').value),
    minute: parseInt(document.getElementById('rebootMinute').value)
},
sleep_mode: {
    mode: parseInt(document.getElementById('sleepMode').value),
    from: document.getElementById('sleepTimeFrom').value,
    to: document.getElementById('sleepTimeTo').value
}
};

try {
const result = await apiRequest(API.SETTINGS_UPDATE, 'POST', settings);

if (result.success) {
    showAlert('Settings saved successfully', 'success');
    hideModal('settingsModal');
    
    // Update graph timers if intervals changed
    if (settings.sensors.graph_interval !== 30 && graphUpdateTimer) {
        clearInterval(graphUpdateTimer);
        graphUpdateTimer = setInterval(updateGraphs, settings.sensors.graph_interval * 1000);
    }
} else {
    showAlert(`Failed to save settings: ${result.message}`, 'danger');
}
} catch (error) {
console.error('Error saving settings:', error);
showAlert('Error saving settings', 'danger');
}
}

/**
* Load network configuration for the network modal
*/
async function loadNetworkConfig() {
try {
const data = await apiRequest(API.NETWORK);

// Populate WiFi settings
document.getElementById('wifi1Ssid').value = data.wifi ? data.wifi.ssid1 : '';
document.getElementById('wifi2Ssid').value = data.wifi ? data.wifi.ssid2 : '';
document.getElementById('wifi3Ssid').value = data.wifi ? data.wifi.ssid3 : '';
document.getElementById('hostname').value = data.wifi ? data.wifi.hostname : 'mushroom';

// Populate IP configuration
const useDHCP = data.wifi && data.wifi.ip_config ? data.wifi.ip_config.dhcp : true;
document.getElementById('dhcpEnabled').checked = useDHCP;
document.getElementById('staticIpEnabled').checked = !useDHCP;
document.getElementById('staticIpFields').style.display = useDHCP ? 'none' : 'block';

if (data.wifi && data.wifi.ip_config) {
    document.getElementById('staticIp').value = data.wifi.ip_config.ip;
    document.getElementById('subnetMask').value = data.wifi.ip_config.subnet;
    document.getElementById('gateway').value = data.wifi.ip_config.gateway;
    document.getElementById('dns1').value = data.wifi.ip_config.dns1;
    document.getElementById('dns2').value = data.wifi.ip_config.dns2;
}

// Populate WiFi watchdog settings
if (data.wifi && data.wifi.watchdog) {
    document.getElementById('minRssi').value = data.wifi.watchdog.min_rssi;
    document.getElementById('wifiCheckInterval').value = data.wifi.watchdog.check_interval;
}

// Populate MQTT settings
const mqttEnabled = data.mqtt ? data.mqtt.enabled : false;
document.getElementById('mqttEnabled').checked = mqttEnabled;
document.getElementById('mqttFields').style.display = mqttEnabled ? 'block' : 'none';

if (data.mqtt) {
    document.getElementById('mqttBroker').value = data.mqtt.broker;
    document.getElementById('mqttPort').value = data.mqtt.port;
    document.getElementById('mqttTopic').value = data.mqtt.topic;
    document.getElementById('mqttUsername').value = data.mqtt.username;
    // Don't populate password for security reasons
}

} catch (error) {
console.error('Failed to load network configuration:', error);
showAlert('Failed to load network configuration', 'danger');
}
}

/**
* Save network configuration from the network modal
*/
async function saveNetworkConfig() {
// Collect network settings from form
const useDHCP = document.getElementById('dhcpEnabled').checked;
const mqttEnabled = document.getElementById('mqttEnabled').checked;

const network = {
wifi: {
    ssid1: document.getElementById('wifi1Ssid').value,
    password1: document.getElementById('wifi1Password').value,
    ssid2: document.getElementById('wifi2Ssid').value,
    password2: document.getElementById('wifi2Password').value,
    ssid3: document.getElementById('wifi3Ssid').value,
    password3: document.getElementById('wifi3Password').value,
    hostname: document.getElementById('hostname').value,
    ip_config: {
        dhcp: useDHCP
    },
    watchdog: {
        min_rssi: parseInt(document.getElementById('minRssi').value),
        check_interval: parseInt(document.getElementById('wifiCheckInterval').value)
    }
},
mqtt: {
    enabled: mqttEnabled
}
};

// Add static IP config if not using DHCP
if (!useDHCP) {
network.wifi.ip_config.ip = document.getElementById('staticIp').value;
network.wifi.ip_config.subnet = document.getElementById('subnetMask').value;
network.wifi.ip_config.gateway = document.getElementById('gateway').value;
network.wifi.ip_config.dns1 = document.getElementById('dns1').value;
network.wifi.ip_config.dns2 = document.getElementById('dns2').value;
}

// Add MQTT config if enabled
if (mqttEnabled) {
network.mqtt.broker = document.getElementById('mqttBroker').value;
network.mqtt.port = parseInt(document.getElementById('mqttPort').value);
network.mqtt.topic = document.getElementById('mqttTopic').value;
network.mqtt.username = document.getElementById('mqttUsername').value;

// Only include password if it's not empty
const mqttPassword = document.getElementById('mqttPassword').value;
if (mqttPassword) {
    network.mqtt.password = mqttPassword;
}
}

try {
const result = await apiRequest(API.NETWORK_UPDATE, 'POST', network);

if (result.success) {
    showAlert('Network configuration saved successfully', 'success');
    hideModal('networkModal');
    
    // If a reboot is needed, show a message
    if (result.needs_reboot) {
        showAlert('Network configuration updated. Device will reboot now...', 'info', 0);
    }
} else {
    showAlert(`Failed to save network configuration: ${result.message}`, 'danger');
}
} catch (error) {
console.error('Error saving network configuration:', error);
showAlert('Error saving network configuration', 'danger');
}
}

/**
* Load profiles for the profiles modal
*/
async function loadProfiles() {
try {
const data = await apiRequest(API.PROFILES);

// Populate profile selector
const profileSelect = document.getElementById('profileSelect');
profileSelect.innerHTML = '';

Object.keys(data.profiles).forEach(profileName => {
    const option = document.createElement('option');
    option.value = profileName;
    option.textContent = profileName;
    profileSelect.appendChild(option);
});

// Select current profile
profileSelect.value = data.current_profile;

// Load the selected profile
loadProfileSettings(data.current_profile, data.profiles[data.current_profile]);

// Create relay times inputs for visible relays
createRelayTimeInputs();

} catch (error) {
console.error('Failed to load profiles:', error);
showAlert('Failed to load profiles', 'danger');
}
}

/**
* Create relay time inputs for all relays
*/
function createRelayTimeInputs() {
const container = document.getElementById('relayTimesContainer');
container.innerHTML = '';

// Get relay status for names
apiRequest(API.RELAYS).then(data => {
data.relays.forEach(relay => {
    // Create row for each relay
    const row = document.createElement('div');
    row.className = 'row mb-2';
    
    // Relay name column
    const nameCol = document.createElement('div');
    nameCol.className = 'col-third';
    
    const nameLabel = document.createElement('label');
    nameLabel.className = 'form-label';
    nameLabel.textContent = `${relay.name} (Relay ${relay.id})`;
    
    nameCol.appendChild(nameLabel);
    
    // Start time column
    const startCol = document.createElement('div');
    startCol.className = 'col-third';
    
    const startInput = document.createElement('input');
    startInput.type = 'time';
    startInput.className = 'form-control';
    startInput.id = `relay${relay.id}Start`;
    
    // Format time value (HH:MM)
    const startHour = relay.operating_time.start_hour.toString().padStart(2, '0');
    const startMinute = relay.operating_time.start_minute.toString().padStart(2, '0');
    startInput.value = `${startHour}:${startMinute}`;
    
    startCol.appendChild(startInput);
    
    // End time column
    const endCol = document.createElement('div');
    endCol.className = 'col-third';
    
    const endInput = document.createElement('input');
    endInput.type = 'time';
    endInput.className = 'form-control';
    endInput.id = `relay${relay.id}End`;
    
    // Format time value (HH:MM)
    const endHour = relay.operating_time.end_hour.toString().padStart(2, '0');
    const endMinute = relay.operating_time.end_minute.toString().padStart(2, '0');
    endInput.value = `${endHour}:${endMinute}`;
    
    endCol.appendChild(endInput);
    
    // Assemble row
    row.appendChild(nameCol);
    row.appendChild(startCol);
    row.appendChild(endCol);
    
    // Add to container
    container.appendChild(row);
});
}).catch(error => {
console.error('Failed to load relay status for time inputs:', error);
});
}

/**
* Load settings for a specific profile
* @param {string} profileName - Name of the profile
* @param {Object} profileData - Profile data
*/
function loadProfileSettings(profileName, profileData) {
// Environmental thresholds
if (profileData.environment) {
document.getElementById('humidityLow').value = profileData.environment.humidity_low;
document.getElementById('humidityHigh').value = profileData.environment.humidity_high;
document.getElementById('temperatureLow').value = profileData.environment.temperature_low;
document.getElementById('temperatureHigh').value = profileData.environment.temperature_high;
document.getElementById('co2Low').value = profileData.environment.co2_low;
document.getElementById('co2High').value = profileData.environment.co2_high;
}

// Cycle configuration
if (profileData.cycle) {
document.getElementById('onDuration').value = profileData.cycle.on_duration;
document.getElementById('cycleInterval').value = profileData.cycle.interval;
}

// Override duration
document.getElementById('overrideDuration').value = profileData.override_duration || 5;
}

/**
* Setup event listeners for buttons and forms
*/
function setupEventListeners() {
// Settings modal
document.getElementById('saveSettingsBtn').addEventListener('click', saveSettings);
document.getElementById('cancelSettingsBtn').addEventListener('click', function() {
hideModal('settingsModal');
});

// Reboot and factory reset buttons
document.getElementById('rebootBtn').addEventListener('click', function() {
confirmAction('Are you sure you want to reboot the device?', function() {
    apiRequest(API.SYSTEM_REBOOT, 'POST')
        .then(() => {
            showAlert('Device is rebooting...', 'info', 0);
        })
        .catch(error => {
            console.error('Reboot failed:', error);
            showAlert('Failed to reboot device', 'danger');
        });
});
});

document.getElementById('factoryResetBtn').addEventListener('click', function() {
confirmAction('WARNING: This will reset all settings to factory defaults! Are you sure?', function() {
    apiRequest(API.SYSTEM_FACTORY_RESET, 'POST')
        .then(() => {
            showAlert('Factory reset initiated. Device is rebooting...', 'warning', 0);
        })
        .catch(error => {
            console.error('Factory reset failed:', error);
            showAlert('Failed to perform factory reset', 'danger');
        });
});
});

// Network modal
document.getElementById('saveNetworkBtn').addEventListener('click', saveNetworkConfig);
document.getElementById('cancelNetworkBtn').addEventListener('click', function() {
hideModal('networkModal');
});

// Static IP toggle
document.getElementById('dhcpEnabled').addEventListener('change', function() {
document.getElementById('staticIpFields').style.display = this.checked ? 'none' : 'block';
});

document.getElementById('staticIpEnabled').addEventListener('change', function() {
document.getElementById('staticIpFields').style.display = this.checked ? 'block' : 'none';
});

// MQTT toggle
document.getElementById('mqttEnabled').addEventListener('change', function() {
document.getElementById('mqttFields').style.display = this.checked ? 'block' : 'none';
});

// WiFi scan buttons
document.getElementById('scanWifi1Btn').addEventListener('click', function() {
scanWiFi(1);
});

document.getElementById('scanWifi2Btn').addEventListener('click', function() {
scanWiFi(2);
});

document.getElementById('scanWifi3Btn').addEventListener('click', function() {
scanWiFi(3);
});

// Profiles modal
document.getElementById('loadProfileBtn').addEventListener('click', function() {
const profileName = document.getElementById('profileSelect').value;
if (profileName) {
    loadProfile(profileName);
}
});

document.getElementById('saveProfileBtn').addEventListener('click', function() {
saveProfile();
});

document.getElementById('renameProfileBtn').addEventListener('click', function() {
const currentName = document.getElementById('profileSelect').value;
const newName = prompt('Enter new profile name:', currentName);
if (newName && newName !== currentName) {
    renameProfile(currentName, newName);
}
});

document.getElementById('exportProfilesBtn').addEventListener('click', function() {
window.location.href = API.PROFILE_EXPORT;
});

document.getElementById('importProfilesBtn').addEventListener('click', function() {
document.getElementById('profileImportInput').click();
});

document.getElementById('profileImportInput').addEventListener('change', function(e) {
if (e.target.files.length > 0) {
    importProfiles(e.target.files[0]);
}
});

document.getElementById('closeProfilesBtn').addEventListener('click', function() {
hideModal('profilesModal');
});

document.getElementById('applyProfileBtn').addEventListener('click', function() {
applyProfileChanges();
});
}

/**
* Scan for WiFi networks
* @param {number} index - WiFi credential index (1-3)
*/
async function scanWiFi(index) {
// Show scan modal
showModal('wifiScanModal');

// Show scanning message
document.getElementById('scanningMessage').style.display = 'block';
document.getElementById('wifiNetworksList').style.display = 'none';

try {
const result = await apiRequest(API.WIFI_SCAN);

// Hide scanning message
document.getElementById('scanningMessage').style.display = 'none';

// Create network list
const networksContainer = document.getElementById('wifiNetworksList');
networksContainer.innerHTML = '';

if (result.networks && result.networks.length > 0) {
    // Sort networks by signal strength
    result.networks.sort((a, b) => b.rssi - a.rssi);
    
    // Create a list of networks
    const networkList = document.createElement('div');
    networkList.className = 'list-group';
    
    result.networks.forEach(network => {
        const networkItem = document.createElement('button');
        networkItem.type = 'button';
        networkItem.className = 'list-group-item list-group-item-action d-flex justify-content-between align-items-center';
        
        // Signal strength icon
        let signalIcon;
        if (network.rssi > -50) {
            signalIcon = '<i class="fas fa-wifi"></i>';
        } else if (network.rssi > -70) {
            signalIcon = '<i class="fas fa-wifi" style="opacity: 0.7;"></i>';
        } else {
            signalIcon = '<i class="fas fa-wifi" style="opacity: 0.4;"></i>';
        }
        
        // Security icon
        const securityIcon = network.encrypted ? 
            '<i class="fas fa-lock"></i>' : 
            '<i class="fas fa-lock-open"></i>';
        
        // Network name and details
        networkItem.innerHTML = `
            <div>
                <div>${network.ssid}</div>
                <small class="text-muted">Ch: ${network.channel} | RSSI: ${network.rssi} dBm | MAC: ${network.bssid}</small>
            </div>
            <div>
                ${signalIcon} ${securityIcon}
            </div>
        `;
        
        // Add click event to select this network
        networkItem.addEventListener('click', function() {
            document.getElementById(`wifi${index}Ssid`).value = network.ssid;
            hideModal('wifiScanModal');
            
            // Focus on password field if encrypted
            if (network.encrypted) {
                document.getElementById(`wifi${index}Password`).focus();
            }
        });
        
        networkList.appendChild(networkItem);
    });
    
    networksContainer.appendChild(networkList);
} else {
    networksContainer.innerHTML = '<p class="text-center">No WiFi networks found</p>';
}

// Show the networks list
networksContainer.style.display = 'block';

} catch (error) {
console.error('WiFi scan failed:', error);
document.getElementById('wifiNetworksList').innerHTML = '<p class="text-center text-danger">WiFi scan failed</p>';
document.getElementById('scanningMessage').style.display = 'none';
document.getElementById('wifiNetworksList').style.display = 'block';
}
}

/**
* Load a profile from the server
* @param {string} profileName - Name of the profile to load
*/
async function loadProfile(profileName) {
try {
const result = await apiRequest(API.PROFILE_LOAD, 'POST', {
    name: profileName
});

if (result.success) {
    showAlert(`Profile '${profileName}' loaded successfully`, 'success');
    
    // Reload environmental thresholds
    loadEnvironmentalThresholds();
    
    // Reload the profiles to get the updated state
    loadProfiles();
} else {
    showAlert(`Failed to load profile: ${result.message}`, 'danger');
}
} catch (error) {
console.error('Error loading profile:', error);
showAlert('Error loading profile', 'danger');
}
}

/**
* Save current settings as a profile
*/
async function saveProfile() {
const profileName = document.getElementById('profileSelect').value;
if (!profileName) {
showAlert('Please select a profile name', 'warning');
return;
}

// Collect settings from the form
const settings = {
environment: {
    humidity_low: parseFloat(document.getElementById('humidityLow').value),
    humidity_high: parseFloat(document.getElementById('humidityHigh').value),
    temperature_low: parseFloat(document.getElementById('temperatureLow').value),
    temperature_high: parseFloat(document.getElementById('temperatureHigh').value),
    co2_low: parseFloat(document.getElementById('co2Low').value),
    co2_high: parseFloat(document.getElementById('co2High').value)
},
cycle: {
    on_duration: parseInt(document.getElementById('onDuration').value),
    interval: parseInt(document.getElementById('cycleInterval').value)
},
override_duration: parseInt(document.getElementById('overrideDuration').value),
relay_times: {}
};

// Collect relay times
apiRequest(API.RELAYS).then(data => {
data.relays.forEach(relay => {/**
* Mushroom Tent Controller - Dashboard JavaScript
* Handles dashboard functionality including sensor displays, graphs, and relay controls
*/

// Global variables
let temperatureChart = null;
let humidityChart = null;
let sensorUpdateTimer = null;
let graphUpdateTimer = null;
let environmentalThresholds = {
humidityLow: 50,
humidityHigh: 85,
temperatureLow: 20,
temperatureHigh: 24,
co2Low: 1000,
co2High: 1600
};

// Initialize the dashboard when the DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
// Initialize charts
initializeCharts();

// Load environmental thresholds
loadEnvironmentalThresholds();

// Setup relay controls
loadRelayStatus();

// Start sensor updates
startSensorUpdates();

// Setup event listeners
setupEventListeners();

// Open settings modal if clicked
document.getElementById('settingsLink').addEventListener('click', function(e) {
e.preventDefault();
loadSettings();
showModal('settingsModal');
});

// Open network modal if clicked
document.getElementById('networkLink').addEventListener('click', function(e) {
e.preventDefault();
loadNetworkConfig();
showModal('networkModal');
});

// Open profiles modal if clicked
document.getElementById('profilesBtn').addEventListener('click', function(e) {
e.preventDefault();
loadProfiles();
showModal('profilesModal');
});

// Clear graphs button
document.getElementById('clearGraphBtn').addEventListener('click', function() {
clearGraphs();
});
});

/**
* Initialize temperature and humidity charts
*/
function initializeCharts() {
// Common chart options
const commonOptions = {
responsive: true,
maintainAspectRatio: false,
animation: {
    duration: 0 // Disable animation for better performance
},
plugins: {
    legend: {
        position: 'top',
        labels: {
            usePointStyle: true
        }
    },
    tooltip: {
        mode: 'index',
        intersect: false
    }
},
scales: {
    x: {
        grid: {
            display: false
        },
        ticks: {
            maxTicksLimit: 10
        }
    },
    y: {
        grid: {
            color: 'rgba(200, 200, 200, 0.2)'
        },
        beginAtZero: false
    }
},
elements: {
    line: {
        tension: 0.3
    },
    point: {
        radius: 1,
        hitRadius: 10,
        hoverRadius: 5
    }
}
};

// Temperature chart
const tempCtx = document.getElementById('temperatureChart').getContext('2d');
temperatureChart = new Chart(tempCtx, {
type: 'line',
data: {
    labels: [],
    datasets: [
        {
            label: 'Upper DHT',
            data: [],
            borderColor: '#36a2eb',
            backgroundColor: 'rgba(54, 162, 235, 0.2)',
            fill: false
        },
        {
            label: 'Lower DHT',
            data: [],
            borderColor: '#4bc0c0',
            backgroundColor: 'rgba(75, 192, 192, 0.2)',
            fill: false
        },
        {
            label: 'SCD40',
            data: [],
            borderColor: '#9966ff',
            backgroundColor: 'rgba(153, 102, 255, 0.2)',
            fill: false
        }
    ]
},
options: {
    ...commonOptions,
    scales: {
        ...commonOptions.scales,
        y: {
            ...commonOptions.scales.y,
            title: {
                display: true,
                text: 'Temperature (°C)'
            }
        }
    }
}
});

// Humidity chart
const humidityCtx = document.getElementById('humidityChart').getContext('2d');
humidityChart = new Chart(humidityCtx, {
type: 'line',
data: {
    labels: [],
    datasets: [
        {
            label: 'Upper DHT',
            data: [],
            borderColor: '#36a2eb',
            backgroundColor: 'rgba(54, 162, 235, 0.2)',
            fill: false
        },
        {
            label: 'Lower DHT',
            data: [],
            borderColor: '#4bc0c0',
            backgroundColor: 'rgba(75, 192, 192, 0.2)',
            fill: false
        },
        {
            label: 'SCD40',
            data: [],
            borderColor: '#9966ff',
            backgroundColor: 'rgba(153, 102, 255, 0.2)',
            fill: false
        }
    ]
},
options: {
    ...commonOptions,
    scales: {
        ...commonOptions.scales,
        y: {
            ...commonOptions.scales.y,
            title: {
                display: true,
                text: 'Humidity (%)'
            }
        }
    }
}
});
}

/**
* Start the sensor update timers
*/
function startSensorUpdates() {
// Update sensors immediately
updateSensors();

// Start sensor update timer (every 5 seconds)
sensorUpdateTimer = setInterval(updateSensors, 5000);

// Update graphs immediately
updateGraphs();

// Start graph update timer (every 30 seconds)
graphUpdateTimer = setInterval(updateGraphs, 30000);
}

/**
* Update sensor displays with the latest data
*/
async function updateSensors() {
try {
const data = await apiRequest(API.SENSORS);

// Update Upper DHT
if (data.upper_dht.valid) {
    document.getElementById('upperDhtTemp').innerHTML = `${data.upper_dht.temperature.toFixed(1)} <span class="sensor-unit">°C</span>`;
    document.getElementById('upperDhtHumidity').innerHTML = `${data.upper_dht.humidity.toFixed(1)} <span class="sensor-unit">%</span>`;
    
    // Apply color based on thresholds
    document.getElementById('upperDhtTemp').className = `sensor-value ${getValueClass(data.upper_dht.temperature, environmentalThresholds.temperatureLow, environmentalThresholds.temperatureHigh)}`;
    document.getElementById('upperDhtHumidity').className = `sensor-value ${getValueClass(data.upper_dht.humidity, environmentalThresholds.humidityLow, environmentalThresholds.humidityHigh)}`;
    
    // Flash update indicator
    flashUpdateIndicator(document.getElementById('upperDhtIndicator'));
} else {
    document.getElementById('upperDhtTemp').innerHTML = `-- <span class="sensor-unit">°C</span>`;
    document.getElementById('upperDhtHumidity').innerHTML = `-- <span class="sensor-unit">%</span>`;
}

// Update Lower DHT
if (data.lower_dht.valid) {
    document.getElementById('lowerDhtTemp').innerHTML = `${data.lower_dht.temperature.toFixed(1)} <span class="sensor-unit">°C</span>`;
    document.getElementById('lowerDhtHumidity').innerHTML = `${data.lower_dht.humidity.toFixed(1)} <span class="sensor-unit">%</span>`;
    
    // Apply color based on thresholds
    document.getElementById('lowerDhtTemp').className = `sensor-value ${getValueClass(data.lower_dht.temperature, environmentalThresholds.temperatureLow, environmentalThresholds.temperatureHigh)}`;
    document.getElementById('lowerDhtHumidity').className = `sensor-value ${getValueClass(data.lower_dht.humidity, environmentalThresholds.humidityLow, environmentalThresholds.humidityHigh)}`;
    
    // Flash update indicator
    flashUpdateIndicator(document.getElementById('lowerDhtIndicator'));
} else {
    document.getElementById('lowerDhtTemp').innerHTML = `-- <span class="sensor-unit">°C</span>`;
    document.getElementById('lowerDhtHumidity').innerHTML = `-- <span class="sensor-unit">%</span>`;
}

// Update SCD40
if (data.scd.valid) {
    document.getElementById('scdTemp').innerHTML = `${data.scd.temperature.toFixed(1)} <span class="sensor-unit">°C</span>`;
    document.getElementById('scdHumidity').innerHTML = `${data.scd.humidity.toFixed(1)} <span class="sensor-unit">%</span>`;
    document.getElementById('scdCo2').innerHTML = `${data.scd.co2.toFixed(0)} <span class="sensor-unit">ppm</span>`;
    
    // Apply color based on thresholds
    document.getElementById('scdTemp').className = `sensor-value ${getValueClass(data.scd.temperature, environmentalThresholds.temperatureLow, environmentalThresholds.temperatureHigh)}`;
    document.getElementById('scdHumidity').className = `sensor-value ${getValueClass(data.scd.humidity, environmentalThresholds.humidityLow, environmentalThresholds.humidityHigh)}`;
    document.getElementById('scdCo2').className = `sensor-value ${getValueClass(data.scd.co2, environmentalThresholds.co2Low, environmentalThresholds.co2High)}`;
    
    // Flash update indicator
    flashUpdateIndicator(document.getElementById('scdIndicator'));
} else {
    document.getElementById('scdTemp').innerHTML = `-- <span class="sensor-unit">°C</span>`;
    document.getElementById('scdHumidity').innerHTML = `-- <span class="sensor-unit">%</span>`;
    document.getElementById('scdCo2').innerHTML = `-- <span class="sensor-unit">ppm</span>`;
}

} catch (error) {
console.error('Failed to update sensors:', error);
}
}

/**
* Update the temperature and humidity graphs
*/
async function updateGraphs() {
try {
// Update temperature graph
const tempData = await apiRequest(`${API.GRAPH}?type=0&points=100`);
updateGraph(temperatureChart, tempData);
flashUpdateIndicator(document.getElementById('tempUpdateIndicator'));

// Update humidity graph
const humidityData = await apiRequest(`${API.GRAPH}?type=1&points=100`);
updateGraph(humidityChart, humidityData);
flashUpdateIndicator(document.getElementById('humidityUpdateIndicator'));

} catch (error) {
console.error('Failed to update graphs:', error);
}
}

/**
* Update a specific chart with new data
* @param {Chart} chart - The chart to update
* @param {Object} data - The new data
*/
function updateGraph(chart, data) {
// Check if we have data
if (!data || !data.timestamps || data.timestamps.length === 0) {
return;
}

// Format timestamps
const formattedLabels = data.timestamps.map(formatTime);

// Update chart data
chart.data.labels = formattedLabels;
chart.data.datasets[0].data = data.upper_dht;
chart.data.datasets[1].data = data.lower_dht;
chart.data.datasets[2].data = data.scd;

// Update the chart
chart.update();
}

/**
* Clear both temperature and humidity graphs
*/
function clearGraphs() {
if (temperatureChart) {
temperatureChart.data.labels = [];
temperatureChart.data.datasets.forEach(dataset => {
    dataset.data = [];
});
temperatureChart.update();
}

if (humidityChart) {
humidityChart.data.labels = [];
humidityChart.data.datasets.forEach(dataset => {
    dataset.data = [];
});
humidityChart.update();
}

showAlert('Graphs cleared successfully', 'success', 3000);
}

/**
* Load and display the current relay status
*/
async function loadRelayStatus() {
try {
const data = await apiRequest(API.RELAYS);

// Clear existing relay controls
const relayContainer = document.getElementById('relayControlsContainer');
relayContainer.innerHTML = '';

// Create relay controls for visible relays
data.relays.forEach(relay => {
    if (relay.visible) {
        // Create relay card
        const relayCard = document.createElement('div');
        relayCard.className = 'relay-card';
        
        // Relay header
        const relayHeader = document.createElement('div');
        relayHeader.className = 'relay-header';
        
        const relayName = document.createElement('h3');
        relayName.className = 'relay-name';
        relayName.textContent = relay.name;
        
        // Determine relay status text and class
        let statusText;
        switch (relay.last_trigger) {
            case 0: statusText = 'Manual'; break;
            case 1: statusText = 'Schedule'; break;
            case 2: statusText = 'Environmental'; break;
            case 3: statusText = 'Dependency'; break;
            default: statusText = 'Unknown';
        }
        
        const relayStatus = document.createElement('span');
        relayStatus.className = 'relay-status';
        relayStatus.textContent = statusText;
        
        relayHeader.appendChild(relayName);
        relayHeader.appendChild(relayStatus);
        
        // Relay toggle
        const relayToggle = document.createElement('label');
        relayToggle.className = 'relay-toggle';
        
        const relayInput = document.createElement('input');
        relayInput.type = 'checkbox';
        relayInput.checked = relay.is_on;
        relayInput.dataset.relayId = relay.id;
        
        const relaySlider = document.createElement('span');
        relaySlider.className = 'relay-slider';
        
        relayToggle.appendChild(relayInput);
        relayToggle.appendChild(relaySlider);
        
        // Add event listener for toggle
        relayInput.addEventListener('change', function() {
            toggleRelay(relay.id, this.checked);
        });
        
        // Relay info (additional details if needed)
        const relayInfo = document.createElement('div');
        relayInfo.className = 'relay-info';
        
        // Time range
        const timeRange = document.createElement('span');
        timeRange.textContent = `${relay.operating_time.start_hour}:${relay.operating_time.start_minute.toString().padStart(2, '0')} - ${relay.operating_time.end_hour}:${relay.operating_time.end_minute.toString().padStart(2, '0')}`;
        
        // State
        const stateText = document.createElement('span');
        switch (relay.state) {
            case 0: stateText.textContent = 'OFF'; break;
            case 1: stateText.textContent = 'ON'; break;
            case 2: stateText.textContent = 'AUTO'; break;
            default: stateText.textContent = 'Unknown';
        }
        
        relayInfo.appendChild(timeRange);
        relayInfo.appendChild(stateText);
        
        // Assemble relay card
        relayCard.appendChild(relayHeader);
        relayCard.appendChild(relayToggle);
        relayCard.appendChild(relayInfo);
        
        // Add to container
        relayContainer.appendChild(relayCard);
    }
});