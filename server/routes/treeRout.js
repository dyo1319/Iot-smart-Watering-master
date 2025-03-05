const express = require('express');
const router = express.Router();
const Tree = require('../models/treeMode');
const db = require('../models/database');

const tree = new Tree(db);

// GET all trees
router.get("/list", async (req, res) => {
    try {
        const trees = await tree.getAllTrees();
        res.status(200).json(trees);
    } catch (error) {
        console.error("Error fetching trees:", error);
        res.status(500).json({ error: "Failed to fetch trees" });
    }
});

// GET a specific tree by ID
router.get("/get/:id", async (req, res) => {
    try {
        const treeId = req.params.id;
        const treeData = await tree.getTreeById(treeId);
        
        if (!treeData) {
            return res.status(404).json({ error: "Tree not found" });
        }
        
        res.status(200).json(treeData);
    } catch (error) {
        console.error("Error fetching tree:", error);
        res.status(500).json({ error: "Failed to fetch tree" });
    }
});

// Add Tree
router.post("/create", async (req, res) => {
    try {
        const { name } = req.body;
        const success = await tree.createTree(name);
        
        if (success) {
            res.status(201).json({message: "create tree success"});
        } else {
            res.status(400).json({message: "failed to create tree"});
        }
    } catch (error) {
        console.log(error);
        res.status(500).json({message: "server error"});
    }
});

// UPDATE a tree
router.put("/update/:id", async (req, res) => {
    try {
        const treeId = req.params.id;
        const { name, date } = req.body;
        
        if (!name && !date) {
            return res.status(400).json({ error: "At least one field (name or date) is required for update" });
        }
        
        const success = await tree.updateTree(treeId, name, date);
        
        if (!success) {
            return res.status(404).json({ error: "Tree not found or update failed" });
        }
        
        // Fetch the updated tree to return in response
        const updatedTree = await tree.getTreeById(treeId);
        
        // Format the date for the response to be consistent
        if (updatedTree && updatedTree.date) {
            // If date is a Date object
            if (updatedTree.date instanceof Date) {
                updatedTree.date = updatedTree.date.toISOString().split('T')[0];
            } 
            // If date is already a string but in ISO format
            else if (typeof updatedTree.date === 'string' && updatedTree.date.includes('T')) {
                updatedTree.date = updatedTree.date.split('T')[0];
            }
        }
        
        res.status(200).json({ 
            message: "Tree updated successfully",
            tree: updatedTree 
        });
    } catch (error) {
        console.error("Error updating tree:", error);
        res.status(500).json({ error: "Failed to update tree" });
    }
});

// DELETE a tree
router.delete("/delete/:id", async (req, res) => {
    try {
        const treeId = req.params.id;
        
        // Get the tree details before deletion (to include in response)
        const treeData = await tree.getTreeById(treeId);
        
        if (!treeData) {
            return res.status(404).json({ error: "Tree not found" });
        }
        
        const success = await tree.deleteTree(treeId);
        
        if (!success) {
            return res.status(500).json({ error: "Failed to delete tree" });
        }
        
        res.status(200).json({ 
            message: "Tree deleted successfully",
            deletedTree: treeData
        });
    } catch (error) {
        console.error("Error deleting tree:", error);
        res.status(500).json({ error: "Failed to delete tree" });
    }
});

// Set watering schedule for a tree
router.post("/schedule/create/:id", async (req, res) => {
    try {
        const treeId = req.params.id;
        const { dayOfWeek, startHour, duration } = req.body;
        
        if (dayOfWeek === undefined || startHour === undefined || !duration) {
            return res.status(400).json({ 
                error: "Day of week, start hour, and duration are required" 
            });
        }
        
        const success = await tree.setWateringSchedule(treeId, dayOfWeek, startHour, duration);
        
        if (!success) {
            return res.status(404).json({ error: "Failed to set watering schedule" });
        }
        
        // Get the updated schedules to return in the response
        const schedules = await tree.getWateringSchedules(treeId);
        
        res.status(201).json({
            message: "Watering schedule set successfully",
            schedules
        });
    } catch (error) {
        console.error("Error setting watering schedule:", error);
        res.status(500).json({ error: "Server error" });
    }
});

// Get all watering schedules for a tree
router.get("/schedule/list/:id", async (req, res) => {
    try {
        const treeId = req.params.id;
        const schedules = await tree.getWateringSchedules(treeId);
        
        if (!schedules) {
            return res.status(404).json({ error: "Tree not found" });
        }
        
        res.status(200).json(schedules);
    } catch (error) {
        console.error("Error getting watering schedules:", error);
        res.status(500).json({ error: "Server error" });
    }
});

// Delete a watering schedule
router.delete("/schedule/delete/:scheduleId", async (req, res) => {
    try {
        const scheduleId = req.params.scheduleId;
        const success = await tree.deleteWateringSchedule(scheduleId);
        
        if (!success) {
            return res.status(404).json({ error: "Schedule not found" });
        }
        
        res.status(200).json({
            message: "Watering schedule deleted successfully"
        });
    } catch (error) {
        console.error("Error deleting watering schedule:", error);
        res.status(500).json({ error: "Server error" });
    }
});

module.exports = router;