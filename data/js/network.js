/**
 * Mushroom Tent Controller - Network JavaScript
 * Handles network configuration functionality
 */

document.addEventListener('DOMContentLoaded', function() {
    // Initialize network page functionality
    if (document.getElementById('networkForm')) {
        setupNetworkHandlers();
    }
});

/**
 * Setup event handlers for the network page
 */
function setupNetworkHandlers() {
    // Save network settings button
    const saveNetworkBtn = document.getElementById('saveNetworkBtn');
    if (saveNetworkBtn) {
        saveNetworkBtn.addEventListener('click', saveNetworkConfig);
    }

    // Cancel network settings button
    const cancelNetworkBtn = document.getElementById('cancelNetworkBtn');
    if (cancelNetworkBtn) {
        cancelNetworkBtn.addEventListener('click', function() {
            hideModal('networkModal');
        });
    }

    // WiFi scan buttons
    const scanWifi1Btn = document.getElementById('scanWifi1Btn');
    if (scanWifi1Btn) {
        scanWifi1Btn.addEventListener('click', function() {
            scanWiFi(1);
        });
    }

    const scanWifi2Btn = document.getElementById('scanWifi2Btn');
    if (scanWifi2Btn) {
        scanWifi2Btn.addEventListener('click', function() {
            scanWiFi(2);
        });
    }

    const scanWifi3Btn = document.getElementById('scanWifi3Btn');
    if (scanWifi3Btn) {
        scanWifi3Btn.addEventListener('click', function() {
            scanWiFi(3);
        });
    }

    // WiFi scan modal close button
    const closeWifiScanModal = document.getElementById('closeWifiScanModal');
    if (closeWifiScanModal) {
        closeWifiScanModal.addEventListener('click', function() {
            hideModal('wifiScanModal');
        });
    }

    // WiFi scan cancel button
    const cancelWifiScanBtn = document.getElementById('cancelWifiScanBtn');
    if (cancelWifiScanBtn) {
        cancelWifiScanBtn.addEventListener('click', function() {
            hideModal('wifiScanModal');
        });
    }

    // WiFi scan refresh button
    const refreshWifiScanBtn = document.getElementById('refreshWifiScanBtn');
    if (refreshWifiScanBtn) {
        refreshWifiScanBtn.addEventListener('click', function() {
            // Get current scan target
            const index = parseInt(document.getElementById('wifiScanModal').dataset.scanTarget || '1');
            scanWiFi(index);
        });
    }

    // IP Configuration radio buttons
    const dhcpEnabled = document.getElementById('dhcpEnabled');
    const staticIpEnabled = document.getElementById('staticIpEnabled');
    const staticIpFields = document.getElementById('staticIpFields');

    if (dhcpEnabled && staticIpEnabled && staticIpFields) {
        dhcpEnabled.addEventListener('change', function() {
            staticIpFields.style.display = this.checked ? 'none' : 'block';
        });

        staticIpEnabled.addEventListener('change', function() {
            staticIpFields.style.display = this.checked ? 'block' : 'none';
        });
    }

    // MQTT toggle
    const mqttEnabled = document.getElementById('mqttEnabled');
    const mqttFields = document.getElementById('mqttFields');
    
    if (mqttEnabled && mqttFields) {
        mqttEnabled.addEventListener('change', function() {
            mqttFields.style.display = this.checked ? 'block' : 'none';
        });
    }
}

/**
 * Load network configuration from the server
 */
async function loadNetworkConfig() {
    try {
        const data = await apiRequest(API.NETWORK);

        // Populate WiFi settings
        document.getElementById('wifi1Ssid').value = data.wifi ? data.wifi.ssid1 : '';
        document.getElementById('wifi1Password').value = ''; // Don't populate password for security
        document.getElementById('wifi2Ssid').value = data.wifi ? data.wifi.ssid2 : '';
        document.getElementById('wifi2Password').value = ''; // Don't populate password for security
        document.getElementById('wifi3Ssid').value = data.wifi ? data.wifi.ssid3 : '';
        document.getElementById('wifi3Password').value = ''; // Don't populate password for security
        document.getElementById('hostname').value = data.wifi ? data.wifi.hostname : 'mushroom';

        // Populate IP configuration
        const useDHCP = data.wifi && data.wifi.ip_config ? data.wifi.ip_config.dhcp : true;
        document.getElementById('dhcpEnabled').checked = useDHCP;
        document.getElementById('staticIpEnabled').checked = !useDHCP;
        document.getElementById('staticIpFields').style.display = useDHCP ? 'none' : 'block';

        if (data.wifi && data.wifi.ip_config) {
            document.getElementById('staticIp').value = data.wifi.ip_config.ip || '';
            document.getElementById('subnetMask').value = data.wifi.ip_config.subnet || '';
            document.getElementById('gateway').value = data.wifi.ip_config.gateway || '';
            document.getElementById('dns1').value = data.wifi.ip_config.dns1 || '';
            document.getElementById('dns2').value = data.wifi.ip_config.dns2 || '';
        }

        // Populate WiFi watchdog settings
        if (data.wifi && data.wifi.watchdog) {
            document.getElementById('minRssi').value = data.wifi.watchdog.min_rssi || -80;
            document.getElementById('wifiCheckInterval').value = data.wifi.watchdog.check_interval || 30;
        }

        // Populate MQTT settings
        const mqttEnabled = data.mqtt ? data.mqtt.enabled : false;
        document.getElementById('mqttEnabled').checked = mqttEnabled;
        document.getElementById('mqttFields').style.display = mqttEnabled ? 'block' : 'none';

        if (data.mqtt) {
            document.getElementById('mqttBroker').value = data.mqtt.broker || '';
            document.getElementById('mqttPort').value = data.mqtt.port || 1883;
            document.getElementById('mqttTopic').value = data.mqtt.topic || '';
            document.getElementById('mqttUsername').value = data.mqtt.username || '';
            document.getElementById('mqttPassword').value = ''; // Don't populate password for security
        }

    } catch (error) {
        console.error('Failed to load network configuration:', error);
        showAlert('Failed to load network configuration', 'danger');
    }
}

/**
 * Save network configuration to the server
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