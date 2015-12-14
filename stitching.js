// Global variables to manipulate Videos/canvases.
var video = [];
var videoImage = [];
var videoImageContext = [];
var imageData = [];

var homography = [[],[],[]];

var isReadyToReceive = true;

// This function is called by common.js when the NaCl module is loaded.
function moduleDidLoad() {
  // Once we load, hide the plugin. In this example, we don't display anything
  // in the plugin, so it is fine to hide it.
  common.hideModule();

  document.getElementById('btn_calibrate').disabled = false;


  console.log("module loaded");
  sendImages();
}

// Get image data from specified canvas
function getImageData(id) {
  var display = document.getElementById(id);
  var ctx = display.getContext('2d');
  var height = display.height;
  var width = display.width;
  var nBytes = height * width * 4;
  var pixels = ctx.getImageData(0, 0, width, height);
  return {width: width, height: height, data: pixels.data.buffer};
}

// Send pair of camera images to NaCl module.
// Assumes canvas1&2 show webcam feeds, and that they are the same size.
function sendImages() {
  if (common.naclModule == undefined || !isReadyToReceive) return;
  var imData1 = getImageData('canvas1');
  var imData2 = getImageData('canvas2');

  common.naclModule.postMessage(
    {'message': 'images',
     'width': imData1.width, 'height': imData1.height,
     'data1': imData1.data, 'data2': imData2.data});

  isReadyToReceive = false;
}

// Draw pixels to canvas with given id.
function drawImage(id, pixels){
  var canvas = document.getElementById(id);
  var ctx = canvas.getContext('2d');
  var imData = ctx.getImageData(0, 0, canvas.width, canvas.height);
  var buf8 = new Uint8ClampedArray(pixels);
  imData.data.set(buf8);
  ctx.putImageData(imData, 0, 0);
  //isReadyToReceive = true;
}

// This function is called by common.js when a message is received from the
// NaCl module.
function handleMessage(message) {
  var res = message.data;
  if (res.message == "rectified") {
    if (res.data1 && res.data2) {
      drawImage('canvas4', res.data1);
      drawImage('canvas5', res.data2);
    } else {
      console.log("not getting rectified data!");
    }
  } else if (res.message == "disparity") {
    if (res.data) {
      //drawImage16('canvas3', res.data);
      drawImage('canvas3', res.data);
      updatePointCloud();
    } else {
      console.log("not getting disparity data!");
    }
    isReadyToReceive = true;
  } else {
    console.log(message.data);
  }

  // Now send more images to process.
  sendImages();
}

// This function is called by common.js when a message is received from the
// NaCl module.
function OLDhandleMessage(message) {
  // Via this single communication channel comes messages to dump to console and
  // hopefully the H matrix as well, coefficient by coefficient.
  if (message.data['message'] == "H") {
    // Uncomment next line for Debug:
    //console.log(message.data['row'] +" " +message.data['column'] +" "+message.data['value']);

    // |row|, |column| range from 0 to 2 and value is float.
    homography[message.data['row']][message.data['column']] =
        message.data['value'];

    if (homography[0].length==3 && homography[1].length==3 && homography[2].length==3) {
      updateWebGLWithHomography(homography);
      homography[0].length = homography[1].length = homography[2].length = 0;
    }
  } else if (message.data['message'] == "I") {
    var int8View = new Int16Array(message.data['value']);
    console.log(" Got image " + message.data['value'] + " - " +
        int8View.byteLength + "B");

    if (0) {
      var cnv1 = document.getElementById('canvas3');
      var ctx1 = cnv1.getContext('2d');
      var imageData1 = ctx1.getImageData(0, 0, 320, 240);
      var data1 = imageData1.data;
      var p = 0;
      for (var i = 0; i < data1.length; i += 4) {
        data1[i]     = int8View[p++];
        data1[i + 1] = int8View[p++];//int8View[i + 1];
        data1[i + 2] = int8View[p++];//int8View[i + 2];
        data1[i + 3] = 255;//int8View[p++];//int8View[i + 3];
        p++;
      }
      ctx1.putImageData(imageData1, 0, 0);
    }
  } else if (message.data['message'] == "D") {

    var int8View = new Int8Array(message.data['value']);
    console.log(" Got disparity " + message.data['value'] + " - " +
        int8View.byteLength + "B");

    if (1) {
      var cnv1 = document.getElementById('canvas3');
      var ctx1 = cnv1.getContext('2d');
      var imageData1 = ctx1.getImageData(0, 0, 320, 240);
      //var imageData1 = ctx1.getImageData(0, 0, 640, 480);
      var data1 = imageData1.data;
      var p = 0;
      for (var i = 0; i < data1.length; i += 4) {
      //for (var i = 0; i < data1.length; i++) {
        var gray = int8View[p];
        //console.log(gray);
        // MAAAAYBE
        // because you're putting it back into itself
        // it goes over the whole thing
        // so just make new 8bit_disparity mat to copy into?
        // maybe not...
        data1[i]     = gray;//int8View[p++];
        data1[i + 1] = gray;//int8View[p++];//int8View[i + 1];
        data1[i + 2] = gray;//int8View[p++];//int8View[i + 2];
        data1[i + 3] = 255;//int8View[p++];//int8View[i + 3];
        p++;
      }
      ctx1.putImageData(imageData1, 0, 0);
    }
  } else if (message.data['message'] == "RL" ||
             message.data['message'] == "RR") {
    var int8View = new Int8Array(message.data['value']);
    console.log(" Got rectified image " + message.data['value'] + " - " +
                int8View.byteLength + "B");

    // print some sample values as a sanity check
    // console.log("sample values:");
    // for (var i = 0; i < 50; ++i) {
    //   console.log(int8View[5000+i*200]);
    // }

    // if (1) {
    //   var cnv1;
    //   if (message.data['message'] == "RL")
    //     cnv1 = document.getElementById('canvas4');
    //   else
    //     cnv1 = document.getElementById('canvas5');
    //   var ctx1 = cnv1.getContext('2d');
    //   var imageData1 = ctx1.getImageData(0, 0, 320, 240);
    //   var data1 = imageData1.data;
    //   var p = 0;
    //   for (var i = 0; i < data1.length; i += 4) {
    //   //for (var i = 0; i < data1.length; i++) {
    //     //var gray = int8View[p];
    //     data1[i]     = int8View[p++]+127;
    //     data1[i + 1] = int8View[p++]+127;//int8View[i + 1];
    //     data1[i + 2] = int8View[p++]+127;//int8View[i + 2];
    //     data1[i + 3] = 255;//int8View[p++];//int8View[i + 3];
    //     p++;
    //   }
    //   ctx1.putImageData(imageData1, 0, 0);

    var res = message.data;
    var pixels = res.data;
    var canvas = document.getElementById('canvas4');
    var ctx = canvas.getContext("2d");
    var imData = ctx.getImageData(0, 0, canvas.width, canvas.height);
    var buf8 = new Uint8ClampedArray(pixels);
    imData.data.set(buf8);
    ctx.putImageData(imData, 0, 0);

  } else {
    // Dump stuff to special PNaCl output area.
    var logEl = document.getElementById('log');
    logEl.textContent += message.data;

    // And/Or to JS Console.
    console.log(message.data);
  }
}

// Calibrate does a whole lot of things: Plugs the <video> tags into their
// respective hidden <canvas> eleemnts, and then passes the data from those
// canvas snapshots to the stiching module, via a protocol of 3 steps:
// first the index of the next data buffer, then the buffer itself, repeat for
// each buffer; when done, send a string asking politely to do the stitching.
function calibrate() {
  // Plug the <video> vidX into canvasX.
  for(var i=0; i<2; i++) {
    video[i] = document.getElementById('vid' + (i+1));

    videoImage[i] = document.getElementById('canvas' + (i+1));
    videoImageContext[i] = videoImage[i].getContext('2d');
    // background color if no video present
    videoImageContext[i].fillStyle = '#000000';
    videoImageContext[i].fillRect( 0, 0, videoImage[0].width, videoImage[0].height);
  }

  for(var i=0; i<2; i++) {
    if (video[i].readyState === video[i].HAVE_ENOUGH_DATA) {
      videoImageContext[i].drawImage(
          video[i], 0, 0, videoImage[0].width, videoImage[0].height);
      imageData[i] = videoImageContext[i].getImageData(
          0, 0, videoImage[0].width, videoImage[0].height);

      // After the NaCl module has loaded, common.naclModule is a reference to
      // the NaCl module's <embed> element. Method postMessage sends a message
      // to it. F.i.:
      common.naclModule.postMessage({'message' : 'data',
                                     'index' : i,
                                     'width' : videoImage[0].width,
                                     'height' : videoImage[0].height,
                                     'data' : imageData[i].data.buffer});
    }
  }
  common.naclModule.postMessage('Please calculate the homography.');
}
