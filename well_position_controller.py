import numpy as np
import csv
import os
from time import sleep
from datetime import datetime
from PyQt5.QtCore import QThread, pyqtSlot, pyqtSignal
import cv2
from random import randint


class WellPositionController(QThread):
    """
    Controls well positioning in the following manner:
    1) Move to the next well coordinates using preset setpoints (feedforward)
    2) Take image
    3) Evaluate well position from image
    4) Adjust well positioning if it's incorrect by using a feedback control loop repeating steps 2-4 until the image is scored as correct

    This class supports an arbitrary number of image evaluation algorithms that can be weighted differently
    """
    sig_msg = pyqtSignal(str)
    
    def __init__(self, setpoints_csv, max_offset_mm, motor_x, motor_y, mm_per_pixel, pio, vs, *evaluators,
                 target_coordinates=None, debug=False, logging=False, debug_mode_max_error_mm=5,
                 debug_mode_min_error_mm=0, enable_offsets=True):
        """
        Args:
            setpoints_csv: csv file path that contains one x,y setpoint per column.
            camera: Camera object instance to capture images
            motor_x: Motor object instance that controls x axis position
            motor_y: Motor object instance that controls y axis position
            mm_per_pixel: mm per pixel used to convert evaluator results (which are given in pixels)
            pio: pigpio instance
            vs: PiVideoStream instance
            max_offset_mm: (x, y) tuple of maximum allowed error in mm. If the absolute offset is lower the position will be considered correct
            target_coordinates: (x, y) tuple of target coordinates in image (diaphragm center). These can also be determined by using the calibrate function.
            *evaluators: List of tuples, each tuple of the format (WellPositionEvaluator, score_weight)
            debug: If set to True a random error will be added to each setpoint. Max error in each direction given by DEBUG_MODE_MAX_ERROR
                   The control loop will also require user input every iteration so that image processing results may be inspected
            logging: Set to True to log data to csv file
            debug_mode_max_error_mm: The maximum random error in mm while in debug mode
            debug_mode_min_error_mm: The minimum random error in mm while in debug mode
            enable_offsets: Enable/disable reading from a _offsets.csv file to adjust setpoints pre emptively.
        """
        super().__init__()
        self.setpoints_csv_filename = setpoints_csv
        self.setpoints = self.load_setpoints_from_csv(setpoints_csv)
        self.evaluators = evaluators
        self.pio = pio
        self.target = target_coordinates
        self.max_offset_mm = max_offset_mm
        self.motor_x = motor_x
        self.motor_y = motor_y
        self.mm_per_pixel = mm_per_pixel
        self.camera_started = False
        self.request_new_image = False  # Used by get_new_image and img_update to update self.img with a new frame from the pivideostream class
        self.img = None  # New image frames are stored here after calling self.get_new_image
        self.debug = debug
        self.logging = logging
        self.debug_mode_max_error_um = debug_mode_max_error_mm * 1000
        self.debug_mode_min_error_um = debug_mode_min_error_mm * 1000
        self.enable_offsets = enable_offsets
        # connect pivideostream frame emitter to the image update slot
        vs.ready.connect(self.img_update)

        if self.logging:
            self.setup_log()

    def setup_log(self):
        # Create logging csv and add header text
        if not os.path.isdir('logs'):
            os.mkdir('logs')
        if not os.path.isdir('images'):
            os.mkdir('images')
        self.logfile = 'logs/{}_WellPositionControllerLog.csv'.format(datetime.now().strftime('%Y%m%d%H%M%S'))
        with open(self.logfile, 'w') as f:
            csv_writer = csv.writer(f, delimiter=',')
            csv_headers = ['Timestamp', 'Target (pixel coordinates (x, y))', 'Setpoint (mm)']
            for evaluator, weight in self.evaluators:
                csv_headers.append('{} (weight {}) offset x (pixels)'.format(evaluator.__class__.__name__, weight))
                csv_headers.append('{} (weight {}) offset y (pixels)'.format(evaluator.__class__.__name__, weight))
                csv_headers.append('{} (weight {}) offset x (mm)'.format(evaluator.__class__.__name__, weight))
                csv_headers.append('{} (weight {}) offset y (mm)'.format(evaluator.__class__.__name__, weight))
            csv_headers.append('Total weighted offset (pixels)')
            csv_headers.append('Total weighted offset (mm)')
            csv_headers.append('Pass 1/Fail 0 (max offset in mm (x, y): {})'.format(self.max_offset_mm))
            csv_writer.writerow(csv_headers)

    def load_setpoints_from_csv(self, filename):
        """Load setpoints from csv file, the setpoint format should be as generated by the script /setpoints/generate_setpoints.py

        Args:
            filename: setpoints csv file path

        Returns: list of tuple(setpoint_x, setpoint_y), with setpoints assumed to be in mm

        """
        self.setpoints_csv_filename = filename
        with open(filename) as f:
            reader = csv.reader(f)
            return [(int(float(row[0])), int(float(row[1]))) for row in reader]

    def calibrate(self):
        """Find the diaphragm center by analyzing an image without a well plate present.
        Sets self.target to the newly calculated diaphragm centroid
        """
        # get new image
        while not self.get_new_image():  # get_new_image returns None when the camera is still starting up
            sleep(0.1)
        # make a list of centroids and their weights
        centroids = []
        weights = []
        for evaluator, weight in self.evaluators:
            centroid = evaluator.evaluate(self.img, target=(0, 0))
            if centroid is not None:
                centroids.append(centroid)
                weights.append(weight)
        # calculate the average centroid
        self.target = tuple(np.average(centroids, 0, weights).astype(int))

    def evaluate_position(self, img, setpoint):
        """
        Evaluates position based on all evaluator classes and their output weights.

        Args:
            img: 2d grayscale matrix
            setpoint: current setpoint (for logging purposes only)

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
        # convert offset to mm
        offset_mm = np.multiply(offset, self.mm_per_pixel)
        if abs(offset_mm[0]) < self.max_offset_mm[0] and abs(offset_mm[1]) < self.max_offset_mm[1]:
            result = True
        else:
            result = offset_mm

        if self.logging:
            # Log data to csv
            with open(self.logfile, 'a+') as f:
                csv_writer = csv.writer(f, delimiter=',')
                csv_data = []
                timestamp = datetime.now().strftime('%Y%m%d%H%M%S')
                csv_data.append(timestamp)  # Timestamp
                csv_data.append(self.target)  # Target (pixel coordinates)
                csv_data.append(setpoint)  # Setpoint (mm)
                for i in range(len(self.evaluators)):
                    # Write offset x and offset y in pixels and mm for each evaluator
                    csv_data.append(offsets[i][0])
                    csv_data.append(offsets[i][1])
                    csv_data.append("{0:.5f}".format(offsets[i][0] * self.mm_per_pixel))
                    csv_data.append("{0:.5f}".format(offsets[i][1] * self.mm_per_pixel))
                csv_data.append(offset)  # Total weighted offset in pixels
                csv_data.append("({0:.5f}, {0:.5f})".format(offset_mm[0], offset_mm[1]))  # Total weighted offset in mm
                if result is True:
                    csv_data.append(1)  # 1 = Pass, 0 = Fail
                else:
                    csv_data.append(0)
                csv_writer.writerow(csv_data)
                # Save image with the timestamp corresponding to the current row in the csv.
                cv2.imwrite('images/{}.png'.format(timestamp), img)

        return result

    @pyqtSlot(np.ndarray)
    def img_update(self, image=None):
        """ Store the latest image frame from PiVideoStream in self.img
        Only overwrite self.img if a new frame has been requested by calling get_new_image

        Args:
            image: bgr image matrix (opencv compatible)
        """
        self.camera_started = True
        try:
            if not self.request_new_image:
                self.sig_msg.emit(self.__class__.__name__ + ": no new image needed, frame dropped.")
            else:
                self.img = image.copy()
                # convert to grayscale
                self.img = cv2.cvtColor(self.img, cv2.COLOR_BGR2GRAY)
                self.request_new_image = False
        except Exception as err:
            self.sig_msg.emit(self.__class__.__name__, ": exception in img_update " + str(err))

    def get_new_image(self):
        if not self.camera_started:
            # Return None if the camera is not yet started -> cannot obtain a new frame yet
            return None
        self.img = None
        # Request new image frame from PiVideoStream
        self.request_new_image = True
        while self.request_new_image:
            # Wait for new image frame
            sleep(0.01)
        return True  # Return True on success

    def move_motors(self, delta_x_mm, delta_y_mm):
        """Move the well plate in two dimensions.

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
        # Move motors 1 by 1
        self.motor_x.go_once(mm=abs(delta_x_mm), clockwise=clockwise_x)
        self.motor_y.go_once(mm=abs(delta_y_mm), clockwise=clockwise_y)

    def control_loop(self):
        """
        Main control loop
        """
        if self.target is None:
            # A target has to be set either by manually setting it or by calling calibrate() without a well plate present.
            raise AssertionError('Target coordinates should be set manually or via calibration.')

        # If setpoint correction offsets are available to load from previous runs, load them now and apply it to the setpoints
        offsets_csv_path = os.path.splitext(self.setpoints_csv_filename)[0] + "_offsets.csv"
        if not self.enable_offsets:
            modified_setpoints = self.setpoints
        else:
            try:
                with open(offsets_csv_path, 'r') as f:
                    reader = csv.reader(f)
                    setpoint_offsets = [(float(row[0]), float(row[1])) for row in reader]
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
        # Todo: this assumes the well plate position starts @ 0,0 : ie a corner of the well plate is in frame
        # Todo: the initial previous setpoint probably needs to be set to some known initial offset
        # Todo: the motors should also start from a known position (probably against the limit switches)
        # Todo: For now assume that we start @ well A1, with calibration being done manually beforehand
        previous_setpoint_x = 0
        previous_setpoint_y = 0
        for i, setpoint in enumerate(modified_setpoints):
            setpoint_x, setpoint_y = setpoint
            # feed forward to the setpoint coordinates
            if not self.debug:
                # for some reason we move twice as far as required? halve setpoint for now
                self.move_motors((setpoint_x - previous_setpoint_x)/2, (setpoint_y - previous_setpoint_y)/2)
                print("current setpoint x, y {}, {}" .format(setpoint_x, setpoint_y))
                previous_setpoint_x = setpoint_x
                previous_setpoint_y = setpoint_y
            else:
                # In debug mode add a random error
                random_error_x = randint(self.debug_mode_min_error_um, self.debug_mode_max_error_um) / 1000
                random_error_y = randint(self.debug_mode_min_error_um, self.debug_mode_max_error_um) / 1000
                # random direction
                if randint(0, 1) == 0:
                    random_error_x *= -1
                if randint(0, 1) == 0:
                    random_error_y *= -1

                setpoint_x2 = setpoint_x + random_error_x
                setpoint_y2 = setpoint_x + random_error_y
                self.move_motors(setpoint_x2 - previous_setpoint_x,
                                 setpoint_y2 - previous_setpoint_y)
                previous_setpoint_x = setpoint_x
                previous_setpoint_y = setpoint_y
                setpoint_x = setpoint_x2
                setpoint_y = setpoint_y2

            total_error = [0, 0]  # Keep track of the total error so that we can save the new offset for the next run
            passed = False
            while not passed:
                # Use camera feedback to improve position until it passes
                # Get new image frame
                while not self.get_new_image():  # get_new_image returns None when the camera is still starting up
                    sleep(0.1)
                # evaluate image
                result = self.evaluate_position(self.img, (setpoint_x, setpoint_y))
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

        # write new offsets to _offsets.csv file (unless in debug mode)
        if not self.debug and self.enable_offsets:
            with open(offsets_csv_path, 'w') as f:
                f.writelines(["{:.3f}, {:.3f}\n".format(x[0], x[1]) for x in new_offsets])

        print("FINISHED")

    def calibrate_mm_per_step_photos(self):
        # Rough setup to generate mm_per_step calibration images
        while not self.get_new_image():  # get_new_image returns None when the camera is still starting up
            sleep(0.5)
        self.get_new_image()
        cv2.imshow('image', self.img)
        cv2.imwrite('images/x_1.png', self.img)
        self.move_motors(-2500, 0)
        self.get_new_image()
        cv2.imshow('image', self.img)
        cv2.imwrite('images/x_2.png', self.img)
        self.move_motors(2500, 0)

        # y
        # while not self.get_new_image():
        # sleep(0.5)
        # self.get_new_image()
        # cv2.imshow('image', self.img)
        # cv2.imwrite('images/y_1.png', self.img)
        # self.move_motors(0, -2200)
        # self.get_new_image()
        # cv2.imshow('image', self.img)
        # cv2.imwrite('images/y_2.png', self.img)
        # self.move_motors(0, 2200)

    def run(self):
        self.calibrate()
        print("calibrated target: {}".format(self.target))
        input("Press enter to continue")
        self.control_loop()
