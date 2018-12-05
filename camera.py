import numpy as np
from PyQt5.QtCore import QThread, pyqtSignal, pyqtSlot
from picamera import PiCamera
from picamera.array import PiRGBArray
import time
import cv2


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
    def __init__(self, resolution=(640, 480), framerate=24, imgformat='bgr', effect='none', use_video_port=False):
        super().__init__()
        self.camera = PiCamera()
        self.init_camera(resolution, framerate, imgformat, effect, use_video_port)
        self.sigMsg.emit(self.name + ": opened.")
        self.frame = None
        self.raw_capture = None
        self.stream = None

    def __del__(self):
        self.wait()

    def run(self):
        try:
            for f in self.stream:
                self.raw_capture.truncate(0)  # clear the stream in preparation for the next frame
                if self.pause:
                    self.sigMsg.emit(self.name + ": paused.")
                    break  # return from thread is needed
                else:
                    self.frame = f.array  # grab the frame from the stream
                    self.ready.emit(self.frame)
        ##                    self.sigMsg.emit(self.name + ": frame captured.")
        except Exception as err:
            self.sigMsg.emit(f"{self.name} + : error running thread. {err}")
        finally:
            self.sigMsg.emit(self.name + ": quit.")

    def init_camera(self, resolution=(640, 480), framerate=24, imgformat='bgr', effect='none', use_video_port=False):
        self.sigMsg.emit(self.name + "Init: resolution = " + str(resolution))
        self.camera.resolution = resolution
        self.camera.image_effect = effect
        self.camera.iso = 60  # should force unity analog gain
        ##        self.camera.color_effects = (128,128) # return monochrome image
        # dunno if setting awb mode manually is really useful
        ##        self.camera.awb_mode = 'off'
        ##        self.camera.awb_gains = 5.0
        ##        self.camera.meter_mode = 'average'
        ##        self.camera.exposure_mode = 'auto'  # 'sports' to reduce motion blur, 'off'after init to freeze settings
        self.camera.framerate = framerate
        self.raw_capture = PiRGBArray(self.camera, size=self.camera.resolution)
        self.stream = self.camera.capture_continuous(self.raw_capture, imgformat, use_video_port)
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
    def change_camera_settings(self, resolution=(640, 480), framerate=24, format="bgr", effect='none',
                               use_video_port=False):
        self.pause = True
        self.wait()
        self.init_camera(resolution, framerate, format, effect, use_video_port)
        self.pause = False
        self.start()  # restart thread


## Digital image processing threads
#
class ImgEnhancer(QThread):
    ## Logging message signal
    sigMsg = pyqtSignal(str)  # Message signal
    ## Image signal as numpy array
    ready = pyqtSignal()
    name = "ImageEnhancer"
    data = None

    def __init__(self):
        """
        TODO:
            Auto-rotate, based on info obtained further down the DIP chain
            Auto-crop, e.g. fixed, or based on rotate and on info obtained further down the DIP chain (cut uncharp edges)
        """
        super().__init__()
        self.cropRect = [0, 0, 0, 0]  # p1_y, p1_x, p2_y, p2_x
        self.rotAngle = 0.0
        self.gamma = 1.0
        self.clahe = None

    def __del__(self):
        self.wait()

    # Note that we need this wrapper around the Thread run function, since the latter will not accept any parameters
    @pyqtSlot(np.ndarray)
    def img_update(self, image=None):
        try:
            #            if self.data is None:  # first image to receive
            #                self.cropRect[2:] = image.shape[0:2]  # set cropping rectangle
            if self.isRunning():  # thread is already running
                self.sigMsg.emit(self.name + ": busy, frame dropped.")  # drop frame
            elif not (image is None):  # we have a new image
                self.data = image.copy()
                self.start()
        except Exception as err:
            self.sigMsg.emit(self.name + ": exception " + str(err))
            pass

    def run(self):
        try:
            if self.data is not None:
                self.sigMsg.emit(self.name + ": started.")
                self.data = cv2.cvtColor(self.data, cv2.COLOR_BGR2GRAY)  # convert to gray scale image
                #self.data = rotateImage(self.data, self.rotAngle)  # rotate
                self.data = self.data[self.cropRect[0]:self.cropRect[2],
                            self.cropRect[1]:self.cropRect[3]]  # crop the image
                ##                self.image = cv2.equalizeHist(self.image)  # histogram equalization
                if not self.clahe is None:
                    self.data = self.clahe.apply(self.data)  # Contrast Limited Adaptive Histogram Equalization.
                #self.data = adjust_gamma(self.data, self.gamma)  # change gamma
                self.ready.emit()
        except Exception as err:
            self.sigMsg.emit("Error in " + self.name)


if __name__ == "__main__":
    vs = PiVideoStream(effect='denoise')
    vs.start()
    #vs.ready.connect(mainWindow.kickTimer)  # Measure time delay
    #vs.ready.connect(imgEnhancer.imgUpdate, type=Qt.QueuedConnection)  # Connect video/image stream to processing
