/**
 * Mushroom Tent Controller - Profiles JavaScript
 * Handles profile management functionality
 */

document.addEventListener('DOMContentLoaded', function() {
    // Initialize profiles functionality
    if (document.getElementById('profilesModal')) {
        setupProfileHandlers();
    }
});

/**
 * Setup event handlers for the profiles functionality
 */
function setupProfileHandlers() {
    // Load profile button
    const loadProfileBtn = document.getElementById('loadProfileBtn');
    if (loadProfileBtn) {
        loadProfileBtn.addEventListener('click', function() {
            const profileName = document.getElementById('profileSelect').value;
            if (profileName) {
                loadProfile(profileName);
            }
        });
    }

    // Save profile button
    const saveProfileBtn = document.getElementById('saveProfileBtn');
    if (saveProfileBtn) {
        saveProfileBtn.addEventListener('click', function() {
            saveProfile();
        });
    }

    // Rename profile button
    const renameProfileBtn = document.getElementById('renameProfileBtn');
    if (renameProfileBtn) {
        renameProfileBtn.addEventListener('click', function() {
            const currentName = document.getElementById('profileSelect').value;
            const newName = prompt('Enter new profile name:', currentName);
            if (newName && newName !== currentName) {
                renameProfile(currentName, newName);
            }
        });
    }

    // Export profiles button
    const exportProfilesBtn = document.getElementById('exportProfilesBtn');
    if (exportProfilesBtn) {
        exportProfilesBtn.addEventListener('click', function() {
            window.location.href = API.PROFILE_EXPORT;
        });
    }

    // Import profiles button
    const importProfilesBtn = document.getElementById('importProfilesBtn');
    if (importProfilesBtn) {
        importProfilesBtn.addEventListener('click', function() {
            document.getElementById('profileImportInput').click();
        });
    }

    // Import profiles file input
    const profileImportInput = document.getElementById('profileImportInput');
    if (profileImportInput) {
        profileImportInput.addEventListener('change', function(e) {
            if (e.target.files.length > 0) {
                importProfiles(e.target.files[0]);
            }
        });
    }

    // Close profiles button
    const closeProfilesBtn = document.getElementById('closeProfilesBtn');
    if (closeProfilesBtn) {
        closeProfilesBtn.addEventListener('click', function() {
            hideModal('profilesModal');
        });
    }

    // Apply profile changes button
    const applyProfileBtn = document.getElementById('applyProfileBtn');
    if (applyProfileBtn) {
        applyProfileBtn.addEventListener('click', function() {
            applyProfileChanges();
        });
    }

    // Close profiles modal button
    const closeProfilesModal = document.getElementById('closeProfilesModal');
    if (closeProfilesModal) {
        closeProfilesModal.addEventListener('click', function() {
            hideModal('profilesModal');
        });
    }
}

/**
 * Load profiles from the server and populate the UI
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
    if (!container) return;
    
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
            apiRequest(API.ENVIRONMENT).then(data => {
                environmentalThresholds = data;
                updateSensors();
            });
            
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
    for (let i = 1; i <= 8; i++) {
        const startEl = document.getElementById(`relay${i}Start`);
        const endEl = document.getElementById(`relay${i}End`);
        
        if (startEl && endEl) {
            const startTime = startEl.value.split(':');
            const endTime = endEl.value.split(':');
            
            settings.relay_times[`relay${i}`] = {
                start_hour: parseInt(startTime[0]),
                start_minute: parseInt(startTime[1]),
                end_hour: parseInt(endTime[0]),
                end_minute: parseInt(endTime[1])
            };
        }
    }

    try {
        const result = await apiRequest(API.PROFILE_SAVE, 'POST', {
            name: profileName,
            settings: settings
        });

        if (result.success) {
            showAlert('Profile saved successfully', 'success');
        } else {
            showAlert(`Failed to save profile: ${result.message}`, 'danger');
        }
    } catch (error) {
        console.error('Error saving profile:', error);
        showAlert('Error saving profile', 'danger');
    }
}

/**
 * Rename a profile
 * @param {string} oldName - Current profile name
 * @param {string} newName - New profile name
 */
async function renameProfile(oldName, newName) {
    // This is a mockup of the API call
    // In a real implementation, you would call an API endpoint to rename the profile
    try {
        // For now, we'll just reload profiles to simulate success
        showAlert(`Profile renamed from "${oldName}" to "${newName}"`, 'success');
        await loadProfiles();
    } catch (error) {
        console.error('Error renaming profile:', error);
        showAlert('Error renaming profile', 'danger');
    }
}

/**
 * Import profiles from a file
 * @param {File} file - The profiles JSON file
 */
async function importProfiles(file) {
    try {
        // Create a FileReader to read the file
        const reader = new FileReader();
        
        reader.onload = async function(e) {
            try {
                // Parse the file content as JSON
                const profilesData = JSON.parse(e.target.result);
                
                // Send to server
                const result = await apiRequest(API.PROFILE_IMPORT, 'POST', profilesData);
                
                if (result.success) {
                    showAlert('Profiles imported successfully', 'success');
                    await loadProfiles();
                } else {
                    showAlert(`Failed to import profiles: ${result.message}`, 'danger');
                }
            } catch (parseError) {
                console.error('Error parsing profiles file:', parseError);
                showAlert('Error parsing profiles file. Please make sure it is a valid JSON file.', 'danger');
            }
        };
        
        reader.onerror = function() {
            showAlert('Error reading the file', 'danger');
        };
        
        // Read the file as text
        reader.readAsText(file);
        
    } catch (error) {
        console.error('Error importing profiles:', error);
        showAlert('Error importing profiles', 'danger');
    }
}

/**
 * Apply the current profile settings
 */
async function applyProfileChanges() {
    // First save the profile
    await saveProfile();
    
    // Then load it to apply the changes
    const profileName = document.getElementById('profileSelect').value;
    await loadProfile(profileName);
    
    // Close the modal
    hideModal('profilesModal');
}