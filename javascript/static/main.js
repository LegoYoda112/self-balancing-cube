//const SerialPort = require("serialport");

// Helpter functions
const el = (id) => document.getElementById(id);
const cloneTemplate = (id) => document.importNode(el(id).content, true);

const fetch = window.fetch;
const json = (response) => response.json();

const controlList = el("controls");

var scene, renderer, camera;
var cube;
var cubeLine, wheelALine, wheelBLine, wheelCLine;
var cubeLineDir;

let offsetPitch = 0;
let offsetRoll = 0;

let saved_data = {};

const control_names = ["kP", "kD", "kOffset"];
const default_values = [-60, -1, -0.03];
const send_button = el("send_button");
const on_button = el("on_button");
const off_button = el("off_button");
const calibrate_button = el("calibrate_button");

window.addEventListener("DOMContentLoaded", function () {

    // Set up controls
    let cnt = 0;
    control_names.forEach(function (name){
        addControl(name, default_values[cnt]);
        cnt++;
    });
})

window.addEventListener("load", function () {
    console.log("testing");
    
    init();
    animate();

    initPlots();
});



// ======================= UPDATER =============================
let updateValue = setInterval(function () {
    getData().then(function (data){
        updatePlot(data);
        // angleObject = data;

        saved_data = data;
        saved_data.roll = parseFloat(data.roll); // Go from text -> float
        saved_data.pitch = parseFloat(data.pitch);
        saved_data.yaw = parseFloat(data.yaw);
        //console.log(data);
    })
}, 50);

function getData() {
    return fetch("/API/angle", {
        "method": "GET",
        "headers": {
            "Content-Type": "application/json"
        }
    }).then(json);
}


// ============================ PLOTLY ========================
const control_plot = "control_plot"
const state_plot = "state_plot"

function initPlots(){
    // Control plot
    Plotly.plot(control_plot, [{
        y:[0],
        mode:'lines',
        name: "Wheel A"
    }, {
        y:[0],
        mode:'lines',
        name: "Wheel B"
    }, {
        y:[0],
        mode:'lines',
        name: "Wheel C"
    }]);

    Plotly.relayout(control_plot,{
         yaxis: {
                   range: [-2, 2]           
                 }
    });
    
    
    Plotly.plot(state_plot, [{
        y:[0],
        mode:'lines',
        name: "Roll"
    }, {
        y:[0],
        mode:'lines',
        name: "Pitch"
    }]);

    Plotly.relayout(state_plot,{
         yaxis: {
                   range: [-1, 1]           
                 }
    });
}

var max_data = 100;

function updatePlot(data){
    
    Plotly.extendTraces(control_plot, { y: [[data.wheelA], [data.wheelB], [data.wheelC]] }, [0, 1, 2], max_data);
    Plotly.extendTraces(state_plot, { y: [[data.roll], [data.pitch]] }, [0, 1], max_data);
}

// =================== THREE ========================
let vizdiv = el("vizdiv");


function init(){

    // === SCENE
    scene = new THREE.Scene();
    scene.background = new THREE.Color( 0x20262E );
    scene.fog = new THREE.FogExp2( 0x20262E, 0.09);

    // ==== RENDERER

    let scene_width = vizdiv.clientWidth;
    let scene_height = vizdiv.clientHeight;

    console.log(scene_width);
    console.log(scene_height);

    renderer = new THREE.WebGLRenderer({ antialias: true } );
    renderer.setPixelRatio( window.devicePixelRatio );
    renderer.setSize( scene_width, scene_height);
    vizdiv.appendChild( renderer.domElement );


    // === CAMERA
    camera = new THREE.PerspectiveCamera( 60, scene_width/scene_height, 0.1, 1000 );

    camera.position.z = 3;
    camera.position.y = 3;

    camera.rotation.x = -0.7;

    // ==== GOEMETRY
    var geometry = new THREE.BoxGeometry( 1, 1, 1 );
    var material = new THREE.MeshPhongMaterial( { color: 0xff9900, flatShading: true } );
    material.opacity = 0.7;
    material.transparent = true;
    cube = new THREE.Mesh( geometry, material );
    scene.add( cube );

    // === Arrows and stuff
    cubeLineDir = new THREE.Vector3(1, 2, 0);
    cubeLineDir.normalize();

    const origin = new THREE.Vector3(0, 0, 0);
    const length = 2;

    cubeLine = new THREE.ArrowHelper(cubeLineDir, origin, length, 0xFA6666, 0.2, 0.1);
    scene.add(cubeLine);

    const upLine = new THREE.ArrowHelper(new THREE.Vector3(0, 1, 0), origin, length, 0x81FA66, 0.2, 0.1);
    scene.add(upLine);

    const wheelADir = new THREE.Vector3(-0.5, 0, 0.866);
    wheelALine = new THREE.ArrowHelper(wheelADir, origin, 1, 0x2279E3, 0.1, 0.3);
    scene.add(wheelALine);

    const wheelBDir = new THREE.Vector3(1, 0, 0);
    wheelBLine = new THREE.ArrowHelper(wheelBDir, origin, 1, 0x34A33F, 0.1, 0.3);
    scene.add(wheelBLine);

    const wheelCDir = new THREE.Vector3(-0.5, 0, -0.866);
    wheelCLine = new THREE.ArrowHelper(wheelCDir, origin, 1, 0xE3A322, 0.1, 0.3);
    scene.add(wheelCLine);

    
    // === Ground Grid

    const size = 20;
    const divisions = 20;

    const gridHelper = new THREE.GridHelper( size, divisions );
    scene.add( gridHelper );

    const dirLight1 = new THREE.DirectionalLight( 0xffffff );
    dirLight1.position.set( 1, 1, 1 );
    scene.add( dirLight1 );

    const ambientLight = new THREE.AmbientLight( 0x222222 );
    scene.add( ambientLight );

}

var animate = function () {
    requestAnimationFrame( animate );
    
	//cube.rotation.x = parseFloat(angleObject.Num)/10;
    cube.rotation.x = -saved_data.pitch + offsetPitch + 3.1415/4; // Pitch
    cube.rotation.z = -saved_data.roll + offsetRoll + 3.1415/4; // Roll

    cube.position.x = Math.cos(cube.rotation.z + 3.1415/4)* (0.5);
    cube.position.y = Math.sin(cube.rotation.z + 3.1415/4)* (0.5);

    cube.position.y = cube.position.y * Math.cos(cube.rotation.x - 3.1415/4);
    cube.position.z = cube.position.y * Math.sin(cube.rotation.x - 3.1415/4);

    //cube.position.y += 0.1;

    var dir = new THREE.Vector3(cube.position.x, cube.position.y, cube.position.z).normalize()
    //cubeLine.setDirection(dir);

    cubeLine.rotation.x = (-saved_data.pitch + offsetPitch);
    cubeLine.rotation.z = (-saved_data.roll + offsetRoll);

    const powerScale = 1;
    wheelALine.setLength(parseFloat(saved_data.wheelA)/powerScale, 0.3, 0.3);
    wheelBLine.setLength(parseFloat(saved_data.wheelB)/powerScale, 0.3, 0.3);
    wheelCLine.setLength(parseFloat(saved_data.wheelC)/powerScale, 0.3, 0.3);

	renderer.render( scene, camera );
};



// ================== CONTROLS =================
function addControl(controlName, defaultVal){
    let newControl = cloneTemplate("control");
    const qs = (query) => newControl.querySelector(query);

    const title =  qs(".control_title");
    title.textContent = controlName;

    const container =  qs("#control_id");
    container.id = controlName;

    const number =  qs(".control_number");
    number.defaultValue = defaultVal;

    controlList.appendChild(newControl);
}

// UPDATE CONSTANTS
send_button.onclick = function () {
    let send_data = {};
    control_names.forEach(function (name){
        const control = el(name);
        const control_value = control.querySelector('.control_number').value;

        send_data[name] = control_value;

    });

    console.log(send_data);
    sendData(send_data);
};

function sendData(data) {
    const body = JSON.stringify(data)
    return fetch("/API/send", {
        "method": "POST",
        "headers": {
            "Content-Type": "application/json"
        },
        "body": body
    });
}


// ON
on_button.onclick = function () {
    console.log("Turning on!");
    sendData({on: "1"});
}

// OFF
off_button.onclick = function () {
    console.log("Turning off! ABORTTT");
    sendData({on: "0"});
}

calibrate_button.onclick = function() {
    console.log("Calibrating offsets");
    offsetRoll = saved_data.roll;
    offsetPitch = saved_data.pitch;
    sendData({rollSP: saved_data.roll});
    sendData({pitchSP: saved_data.pitch});
    sendData({yawSP: saved_data.yaw});
}





// function onDocumentMouseMove(event) {
//     event.preventDefault();
//     mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
//     mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;
// }

// function updateCamera() {
//     //offset the camera x/y based on the mouse's position in the window
//     camera.rotation.x = (mouse.y);
//     camera.rotation.y = -(mouse.x);
// }