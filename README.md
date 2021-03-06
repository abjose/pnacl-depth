PNaCl stereo depth app
======================

(NOTE: this code is a work-in-progress conversion of [miguelao's panostream](https://github.com/WebRTC-Labs/panostream) to display a depth map from stereo cameras. This README.md hasn't been updated.)

This web page shows how to open two different camera feeds, represent them, and estimate the homography (the mathematical transformation that relates one and the other) via OpenCV in a PNaCl plugin, to produce a 3d rendering of both feeds "stitched" or in the form of a "panorama". The calibration is produced on-demand by clicking on the button at the bottom after requesting the videos and seeing them playing.

This demo would work only in Chrome, and PNaCl needs to be enabled via Chrome flags (type about:flags in the browser bar) -- just search for NaCl in that page.

### How to compile and run the PNaCl module

Make sure you have [NaCl SDK](https://developers.google.com/native-client/sdk/download)
installed in your (Linux/Mac) machine. Export its root
`export NACL_SDK_ROOT=/path/to/naclsdk/pepper_33`, then from the panostream
folder run `make` and then `make serve` (python needed). The served
page can be accessed locally in [http://localhost:5103/panostream.html.]

Under the hood PNaCl compiler will produce a .pexe file that is served by this python mini server.

### GitHub Tricks

For quick development, panostream.html (actually any html in GitHub) can be accessed on this URL:
http://htmlpreview.github.io/?https://github.com/Miguelao/panostream/blob/master/panostream.html
(but it would not have the PNaCl .pexe obviously, so is kind-of pointless ;) )
