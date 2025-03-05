const express = require('express');
const router = express.Router();
const fs = require('fs');

router.get('/', (req,res) => {
    const {temp, linght, moisture} = req.query;
    console.log(linght);
})

router.get('/state', (req,res) =>{
    let data = JSON.parse(fs.readFileSync("Inside_information.json", "utf8"));
    data = {
        state: data.state,
        date: new Date().getHours()
    }
    res.json(data);
})

router.get('/dataMode', (req,res) =>{
    const { state } = req.query;
    let data = JSON.parse(fs.readFileSync("Inside_information.json", "utf8"));
    res.json(data[state]);
})

module.exports = router;