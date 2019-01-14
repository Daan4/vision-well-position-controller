from well_position_controller import WellPositionController
from well_position_evaluators import WellBottomFeaturesEvaluator, HoughTransformEvaluator
from camera import PiVideoStream
from motor import Stepper
import pigpio
import sys
from PyQt5.QtWidgets import QApplication
import signal

## define motor driver pins
X_NEN_pin = 14
X_STP_pin = 15
X_DIR_pin = 18
X_LIM_pin = 0

Y_NEN_pin = 23
Y_STP_pin = 24
Y_DIR_pin = 25
Y_LIM_pin = 0

## define camera settings
RESOLUTION = (640, 480)  # width, height
FRAME_RATE = 30
USE_VIDEO_PORT = True

## define evaluator settings
DEBUG_MODE = False

motor_x = None
motor_y = None

def catch_sigint(sig, frame):
    print("stopping motors and quitting")
    motor_x.stop()
    motor_y.stop()
    sys.exit(0)

# Run this file to start
if __name__ == '__main__':
    # Set up QApplication
    app = QApplication(sys.argv)
    
    # Set up camera video stream
    vs = PiVideoStream(resolution=RESOLUTION, framerate=FRAME_RATE, use_video_port=USE_VIDEO_PORT)
    print("starting pivideostream")
    vs.start()
    
    # Set up Stepper motor driver
    pio = pigpio.pi()
    motor_x = Stepper(pio, NEN_pin=X_NEN_pin, DIR_pin=X_DIR_pin, STP_pin=X_STP_pin)
    motor_y = Stepper(pio, NEN_pin=Y_NEN_pin, DIR_pin=Y_DIR_pin, STP_pin=Y_STP_pin)
    

    # Set up well position controller and evaluators
    setpoints_csv_file = "setpoints/24.csv"
    target_coordinates = (0, 0)  # to be determined
    e = ((WellBottomFeaturesEvaluator(RESOLUTION, DEBUG_MODE), 1),
         (HoughTransformEvaluator(RESOLUTION, DEBUG_MODE), 1))
    wpc = WellPositionController(setpoints_csv_file, 
                                 (16, 16), 
                                 motor_x, 
                                 motor_y,
                                 vs,
                                 e, 
                                 target_coordinates=target_coordinates)
                                 
    # connect pivideostream frame emitter to update function in wpc
    vs.ready.connect(wpc.img_update)

    print("starting wpc")
    wpc.start()
    
    print("starting event loop")
    signal.signal(signal.SIGINT, catch_sigint)
    app.exec_()
    
