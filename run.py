"""Run this file to start. 'python3 run.py'
NOTE: the pigpio daemon should be running on the target: 'sudo pigpiod'"""
from well_position_controller import WellPositionController
from well_position_evaluators import WellBottomFeaturesEvaluator, HoughTransformEvaluator
from camera import PiVideoStream
from motor import Stepper
import pigpio
import sys
from PyQt5.QtWidgets import QApplication
import signal
from qtui import MainWindow

# define motor driver pins
X_NEN_pin = 14
X_STP_pin = 15
X_DIR_pin = 18
X_LIM_pin = 0

Y_NEN_pin = 23
Y_STP_pin = 24
Y_DIR_pin = 25
Y_LIM_pin = 0

# mm per step
MM_PER_STEP = 0.005  # appears to be the same for x and y

# used x_1, x_2, y_1, y_2 images to determine above variables
# div = 0.1mm
# steps_x = 2500
# steps_y = 2200
# mm_x = 12.5
# mm_y = 10.6


# mm per pixel
MM_PER_PIXEL = 0.0254  # (depends on height?, this is for settings on 17/1/19), used image mm_per_pixel_test

# define camera settings
RESOLUTION = (640, 480)  # width, height
FRAME_RATE = 30
USE_VIDEO_PORT = False

# enable debug mode for controller and evaluators
ENABLE_DEBUG_MODE_EVALUATOR = True
ENABLE_DEBUG_MODE_CONTROLLER = False
# set max random error in debug mode (applied +- in both x and y direction)
DEBUG_MODE_MAX_ERROR_MM = 2.5
DEBUG_MODE_MIN_ERROR_MM = 0.5

MAX_ALLOWED_ERROR_MM = (0.2, 0.2)

# enable logging data to csv file
ENABLE_LOGGING = True

# global reference to motor classes, to stop them after a sigint
motor_x = None
motor_y = None

# SETPOINTS_FILE = "setpoints/debug_mode_test.csv"
SETPOINTS_FILE = "setpoints/48.csv"


def catch_sigint(sig, frame):
    # this function handles sigints to stop the motors
    # todo: sometimes this will fail to actually stop the motor.
    # capture and investigate the exception that pops up in the terminal when this happens
    print("stopping motors and quitting")
    motor_x.stop()
    motor_y.stop()
    sys.exit(0)


# Run this file to start
if __name__ == '__main__':
    # Set up QApplication
    app = QApplication(sys.argv)

    # Set up gui
    main_window = MainWindow()

    # Set up camera video stream
    vs = PiVideoStream(resolution=RESOLUTION, framerate=FRAME_RATE, use_video_port=USE_VIDEO_PORT)
    vs.start()

    # Set up Stepper motor driver
    pio = pigpio.pi()
    motor_x = Stepper(pio, MM_PER_STEP, NEN_pin=X_NEN_pin, DIR_pin=X_DIR_pin, STP_pin=X_STP_pin)
    motor_y = Stepper(pio, MM_PER_STEP, NEN_pin=Y_NEN_pin, DIR_pin=Y_DIR_pin, STP_pin=Y_STP_pin)

    # Set up well position controller and evaluators
    target_coordinates = (0, 0)  # to be determined
    e1 = (WellBottomFeaturesEvaluator(RESOLUTION, ENABLE_DEBUG_MODE_EVALUATOR), 1)
    # e2 = (HoughTransformEvaluator(RESOLUTION, ENABLE_DEBUG_MODE), 1)
    wpc = WellPositionController(SETPOINTS_FILE,
                                 MAX_ALLOWED_ERROR_MM,
                                 motor_x,
                                 motor_y,
                                 MM_PER_PIXEL,
                                 pio,
                                 vs,
                                 e1,
                                 # e2,
                                 target_coordinates=target_coordinates,
                                 debug=ENABLE_DEBUG_MODE_CONTROLLER,
                                 logging=ENABLE_LOGGING,
                                 debug_mode_max_error_mm=DEBUG_MODE_MAX_ERROR_MM,
                                 debug_mode_min_error_mm=DEBUG_MODE_MIN_ERROR_MM)

    # Connect qt signals and slots for displaying images
    # connect evaluator emitters to update function in gui to display images using qt
    e1[0].update_blur.connect(main_window.update_blur)
    e1[0].update_gamma.connect(main_window.update_gamma)
    e1[0].update_threshold.connect(main_window.update_threshold)
    e1[0].update_scores.connect(main_window.update_scores)
    e1[0].update_result.connect(main_window.update_result)

    wpc.start()

    # Set up a sigint handler function
    signal.signal(signal.SIGINT, catch_sigint)

    main_window.show()
    print("starting event loop")
    app.exec_()
