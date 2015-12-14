// webgl.js is used via startWebGL() to create a GL canvas inside a div of name
// called "glcanvas". It will take the video feeds from up to three cameras
// rendered on <video> tags of IDs |view1| to |view3|. Up to three non visible
// canvases are needed named |canvas1| to |canvas3|, not displayed.
// The code in this file has taken inspiration from:
// Adapted from http://stemkoski.github.io/Three.js/#webcam-texture

// Amount of cameras to render in the 3D world.
var NUM_CAMERAS = 2;

// ThreeJS global variables.
var container, scene, camera, renderer, controls;

// Global variables to manipulate Videos/canvases.
var video = [];
var videoImage = [];
var videoImageContext = [];
var videoTexture = [];

var movieScreen = [];

var pcBuffer = undefined;

// Profiler variable;
var statprofiler = new profiler();

// Entry point of the webgl.js file.
function startWebGL() {
  init();
  animate();
}

function init() {
  // New scene needed.
  scene = new THREE.Scene();

  // Retrieve canvas and Field Of View constants.
  container = document.getElementById("glcanvas");
  var SCREEN_WIDTH = 960;
  var SCREEN_HEIGHT = 240;
  var VIEW_ANGLE = 45;
  var ASPECT = SCREEN_WIDTH / SCREEN_HEIGHT;
  var NEAR = 0.1;
  var FAR = 10000;

  // Scene camera.
  camera = new THREE.PerspectiveCamera(VIEW_ANGLE, ASPECT, NEAR, FAR);
  scene.add(camera);
  camera.position.set(0,150,800);
  camera.lookAt(scene.position);
  console.log("Three.JS camera initialized");

  // Renderer.
  //renderer = new THREE.CanvasRenderer();
  renderer = new THREE.WebGLRenderer();
  renderer.setSize(SCREEN_WIDTH, SCREEN_HEIGHT);
  container.appendChild(renderer.domElement);
  console.log("Three.JS renderer initialized");

  // consider 'OrbitControls'
  controls = new THREE.TrackballControls(camera, container);
  //controls = new THREE.OrbitControls(camera, container);
  controls.rotateSpeed = 1.0;
  controls.zoomSpeed = 1.2;
  controls.panSpeed = 0.8;
  controls.noZoom = false;
  controls.noPan = false;
  controls.staticMoving = true;
  controls.dynamicDampingFactor = 0.3;
  //controls.keys = [ 65, 83, 68 ];

  // Light source.
  var light = new THREE.PointLight(0xffffff);
  light.position.set(0,250,0);
  scene.add(light);
  console.log("Three.JS light source initialized");

  // Floor -> Disconnected but is useful when the camera is looking AWOL.
  var floorTexture = new THREE.ImageUtils.loadTexture( 'images/checkerboard.jpg' );
  floorTexture.wrapS = floorTexture.wrapT = THREE.RepeatWrapping;
  floorTexture.repeat.set( 10, 10 );
  var floorMaterial = new THREE.MeshBasicMaterial( { map: floorTexture, side: THREE.DoubleSide } );
  var floorGeometry = new THREE.PlaneGeometry(1000, 1000, 10, 10);
  var floor = new THREE.Mesh(floorGeometry, floorMaterial);
  floor.position.y = -240;
  floor.rotation.x = Math.PI / 2;
  scene.add(floor);

  // Camera video input. The idea is to plug a camera <video> feed into a canvas
  // and use it to retrieve the data.
  for (var i=0; i < NUM_CAMERAS; i++) {
    video[i] = document.getElementById('vid' + (i+1));

    videoImage[i] = document.getElementById('canvas' + (i+1));
    videoImageContext[i] = videoImage[i].getContext('2d');
    // background color if no video present
    videoImageContext[i].fillStyle = '#000000';
    videoImageContext[i].fillRect( 0, 0, videoImage[0].width, videoImage[0].height);

    videoTexture[i] = new THREE.Texture(videoImage[i]);
    videoTexture[i].minFilter = THREE.LinearFilter;
    videoTexture[i].magFilter = THREE.LinearFilter;

    // Here is the magic: assign the video as a material of basic mesh.
    var movieMaterial = new THREE.MeshBasicMaterial(
        {map: videoTexture[i], overdraw: true, side:THREE.DoubleSide});
    var movieGeometry =
        new THREE.PlaneGeometry(videoImage[0].width, videoImage[0].height, 1, 1);
    movieScreen[i] = new THREE.Mesh(movieGeometry, movieMaterial);

    movieScreen[i].position.set(0, 0, -100);
    movieScreen[i].rotation.set(0, 0, 0); // (Math.PI /8)*(1-i)
    scene.add(movieScreen[i]);
  }
  camera.position.set(0,50,800);
  // if (NUM_CAMERAS >1)
  //   camera.lookAt(movieScreen[1].position);
  // else
  //   camera.lookAt(movieScreen[0].position);
  console.log("Three.JS GL context and video feeds initialized.");


  console.log(width, length);
  pcBuffer = generatePointcloud(new THREE.Color(1,0,0), width, length);
  pcBuffer.scale.set(10, 10, 10);
  pcBuffer.position.set(-5, 0, 5);
  scene.add(pcBuffer);


  statprofiler.add("Render time");
  console.log("Profiler initialized.");
}

function animate() {
  requestAnimationFrame(animate);
  update();
  render();
}

function render()  {
  statprofiler.new_frame();
  controls.update();

  statprofiler.start("Render time");
  for (var i=0; i < NUM_CAMERAS; i++) {
    if (video[i].readyState === video[i].HAVE_ENOUGH_DATA) {
      videoImageContext[i].drawImage(video[i],
                                     0, 0, videoImage[i].width, 
                                     videoImage[i].height);
      if (videoTexture[i]) {
        videoTexture[i].needsUpdate = true;
        //renderer.render(scene, camera);
      }
    }
  }
  renderer.render( scene, camera );
  statprofiler.stop("Render time");

  document.getElementById('log').innerHTML  = (statprofiler.log());
}

function update() {
  //updatePointCloud();
}

var d = 0.01;

function updatePointCloud() {
  console.log("updating point cloud!");
  var canvas = document.getElementById('canvas3');
  var img_canvas = document.getElementById('canvas4');
  var ctx = canvas.getContext('2d');
  var img_ctx = img_canvas.getContext('2d');
  var data = ctx.getImageData(0, 0, canvas.width, canvas.height).data;
  var img_data = img_ctx.getImageData(0, 0, canvas.width, canvas.height).data;

  var positions = pcBuffer.geometry.vertices;
  var colors = pcBuffer.geometry.colors;

  if (!positions[0]) return;

  var k = 0;

  for (var j = 0; j < length; j++) {
    for (var i = 0; i < width; i++) {
      var x = i * d;
      //var y = 1 + data[k] * 0.005;
      var y = 1;
      var z = j * d;

      var r = img_data[k] / 255;
      var g = img_data[k+1] / 255;
      var b = img_data[k+2] / 255;

      var offset = j*width + i;
      positions[offset].set(x, y, z);
      colors[offset].setRGB(r, g, b);

      k += 4;
    }
  }

  pcBuffer.geometry.verticesNeedUpdate = true;
  pcBuffer.geometry.colorsNeedUpdate = true;
}

var pointSize = 0.05;
var width = 320;
var length = 240;
//var rotateY = new THREE.Matrix4().makeRotationY( 0.005 );

function generatePointCloudGeometry(color, width, length){
  var geometry = new THREE.Geometry();
  var numPoints = width*length;

  // var positions = new Float32Array( numPoints*3 );
  // var colors = new Float32Array( numPoints*3 );

  var k = 0;

  for( var i = 0; i < width; i++ ) {
    for( var j = 0; j < length; j++ ) {
      var u = i / width;
      var v = j / length;
      var x = u - 0.5;
      var y = ( Math.cos( u * Math.PI * 8 ) + Math.sin( v * Math.PI * 8 ) ) / 20;
      var z = v - 0.5;

      // positions[ 3 * k ] = x;
      // positions[ 3 * k + 1 ] = y;
      // positions[ 3 * k + 2 ] = z;

      // var intensity = (y + 0.1) * 5;
      // colors[ 3 * k ] = color.r * intensity;
      // colors[ 3 * k + 1 ] = color.g * intensity;
      // colors[ 3 * k + 2 ] = color.b * intensity;

      geometry.vertices.push(new THREE.Vector3(x, y, z));
      geometry.colors.push(new THREE.Color(color.r, color.g, color.b));

      //k++;
    }
  }

  // geometry.addAttribute('position', new THREE.BufferAttribute(positions, 3));
  // geometry.addAttribute('color', new THREE.BufferAttribute(colors, 3));
  // geometry.computeBoundingBox();

  return geometry;
}

function generatePointcloud(color, width, length) {
  var geometry = generatePointCloudGeometry(color, width, length);

  var material = new THREE.PointCloudMaterial({ size: pointSize, vertexColors: THREE.VertexColors });
  var pointcloud = new THREE.PointCloud(geometry, material);

  return pointcloud;
}
