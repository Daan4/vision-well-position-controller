import numpy as np
import csv
import os
from camera import Camera
from motor import Motor
from well_position_evaluators import WellBottomFeaturesEvaluator


class WellPositionController:
    """
    Controls well positioning in the following manner:
    1) Move to the next well coordinates using preset setpoints (feedforward)
    2) Take image
    3) Evaluate well position from image
    4) Adjust well positioning if its wrong using a feedback control loop by repeating steps 2-4 until the image is scored as correct

    This class supports multiple scoring algorithms that can be weighted differently
    """
    def __init__(self, setpoints_csv, camera, motor_x, motor_y, max_offset, *evaluators, target_coordinates=None):
        """
        Args:
            setpoints_csv: csv file path that contains one x,y setpoint per column.
            camera: Camera object instance to capture images
            motor_x: Motor object instance that controls x axis position
            motor_y: Motor object instance that controls y axis position
            max_offset: (x, y) tuple of maximum allowed error. If the absolute offset is lower the position will be considered correct
            target_coordinates: (x, y) tuple of target coordinates in image (diaphragm center). These can also be determined by using the calibrate function.
            *evaluators: List of tuples, each tuple of the format (WellPositionEvaluator, score_weight)
        """
        self.setpoints_csv_filename = setpoints_csv
        self.setpoints = self.load_setpoints_from_csv(setpoints_csv)
        self.evaluators = evaluators
        self.target = target_coordinates
        self.camera = camera
        self.motor_x = motor_x
        self.motor_y = motor_y
        self.max_offset = max_offset

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
        for i, setpoint in enumerate(modified_setpoints):

            # todo: feedforward first to move to the next setpoint position

            total_error = [0, 0]  # Keep track of the total error so that we can save the new offset for the next run
            passed = False
            while not passed:
                # Use camera feedback to improve position until it passes
                img = self.camera.capture_raw_image()
                result = self.evaluate_position(img)
                if result is True:
                    # the position is correct -> continue by taking a final full resolution picture for analysis and
                    # continue with the next well
                    # todo: take full resolution image and analyze for worm count / life stage
                    passed = True
                    # Store the total_error offset for writing to the _offsets.csv file later
                    new_offsets.append(tuple(np.add(total_error, setpoint_offsets[i])))
                else:
                    total_error = list(np.add(total_error, result))

                    # todo feedforward based on newton's method, where the new setpoint is based on the error
                    # basically feedforward by result[0] in x and result[1] in y directions

        # write new offsets to _offsets.csv file
        with open(offsets_csv_path, 'w') as f:
            f.writelines([f"{x[0]}, {x[1]}" for x in new_offsets])

    def start(self):
        """
        Start control loop.
        """
        self.control_loop()

    def stop(self):
        """
        Stop control loop.
        """
        pass


if __name__ == '__main__':
    setpoints_csv_file = "setpoints\\24.csv"
    e = ((WellBottomFeaturesEvaluator(True), 1),)
    wpc = WellPositionController(setpoints_csv_file, Camera(), Motor(), Motor(), (16, 16), (WellBottomFeaturesEvaluator(True), 1), target_coordinates=(0, 0))
    wpc.start()
