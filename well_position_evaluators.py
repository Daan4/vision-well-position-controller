from abc import abstractmethod, ABC
import cv2
import numpy as np


class WellPositionEvaluator(ABC):
    @abstractmethod
    def __init__(self, target, debug):
        pass

    @abstractmethod
    def evaluate(self, img):
        """
            Vision algorithm implementation of the evaluator.

        Args:
            img: grayscale 2d opencv image matrix to evaluate
            debug_mode: Set to True to display live visual feedback for debugging purposes

        Returns:
            A vector (x, y) that represents the found offset between the well center and diaphragm center.
            Should return None if no vector is found.
        """
        pass


class WellBottomFeaturesEvaluator(WellPositionEvaluator):
    # todo
    # current implementation assumes 410x308 resolution
    # scale vision parameters accordingly to actual rpi camera resolution
    def __init__(self, target, debug=False):
        super().__init__(target, debug)
        # Set up debug windows if debug mode is on
        self.target = target
        self.debug = debug
        if self.debug:
            cv2.namedWindow('Blur', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Blur', 410, 308)
            cv2.moveWindow('Blur', 50, 100)

            cv2.namedWindow('Gamma', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Gamma', 410, 308)
            cv2.moveWindow('Gamma', 460, 100)

            cv2.namedWindow('Threshold', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Threshold', 410, 308)
            cv2.moveWindow('Threshold', 870, 100)

            cv2.namedWindow('Morphology', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Morphology', 410, 308)
            cv2.moveWindow('Morphology', 1280, 100)

            cv2.namedWindow('Result', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Result', 410, 308)
            cv2.moveWindow('Result', 50, 500)

    def evaluate(self, img):
        if self.debug:
            # Make a copy when debug mode is on so that we can overlay the results on it later.
            original = img.copy()

        # Gaussian filter
        blur_kernelsize = (25, 25)
        blur_sigma = 100
        img = cv2.GaussianBlur(img, blur_kernelsize, blur_sigma)
        if self.debug:
            cv2.imshow('Blur', img)

        # Auto contrast
        _min = np.min(img)
        _max = np.max(img)
        img = np.subtract(img, _min)
        img = np.divide(img, (_max - _min))
        img = np.multiply(img, 255)
        cv2.normalize(img, img, 1, 0, cv2.NORM_MINMAX)

        # Gamma
        c = 2
        gamma = 6
        img = np.power(img, gamma)
        img = np.multiply(img, c * 255)
        np.putmask(img, img > 255, 255)
        img = img.astype(np.uint8)
        if self.debug:
            cv2.imshow('Gamma', img)

        # Otsu threshold
        _, img = cv2.threshold(img, 0, 255, cv2.THRESH_OTSU)
        if self.debug:
            cv2.imshow('Threshold', img)

        # Morphology
        open_kernelsize = (50, 50)
        kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, open_kernelsize)
        img = cv2.morphologyEx(img, cv2.MORPH_OPEN, kernel)
        if self.debug:
            cv2.imshow('Morphology', img)

        # Keep only the well bottom region
        # By looking at region features
        # step 1: Keep only blobs that score above roundness threshold
        # step 2: Keep largest blob remaining after step 1
        metric_threshold = 0.8
        largest_area_above_threshold = -1
        best_match = None
        im2, contours, hierarchy = cv2.findContours(img, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
        for c in contours:
            area = cv2.contourArea(c)
            perimeter = cv2.arcLength(c, True)
            metric = 4 * np.pi * area / perimeter ** 2
            if metric > metric_threshold and area > largest_area_above_threshold:
                largest_area_above_threshold = area
                best_match = c

        # calculate centroid for best match blob
        if best_match is None:
            if self.debug:
                cv2.imshow('Result', original)
            return None
        else:
            M = cv2.moments(best_match)
            cX = int(M["m10"] / M["m00"])
            cY = int(M["m01"] / M["m00"])
            centroid = (cX, cY)
            offset = tuple(np.subtract(self.target, centroid))
            if self.debug:
                # Overlay results on source image and display them
                cv2.circle(original, self.target, 5, 0, 1)
                cv2.circle(original, centroid, 4, 0, 5)
                cv2.imshow('Result', original)
            return offset


if __name__ == '__main__':
    # Test WellBottomFeaturesEvaluator with a test image
    #imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_2_downscaled.png'
    imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_6_downscaled.png'
    x = WellBottomFeaturesEvaluator((227, 144), False)
    print(x.evaluate(cv2.imread(imgpath, cv2.CV_8UC1)))
    cv2.waitKey(0)
