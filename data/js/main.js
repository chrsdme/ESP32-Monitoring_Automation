/**
 * Mushroom Tent Controller - Main JavaScript
 * Contains shared utility functions and global handlers
 */

// API endpoints
const API = {
    SENSORS: '/api/sensors/data',
    GRAPH: '/api/sensors/graph',
    RELAYS: '/api/relays/status',
    RELAY_SET: '/api/relays/set',
    SETTINGS: '/api/settings',
    SETTINGS_UPDATE: '/api/settings/update',
    NETWORK: '/api/network/config',
    NETWORK_UPDATE: '/api/network/update',
    WIFI_SCAN: '/api/wifi/scan',
    WIFI_TEST: '/api/wifi/test',
    ENVIRONMENT: '/api/environment/thresholds',
    ENVIRONMENT_UPDATE: '/api/environment/update',
    SYSTEM_INFO: '/api/system/info',
    SYSTEM_FILES: '/api/system/files',
    SYSTEM_DELETE: '/api/system/delete',
    SYSTEM_REBOOT: '/api/system/reboot',
    SYSTEM_FACTORY_RESET: '/api/system/factory-reset',
    UPLOAD: '/api/upload',
    PROFILES: '/api/profiles',
    PROFILE_SAVE: '/api/profiles/save',
    PROFILE_LOAD: '/api/profiles/load',
    PROFILE_EXPORT: '/api/profiles/export',
    PROFILE_IMPORT: '/api/profiles/import',
    CONFIG_SAVE: '/api/config/save',
};

// Global variables
let darkMode = localStorage.getItem('darkMode') === 'true';

// Initialize theme based on saved preference
document.addEventListener('DOMContentLoaded', function() {
    // Apply saved theme preference
    if (darkMode) {
        document.documentElement.setAttribute('data-theme', 'dark');
    } else {
        document.documentElement.setAttribute('data-theme', 'light');
    }
    
    // Setup theme toggle button if it exists
    const themeToggleBtn = document.getElementById('lightDarkToggle');
    if (themeToggleBtn) {
        themeToggleBtn.addEventListener('click', toggleTheme);
        updateThemeToggleIcon();
    }
    
    // Attach global event listeners to modals
    setupModalListeners();
});

/**
 * Toggle between light and dark themes
 */
function toggleTheme() {
    darkMode = !darkMode;
    localStorage.setItem('darkMode', darkMode);
    
    if (darkMode) {
        document.documentElement.setAttribute('data-theme', 'dark');
    } else {
        document.documentElement.removeAttribute('data-theme');
    }
    
    updateThemeToggleIcon();
}

/**
 * Update the theme toggle button icon based on current theme
 */
function updateThemeToggleIcon() {
    const themeToggleBtn = document.getElementById('lightDarkToggle');
    if (themeToggleBtn) {
        if (darkMode) {
            themeToggleBtn.innerHTML = '<i class="fas fa-moon"></i>';
            themeToggleBtn.title = 'Switch to light mode';
        } else {
            themeToggleBtn.innerHTML = '<i class="fas fa-lightbulb"></i>';
            themeToggleBtn.title = 'Switch to dark mode';
        }
    }
}

/**
 * Display an alert message to the user
 * @param {string} message - The message to display
 * @param {string} type - The type of alert (success, danger, warning, info)
 * @param {number} duration - How long to display the alert in milliseconds (0 for persistent)
 */
function showAlert(message, type = 'info', duration = 5000) {
    const alertContainer = document.getElementById('alertContainer');
    if (!alertContainer) return;
    
    // Clear any existing alerts
    alertContainer.innerHTML = '';
    
    // Create new alert
    const alert = document.createElement('div');
    alert.className = `alert alert-${type}`;
    alert.innerHTML = message;
    
    // Add close button for persistent alerts
    if (duration === 0) {
        const closeBtn = document.createElement('button');
        closeBtn.className = 'close';
        closeBtn.innerHTML = '&times;';
        closeBtn.style.float = 'right';
        closeBtn.style.cursor = 'pointer';
        closeBtn.style.border = 'none';
        closeBtn.style.background = 'none';
        closeBtn.style.fontSize = '1.25rem';
        closeBtn.style.fontWeight = 'bold';
        closeBtn.style.lineHeight = '1';
        closeBtn.style.opacity = '0.5';
        closeBtn.addEventListener('click', function() {
            alertContainer.style.display = 'none';
        });
        
        alert.prepend(closeBtn);
    }
    
    // Add to container and show
    alertContainer.appendChild(alert);
    alertContainer.style.display = 'block';
    
    // Auto hide after duration (if not persistent)
    if (duration > 0) {
        setTimeout(function() {
            alertContainer.style.display = 'none';
        }, duration);
    }
}

/**
 * Make an API request
 * @param {string} endpoint - API endpoint to call
 * @param {string} method - HTTP method (GET, POST, PUT, DELETE)
 * @param {Object} data - Data to send (for POST/PUT requests)
 * @returns {Promise} - Promise that resolves with the API response
 */
async function apiRequest(endpoint, method = 'GET', data = null) {
    const options = {
        method: method,
        headers: {
            'Content-Type': 'application/json'
        }
    };
    
    if (data && (method === 'POST' || method === 'PUT')) {
        options.body = JSON.stringify(data);
    }
    
    try {
        const response = await fetch(endpoint, options);
        
        // Check for HTTP errors
        if (!response.ok) {
            throw new Error(`HTTP error ${response.status}: ${response.statusText}`);
        }
        
        // Parse response as JSON
        const result = await response.json();
        return result;
    } catch (error) {
        console.error('API request failed:', error);
        throw error;
    }
}

/**
 * Format a timestamp for display
 * @param {number} timestamp - Timestamp in seconds
 * @returns {string} - Formatted time string (HH:MM:SS)
 */
function formatTime(timestamp) {
    const date = new Date(timestamp * 1000);
    const hours = date.getHours().toString().padStart(2, '0');
    const minutes = date.getMinutes().toString().padStart(2, '0');
    const seconds = date.getSeconds().toString().padStart(2, '0');
    return `${hours}:${minutes}:${seconds}`;
}

/**
 * Format a file size for display
 * @param {number} bytes - Size in bytes
 * @returns {string} - Formatted size with appropriate unit
 */
function formatFileSize(bytes) {
    if (bytes < 1024) {
        return bytes + ' B';
    } else if (bytes < 1024 * 1024) {
        return (bytes / 1024).toFixed(1) + ' KB';
    } else if (bytes < 1024 * 1024 * 1024) {
        return (bytes / (1024 * 1024)).toFixed(1) + ' MB';
    } else {
        return (bytes / (1024 * 1024 * 1024)).toFixed(1) + ' GB';
    }
}

/**
 * Show a modal dialog
 * @param {string} modalId - ID of the modal to show
 */
function showModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.add('show');
    }
}

/**
 * Hide a modal dialog
 * @param {string} modalId - ID of the modal to hide
 */
function hideModal(modalId) {
    const modal = document.getElementById(modalId);
    if (modal) {
        modal.classList.remove('show');
    }
}

/**
 * Setup event listeners for modals
 */
function setupModalListeners() {
    // Get all modal close buttons
    const closeButtons = document.querySelectorAll('.modal-close');
    
    // Add click event to each close button
    closeButtons.forEach(function(button) {
        button.addEventListener('click', function() {
            // Find the parent modal
            const modal = button.closest('.modal');
            if (modal) {
                modal.classList.remove('show');
            }
        });
    });
    
    // Close modal when clicking outside content
    document.addEventListener('click', function(event) {
        if (event.target.classList.contains('modal') && event.target.classList.contains('show')) {
            event.target.classList.remove('show');
        }
    });
    
    // Prevent modal content clicks from closing the modal
    const modalContents = document.querySelectorAll('.modal-content');
    modalContents.forEach(function(content) {
        content.addEventListener('click', function(event) {
            event.stopPropagation();
        });
    });
}

/**
 * Generate a random ID string
 * @returns {string} - Random ID
 */
function generateId() {
    return '_' + Math.random().toString(36).substr(2, 9);
}

/**
 * Check if a value is within a range
 * @param {number} value - Value to check
 * @param {number} low - Low threshold
 * @param {number} high - High threshold
 * @returns {string} - CSS class name based on comparison (optimal/warning/danger)
 */
function getValueClass(value, low, high) {
    if (value < low || value > high) {
        return 'value-danger';
    } else if (value <= low + (high - low) * 0.1 || value >= high - (high - low) * 0.1) {
        return 'value-warning';
    } else {
        return 'value-optimal';
    }
}

/**
 * Flash an indicator element to show data update
 * @param {HTMLElement} element - The indicator element to flash
 */
function flashUpdateIndicator(element) {
    if (!element) return;
    
    element.classList.add('updating');
    
    setTimeout(function() {
        element.classList.remove('updating');
    }, 1000);
}

/**
 * Get the file extension from a filename
 * @param {string} filename - The filename to analyze
 * @returns {string} - The file extension (without the dot)
 */
function getFileExtension(filename) {
    return filename.split('.').pop().toLowerCase();
}

/**
 * Check if a file is a text file based on extension
 * @param {string} filename - The filename to check
 * @returns {boolean} - True if it's a text file
 */
function isTextFile(filename) {
    const textExtensions = ['txt', 'html', 'htm', 'css', 'js', 'json', 'xml', 'csv', 'md', 'log'];
    return textExtensions.includes(getFileExtension(filename));
}

/**
 * Check if a file is an image file based on extension
 * @param {string} filename - The filename to check
 * @returns {boolean} - True if it's an image file
 */
function isImageFile(filename) {
    const imageExtensions = ['jpg', 'jpeg', 'png', 'gif', 'svg', 'bmp', 'ico'];
    return imageExtensions.includes(getFileExtension(filename));
}

/**
 * Upload a file to the server
 * @param {File} file - The file to upload
 * @param {string} endpoint - API endpoint for upload
 * @param {Function} progressCallback - Callback for upload progress updates
 * @param {Function} completeCallback - Callback for upload completion
 * @param {Function} errorCallback - Callback for upload errors
 */
function uploadFile(file, endpoint, progressCallback, completeCallback, errorCallback) {
    const xhr = new XMLHttpRequest();
    const formData = new FormData();
    
    // Add file to form data
    formData.append('file', file);
    
    // Setup progress monitoring
    xhr.upload.addEventListener('progress', function(event) {
        if (event.lengthComputable) {
            const percentComplete = (event.loaded / event.total) * 100;
            if (progressCallback) {
                progressCallback(percentComplete);
            }
        }
    });
    
    // Setup completion handler
    xhr.addEventListener('load', function() {
        if (xhr.status >= 200 && xhr.status < 300) {
            if (completeCallback) {
                completeCallback(xhr.responseText);
            }
        } else {
            if (errorCallback) {
                errorCallback(`HTTP Error ${xhr.status}: ${xhr.statusText}`);
            }
        }
    });
    
    // Setup error handler
    xhr.addEventListener('error', function() {
        if (errorCallback) {
            errorCallback('Upload failed. Network error.');
        }
    });
    
    // Start the upload
    xhr.open('POST', endpoint, true);
    xhr.send(formData);
}

/**
 * Confirm an action with the user
 * @param {string} message - Confirmation message
 * @param {Function} callback - Function to call if user confirms
 */
function confirmAction(message, callback) {
    if (confirm(message)) {
        callback();
    }
}