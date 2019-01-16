import numpy as np
import csv
import os
from time import sleep
from well_position_evaluators import WellBottomFeaturesEvaluator
from PyQt5.QtCore import QThread, pyqtSignal, pyqtSlot
import cv2
import threading


class WellPositionController(QThread):
    """
    Controls well positioning in the following manner:
    1) Move to the next well coordinates using preset setpoints (feedforward)
    2) Take image
    3) Evaluate well position from image
    4) Adjust well positioning if its wrong using a feedback control loop by repeating steps 2-4 until the image is scored as correct

    This class supports multiple scoring algorithms that can be weighted differently
    """
    sig_msg = pyqtSignal(str)
    ready = pyqtSignal()
    name = "WellPositionController"
    data = None

    def __init__(self, setpoints_csv, max_offset, motor_x, motor_y, camera, *evaluators, target_coordinates=None):
        """
        Args:
            setpoints_csv: csv file path that contains one x,y setpoint per column.
            camera: Camera object instance to capture images
            motor_x: Motor object instance that controls x axis position
            motor_y: Motor object instance that controls y axis position
            camera: PiVideoStream object instance
            max_offset: (x, y) tuple of maximum allowed error. If the absolute offset is lower the position will be considered correct
            target_coordinates: (x, y) tuple of target coordinates in image (diaphragm center). These can also be determined by using the calibrate function.
            *evaluators: List of tuples, each tuple of the format (WellPositionEvaluator, score_weight)
        """
        super().__init__()
        self.setpoints_csv_filename = setpoints_csv
        self.setpoints = self.load_setpoints_from_csv(setpoints_csv)
        self.evaluators = evaluators
        self.target = target_coordinates
        self.max_offset = max_offset
        self.motor_x = motor_x
        self.motor_y = motor_y
        self.camera_started = False
        self.request_new_image = False  # Set to True to request a new image frame from PiVideoStream that will be stored in self.img
        self.img = None

    def __del__(self):
        self.wait()

    def load_setpoints_from_csv(self, filename):
        self.setpoints_csv_filename = filename
        with open(filename) as f:
            reader = csv.reader(f)
            return [tuple(row) for row in reader]

    def calibrate(self):
        """
        Find the diaphragm center by analyzing an image without a well plate present.
        Sets self.target to the newly calculated diaphragm center.
        """
        # capture image
        img = self.camera.capture_raw_image()
        # make a list of centroids and their weights
        centroids = []
        weights = []
        for evaluator, weight in self.evaluators:
            evaluator.evaluate(img)
            centroid = evaluator.centroid
            if centroid is not None:
                centroids.append(evaluator.centroid)
                weights.append(weight)
        # calculate the average centroid
        self.target = tuple(np.average(centroids, 0, weights))

    def evaluate_position(self, img):
        """
        Evaluates position based on all evaluator classes and their output weights.
        Returns:
             an (x,y) offset vector if the image is not correct or True if the image is correct
        """
        offsets = []
        weights = []
        for evaluator, weight in self.evaluators:
            offset = evaluator.evaluate(img, self.target)
            if offset is not None:
                offsets.append(offset)
                weights.append(weight)
        # calculate the average offset
        offset = np.average(offsets, 0, weights)
        if abs(offset[0]) < self.max_offset[0] and abs(offset[1]) < self.max_offset[1]:
            return True
        else:
            return offset

    @pyqtSlot(np.ndarray)
    def img_update(self, image=None):
        """ Store the latest image frame from PiVideoStream in self.img
        Only update the image if it's not currently being analyzed

        Args:
            image: bgr image matrix (opencv compatible)
        """
        self.camera_started = True
        try:
            if not self.request_new_image:
                self.sig_msg.emit(self.name + ": no new image needed, frame dropped.")
            elif not (image is None):
                self.img = image.copy()
                self.request_new_image = False
        except Exception as err:
            self.sig_msg.emit(self.name, ": exception in img_update " + str(err))
            
    def get_new_image(self):
        if not self.camera_started:
            # Return None if the camera is not yet started -> cannot obtain a new frame
            return None
        self.img = None
        # Request new image frame from PiVideoStream
        self.request_new_image = True
        while self.img is None:
            # Wait for new image frame
            sleep(0.01)
        return True  # Return True on success

    def move_motors(self, delta_x_mm, delta_y_mm):
        """Move the well plate in two dimensions simultaneously.

        Args:
            delta_x_mm: delta x given in mm, negative number moves counterclockwise
            delta_y_mm: delta y given in mm, negative number moves counterclockwise
        """
        # Determine motor direction
        if delta_x_mm > 0:
            clockwise_x = True
        else:
            clockwise_x = False
        if delta_y_mm > 0:
            clockwise_y = True
        else:
            clockwise_y = False
        # Since the go_once function blocks until the target is reached, start them threaded and wait for the threads to finish.
        t1 = threading.Thread(target=self.motor_x.go_once, kwargs={'mm': abs(delta_x_mm), 'clockwise': clockwise_x})
        t2 = threading.Thread(target=self.motor_y.go_once, kwargs={'mm': abs(delta_y_mm), 'clockwise': clockwise_y})
        t1.start()
        t2.start()
        t1.join()
        t2.join()

    def control_loop(self):
        """
        Main control loop
        """
        if self.target is None:
            # A target has to be set either by manually setting it or by calling calibrate() without a well plate present.
            raise AssertionError('Target coordinates should be set manually or via calibration.')

        # If setpoint correction offsets are available to load from previous runs, load them now and apply it to the setpoints
        offsets_csv_path = os.path.splitext(self.setpoints_csv_filename)[0] + "_offsets.csv"
        try:
            with open(offsets_csv_path, 'r') as f:
                reader = csv.reader(f)
                setpoint_offsets = [tuple(row) for row in reader]
            modified_setpoints = np.add(self.setpoints, setpoint_offsets)
        except FileNotFoundError as ex:
            # initialise setpoints offsets csv file with all zeroes
            with open(offsets_csv_path, 'w') as f:
                setpoint_offsets = []
                for _ in range(len(self.setpoints)):
                    f.write("0, 0\n")
                    setpoint_offsets.append(0)
            modified_setpoints = self.setpoints

        new_offsets = []
        previous_setpoint_x = 0
        previous_setpoint_y = 0
        for i, setpoint in enumerate(modified_setpoints):
            setpoint_x, setpoint_y = setpoint
            # feed forward to the setpoint coordinates
            self.move_motors(setpoint_x - previous_setpoint_x, setpoint_y - previous_setpoint_y)

            total_error = [0, 0]  # Keep track of the total error so that we can save the new offset for the next run
            passed = False
            while not passed:
                # Use camera feedback to improve position until it passes
                # Get new image frame
                self.get_new_image()
                # convert to grayscale
                self.img = cv2.cvtColor(self.img, cv2.COLOR_BGR2GRAY)
                # evaluate image
                result = self.evaluate_position(self.img)
                if result is True:
                    # the position is correct -> continue by taking a final full resolution picture for analysis and
                    # continue with the next well
                    # todo: take full resolution image and analyze for worm count / life stage
                    passed = True
                    # Store the total_error offset for writing to the _offsets.csv file later
                    new_offsets.append(tuple(np.add(total_error, setpoint_offsets[i])))
                else:
                    total_error = list(np.add(total_error, result))
                    # basically feedforward by result[0] in x and result[1] in y directions
                    self.move_motors(result[0], result[1])

            previous_setpoint_x = setpoint_x
            previous_setpoint_y = setpoint_y

        # write new offsets to _offsets.csv file
        with open(offsets_csv_path, 'w') as f:
            f.writelines(["{}, {}".format(x[0], x[1]) for x in new_offsets])
            
    def test(self):
        #while True:
            # Display new image frame
          #  if self.get_new_image():
          #      cv2.imshow('image', self.img)
          #  else:
          #      print("Camera is paused")

         #   self.motor_x.stop()
         print("ccw")
         self.motor_x.stop()
         self.motor_x.go_once(5, clockwise=False)

    def run(self):
        self.test()
        #self.control_loop()


if __name__ == '__main__':
    setpoints_csv_file = "setpoints\\24.csv"
    e = ((WellBottomFeaturesEvaluator(True), 1),)
    wpc = WellPositionController(setpoints_csv_file, (16, 16), (WellBottomFeaturesEvaluator(True), 1), target_coordinates=(0, 0))
    wpc.start()
