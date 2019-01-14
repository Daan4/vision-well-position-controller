#!/usr/bin/python3
# -*- coding: utf-8 -*-
import numpy as np
from PyQt5.QtCore import QThread, pyqtSignal, pyqtSlot
from picamera import PiCamera
from picamera.array import PiRGBArray, PiYUVArray
import time

## PiVideoStream class streams camera images to a pyqt signal in a separate thread.
#
class PiVideoStream(QThread):
    name = "PiVideoStream"
    ## Logging message signal
    sigMsg = pyqtSignal(str)  # Message signal
    ## Image signal as numpy array
    ready = pyqtSignal(np.ndarray)
    pause = False
    
    ## The constructor.
    def __init__(self, resolution=(640,480), framerate=24, imgformat='bgr', effect='none', use_video_port=False):
        super().__init__()        
        self.camera = PiCamera()
        self.initCamera(resolution, framerate, imgformat, effect, use_video_port)
        self.sigMsg.emit(self.name + ": opened.")

    def __del__(self):
        self.wait()

    def run(self):
        try:
            for f in self.stream:
                self.rawCapture.truncate(0)  # clear the stream in preparation for the next frame
                if (self.pause == True):
                    self.sigMsg.emit(self.name + ": paused.")
                    break  # return from thread is needed
                else:
                    self.frame = f.array  # grab the frame from the stream
                    self.ready.emit(self.frame)
##                    self.sigMsg.emit(self.name + ": frame captured.")
        except Exception as err:
            self.sigMsg.emit(self.name + ": error running thread.")
            pass
        finally:
            self.sigMsg.emit(self.name + ": quit.")

    def initCamera(self, resolution=(640,480), framerate=24, imgformat='bgr', effect='none', use_video_port=False):
        self.sigMsg.emit(self.name + "Init: resolution = " + str(resolution))
        self.camera.resolution = resolution
        self.camera.image_effect = effect
        self.camera.iso = 60 # should force unity analog gain
##        self.camera.color_effects = (128,128) # return monochrome image
        # dunno if setting awb mode manually is really useful
##        self.camera.awb_mode = 'off'
##        self.camera.awb_gains = 5.0
##        self.camera.meter_mode = 'average'
##        self.camera.exposure_mode = 'auto'  # 'sports' to reduce motion blur, 'off'after init to freeze settings
        self.camera.framerate = framerate
        self.rawCapture = PiRGBArray(self.camera, size=self.camera.resolution)
        self.stream = self.camera.capture_continuous(self.rawCapture, imgformat, use_video_port)
        self.frame = None
        time.sleep(2)  

    @pyqtSlot()
    def stop(self):
        self.pause = True
        self.wait()
##        self.rawCapture.close()
##        self.camera.close()
        self.quit()
        self.sigMsg.emit(self.name + ": closed.")

    @pyqtSlot()
    def changeCameraSettings(self, resolution=(640,480), framerate=24, format="bgr", effect='none', use_video_port=False):
        self.pause = True
        self.wait()
        self.initCamera(resolution, framerate, format, effect, use_video_port)
        self.pause = False
        self.start()  # restart thread


if __name__ == "__main__":
    vs = PiVideoStream(effect='denoise')
    vs.start()
    #vs.ready.connect(mainWindow.kickTimer)  # Measure time delay
    #vs.ready.connect(imgEnhancer.imgUpdate, type=Qt.QueuedConnection)  # Connect video/image stream to processing
