// routes/index.js
const express = require('express');
const router = express.Router();
const controller = require('../controllers/sampleController');
router.get('/v1/data', controller.getData);
module.exports = router;
