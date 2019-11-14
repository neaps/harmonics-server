require('dotenv').config()
const express = require('express')
const spawn = require('child_process').spawn
const bodyParser = require('body-parser')
const fs = require('fs')
const moment = require('moment')
const app = express()
const port = process.env.PORT ? process.env.PORT : 8080

/** TODO - convert "T" from ISO to timestamp! */
app.use(bodyParser.json({ limit: '50mb' }))

app.all('/api/*', (request, response, next) => {
  response.setHeader('Content-Type', 'application/json')
  response.setHeader('Access-Control-Allow-Origin', '*')
  response.setHeader(
    'Access-Control-Allow-Headers',
    'Content-Type, Access-Control-Allow-Headers, Authorization, X-Requested-With'
  )
  next()
})

app.post('/api/v1/generate', (request, response) => {
  const levels = []
  const inputFile = `/tmp/${request.body.uuid}.txt`
  request.body.levels.forEach(level => {
    levels.push(`${moment(level.t).unix()} ${level.v}`)
  })
  fs.writeFileSync(inputFile, levels.join('\n'))
  spawn('harmgen', [
    '/usr/local/share/harmgen/congen_1yr.txt',
    inputFile,
    `/tmp/result-${request.body.uuid}.json`
  ])
  response.end(JSON.stringify({ id: request.body.uuid }))
})

app.get('/api/v1/status/:id', (request, response) => {
  const exists = fs.existsSync(`/tmp/result-${request.params.id}.json`)
  response.end(JSON.stringify({ done: exists }))
})

app.get('/api/v1/get/:id', (request, response) => {
  const result = fs.readFileSync(
    `/tmp/result-${request.params.id}.json`,
    'utf8'
  )
  response.end(result)
})

app.listen(port, () =>
  console.log(`Tide harmonics API listening on port ${port}!`)
)
