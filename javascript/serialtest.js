const SerialPort = require('serialport');
const Readline = require('@serialport/parser-readline')

const port = new SerialPort('COM4', {baudRate: 9600});

SerialPort.list().then(
    ports => ports.forEach(console.log),
    err => console.log(err)
)



let regex = /([A-Za-z]+):\s([-0-9.]+)/g;

const parser = port.pipe(new Readline());

parser.on('data', function (data) {
    console.log('Data:', data)
    const matches = [...data.matchAll(regex)]
    let output = {};

    matches.forEach(function (match){
        output[match[1]] = match[2];
    })

    console.log(output);
})


