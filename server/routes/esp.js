const express = require('express');
const router = express.Router();
const fs = require('fs').promises; // Use async file operations
const db = require('../models/database');

// Handle sensor data storage
router.get('/', async (req, res) => {
    try {
        const { temp, linght, moisture, treeId: rawTreeId, isRunning: rawIsRunning } = req.query;
        
        console.log(`Raw data received - Temperature: ${temp}, Light: ${linght}, Moisture: ${moisture}, Tree ID: ${rawTreeId}, Running: ${rawIsRunning}`);
        
        let treeId;
        if (rawTreeId === undefined || rawTreeId === '') {
            treeId = 1; // Default if not provided
            console.log(`No tree ID provided, using default: ${treeId}`);
        } else {
            treeId = parseInt(rawTreeId, 10);
            if (isNaN(treeId)) {
                treeId = 1; // Default if parsing fails
                console.log(`Invalid tree ID format: "${rawTreeId}", using default: ${treeId}`);
            } else {
                console.log(`Parsed tree ID: ${treeId} from input: "${rawTreeId}"`);
            }
        }
        
        let isRunning;
        if (rawIsRunning === undefined || rawIsRunning === '') {
            isRunning = 0; // Default if not provided
        } else {
            isRunning = parseInt(rawIsRunning, 10);
            if (isNaN(isRunning)) {
                isRunning = 0; // Default if parsing fails
            }
        }
        
        // Validation check
        if (treeId <= 0) {
            return res.status(400).json({ message: 'Invalid tree ID: must be greater than 0' });
        }
        
        // Filter and validate sensor data
        const sensorData = [
            { name: 'temperature', value: temp },
            { name: 'light', value: linght },
            { name: 'moisture', value: moisture }
        ].filter(sensor => {
            const isValid = sensor.value !== undefined && 
                            sensor.value !== '' && 
                            !isNaN(parseFloat(sensor.value));
            if (!isValid && sensor.value !== undefined) {
                console.log(`Invalid ${sensor.name} value: "${sensor.value}"`);
            }
            return isValid;
        }).map(sensor => ({ 
            ...sensor, 
            value: parseFloat(sensor.value) 
        }));
        
        // Check if we have any valid sensor data
        if (sensorData.length === 0) {
            return res.status(400).json({ message: 'No valid sensor data received' });
        }
        
        console.log(`Processed ${sensorData.length} valid sensor readings`);
        
        // Get the current date
        const currentDate = new Date().toISOString().split('T')[0];
        
        // Check if the tree exists in the database
        console.log(`Checking if tree with ID ${treeId} exists...`);
        const [treeResult] = await db.execute("SELECT id FROM threes WHERE id = ?", [treeId]);
        
        if (treeResult.length === 0) {
            console.log(`Tree with ID ${treeId} not found in database`);
            return res.status(404).json({ message: `Tree with ID ${treeId} not found` });
        }
        
        console.log(`Tree with ID ${treeId} found, proceeding with data insertion`);
        
        // Insert sensor data into database
        const queries = sensorData.map(sensor => db.execute(
            "INSERT INTO datasensors (id_threes, name_sensor, avg, date, isRunning) VALUES (?, ?, ?, ?, ?)",
            [treeId, sensor.name, sensor.value, currentDate, isRunning]
        ));
        
        const results = await Promise.all(queries);
        console.log(`Successfully inserted ${results.length} sensor readings for tree ID ${treeId}`);
        
        // Send success response
        res.status(200).json({
            message: 'Sensor data stored successfully',
            inserted: sensorData.map(sensor => ({
                ...sensor,
                tree_id: treeId,
                date: currentDate,
                isRunning: isRunning
            }))
        });
    } catch (error) {
        console.error("Error processing sensor data:", error);
        res.status(500).json({ 
            message: 'Internal server error', 
            error: error.message 
        });
    }
});

//  Get current system state
router.get('/state', async (req, res) => {
    try {
        let data = JSON.parse(await fs.readFile("Inside_information.json", "utf8"));
        res.json({ state: data.state, date: new Date().getHours() });
    } catch (error) {
        console.error("Error reading state:", error);
        res.status(500).json({ error: "Failed to retrieve system state" });
    }
});

//  Get specific mode data
router.get('/dataMode', async (req, res) => {
    try {
        const { state } = req.query;
        let data = JSON.parse(await fs.readFile("Inside_information.json", "utf8"));
        res.json(data[state] || { message: "State not found" });
    } catch (error) {
        console.error("Error fetching mode data:", error);
        res.status(500).json({ error: "Failed to retrieve mode data" });
    }
});

//  Update pump state manually
router.post('/pump', async (req, res) => {
    try {
        const { state } = req.body;
        let data = JSON.parse(await fs.readFile("Inside_information.json", "utf8"));
        
        if (!data.MANUAL) data.MANUAL = {};
        data.MANUAL.pumpState = state === true || state === "true";
        data.MANUAL.lastCommand = Date.now();

        await fs.writeFile("Inside_information.json", JSON.stringify(data, null, 4));
        res.json({ success: true });
    } catch (error) {
        console.error("Error setting pump state:", error);
        res.status(500).json({ error: "Failed to set pump state" });
    }
});

//  Update system state
router.post('/updateState', async (req, res) => {
    try {
        const { state } = req.body;
        let data = JSON.parse(await fs.readFile("Inside_information.json", "utf8"));
        data.state = state;

        await fs.writeFile("Inside_information.json", JSON.stringify(data, null, 4));
        res.json({ success: true });
    } catch (error) {
        console.error("Error updating state:", error);
        res.status(500).json({ error: "Failed to update state" });
    }
});

//  Store sensor samples efficiently
router.post('/samples', async (req, res) => {
    try {
        const { treeId, temperature, light, moisture, pumpStatus } = req.body;
        if (!treeId || isNaN(parseInt(treeId)) || treeId <= 0) {
            return res.status(400).json({ error: "Invalid tree ID" });
        }

        const currentDate = new Date().toISOString().split('T')[0];
        const sensorValues = [
            { name: "temperature", value: temperature },
            { name: "light", value: light },
            { name: "moisture", value: moisture }
        ].filter(sensor => sensor.value !== undefined && !isNaN(parseFloat(sensor.value)))
         .map(sensor => [treeId, sensor.name, parseFloat(sensor.value), currentDate, pumpStatus]);

        if (sensorValues.length === 0) {
            return res.status(400).json({ message: "No valid sensor data received" });
        }

        const insertQuery = "INSERT INTO datasensors (id_threes, name_sensor, avg, date, isRunning) VALUES ?";
        await db.query(insertQuery, [sensorValues]);

        res.json({ success: true, inserted: sensorValues.length });
    } catch (error) {
        console.error("Error storing sensor samples:", error);
        res.status(500).json({ error: "Failed to store sensor samples" });
    }
});

module.exports = router;
