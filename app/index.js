require('dotenv').config()
const express = require('express')
const spawn = require('child_process').spawn
const bodyParser = require('body-parser')
const fs = require('fs')
const moment = require('moment')
const app = express()
const expressWs = require('express-ws')(app)
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
  response.end(JSON.stringify({ id: request.body.uuid }))
})

app.ws('/', function(websocket, request) {
  websocket.on('message', function(message) {
    message = JSON.parse(message)
    if (message.uuid) {
      const child = spawn('harmgen', [
        '/usr/local/share/harmgen/congen_1yr.txt',
        `/tmp/${message.uuid}.txt`,
        `/tmp/result-${message.uuid}.json`
      ])

      child.stdout.on('data', data => {
        websocket.send(
          JSON.stringify({ type: 'message', data: data.toString() })
        )
      })

      child.stderr.on('data', data => {
        websocket.send(JSON.stringify({ type: 'error', data: data.toString() }))
      })

      child.on('close', code => {
        websocket.send(JSON.stringify({ type: 'close', data: code }))
      })

      child.on('exit', code => {
        websocket.send(JSON.stringify({ type: 'exit', data: code }))
      })
    }
  })
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
