/**
 * Mushroom Tent Controller - System Page JavaScript
 * Handles system page functionality including system info, OTA updates, logs, and file management
 */

// Global variables
let logUpdateTimer = null;
let logPaused = false;
let systemInfoTimer = null;
let selectedFilePath = '';

// Initialize the system page when the DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    // Load system information
    loadSystemInfo();
    
    // Start system information updates
    systemInfoTimer = setInterval(loadSystemInfo, 10000);  // Update every 10 seconds
    
    // Load log data
    updateLogs();
    
    // Start log updates
    logUpdateTimer = setInterval(updateLogs, 5000);  // Update every 5 seconds
    
    // Load file list
    loadFileList();
    
    // Setup event listeners
    setupEventListeners();
});

/**
 * Load and display system information
 */
async function loadSystemInfo() {
    try {
        const data = await apiRequest(API.SYSTEM_INFO);
        
        // Update IP and WiFi info
        document.getElementById('ipAddress').textContent = data.network ? data.network.ip_address : '--';
        document.getElementById('wifiSsid').textContent = data.network ? data.network.ssid : '--';
        document.getElementById('wifiRssi').textContent = data.network ? data.network.rssi : '--';
        
        // Update version info
        document.getElementById('fsVersion').textContent = data.app_version || '--';
        document.getElementById('fwVersion').textContent = data.fs_version || '--';
        
        // Update memory usage
        updateMemoryUsage(data);
        
    } catch (error) {
        console.error('Failed to load system information:', error);
        showAlert('Failed to load system information', 'danger');
    }
}

/**
 * Update memory usage displays and progress bars
 * @param {Object} data - System information data
 */
function updateMemoryUsage(data) {
    // Update HEAP usage
    if (data.memory) {
        const heapUsed = Math.round(data.memory.heap_size - data.memory.free_heap) / 1024;
        const heapTotal = Math.round(data.memory.heap_size / 1024);
        const heapFree = Math.round(data.memory.free_heap / 1024);
        const heapPercent = Math.round((heapUsed / heapTotal) * 100);
        
        document.getElementById('heapUsage').textContent = 
            `${heapUsed.toFixed(1)} KB used / ${heapFree.toFixed(1)} KB Free / Total: ${heapTotal.toFixed(1)} KB`;
        
        const heapBar = document.getElementById('heapProgressBar');
        heapBar.style.width = `${heapPercent}%`;
        
        // Set color based on usage
        if (heapPercent > 80) {
            heapBar.style.backgroundColor = 'var(--danger-color)';
        } else if (heapPercent > 60) {
            heapBar.style.backgroundColor = 'var(--warning-color)';
        } else {
            heapBar.style.backgroundColor = 'var(--success-color)';
        }
    }
    
    // Update SPIFFS usage
    if (data.spiffs) {
        const spiffsUsed = Math.round(data.spiffs.used_bytes / 1024);
        const spiffsTotal = Math.round(data.spiffs.total_bytes / 1024);
        const spiffsFree = Math.round((data.spiffs.total_bytes - data.spiffs.used_bytes) / 1024);
        const spiffsPercent = Math.round((data.spiffs.used_bytes / data.spiffs.total_bytes) * 100);
        
        document.getElementById('fsUsage').textContent = 
            `${spiffsUsed.toFixed(1)} KB used / ${spiffsFree.toFixed(1)} KB Free / Total: ${spiffsTotal.toFixed(1)} KB`;
        
        const fsBar = document.getElementById('fsProgressBar');
        fsBar.style.width = `${spiffsPercent}%`;
        
        // Set color based on usage
        if (spiffsPercent > 80) {
            fsBar.style.backgroundColor = 'var(--danger-color)';
        } else if (spiffsPercent > 60) {
            fsBar.style.backgroundColor = 'var(--warning-color)';
        } else {
            fsBar.style.backgroundColor = 'var(--success-color)';
        }
    }
    
    // Update NVS usage
    if (data.cpu) {
        const nvsUsed = data.cpu.nvs_used || 0;
        const nvsFree = data.cpu.nvs_free || 0;
        const nvsTotal = nvsUsed + nvsFree;
        const nvsPercent = nvsTotal > 0 ? Math.round((nvsUsed / nvsTotal) * 100) : 0;
        
        document.getElementById('nvsUsage').textContent = 
            `${nvsUsed} Used / ${nvsFree} Free / ${nvsTotal} total entries`;
        
        const nvsBar = document.getElementById('nvsProgressBar');
        nvsBar.style.width = `${nvsPercent}%`;
        
        // Set color based on usage
        if (nvsPercent > 80) {
            nvsBar.style.backgroundColor = 'var(--danger-color)';
        } else if (nvsPercent > 60) {
            nvsBar.style.backgroundColor = 'var(--warning-color)';
        } else {
            nvsBar.style.backgroundColor = 'var(--success-color)';
        }
    }
}

/**
 * Update the log display
 */
async function updateLogs() {
    if (logPaused) {
        return;
    }
    
    try {
        // In a real implementation, we would fetch logs from the server
        // Here we'll just simulate with some example log entries
        
        const logBox = document.getElementById('logBox');
        const currentTime = new Date().toLocaleTimeString();
        
        // Append a simulated log entry
        logBox.value += `${currentTime} INFO [System] Log update check\n`;
        
        // Keep the most recent logs visible by scrolling to the bottom
        logBox.scrollTop = logBox.scrollHeight;
        
    } catch (error) {
        console.error('Failed to update logs:', error);
    }
}

/**
 * Load and display the file list
 */
async function loadFileList() {
    try {
        const data = await apiRequest(API.SYSTEM_FILES);
        
        const fileListContainer = document.getElementById('fileListContainer');
        fileListContainer.innerHTML = '';
        
        if (data.files && data.files.length > 0) {
            // Create a table for the files
            const table = document.createElement('table');
            table.className = 'w-100';
            
            // Create table header
            const thead = document.createElement('thead');
            const headerRow = document.createElement('tr');
            headerRow.innerHTML = `
                <th>File Name</th>
                <th class="text-right">Size</th>
                <th class="text-right">Actions</th>
            `;
            thead.appendChild(headerRow);
            table.appendChild(thead);
            
            // Create table body
            const tbody = document.createElement('tbody');
            
            // Add file entries
            data.files.forEach(file => {
                const row = document.createElement('tr');
                row.className = 'file-item';
                
                // File name with link
                const nameCell = document.createElement('td');
                nameCell.className = 'file-name';
                const nameLink = document.createElement('a');
                nameLink.href = '#';
                nameLink.textContent = file.name;
                nameLink.addEventListener('click', function(e) {
                    e.preventDefault();
                    viewFile(file.url);
                });
                nameCell.appendChild(nameLink);
                
                // File size
                const sizeCell = document.createElement('td');
                sizeCell.className = 'file-size text-right';
                sizeCell.textContent = formatFileSize(file.size);
                
                // Actions
                const actionsCell = document.createElement('td');
                actionsCell.className = 'file-actions text-right';
                
                const downloadBtn = document.createElement('a');
                downloadBtn.href = file.url;
                downloadBtn.className = 'btn btn-sm btn-info mr-1';
                downloadBtn.innerHTML = '<i class="fas fa-download"></i>';
                downloadBtn.title = 'Download';
                downloadBtn.download = file.name;
                
                const deleteBtn = document.createElement('button');
                deleteBtn.type = 'button';
                deleteBtn.className = 'btn btn-sm btn-danger';
                deleteBtn.innerHTML = '<i class="fas fa-trash-alt"></i>';
                deleteBtn.title = 'Delete';
                deleteBtn.addEventListener('click', function() {
                    confirmDeleteFile(file.url, file.name);
                });
                
                actionsCell.appendChild(downloadBtn);
                actionsCell.appendChild(deleteBtn);
                
                // Add cells to row
                row.appendChild(nameCell);
                row.appendChild(sizeCell);
                row.appendChild(actionsCell);
                
                // Add row to table
                tbody.appendChild(row);
            });
            
            table.appendChild(tbody);
            fileListContainer.appendChild(table);
            
            // Add filesystem statistics
            if (data.total_bytes && data.used_bytes) {
                const statsDiv = document.createElement('div');
                statsDiv.className = 'mt-3 text-right';
                statsDiv.innerHTML = `
                    <small>
                        Total: ${formatFileSize(data.total_bytes)}, 
                        Used: ${formatFileSize(data.used_bytes)}, 
                        Free: ${formatFileSize(data.total_bytes - data.used_bytes)}
                    </small>
                `;
                fileListContainer.appendChild(statsDiv);
            }
            
        } else {
            fileListContainer.innerHTML = '<p>No files found in filesystem</p>';
        }
        
    } catch (error) {
        console.error('Failed to load file list:', error);
        document.getElementById('fileListContainer').innerHTML = 
            '<p class="text-danger">Error loading files</p>';
    }
}

/**
 * View a file's contents
 * @param {string} path - File path
 */
async function viewFile(path) {
    // Store the selected file path
    selectedFilePath = path;
    
    // Update the modal title
    const fileName = path.split('/').pop();
    document.getElementById('fileContentTitle').textContent = `File: ${fileName}`;
    
    // Get download link ready
    const downloadBtn = document.getElementById('downloadFileBtn');
    downloadBtn.href = path;
    downloadBtn.download = fileName;
    
    // Show loading message
    document.getElementById('fileContent').textContent = 'Loading file content...';
    document.getElementById('fileContentContainer').style.display = 'block';
    document.getElementById('fileImageContainer').style.display = 'none';
    document.getElementById('fileBinaryContainer').style.display = 'none';
    
    // Show the modal
    showModal('fileContentModal');
    
    try {
        // Fetch file content
        const response = await fetch(path);
        
        // Check if response is OK
        if (!response.ok) {
            throw new Error(`HTTP error ${response.status}`);
        }
        
        const contentType = response.headers.get('Content-Type');
        
        // Handle text files
        if (contentType && (contentType.includes('text') || 
                            contentType.includes('application/json') ||
                            contentType.includes('application/xml'))) {
            const text = await response.text();
            document.getElementById('fileContent').textContent = text;
            document.getElementById('fileContentContainer').style.display = 'block';
        }
        // Handle image files
        else if (contentType && contentType.includes('image')) {
            document.getElementById('fileImage').src = path;
            document.getElementById('fileContentContainer').style.display = 'none';
            document.getElementById('fileImageContainer').style.display = 'block';
        }
        // Handle binary/other files
        else {
            document.getElementById('fileContentContainer').style.display = 'none';
            document.getElementById('fileBinaryContainer').style.display = 'block';
        }
        
    } catch (error) {
        console.error('Failed to load file content:', error);
        document.getElementById('fileContent').textContent = 'Error loading file content: ' + error.message;
    }
}

/**
 * Confirm deletion of a file
 * @param {string} path - File path
 * @param {string} name - File name
 */
function confirmDeleteFile(path, name) {
    // Store the selected file path
    selectedFilePath = path;
    
    // Update the confirmation message
    document.getElementById('fileToDelete').textContent = name;
    
    // Show the confirmation modal
    showModal('deleteConfirmModal');
}

/**
 * Delete a file
 */
async function deleteFile() {
    if (!selectedFilePath) {
        return;
    }
    
    try {
        const result = await apiRequest(`${API.SYSTEM_DELETE}?path=${encodeURIComponent(selectedFilePath)}`, 'DELETE');
        
        if (result.success) {
            showAlert(`File deleted successfully`, 'success');
            hideModal('deleteConfirmModal');
            
            // Reload file list
            loadFileList();
        } else {
            showAlert(`Failed to delete file: ${result.message}`, 'danger');
        }
    } catch (error) {
        console.error('Error deleting file:', error);
        showAlert('Error deleting file', 'danger');
    }
}

/**
 * Upload a firmware update
 */
async function uploadFirmware() {
    const fileInput = document.getElementById('fwFileInput');
    if (!fileInput.files || fileInput.files.length === 0) {
        showAlert('Please select a firmware file to upload', 'warning');
        return;
    }
    
    const file = fileInput.files[0];
    
    // Show progress container
    const progressContainer = document.getElementById('otaProgressContainer');
    const progressBar = document.getElementById('otaProgressBar');
    const statusElement = document.getElementById('otaStatus');
    
    progressContainer.style.display = 'block';
    progressBar.style.width = '0%';
    statusElement.textContent = 'Uploading firmware...';
    
    // Upload the file
    uploadFile(
        file,
        '/update',  // OTA update endpoint
        // Progress callback
        (percent) => {
            progressBar.style.width = `${percent}%`;
        },
        // Success callback
        (response) => {
            statusElement.textContent = 'Firmware update complete!';
            showAlert('Firmware update successful', 'success');
            
            // Check if reboot is requested
            const rebootRequested = document.getElementById('rebootAfterUpdate').checked;
            if (rebootRequested) {
                statusElement.textContent = 'Rebooting device...';
                setTimeout(() => {
                    apiRequest(API.SYSTEM_REBOOT, 'POST')
                        .then(() => {
                            showAlert('Device is rebooting...', 'info', 0);
                        })
                        .catch(error => {
                            console.error('Reboot failed:', error);
                        });
                }, 2000);
            }
        },
        // Error callback
        (error) => {
            statusElement.textContent = 'Update failed: ' + error;
            showAlert('Firmware update failed: ' + error, 'danger');
        }
    );
}

/**
 * Upload a filesystem update
 */
async function uploadFilesystem() {
    const fileInput = document.getElementById('fsFileInput');
    if (!fileInput.files || fileInput.files.length === 0) {
        showAlert('Please select a filesystem file to upload', 'warning');
        return;
    }
    
    const file = fileInput.files[0];
    
    // Show progress container
    const progressContainer = document.getElementById('otaProgressContainer');
    const progressBar = document.getElementById('otaProgressBar');
    const statusElement = document.getElementById('otaStatus');
    
    progressContainer.style.display = 'block';
    progressBar.style.width = '0%';
    statusElement.textContent = 'Uploading filesystem...';
    
    // Upload the file
    uploadFile(
        file,
        '/updatefs',  // Filesystem update endpoint
        // Progress callback
        (percent) => {
            progressBar.style.width = `${percent}%`;
        },
        // Success callback
        (response) => {
            statusElement.textContent = 'Filesystem update complete!';
            showAlert('Filesystem update successful', 'success');
            
            // Check if reboot is requested
            const rebootRequested = document.getElementById('rebootAfterUpdate').checked;
            if (rebootRequested) {
                statusElement.textContent = 'Rebooting device...';
                setTimeout(() => {
                    apiRequest(API.SYSTEM_REBOOT, 'POST')
                        .then(() => {
                            showAlert('Device is rebooting...', 'info', 0);
                        })
                        .catch(error => {
                            console.error('Reboot failed:', error);
                        });
                }, 2000);
            }
        },
        // Error callback
        (error) => {
            statusElement.textContent = 'Update failed: ' + error;
            showAlert('Filesystem update failed: ' + error, 'danger');
        }
    );
}

/**
 * Upload both firmware and filesystem
 */
async function uploadBoth() {
    const fwFileInput = document.getElementById('fwFileInput');
    const fsFileInput = document.getElementById('fsFileInput');
    
    if (!fwFileInput.files || fwFileInput.files.length === 0) {
        showAlert('Please select a firmware file to upload', 'warning');
        return;
    }
    
    if (!fsFileInput.files || fsFileInput.files.length === 0) {
        showAlert('Please select a filesystem file to upload', 'warning');
        return;
    }
    
    // Upload firmware first, then filesystem
    uploadFirmware();
}

/**
 * Upload a file to the filesystem
 */
async function uploadFile() {
    const fileInput = document.getElementById('fileToUpload');
    if (!fileInput.files || fileInput.files.length === 0) {
        showAlert('Please select a file to upload', 'warning');
        return;
    }
    
    const file = fileInput.files[0];
    
    // Show progress container
    const progressContainer = document.getElementById('uploadProgressContainer');
    const progressBar = document.getElementById('uploadProgressBar');
    const statusElement = document.getElementById('uploadStatus');
    
    progressContainer.style.display = 'block';
    progressBar.style.width = '0%';
    statusElement.textContent = 'Uploading file...';
    
    // Upload the file
    uploadFile(
        file,
        API.UPLOAD,
        // Progress callback
        (percent) => {
            progressBar.style.width = `${percent}%`;
        },
        // Success callback
        (response) => {
            statusElement.textContent = 'File uploaded successfully!';
            showAlert('File uploaded successfully', 'success');
            
            // Clear file input
            fileInput.value = '';
            
            // Reload file list after a short delay
            setTimeout(() => {
                loadFileList();
                
                // Hide progress
                progressContainer.style.display = 'none';
                statusElement.textContent = '';
            }, 2000);
        },
        // Error callback
        (error) => {
            statusElement.textContent = 'Upload failed: ' + error;
            showAlert('File upload failed: ' + error, 'danger');
        }
    );
}

/**
 * Clear logs
 */
function clearLogs() {
    document.getElementById('logBox').value = '';
}

/**
 * Download logs
 */
function downloadLogs() {
    const logs = document.getElementById('logBox').value;
    
    // Create a blob with the logs
    const blob = new Blob([logs], { type: 'text/plain' });
    
    // Create download link
    const downloadLink = document.createElement('a');
    downloadLink.href = URL.createObjectURL(blob);
    downloadLink.download = 'mushroom_controller_logs.txt';
    
    // Trigger download
    document.body.appendChild(downloadLink);
    downloadLink.click();
    document.body.removeChild(downloadLink);
}

/**
 * Toggle log pausing
 */
function toggleLogPause() {
    logPaused = !logPaused;
    
    // Update button text
    const pauseBtn = document.getElementById('pauseLogsBtn');
    pauseBtn.innerHTML = logPaused ? '<i class="fas fa-play"></i> Resume' : '<i class="fas fa-pause"></i> Pause';
    
    // Show alert
    showAlert(logPaused ? 'Logs paused' : 'Logs resumed', 'info', 1000);
}

/**
 * Setup event listeners for buttons and forms
 */
function setupEventListeners() {
    // Clear logs button
    document.getElementById('clearLogsBtn').addEventListener('click', clearLogs);
    
    // Download logs button
    document.getElementById('downloadLogsBtn').addEventListener('click', downloadLogs);
    
    // Pause logs button
    document.getElementById('pauseLogsBtn').addEventListener('click', toggleLogPause);
    
    // Refresh files button
    document.getElementById('refreshFilesBtn').addEventListener('click', loadFileList);
    
    // Upload file button
    document.getElementById('uploadFileBtn').addEventListener('click', uploadFile);
    
    // OTA firmware upload button
    document.getElementById('uploadFwBtn').addEventListener('click', uploadFirmware);
    
    // OTA filesystem upload button
    document.getElementById('uploadFsBtn').addEventListener('click', uploadFilesystem);
    
    // Update both button
    document.getElementById('updateBothBtn').addEventListener('click', uploadBoth);
    
    // File modal close button
    document.getElementById('closeFileContentBtn').addEventListener('click', function() {
        hideModal('fileContentModal');
    });
    
    // Delete confirmation buttons
    document.getElementById('confirmDeleteBtn').addEventListener('click', deleteFile);
    document.getElementById('cancelDeleteBtn').addEventListener('click', function() {
        hideModal('deleteConfirmModal');
    });
}