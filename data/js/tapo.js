/**
 * Mushroom Tent Controller - Tapo P100 JavaScript
 * Handles Tapo smart plug devices functionality
 */

document.addEventListener('DOMContentLoaded', function() {
    // Initialize Tapo page functionality
    if (document.getElementById('tapoConfigForm')) {
        setupTapoHandlers();
        loadTapoConfig();
    }
});

/**
 * Setup event handlers for the Tapo page
 */
function setupTapoHandlers() {
    // Test individual device connections
    for (let i = 1; i <= 3; i++) {
        const testBtn = document.getElementById(`testTapoDevice${i}Btn`);
        if (testBtn) {
            testBtn.addEventListener('click', function() {
                testTapoDeviceConnection(i);
            });
        }

        const toggleBtn = document.getElementById(`tapoDevice${i}Toggle`);
        if (toggleBtn) {
            toggleBtn.addEventListener('change', function() {
                toggleTapoDevice(i, this.checked);
            });
        }
    }

    // Test all devices
    const testAllBtn = document.getElementById('testAllTapoDevicesBtn');
    if (testAllBtn) {
        testAllBtn.addEventListener('click', function() {
            testAllTapoDevices();
        });
    }

    // Save configuration
    const saveConfigBtn = document.getElementById('saveTapoConfigBtn');
    if (saveConfigBtn) {
        saveConfigBtn.addEventListener('click', function() {
            saveTapoConfig();
        });
    }
}

/**
 * Load Tapo configuration from the server
 */
async function loadTapoConfig() {
    try {
        // In a real implementation, this would fetch from an API endpoint
        // For now, we'll just populate with default values or mock data
        
        // Example mock data for testing UI
        const mockConfig = {
            username: "test@example.com",
            devices: [
                {
                    ip: "192.168.1.100",
                    alias: "Main PSU",
                    relay: 1,
                    status: false
                },
                {
                    ip: "192.168.1.101",
                    alias: "Heater",
                    relay: 6,
                    status: false
                },
                {
                    ip: "",
                    alias: "",
                    relay: 0,
                    status: false
                }
            ]
        };

        // Populate form fields
        document.getElementById('tapoUsername').value = mockConfig.username;
        document.getElementById('tapoPassword').value = ''; // Don't populate password for security
        
        // Populate device fields
        for (let i = 1; i <= 3; i++) {
            if (i <= mockConfig.devices.length) {
                const device = mockConfig.devices[i-1];
                document.getElementById(`tapoDevice${i}IP`).value = device.ip;
                document.getElementById(`tapoDevice${i}Alias`).value = device.alias;
                document.getElementById(`tapoDevice${i}Relay`).value = device.relay;
                
                // Update toggle state
                const toggle = document.getElementById(`tapoDevice${i}Toggle`);
                toggle.checked = device.status;
                toggle.disabled = !device.ip; // Disable toggle if no IP
            }
        }
        
    } catch (error) {
        console.error('Failed to load Tapo configuration:', error);
        showAlert('Failed to load Tapo configuration', 'danger');
    }
}

/**
 * Test connection to a Tapo device
 * @param {number} deviceIndex - Device index (1-3)
 */
async function testTapoDeviceConnection(deviceIndex) {
    const ipElement = document.getElementById(`tapoDevice${deviceIndex}IP`);
    const statusElement = document.getElementById(`tapoDevice${deviceIndex}Status`);
    const toggleElement = document.getElementById(`tapoDevice${deviceIndex}Toggle`);
    
    if (!ipElement || !statusElement) return;
    
    const ip = ipElement.value.trim();
    if (!ip) {
        statusElement.innerHTML = '<div class="text-danger"><i class="fas fa-exclamation-circle"></i> Please enter an IP address</div>';
        return;
    }
    
    // Show testing message
    statusElement.innerHTML = '<div class="text-info"><i class="fas fa-spinner fa-spin"></i> Testing connection...</div>';
    
    try {
        // This would be an actual API call in the real implementation
        // For demonstration, we'll simulate a response after a delay
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        // Simulate 80% success rate for demonstration
        const success = Math.random() > 0.2;
        
        if (success) {
            statusElement.innerHTML = '<div class="text-success"><i class="fas fa-check-circle"></i> Connection successful!</div>';
            
            // Enable toggle
            toggleElement.disabled = false;
            
            // Get device status (fake for now)
            const isOn = Math.random() > 0.5;
            toggleElement.checked = isOn;
        } else {
            statusElement.innerHTML = '<div class="text-danger"><i class="fas fa-times-circle"></i> Connection failed! Please check IP and credentials</div>';
            toggleElement.disabled = true;
        }
    } catch (error) {
        console.error(`Error testing Tapo device ${deviceIndex}:`, error);
        statusElement.innerHTML = '<div class="text-danger"><i class="fas fa-times-circle"></i> Error testing connection</div>';
        toggleElement.disabled = true;
    }
}

/**
 * Toggle a Tapo device on/off
 * @param {number} deviceIndex - Device index (1-3)
 * @param {boolean} turnOn - Whether to turn the device on or off
 */
async function toggleTapoDevice(deviceIndex, turnOn) {
    const statusElement = document.getElementById(`tapoDevice${deviceIndex}Status`);
    const toggleElement = document.getElementById(`tapoDevice${deviceIndex}Toggle`);
    
    if (!statusElement || !toggleElement) return;
    
    // Show action message
    statusElement.innerHTML = `<div class="text-info"><i class="fas fa-spinner fa-spin"></i> Turning ${turnOn ? 'on' : 'off'}...</div>`;
    
    try {
        // This would be an actual API call in the real implementation
        // For demonstration, we'll simulate a response after a delay
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        // Simulate 90% success rate for demonstration
        const success = Math.random() > 0.1;
        
        if (success) {
            statusElement.innerHTML = `<div class="text-success"><i class="fas fa-check-circle"></i> Device ${turnOn ? 'turned on' : 'turned off'} successfully!</div>`;
            
            // Keep toggle in new state
            toggleElement.checked = turnOn;
        } else {
            statusElement.innerHTML = '<div class="text-danger"><i class="fas fa-times-circle"></i> Failed to change device state</div>';
            
            // Reset toggle to previous state
            toggleElement.checked = !turnOn;
        }
    } catch (error) {
        console.error(`Error toggling Tapo device ${deviceIndex}:`, error);
        statusElement.innerHTML = '<div class="text-danger"><i class="fas fa-times-circle"></i> Error changing device state</div>';
        
        // Reset toggle to previous state
        toggleElement.checked = !turnOn;
    }
}

/**
 * Test connection to all configured Tapo devices
 */
async function testAllTapoDevices() {
    showAlert('Testing all Tapo device connections...', 'info');
    
    // Test each device sequentially
    for (let i = 1; i <= 3; i++) {
        const ipElement = document.getElementById(`tapoDevice${i}IP`);
        if (ipElement && ipElement.value.trim()) {
            await testTapoDeviceConnection(i);
        }
    }
    
    showAlert('Completed testing all configured Tapo devices', 'success');
}

/**
 * Save Tapo configuration to the server
 */
async function saveTapoConfig() {
    // Collect configuration from form
    const username = document.getElementById('tapoUsername').value.trim();
    const password = document.getElementById('tapoPassword').value;
    
    if (!username) {
        showAlert('Please enter your Tapo account username (email)', 'warning');
        return;
    }
    
    // Collect device configurations
    const devices = [];
    for (let i = 1; i <= 3; i++) {
        const ip = document.getElementById(`tapoDevice${i}IP`).value.trim();
        const alias = document.getElementById(`tapoDevice${i}Alias`).value.trim();
        const relay = parseInt(document.getElementById(`tapoDevice${i}Relay`).value);
        
        if (ip) {
            devices.push({
                ip: ip,
                alias: alias || `Device ${i}`,
                relay: relay
            });
        }
    }
    
    // Create config object
    const config = {
        username: username,
        password: password, // In a real implementation, this should be handled securely
        devices: devices
    };
    
    try {
        // This would be an actual API call in the real implementation
        // For demonstration, we'll simulate a response after a delay
        await new Promise(resolve => setTimeout(resolve, 1000));
        
        // Simulate success
        showAlert('Tapo configuration saved successfully', 'success');
        
        // Reload configuration
        await loadTapoConfig();
    } catch (error) {
        console.error('Error saving Tapo configuration:', error);
        showAlert('Error saving Tapo configuration', 'danger');
    }
}