// DOM Elements
const statusIndicator = document.getElementById('status-indicator');
const statusText = document.getElementById('status-text');
const ipAddress = document.getElementById('ip-address');
const connectionStatus = document.getElementById('connection-status');

const toggle1 = document.getElementById('toggle1');
const toggle2 = document.getElementById('toggle2');
const toggle3 = document.getElementById('toggle3');
const auto1Btn = document.getElementById('auto1');
const auto2Btn = document.getElementById('auto2');
const auto3Btn = document.getElementById('auto3');
const saveCooldownBtn = document.getElementById('save-cooldown');
const cooldownInput = document.getElementById('cooldown-time');
const timeoutDisplay = document.getElementById('timeout-value');

const bulb1 = document.getElementById('bulb1');
const bulb2 = document.getElementById('bulb2');
const bulb3 = document.getElementById('bulb3');
const bulb1Status = document.getElementById('bulb1-status');
const bulb2Status = document.getElementById('bulb2-status');
const bulb3Status = document.getElementById('bulb3-status');
const sensor1Status = document.getElementById('sensor1-status');
const sensor2Status = document.getElementById('sensor2-status');

const toggleAllOnBtn = document.getElementById('toggle-all-on');
const toggleAllOffBtn = document.getElementById('toggle-all-off');

// WebSocket connection
let websocket;
let lastData = {};
let reconnectAttempts = 0;
const maxReconnectAttempts = 5;
const reconnectDelay = 3000; // 3 seconds

// Initialize WebSocket connection
function initWebSocket() {
  // Get the current location (IP address or hostname)
  const gateway = `ws://${window.location.hostname}/ws`;
  
  // Set the IP address display
  ipAddress.textContent = window.location.hostname;
  
  console.log('Connecting to WebSocket at:', gateway);
  
  websocket = new WebSocket(gateway);
  
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

// WebSocket event handlers
function onOpen(event) {
  console.log('Connection opened');
  reconnectAttempts = 0;
  updateConnectionStatus(true);
}

function onClose(event) {
  console.log('Connection closed');
  updateConnectionStatus(false);
  
  // Attempt to reconnect if not max attempts reached
  if (reconnectAttempts < maxReconnectAttempts) {
    reconnectAttempts++;
    console.log(`Attempting to reconnect (${reconnectAttempts}/${maxReconnectAttempts})...`);
    setTimeout(initWebSocket, reconnectDelay);
  } else {
    console.log('Max reconnect attempts reached.');
  }
}

function onMessage(event) {
  console.log('Message received:', event.data);
  try {
    lastData = JSON.parse(event.data);
    updateUI(lastData);
  } catch (error) {
    console.error('Error parsing message:', error);
  }
}

// Update connection status in UI
function updateConnectionStatus(connected) {
  if (connected) {
    statusIndicator.classList.remove('disconnected');
    statusIndicator.classList.add('connected');
    statusText.textContent = 'Connected';
    connectionStatus.textContent = 'Connected';
  } else {
    statusIndicator.classList.remove('connected');
    statusIndicator.classList.add('disconnected');
    statusText.textContent = 'Disconnected';
    connectionStatus.textContent = 'Disconnected';
    
    // Reset bulbs to off when disconnected
    updateBulbState(bulb1, bulb1Status, false);
    updateBulbState(bulb2, bulb2Status, false);
    updateBulbState(bulb3, bulb3Status, false);
  }
}

// Update UI based on received data
function updateUI(data) {
  // Update bulb states
  updateBulbState(bulb1, bulb1Status, data.relay1);
  updateBulbState(bulb2, bulb2Status, data.relay2);
  updateBulbState(bulb3, bulb3Status, data.relay3);
  
  // Update sensor indicators
  updateSensorState(sensor1Status, data.sensor1, 1);
  updateSensorState(sensor2Status, data.sensor2, 2);
  
  // Update toggle states (without triggering event)
  if (toggle1.checked !== data.override1) {
    toggle1.checked = data.override1;
  }
  
  if (toggle2.checked !== data.override2) {
    toggle2.checked = data.override2;
  }
  
  if (toggle3.checked !== data.override3) {
    toggle3.checked = data.override3;
  }
  
  // Update auto mode buttons
  if (data.override1) {
    auto1Btn.classList.remove('active');
  } else {
    auto1Btn.classList.add('active');
  }
  
  if (data.override2) {
    auto2Btn.classList.remove('active');
  } else {
    auto2Btn.classList.add('active');
  }
  
  if (data.override3) {
    auto3Btn.classList.remove('active');
  } else {
    auto3Btn.classList.add('active');
  }
  
  // Update cooldown time if present in data
  if (data.cooldownTime) {
    cooldownInput.value = data.cooldownTime;
    timeoutDisplay.textContent = data.cooldownTime;
  }
}

// Update bulb visual state
function updateBulbState(bulbElement, statusElement, isOn) {
  if (isOn) {
    bulbElement.classList.add('bulb-on');
    statusElement.textContent = 'ON';
    statusElement.style.color = 'var(--success-color)';
  } else {
    bulbElement.classList.remove('bulb-on');
    statusElement.textContent = 'OFF';
    statusElement.style.color = 'var(--danger-color)';
  }
}

// Update sensor visual state
function updateSensorState(sensorElement, isActive, sensorNumber) {
  if (isActive) {
    sensorElement.innerHTML = `<i class="fas fa-user"></i><span>Motion Detected (Sensor ${sensorNumber})</span>`;
    sensorElement.classList.remove('sensor-inactive');
    sensorElement.classList.add('sensor-active');
  } else {
    sensorElement.innerHTML = `<i class="fas fa-user-slash"></i><span>No Motion (Sensor ${sensorNumber})</span>`;
    sensorElement.classList.remove('sensor-active');
    sensorElement.classList.add('sensor-inactive');
  }
}

// Send commands to the server
function sendCommand(command) {
  if (websocket && websocket.readyState === WebSocket.OPEN) {
    console.log('Sending command:', command);
    websocket.send(command);
  } else {
    console.error('WebSocket is not connected');
    // Try to reconnect
    initWebSocket();
  }
}

// Event listeners
toggle1.addEventListener('change', function() {
  sendCommand('toggle1');
});

toggle2.addEventListener('change', function() {
  sendCommand('toggle2');
});

toggle3.addEventListener('change', function() {
  sendCommand('toggle3');
});

auto1Btn.addEventListener('click', function() {
  sendCommand('auto1');
  this.classList.add('active');
  toggle1.checked = false;
});

auto2Btn.addEventListener('click', function() {
  sendCommand('auto2');
  this.classList.add('active');
  toggle2.checked = false;
});

auto3Btn.addEventListener('click', function() {
  sendCommand('auto3');
  this.classList.add('active');
  toggle3.checked = false;
});

toggleAllOnBtn.addEventListener('click', function() {
  sendCommand('allOn');
});

toggleAllOffBtn.addEventListener('click', function() {
  sendCommand('allOff');
});

// Cooldown slider updates
cooldownInput.addEventListener('input', function() {
  timeoutDisplay.textContent = this.value;
});

saveCooldownBtn.addEventListener('click', function() {
  const cooldownValue = parseInt(cooldownInput.value);
  if (cooldownValue > 0 && cooldownValue <= 300) { // Updated max value to 300
    sendCommand(`setCooldown=${cooldownValue}`);
    showNotification('Timeout saved');
  } else {
    showNotification('Please enter a valid timeout (1-300 seconds)', 'error'); // Updated error message
  }
});

// Show notification
function showNotification(message, type = 'success') {
  // Create notification element
  const notification = document.createElement('div');
  notification.className = `notification ${type}`;
  notification.textContent = message;
  
  // Add to body
  document.body.appendChild(notification);
  
  // Show notification
  setTimeout(() => {
    notification.classList.add('show');
  }, 10);
  
  // Remove after 3 seconds
  setTimeout(() => {
    notification.classList.remove('show');
    setTimeout(() => {
      notification.remove();
    }, 300); // Match transition duration
  }, 3000);
}

// Check for connection loss regularly
function checkConnection() {
  if (websocket && websocket.readyState !== WebSocket.OPEN) {
    updateConnectionStatus(false);
  }
}

// Initialize on page load
window.addEventListener('load', function() {
  initWebSocket();
  
  // Check connection every 5 seconds
  setInterval(checkConnection, 5000);
  
  // Add ability to quickly test the UI without a connection
  if (window.location.hostname === '127.0.0.1' || window.location.hostname === 'localhost') {
    console.log('Development mode detected');
    // Add a button to simulate data
    const devButton = document.createElement('button');
    devButton.textContent = 'Simulate Data';
    devButton.style.position = 'fixed';
    devButton.style.bottom = '10px';
    devButton.style.right = '10px';
    devButton.style.zIndex = '1000';
    devButton.addEventListener('click', function() {
      const testData = {
        relay1: Math.random() > 0.5,
        relay2: Math.random() > 0.5,
        relay3: Math.random() > 0.5,
        sensor1: Math.random() > 0.5,
        sensor2: Math.random() > 0.5,
        override1: toggle1.checked,
        override2: toggle2.checked,
        override3: toggle3.checked,
        cooldownTime: parseInt(cooldownInput.value)
      };
      updateUI(testData);
    });
    document.body.appendChild(devButton);
  }
});

// Handle visibility change to reconnect when tab becomes active again
document.addEventListener('visibilitychange', function() {
  if (document.visibilityState === 'visible') {
    // Page is visible, check connection
    if (websocket && websocket.readyState !== WebSocket.OPEN) {
      console.log('Page visible again, checking connection');
      initWebSocket();
    }
  }
});
