let express = require('express');

const port = 8080;
const app = express();


// Serial parsing
let currentValue = 0;

const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline')

const serial_port = new SerialPort('COM6', {baudRate: 115200});

SerialPort.list().then(
    ports => ports.forEach(console.log),
    err => console.log(err)
)

let regex = /([A-Za-z]+):\s([-0-9.]+)/g;

const parser = serial_port.pipe(new Readline());

parser.on('data', function (data) {
    //console.log('Data:', data)
    const matches = [...data.matchAll(regex)]
    let output = {};

    matches.forEach(function (match){
        output[match[1]] = match[2];
    })

    //console.log(output);
    currentValue = output;
})






// Server and API
app.use("/", express.static("static"));

app.use('/API/', express.json());
app.get('/API/', function (req, res) {
    console.log("API is alive!");
    res.end("API is alive!");
});

app.get('/API/angle', function (req, res) {
    //console.log("PostingX");
    res.end(JSON.stringify(currentValue));
});

app.post('/API/send', function(req, res) {
    body = req.body;

    for(var k in body){
        send_data = k + " " + body[k] + "\n";
        serial_port.write(send_data);
        console.log(send_data)
    }
    
    res.end("thanks.");
});

// Start server
app.listen(port, function () {
    console.log("Listening on port " + port);
});