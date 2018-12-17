from abc import abstractmethod, ABC
import cv2
import numpy as np
import wormvision
import timeit

# todo maybe implement some other evaluators in opencv?-


class WellPositionEvaluator(ABC):
    @abstractmethod
    def __init__(self, debug):
        self.centroid = None  # If a centroid is found in the evaluate function it can be stored here.
                              # The centroids are used during calibration to determine the target.

    @abstractmethod
    def evaluate(self, img, target):
        """
            Vision algorithm implementation of the evaluator.

        Args:
            img: grayscale 2d opencv image matrix to evaluate
            target: target position (x,y) tuple9
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
    def __init__(self, debug=False):
        super().__init__(debug)
        # Set up debug windows if debug mode is on
        self.debug = debug
        self.centroid = None
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

    def evaluate(self, img, target=(0, 0), benchmarking=False):
        if not self.debug:
            # use custom vision library
            # convert img to 1d list
            cols = img.shape[1]
            rows = img.shape[0]
            data = list(img.flat)
            return wormvision.WBFE_evaluate(data, cols, rows, target)
        else:
            if benchmarking:
                self.debug = False # dont show live images when benchmarking
            # use opencv library and show live images
            if self.debug:
                # Make a copy when debug mode is on so that we can overlay the results on it later.
                original = img.copy()

            # todo convert to grayscale

            # Gaussian filter
            blur_kernelsize = (25, 25)  # todo scale this with resolution
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
            # NOTE: img is a float image at this point
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
            open_kernelsize = (49, 49)  # todo scale this with resolution
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
            offset = None
            if best_match is None:
                self.centroid = None
                if self.debug:
                    cv2.imshow('Result', original)
            else:
                M = cv2.moments(best_match)
                cX = int(M["m10"] / M["m00"])
                cY = int(M["m01"] / M["m00"])
                centroid = (cX, cY)
                offset = tuple(np.subtract(target, centroid))
                self.centroid = centroid
                if self.debug:
                    # Overlay results on source image and display them
                    cv2.circle(original, target, 5, 0, 1)
                    cv2.circle(original, centroid, 4, 0, 5)
                    cv2.imshow('Result', original)
            if benchmarking:
                self.debug = True
            return offset


if __name__ == '__main__':
    # Test WellBottomFeaturesEvaluator with a test image
    #imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_2_downscaled.png'
    #imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_6_downscaled.png'
    imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\all_downscaled\\manualControl_v0.2.py_1538674924133_downscaled.png'

    # benchmark opencv vs c
    runtimes_c = []
    runtimes_opencv = []

    x = WellBottomFeaturesEvaluator(False)
    for _ in range(10):

        start = timeit.default_timer()
        print(x.evaluate(cv2.imread(imgpath, cv2.CV_8UC1), (227, 144)))
        stop = timeit.default_timer()
        print(f'Time C ({_+1}): {stop-start}')
        runtimes_c.append(stop-start)

    x.debug = True
    for _ in range(10):

        start = timeit.default_timer()
        print(x.evaluate(cv2.imread(imgpath, cv2.CV_8UC1), (227, 144), True))
        stop = timeit.default_timer()
        print(f'Time opencv ({_+1}): {stop-start}')
        runtimes_opencv.append(stop-start)

    print('avg time c: ', sum(runtimes_c) / len(runtimes_c))
    print('avg time opencv: ', sum(runtimes_opencv) / len(runtimes_opencv))

    """ benchmark results avg over 10 attempts (on my home pc):
        opencv: 0.023s
        c: 1.2s
        c (no gaussian blur): 0.98s (->gaussian blur takes ~0.22s), note: no blob found so no centroid/return value either
        c (no contrast stretch): 1.2s (->does not contribute significantly to runtime)
        c (no gamma): 1.13s (->gamma takes ~0.07s)
        c (no threshold): 1.14s (->otsu takes ~0.06s)
        c (no morph): 0.24s (->morphology takes ~0.96s), note: no blob found so no centroid/return value either
        c (no label/features/classify): 1.2s (->does not contribute significantly to runtime)
        c (no centroid finding): 1.2s (->does not contribute significantly to runtime)
        
        conclusion: using opencv very much preferred, already super optimized in python.
                    could probably speed c algorithm up significantly to <0.2s at the very least, but is it worth the effort?
                    doubt 0.02 is attainable in the given timespan
    """

    cv2.waitKey(0)
