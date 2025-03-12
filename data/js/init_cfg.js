/**
 * Mushroom Tent Controller - Initial Configuration JavaScript
 * Handles initial setup wizard functionality
 */

// Global variables
let currentStep = 1;
const totalSteps = 6;
let testSuccessful = false;
let rebootTimer = null;

// Initialize the setup page when the DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    // Initialize form fields
    initializeFormFields();
    
    // Setup event listeners
    setupEventListeners();
});

/**
 * Initialize form fields with default values
 */
function initializeFormFields() {
    // Set default hostname
    document.getElementById('hostname').value = 'mushroom';
    
    // Set default HTTP auth
    document.getElementById('httpUsername').value = 'admin';
    document.getElementById('httpPassword').value = 'admin';
    
    // Set default GPIO pins
    document.getElementById('upperDhtPin').value = '13';
    document.getElementById('lowerDhtPin').value = '14';
    document.getElementById('scdSdaPin').value = '21';
    document.getElementById('scdSclPin').value = '22';
    
    document.getElementById('relay1Pin').value = '16';
    document.getElementById('relay2Pin').value = '17';
    document.getElementById('relay3Pin').value = '19';
    document.getElementById('relay4Pin').value = '26';
    document.getElementById('relay5Pin').value = '33';
    document.getElementById('relay6Pin').value = '23';
    document.getElementById('relay7Pin').value = '25';
    document.getElementById('relay8Pin').value = '35';
    
    // Set default update intervals
    document.getElementById('dhtInterval').value = '5';
    document.getElementById('scdInterval').value = '10';
    document.getElementById('graphInterval').value = '30';
    document.getElementById('graphPoints').value = '100';
    
    // Set default environmental thresholds
    document.getElementById('humidityLow').value = '50';
    document.getElementById('humidityHigh').value = '85';
    document.getElementById('temperatureLow').value = '20';
    document.getElementById('temperatureHigh').value = '24';
    document.getElementById('co2Low').value = '1000';
    document.getElementById('co2High').value = '1600';
    document.getElementById('overrideDuration').value = '5';
}

/**
 * Setup event listeners for buttons and forms
 */
function setupEventListeners() {
    // Next button
    document.getElementById('nextBtn').addEventListener('click', function() {
        if (validateCurrentStep()) {
            nextStep();
        }
    });
    
    // Previous button
    document.getElementById('prevBtn').addEventListener('click', prevStep);
    
    // Submit button
    document.getElementById('submitBtn').addEventListener('click', submitSetup);
    
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
    
    // WiFi test buttons
    document.getElementById('testWifi1Btn').addEventListener('click', function() {
        testWiFiConnection(1);
    });
    
    document.getElementById('testWifi2Btn').addEventListener('click', function() {
        testWiFiConnection(2);
    });
    
    document.getElementById('testWifi3Btn').addEventListener('click', function() {
        testWiFiConnection(3);
    });
    
    // MQTT toggle
    document.getElementById('mqttEnabled').addEventListener('change', function() {
        document.getElementById('mqttFields').style.display = this.checked ? 'block' : 'none';
    });
    
    // GPIO toggle
    document.getElementById('useDefaultGpio').addEventListener('change', function() {
        document.getElementById('customGpioFields').style.display = this.checked ? 'none' : 'block';
    });
    
    // WiFi scan modal
    document.getElementById('closeWifiScanModal').addEventListener('click', function() {
        hideModal('wifiScanModal');
    });
    
    document.getElementById('cancelWifiScanBtn').addEventListener('click', function() {
        hideModal('wifiScanModal');
    });
    
    document.getElementById('refreshWifiScanBtn').addEventListener('click', function() {
        // Get current scan target
        const index = parseInt(document.getElementById('wifiScanModal').dataset.scanTarget || '1');
        scanWiFi(index);
    });
}

/**
 * Validate the current step
 * @returns {boolean} Whether the step is valid
 */
function validateCurrentStep() {
    switch (currentStep) {
        case 1: // WiFi configuration
            // Check if at least primary WiFi is set
            const ssid1 = document.getElementById('wifi1Ssid').value.trim();
            const password1 = document.getElementById('wifi1Password').value.trim();
            
            if (!ssid1) {
                showAlert('Please enter the primary WiFi SSID', 'warning');
                return false;
            }
            
            if (!password1) {
                showAlert('Please enter the primary WiFi password', 'warning');
                return false;
            }
            
            // Check hostname
            const hostname = document.getElementById('hostname').value.trim();
            if (!hostname) {
                showAlert('Please enter a hostname for the device', 'warning');
                return false;
            }
            
            return true;
            
        case 2: // HTTP Authentication
            // Check username and password
            const username = document.getElementById('httpUsername').value.trim();
            const password = document.getElementById('httpPassword').value.trim();
            
            if (!username) {
                showAlert('Please enter a username for HTTP authentication', 'warning');
                return false;
            }
            
            if (!password) {
                showAlert('Please enter a password for HTTP authentication', 'warning');
                return false;
            }
            
            return true;
            
        case 3: // MQTT (optional)
            // If MQTT is enabled, check required fields
            const mqttEnabled = document.getElementById('mqttEnabled').checked;
            
            if (mqttEnabled) {
                const broker = document.getElementById('mqttBroker').value.trim();
                const topic = document.getElementById('mqttTopic').value.trim();
                
                if (!broker) {
                    showAlert('Please enter the MQTT broker address', 'warning');
                    return false;
                }
                
                if (!topic) {
                    showAlert('Please enter the MQTT topic', 'warning');
                    return false;
                }
            }
            
            return true;
            
        case 4: // GPIO Configuration
            // Only validate if custom GPIO config is selected
            const useDefault = document.getElementById('useDefaultGpio').checked;
            
            if (!useDefault) {
                // Simplistic validation - just check for empty fields
                const pins = [
                    'upperDhtPin', 'lowerDhtPin', 'scdSdaPin', 'scdSclPin',
                    'relay1Pin', 'relay2Pin', 'relay3Pin', 'relay4Pin',
                    'relay5Pin', 'relay6Pin', 'relay7Pin', 'relay8Pin'
                ];
                
                for (const pinId of pins) {
                    const value = document.getElementById(pinId).value.trim();
                    if (!value) {
                        showAlert(`Please enter a value for ${pinId}`, 'warning');
                        return false;
                    }
                }
            }
            
            return true;
            
        case 5: // Update Timings
            // Check for empty or invalid values
            const fields = [
                'dhtInterval', 'scdInterval', 'graphInterval', 'graphPoints'
            ];
            
            for (const fieldId of fields) {
                const value = document.getElementById(fieldId).value.trim();
                if (!value || isNaN(value) || parseInt(value) <= 0) {
                    showAlert(`Please enter a valid value for ${fieldId}`, 'warning');
                    return false;
                }
            }
            
            return true;
            
        case 6: // Relay Configuration
            // Check environmental thresholds
            if (parseFloat(document.getElementById('humidityLow').value) >= 
                parseFloat(document.getElementById('humidityHigh').value)) {
                showAlert('Humidity low threshold must be less than high threshold', 'warning');
                return false;
            }
            
            if (parseFloat(document.getElementById('temperatureLow').value) >= 
                parseFloat(document.getElementById('temperatureHigh').value)) {
                showAlert('Temperature low threshold must be less than high threshold', 'warning');
                return false;
            }
            
            if (parseFloat(document.getElementById('co2Low').value) >= 
                parseFloat(document.getElementById('co2High').value)) {
                showAlert('CO2 low threshold must be less than high threshold', 'warning');
                return false;
            }
            
            return true;
            
        default:
            return true;
    }
}

/**
 * Move to the next step
 */
function nextStep() {
    if (currentStep < totalSteps) {
        // Hide current step
        document.getElementById(`step-${currentStep}`).classList.remove('active');
        
        // Update step indicator
        document.getElementById(`step-indicator-${currentStep}`).classList.remove('active');
        document.getElementById(`step-indicator-${currentStep}`).classList.add('completed');
        
        // Increment step counter
        currentStep++;
        
        // Show next step
        document.getElementById(`step-${currentStep}`).classList.add('active');
        document.getElementById(`step-indicator-${currentStep}`).classList.add('active');
        
        // Show/hide navigation buttons
        document.getElementById('prevBtn').style.display = 'block';
        
        if (currentStep === totalSteps) {
            document.getElementById('nextBtn').style.display = 'none';
            document.getElementById('submitBtn').style.display = 'block';
        } else {
            document.getElementById('nextBtn').style.display = 'block';
            document.getElementById('submitBtn').style.display = 'none';
        }
    }
}

/**
 * Move to the previous step
 */
function prevStep() {
    if (currentStep > 1) {
        // Hide current step
        document.getElementById(`step-${currentStep}`).classList.remove('active');
        
        // Update step indicator
        document.getElementById(`step-indicator-${currentStep}`).classList.remove('active');
        document.getElementById(`step-indicator-${currentStep}`).classList.remove('completed');
        
        // Decrement step counter
        currentStep--;
        
        // Show previous step
        document.getElementById(`step-${currentStep}`).classList.add('active');
        document.getElementById(`step-indicator-${currentStep}`).classList.add('active');
        
        // Show/hide navigation buttons
        if (currentStep === 1) {
            document.getElementById('prevBtn').style.display = 'none';
        } else {
            document.getElementById('prevBtn').style.display = 'block';
        }
        
        document.getElementById('nextBtn').style.display = 'block';
        document.getElementById('submitBtn').style.display = 'none';
    }
}

/**
 * Scan for WiFi networks
 * @param {number} index - WiFi credential index (1-3)
 */
async function scanWiFi(index) {
    // Store current scan target
    document.getElementById('wifiScanModal').dataset.scanTarget = index;
    
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
 * Test WiFi connection with entered credentials
 * @param {number} index - WiFi credential index (1-3)
 */
async function testWiFiConnection(index) {
    const ssid = document.getElementById(`wifi${index}Ssid`).value.trim();
    const password = document.getElementById(`wifi${index}Password`).value.trim();
    
    if (!ssid) {
        showAlert('Please enter an SSID', 'warning');
        return;
    }
    
    // Show testing message
    const resultElement = document.getElementById(`wifiTestResult${index}`);
    resultElement.innerHTML = '<div class="text-info"><i class="fas fa-spinner fa-spin"></i> Testing connection...</div>';
    
    try {
        const result = await apiRequest(API.WIFI_TEST, 'POST', {
            ssid: ssid,
            password: password
        });
        
        if (result.success) {
            resultElement.innerHTML = '<div class="text-success"><i class="fas fa-check-circle"></i> Connection successful!</div>';
            
            // Set flag for first WiFi
            if (index === 1) {
                testSuccessful = true;
            }
        } else {
            resultElement.innerHTML = `<div class="text-danger"><i class="fas fa-times-circle"></i> Connection failed: ${result.message}</div>`;
            
            // Reset flag for first WiFi
            if (index === 1) {
                testSuccessful = false;
            }
        }
    } catch (error) {
        console.error('WiFi test failed:', error);
        resultElement.innerHTML = '<div class="text-danger"><i class="fas fa-times-circle"></i> Test failed: Network error</div>';
        
        // Reset flag for first WiFi
        if (index === 1) {
            testSuccessful = false;
        }
    }
}

/**
 * Submit the setup form
 */
async function submitSetup() {
    // Validate final step
    if (!validateCurrentStep()) {
        return;
    }
    
    // Collect all form data
    const formData = {
        wifi: {
            ssid1: document.getElementById('wifi1Ssid').value.trim(),
            password1: document.getElementById('wifi1Password').value.trim(),
            ssid2: document.getElementById('wifi2Ssid').value.trim(),
            password2: document.getElementById('wifi2Password').value.trim(),
            ssid3: document.getElementById('wifi3Ssid').value.trim(),
            password3: document.getElementById('wifi3Password').value.trim(),
            hostname: document.getElementById('hostname').value.trim()
        },
        http_auth: {
            username: document.getElementById('httpUsername').value.trim(),
            password: document.getElementById('httpPassword').value.trim()
        },
        mqtt: {
            enabled: document.getElementById('mqttEnabled').checked,
            broker: document.getElementById('mqttBroker').value.trim(),
            port: parseInt(document.getElementById('mqttPort').value) || 1883,
            topic: document.getElementById('mqttTopic').value.trim(),
            username: document.getElementById('mqttUsername').value.trim(),
            password: document.getElementById('mqttPassword').value.trim()
        },
        gpio_config: {
            use_default: document.getElementById('useDefaultGpio').checked
        },
        update_timings: {
            dht_interval: parseInt(document.getElementById('dhtInterval').value) || 5,
            scd_interval: parseInt(document.getElementById('scdInterval').value) || 10,
            graph_interval: parseInt(document.getElementById('graphInterval').value) || 30,
            graph_points: parseInt(document.getElementById('graphPoints').value) || 100
        },
        relay_config: {
            humidity_low: parseFloat(document.getElementById('humidityLow').value) || 50,
            humidity_high: parseFloat(document.getElementById('humidityHigh').value) || 85,
            temperature_low: parseFloat(document.getElementById('temperatureLow').value) || 20,
            temperature_high: parseFloat(document.getElementById('temperatureHigh').value) || 24,
            co2_low: parseFloat(document.getElementById('co2Low').value) || 1000,
            co2_high: parseFloat(document.getElementById('co2High').value) || 1600,
            override_timer: parseInt(document.getElementById('overrideDuration').value) || 5
        }
    };
    
    // Add custom GPIO configuration if not using defaults
    if (!formData.gpio_config.use_default) {
        formData.gpio_config.upper_dht_pin = parseInt(document.getElementById('upperDhtPin').value) || 13;
        formData.gpio_config.lower_dht_pin = parseInt(document.getElementById('lowerDhtPin').value) || 14;
        formData.gpio_config.scd_sda_pin = parseInt(document.getElementById('scdSdaPin').value) || 21;
        formData.gpio_config.scd_scl_pin = parseInt(document.getElementById('scdSclPin').value) || 22;
        formData.gpio_config.relay1_pin = parseInt(document.getElementById('relay1Pin').value) || 16;
        formData.gpio_config.relay2_pin = parseInt(document.getElementById('relay2Pin').value) || 17;
        formData.gpio_config.relay3_pin = parseInt(document.getElementById('relay3Pin').value) || 19;
        formData.gpio_config.relay4_pin = parseInt(document.getElementById('relay4Pin').value) || 26;
        formData.gpio_config.relay5_pin = parseInt(document.getElementById('relay5Pin').value) || 33;
        formData.gpio_config.relay6_pin = parseInt(document.getElementById('relay6Pin').value) || 23;
        formData.gpio_config.relay7_pin = parseInt(document.getElementById('relay7Pin').value) || 25;
        formData.gpio_config.relay8_pin = parseInt(document.getElementById('relay8Pin').value) || 35;
    }
    
    // If MQTT is not enabled, remove MQTT config
    if (!formData.mqtt.enabled) {
        delete formData.mqtt;
    }
    
    try {
        // Submit configuration to the server
        const result = await apiRequest(API.CONFIG_SAVE, 'POST', formData);
        
        if (result.success) {
            // Show completion modal
            showSetupCompleteModal();
        } else {
            showAlert(`Configuration failed: ${result.message}`, 'danger');
        }
    } catch (error) {
        console.error('Setup submission failed:', error);
        showAlert('Failed to save configuration. Please try again.', 'danger');
    }
}

/**
 * Show the setup complete modal
 */
function showSetupCompleteModal() {
    // Update device URL
    const hostname = document.getElementById('hostname').value.trim();
    document.getElementById('deviceUrl').textContent = `http://${hostname}.local`;
    
    // Show modal
    showModal('setupCompleteModal');
    
    // Start reboot countdown
    let countdown = 5;
    document.getElementById('rebootStatus').textContent = `Rebooting in ${countdown} seconds...`;
    
    // Update progress bar
    const progressBar = document.getElementById('rebootProgressBar');
    progressBar.style.width = '0%';
    
    // Start countdown
    rebootTimer = setInterval(() => {
        countdown--;
        if (countdown <= 0) {
            clearInterval(rebootTimer);
            document.getElementById('rebootStatus').textContent = 'Rebooting now...';
            progressBar.style.width = '100%';
            
            // Simulate reboot by redirecting to the new URL after a delay
            setTimeout(() => {
                const hostname = document.getElementById('hostname').value.trim();
                window.location.href = `http://${hostname}.local`;
            }, 2000);
        } else {
            document.getElementById('rebootStatus').textContent = `Rebooting in ${countdown} seconds...`;
            const progress = ((5 - countdown) / 5) * 100;
            progressBar.style.width = `${progress}%`;
        }
    }, 1000);
}