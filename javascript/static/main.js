const el = (id) => document.getElementById(id);

const fetch = window.fetch;
const json = (response) => response.json();

let value = el("value");

var scene, renderer, camera;
var cube;
var cube_x_angle = 0;

window.addEventListener("DOMContentLoaded", function () {
    console.log("testing");
});



// ======================= UPDATER =============================
var angleObject =  {};
let updateValue = setInterval(function () {
    getData().then(function (data){
        value.innerHTML = data.angle;
        updatePlot(data.angle);
        angleObject = data;
        cube_x_angle = -parseFloat(data.angle) * (2*3.1415/360);
        //console.log(data);
    })
}, 30);

function getData() {
    return fetch("/API/angle", {
        "method": "GET",
        "headers": {
            "Content-Type": "application/json"
        }
    }).then(json);
}


// ============================ PLOTLY ========================
function initPlots(){
    Plotly.plot('chart', [{
        y:[0],
        type:'line'
    }]);
}

var data_count = 0;
var max_data = 20;
function updatePlot(data){
    Plotly.extendTraces('chart', { y: [[data]] }, [0]);
    data_count ++;

    if(data_count > max_data){
        Plotly.relayout('chart',{
            xaxis: {
                      range: [data_count-max_data ,data_count]
                   }
       });
    }
}

// =================== THREE ========================
function init(){

    // === SCENE
    scene = new THREE.Scene();
    scene.background = new THREE.Color( 0xcccccc );
    scene.fog = new THREE.FogExp2( 0xcccccc, 0.002);

    // === CAMERA
    camera = new THREE.PerspectiveCamera( 60, (window.innerWidth)/(window.innerHeight/2), 0.1, 1000 );

    camera.position.z = 5;
    camera.position.y = 3;

    camera.rotation.x = -0.5;

    // ==== RENDERER
    renderer = new THREE.WebGLRenderer({ antialias: true } );
    renderer.setPixelRatio( window.devicePixelRatio );
    renderer.setSize( window.innerWidth, window.innerHeight/2);
    document.body.appendChild( renderer.domElement );

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


init();
animate();

initPlots();










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