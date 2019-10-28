require('dotenv').config()
const express = require('express')
const bodyParser = require('body-parser')

const app = express()
const port = process.env.TIDE_PORT ? process.env.TIDE_PORT : 8000

app.use(bodyParser.json())

app.post('/api/v1/generate', (req, res) => {})

app.listen(port, () => console.log(`Example app listening on port ${port}!`))
