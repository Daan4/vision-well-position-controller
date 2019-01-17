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

## mm per step
MM_PER_STEP = 0.005  # appears to be the same for x and y

# used x_1, x_2, y_1, y_2 images to determine above variables
# div = 0.1mm
# steps_x = 2500
# steps_y = 2200
# mm_x = 12.5
# mm_y = 10.6

## mm per pixel
MM_PER_PIXEL = 0.00254 # (depends on height?, this is for settings on 17/1/19)


## define camera settings
RESOLUTION = (640, 480)  # width, height
FRAME_RATE = 30
USE_VIDEO_PORT = False


## define evaluator settings
DEBUG_MODE = True

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
    motor_x = Stepper(pio, MM_PER_STEP, NEN_pin=X_NEN_pin, DIR_pin=X_DIR_pin, STP_pin=X_STP_pin)
    motor_y = Stepper(pio, MM_PER_STEP, NEN_pin=Y_NEN_pin, DIR_pin=Y_DIR_pin, STP_pin=Y_STP_pin)
    

    # Set up well position controller and evaluators
    setpoints_csv_file = "setpoints/debug_mode_test.csv"
    target_coordinates = (0, 0)  # to be determined
    e = (WellBottomFeaturesEvaluator(RESOLUTION, DEBUG_MODE), 1)
         #(HoughTransformEvaluator(RESOLUTION, DEBUG_MODE), 1))
    wpc = WellPositionController(setpoints_csv_file, 
                                 (16, 16), 
                                 motor_x, 
                                 motor_y,
                                 MM_PER_PIXEL,
                                 pio,
                                 e, 
                                 target_coordinates=target_coordinates,
                                 debug=DEBUG_MODE)
                                 
    # connect pivideostream frame emitter to update function in wpc
    vs.ready.connect(wpc.img_update)

    print("starting wpc")
    wpc.start()
    
    print("starting event loop")
    signal.signal(signal.SIGINT, catch_sigint)
    app.exec_()
