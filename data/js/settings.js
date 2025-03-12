/**
 * Mushroom Tent Controller - Settings JavaScript
 * Handles settings page functionality
 */

document.addEventListener('DOMContentLoaded', function() {
    // Initialize settings page functionality
    if (document.getElementById('settingsForm')) {
        setupSettingsHandlers();
    }
});

/**
 * Setup event handlers for the settings page
 */
function setupSettingsHandlers() {
    // Save settings button
    const saveSettingsBtn = document.getElementById('saveSettingsBtn');
    if (saveSettingsBtn) {
        saveSettingsBtn.addEventListener('click', saveSettings);
    }

    // Cancel settings button
    const cancelSettingsBtn = document.getElementById('cancelSettingsBtn');
    if (cancelSettingsBtn) {
        cancelSettingsBtn.addEventListener('click', function() {
            hideModal('settingsModal');
        });
    }

    // Reboot button
    const rebootBtn = document.getElementById('rebootBtn');
    if (rebootBtn) {
        rebootBtn.addEventListener('click', function() {
            confirmAction('Are you sure you want to reboot the device?', performReboot);
        });
    }

    // Factory reset button
    const factoryResetBtn = document.getElementById('factoryResetBtn');
    if (factoryResetBtn) {
        factoryResetBtn.addEventListener('click', function() {
            confirmAction('WARNING: This will reset all settings to factory defaults! Are you sure?', performFactoryReset);
        });
    }

    // Populate hours in reboot hour dropdown if empty
    const rebootHour = document.getElementById('rebootHour');
    if (rebootHour && rebootHour.children.length === 0) {
        for (let h = 0; h < 24; h++) {
            const option = document.createElement('option');
            option.value = h;
            option.textContent = h.toString().padStart(2, '0');
            rebootHour.appendChild(option);
        }
    }

    // Populate minutes in reboot minute dropdown if empty
    const rebootMinute = document.getElementById('rebootMinute');
    if (rebootMinute && rebootMinute.children.length === 0) {
        for (let m = 0; m < 60; m += 5) {
            const option = document.createElement('option');
            option.value = m;
            option.textContent = m.toString().padStart(2, '0');
            rebootMinute.appendChild(option);
        }
    }
}

/**
 * Load settings from the server
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
        document.getElementById('rebootHour').value = data.reboot_scheduler ? data.reboot_scheduler.hour : 3;
        document.getElementById('rebootMinute').value = data.reboot_scheduler ? data.reboot_scheduler.minute : 0;
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
 * Save settings to the server
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
 * Perform device reboot
 */
async function performReboot() {
    try {
        showAlert('Rebooting device...', 'info');
        
        const result = await apiRequest(API.SYSTEM_REBOOT, 'POST');
        
        // The device is rebooting, so we won't get a response
        // Show a message that will stay on screen
        showAlert('Device is rebooting. Please wait a moment before refreshing the page.', 'info', 0);
        
    } catch (error) {
        // This error is expected as the device will reboot before responding
        console.log('Reboot initiated');
    }
}

/**
 * Perform factory reset
 */
async function performFactoryReset() {
    try {
        showAlert('Performing factory reset...', 'warning');
        
        const result = await apiRequest(API.SYSTEM_FACTORY_RESET, 'POST');
        
        // The device is resetting, so we won't get a response
        // Show a message that will stay on screen
        showAlert('Factory reset initiated. The device will reboot to AP mode for reconfiguration.', 'warning', 0);
        
    } catch (error) {
        // This error is expected as the device will reboot before responding
        console.log('Factory reset initiated');
    }
}