class Tree{
    constructor(db){
        this.DB = db;
    }

    // Get: AllTrees information
    async getAllTrees() {
        try {
            const [rows] = await this.DB.execute(`
                SELECT t.id, p.id AS plant_id, p.name, t.date 
                FROM threes t
                JOIN plants p ON t.id_plants = p.id
                ORDER BY t.date DESC
            `);
            return rows;
        } catch (error) {
            console.error("Error fetching trees:", error);
            return [];
        }
    }
    // Get: specfic tree information
    async getTreeById(id) {
        try {
            const [rows] = await this.DB.execute(`
                SELECT t.id, p.id AS plant_id, p.name, t.date 
                FROM threes t
                JOIN plants p ON t.id_plants = p.id
                WHERE t.id = ?
            `, [id]);
            
            if (rows.length === 0) {
                return null;
            }
            return rows[0];
        } catch (error) {
            console.error("Error fetching tree:", error);
            return null;
        }
    }
    // Create: Create tree information
    async createTree(nameTree) {
        try {
            // Validate input
            if (!nameTree) {
                console.log("Error: Tree name is required");
                return false;
            }
            
            const date = new Date();
            const formattedDate = date.toISOString().split('T')[0];
            
            // Check if plant exists
            const [existingPlants] = await this.DB.execute("SELECT id FROM plants WHERE name = ?",  [nameTree]);
            let plantId;
            
            if (existingPlants.length > 0) {
                // Use existing plant
                plantId = existingPlants[0].id;
            } else {
                // Create new plant
                const [newPlantResult] = await this.DB.execute("INSERT INTO plants(name) VALUES(?)",  [nameTree] );
                plantId = newPlantResult.insertId;
            }
            
            // Create tree using the plant ID 
            await this.DB.execute( "INSERT INTO threes(id_plants, date) VALUES(?, ?)", [plantId, formattedDate]);
            
            return true;
        } 
        catch (error) {
            console.error("Error in createTree:", error);
            return false;
        }
    }
    // UPDATE: Update tree information
    async updateTree(id, name, date) {
        try {
            // First check if tree exists
            const [treeCheck] = await this.DB.execute(`SELECT t.id, t.id_plants, p.name, t.date FROM threes t JOIN plants p ON t.id_plants = p.id WHERE t.id = ?`, [id]);
            
            if (treeCheck.length === 0) {
                console.log("Tree not found");
                return false;
            }
            
            const tree = treeCheck[0];
            
            // If no new values provided, no need to update
            if (!name && !date) {
                return true;
            }
            
            // Format date if provided, make sure it's in YYYY-MM-DD format
            let formattedDate;
            if (date) {
                // Convert to a JavaScript Date object first to handle various formats
                const dateObj = new Date(date);
                if (isNaN(dateObj.getTime())) {
                    console.log("Invalid date format");
                    return false;
                }
                // Format as YYYY-MM-DD
                formattedDate = dateObj.toISOString().split('T')[0];
            } else {
                // If no new date provided, use existing date
                if (tree.date instanceof Date) {
                    formattedDate = tree.date.toISOString().split('T')[0];
                } else {
                    // If it's already a string, make sure it's the right format
                    const dateObj = new Date(tree.date);
                    formattedDate = dateObj.toISOString().split('T')[0];
                }
            }
            
            if (name && name !== tree.name) {
                // Check if the new plant name already exists
                const [existingPlants] = await this.DB.execute(`SELECT id FROM plants WHERE name = ?`, [name]);
                
                let plantId;
                
                if (existingPlants.length > 0) {
                    // Use existing plant
                    plantId = existingPlants[0].id;
                } else {
                    // Create new plant
                    const [newPlantResult] = await this.DB.execute(`INSERT INTO plants(name) VALUES(?)`, [name]);
                    plantId = newPlantResult.insertId;
                }
                
                // Update tree with new plant ID
                await this.DB.execute(`UPDATE threes SET id_plants = ?, date = ? WHERE id = ?`, [plantId, formattedDate, id]);
            } else if (formattedDate !== tree.date) {
                // Only update date
                await this.DB.execute(`UPDATE threes SET date = ? WHERE id = ?`, [formattedDate, id]);
            }
            
            return true;
        } catch (error) {
            console.error("Error updating tree:", error);
            return false;
        }
    }
    // DELETE: Delete a tree
    async deleteTree(id) {
        try {
            // First check if tree exists
            const [treeCheck] = await this.DB.execute(`SELECT t.id, t.id_plants FROM threes t WHERE t.id = ?`, [id]);
            
            if (treeCheck.length === 0) {
                console.log("Tree not found");
                return false;
            }
            
            const tree = treeCheck[0];
            
            // Delete the tree
            await this.DB.execute(`DELETE FROM threes WHERE id = ?`, [id]);
            
            // Check if this plant is used by other trees
            const [otherTrees] = await this.DB.execute(`SELECT COUNT(*) as count FROM threes WHERE id_plants = ?`, [tree.id_plants]);
            
            // If no other trees use this plant, delete the plant as well
            if (otherTrees[0].count === 0) { await this.DB.execute(`DELETE FROM plants WHERE id = ?`, [tree.id_plants]);}
            
        return true;

        }
        catch (error) {
            console.error("Error deleting tree:", error);
            return false;
        }
    }

    // Set watering schedule for a tree on a specific day
    async setWateringSchedule(treeId, dayOfWeek, startHour, duration) {
        try {
            // Validate inputs
            if (dayOfWeek < 0 || dayOfWeek > 6) {
                console.error("Day of week must be between 0 (Sunday) and 6 (Saturday)");
                return false;
            }
            
            if (startHour < 0 || startHour > 23) {
                console.error("Start hour must be between 0 and 23");
                return false;
            }
            
            if (duration <= 0) {
                console.error("Duration must be greater than 0");
                return false;
            }

            // Check if tree exists
            const [treeCheck] = await this.DB.execute(`
                SELECT id FROM threes WHERE id = ?`, [treeId]);
            
            if (treeCheck.length === 0) {
                console.error("Tree not found");
                return false;
            }

            // Check if a schedule already exists for this tree and day
            const [existingSchedule] = await this.DB.execute(`
                SELECT id FROM watering_schedule WHERE tree_id = ? AND day_of_week = ?`, [treeId, dayOfWeek]);

            if (existingSchedule.length > 0) {
                // Update existing schedule
                await this.DB.execute(`UPDATE watering_schedule SET start_hour = ?, duration = ? WHERE id = ?`, [startHour, duration, existingSchedule[0].id]);
            } else {
                // Create new schedule
                await this.DB.execute(`INSERT INTO watering_schedule (tree_id, day_of_week, start_hour, duration) VALUES (?, ?, ?, ?)`, [treeId, dayOfWeek, startHour, duration]);
            }

            return true;
        } catch (error) {
            console.error("Error setting watering schedule:", error);
            return false;
        }
    }

    // Get watering schedules for a specific tree
    async getWateringSchedules(treeId) {
        try {
            // Check if tree exists
            const [treeCheck] = await this.DB.execute(`SELECT id FROM threes WHERE id = ?`, [treeId]);
            
            if (treeCheck.length === 0) {
                console.error("Tree not found");
                return null;
            }

            // Get all schedules for this tree
            const [schedules] = await this.DB.execute(`SELECT id, day_of_week, start_hour, duration FROM watering_schedule WHERE tree_id = ? ORDER BY day_of_week, start_hour`, [treeId]);

            return schedules;
        } catch (error) {
            console.error("Error getting watering schedules:", error);
            return null;
        }
    }

    // Delete a watering schedule
    async deleteWateringSchedule(scheduleId) {
        try {
            // Check if schedule exists
            const [scheduleCheck] = await this.DB.execute(`
                SELECT id FROM watering_schedule WHERE id = ?`, [scheduleId]);
            
            if (scheduleCheck.length === 0) {
                console.error("Schedule not found");
                return false;
            }

            // Delete the schedule
            await this.DB.execute(`DELETE FROM watering_schedule WHERE id = ?`, [scheduleId]);

            return true;
        } catch (error) {
            console.error("Error deleting watering schedule:", error);
            return false;
        }
    }
}
module.exports = Tree;

