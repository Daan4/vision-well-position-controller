import numpy as np
from PyQt5.QtCore import QThread, pyqtSignal, pyqtSlot
from picamera import PiCamera
from picamera.array import PiRGBArray
import time


class PiVideoStream(QThread):
    sig_msg = pyqtSignal(str)  # logging message signal
    ready = pyqtSignal(np.ndarray)  # image signal as numpy array
    def __init__(self, resolution=(640, 480), framerate=24, imgformat='bgr', effect='none', use_video_port=False):
        """

        Args:
            resolution: frame resolution
            framerate: frames per second
            imgformat: image format (defaults to bgr for opencv compatibility)
            effect: camera effect
            use_video_port: ???
        """
        super().__init__()
        self.camera = PiCamera()
        self.frame = None
        self.rawCapture = None
        self.stream = None
        self.init_camera(resolution, framerate, imgformat, effect, use_video_port)
        self.pause = False
        self.sig_msg.emit(self.__class__.__name__ + ": opened.")

    def __del__(self):
        self.wait()

    def run(self):
        try:
            for f in self.stream:
                self.rawCapture.truncate(0)  # clear the stream in preparation for the next frame
                if self.pause:
                    self.sig_msg.emit(self.__class__.__name__ + ": paused.")
                    break   # return from thread is needed
                else:
                    self.frame = f.array  # grab the frame from the stream
                    self.ready.emit(self.frame)
        except Exception as err:
            self.sig_msg.emit(self.__class__.__name__ + ": error running thread. " + str(err))
        finally:
            self.sig_msg.emit(self.__class__.__name__ + ": quit.")

    def init_camera(self, resolution=(640, 480), framerate=24, imgformat='bgr', effect='none', use_video_port=False):
        self.sig_msg.emit(self.__class__.__name__ + "Init: resolution = " + str(resolution))
        self.camera.resolution = resolution
        self.camera.image_effect = effect
        self.camera.iso = 60
        self.camera.framerate = framerate
        self.rawCapture = PiRGBArray(self.camera, size=self.camera.resolution)
        self.stream = self.camera.capture_continuous(self.rawCapture, imgformat, use_video_port)
        self.frame = None
        time.sleep(2)

    @pyqtSlot()
    def stop(self):
        self.pause = True
        self.wait()
        self.quit()
        self.sig_msg.emit(self.__class__.__name__ + ": closed.")

    @pyqtSlot()
    def change_camera_settings(self, resolution=(640, 480), framerate=24, _format="bgr", effect='none',
                               use_video_port=False):
        self.pause = True
        self.wait()
        self.init_camera(resolution, framerate, _format, effect, use_video_port)
        self.pause = False
        self.start()  # restart thread
