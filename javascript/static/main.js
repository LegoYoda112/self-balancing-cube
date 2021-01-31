const el = (id) => document.getElementById(id);

const fetch = window.fetch;
const json = (response) => response.json();

let value = el("value");

var scene, renderer, camera;
var cube;
var cube_x_angle = 0;

window.addEventListener("load", function () {
    console.log("testing");
    
    init();
    animate();

    initPlots();
});



// ======================= UPDATER =============================
var angleObject =  {};
let updateValue = setInterval(function () {
    getData().then(function (data){
        value.innerHTML = data.angle;
        updatePlot(data);
        angleObject = data;
        cube_x_angle = -parseFloat(data.angle) * (2*3.1415/360);
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
        name: "Proportional"
    }, {
        y:[0],
        mode:'lines',
        name: "Derivative"
    }]);

    Plotly.relayout(control_plot,{
         yaxis: {
                   range: [-2, 2]           
                 }
    });
    
    
    Plotly.plot(state_plot, [{
        y:[0],
        mode:'lines',
        name: "test"
    }]);

    Plotly.relayout(state_plot,{
         yaxis: {
                   range: [-45, 45]           
                 }
    });
}

var max_data = 100;

function updatePlot(data){
    
    Plotly.extendTraces(control_plot, { y: [[data.P], [data.D]] }, [0, 1], max_data);
    Plotly.extendTraces(state_plot, { y: [[data.angle]] }, [0], max_data);
}

// =================== THREE ========================
let vizdiv = el("vizdiv");


function init(){

    // === SCENE
    scene = new THREE.Scene();
    scene.background = new THREE.Color( 0xcccccc );
    scene.fog = new THREE.FogExp2( 0xcccccc, 0.002);

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

    camera.position.z = 5;
    camera.position.y = 3;

    camera.rotation.x = -0.5;

    // ==== GOEMETRY
    var geometry = new THREE.BoxGeometry( 1, 1, 1 );
    var material = new THREE.MeshPhongMaterial( { color: 0xffffff, flatShading: true } );
    cube = new THREE.Mesh( geometry, material );
    scene.add( cube );

    // === Ground Grid

    const size = 10;
    const divisions = 10;

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
    //cube.rotation.y = 0;
    cube.rotation.z = cube_x_angle + 3.1415/4;

    cube.position.x = Math.cos(cube.rotation.z + 3.1415/4)/Math.sqrt(2)
    cube.position.y = Math.sin(cube.rotation.z + 3.1415/4)/Math.sqrt(2)

	renderer.render( scene, camera );
};










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