import numpy as np


class WellPositionController:
    """
    Controls well positioning in the following manner:
    1) Move to the next well coordinates using preset setpoints (feedforward)
    2) Take image
    3) Evaluate well position from image
    4) Adjust well positioning if its wrong using a feedback control loop by repeating steps 2-4 until the image is scored as correct

    This class supports multiple scoring algorithms that are weighted differently
    """
    def __init__(self, setpoints, camera, motor_x, motor_y, max_offset, *evaluators, target_coordinates=None):
        """
        Args:
            setpoints: List of tuples, each tuple of the format (x, y)
            camera: Camera object instance to capture images
            motor_x: Motor object instance that controls x axis position
            motor_y: Motor object instance that controls y axis position
            max_offset: (x, y) tuple of maximum allowed error. If the absolute offset is lower the position will be considered correct
            target_coordinates: (x, y) tuple of target coordinates in image (diaphragm center). These can also be determined by using the calibrate function.
            *evaluators: List of tuples, each tuple of the format (WellPositionEvaluator, score_weight)
        """
        self.setpoints = setpoints
        self.evaluators = evaluators
        self.target = target_coordinates
        self.camera = camera
        self.motor_x = motor_x
        self.motor_y = motor_y
        self.max_offset = max_offset

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
        if self.target is None:
            # A target has to be set either by manually setting it or by calling calibrate() without a well plate present.
            raise AssertionError('Target coordinates should be set manually or via calibration.')
        offsets = []
        weights = []
        for evaluator, weight in self.evaluators:
            offset = evaluator.evaluate(img, self.target)
            if offset is not None:
                offsets.append(offset)
                weights.append(weight)
        # calculate the absolute average offset
        offset = abs(np.average(offsets, 0, weights))
        if offset[0] < self.max_offset[0] and offset[1] < self.max_offset[1]:
            return True
        else:
            return offset

    def control_loop(self):
        """
        Main control loop
        """
        for setpoint in self.setpoints:
            # todo: feedforward to move to the next setpoint position

            passed = False
            while not passed:
                # Use camera feedback to improve position until it passes
                img = self.camera.capture_raw_image()
                result = self.evaluate_position(img)
                if result is True:
                    # todo the position is correct -> stop the motors
                    passed = True
                else:
                    # todo adjust motor speed and direction via PID
                    pass

            # todo: take full resolution image and analyze for worm count / life stage

    def start(self):
        """
        Start control loop.
        """
        pass

    def stop(self):
        """
        Stop control loop.
        """
        pass
