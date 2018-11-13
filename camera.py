from picamera import PiCamera
from picamera.exc import PiCameraError
from picamera.array import PiRGBArray
import time


class Camera:
    def __init__(self, resolution=(410, 308)):
        try:
            self.camera = PiCamera()
            self.camera.resolution = resolution
            self.rawCapture = PiRGBArray(self.camera)
            time.sleep(0.1)
        except PiCameraError as e:
            # Camera not connected
            self.camera = None

    def capture_raw_image(self):
        self.camera.capture(self.rawCapture, format="bgr")
        return self.rawCapture.array
