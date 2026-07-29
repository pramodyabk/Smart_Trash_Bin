require('dotenv').config();

const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const mqtt = require('mqtt');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

app.use(express.static('public')); 

// --- HiveMQ Cloud Secure Configuration ---
const MQTT_BROKER_URL = process.env.HIVEMQ_URL; 
const MQTT_TOPIC = 'smartbin/detections/my_unique_bin_001'; 
const MQTT_OPTIONS = {
    username: process.env.HIVEMQ_USERNAME,
    password: process.env.HIVEMQ_PASSWORD
};

// Data store to hold counts and recent history
let categoryCounts = {
    cardboard: 0, glass: 0, metal: 0, paper: 0, plastic: 0, trash: 0
};
let recentDetections = []; // Array of { timestamp, item, confidence }

console.log(`Connecting to secure HiveMQ Cloud at ${MQTT_BROKER_URL}...`);
const mqttClient = mqtt.connect(MQTT_BROKER_URL, MQTT_OPTIONS);

mqttClient.on('connect', () => {
    console.log('Connected to HiveMQ Cloud');
    mqttClient.subscribe(MQTT_TOPIC, (err) => {
        if (!err) {
            console.log(`Subscribed to topic: ${MQTT_TOPIC}`);
        } else {
            console.error('Subscription error:', err);
        }
    });
});

mqttClient.on('message', (topic, message) => {
    try {
        const telemetry = JSON.parse(message.toString());
        
        if (telemetry && telemetry.item) {
            const item = telemetry.item;
            
            // Increment count
            if(categoryCounts[item] !== undefined) {
                categoryCounts[item]++;
            }

            // Create log entry
            const logEntry = {
                timestamp: new Date().toLocaleTimeString(),
                item: item,
                confidence: (telemetry.confidence * 100).toFixed(1)
            };
            
            // Keep only last 20 logs
            recentDetections.unshift(logEntry);
            if(recentDetections.length > 20) recentDetections.pop();

            // Broadcast to clients
            io.emit('update', {
                counts: categoryCounts,
                newLog: logEntry
            });
        }
    } catch (e) {
        console.error("Failed to parse message:", e);
    }
});

mqttClient.on('error', (err) => {
    console.error('MQTT Client Error:', err);
});

// When a client connects, send them the current state
io.on('connection', (socket) => {
    socket.emit('initialData', { counts: categoryCounts, logs: recentDetections });
});

const PORT = 3000;
server.listen(PORT, () => console.log(`Server running at http://localhost:${PORT}`));