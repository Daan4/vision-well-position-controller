from abc import abstractmethod
import cv2
import numpy as np
import wormvision
import timeit
from PyQt5.QtCore import pyqtSignal, QObject


class WellPositionEvaluator(QObject):
    @abstractmethod
    def __init__(self):
        super().__init__()

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


class HoughTransformEvaluator(WellPositionEvaluator):
    def __init__(self, resolution, debug=False):
        super().__init__()
        # Set up debug windows if debug mode is on
        self.debug = debug
        self.img_width, self.img_height = resolution
        if self.debug:
            cv2.namedWindow('Blur', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Blur', self.img_width, self.img_height)
            cv2.moveWindow('Blur', 50, 100)

            cv2.namedWindow('Gamma', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Gamma', self.img_width, self.img_height)
            cv2.moveWindow('Gamma', 460, 100)

            cv2.namedWindow('Result', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Result', self.img_width, self.img_height)
            cv2.moveWindow('Result', 1280, 100)

        # Evaluation function parameters, scaled by image width if needed
        # parameters were originally determined for resolution of 410x308
        # blur
        self.blur_kernelsize = (25, 25)
        self.blur_kernelsize = tuple(np.multiply(self.blur_kernelsize, self.img_width / 410))
        self.blur_sigma = 100

        # gamma
        self.c = 1
        self.gamma = 5

        # hough
        self.min_radius = 50
        self.min_radius *= self.img_width / 410
        self.max_radius = 100
        self.max_radius *= self.img_width / 410
        self.min_distance = 50
        self.min_distance *= self.img_width / 410
        self.param1 = 25  # = higher threshold passed to canny, lower threshold is half of this
        self.param2 = 50  # = accumulator threshold -> smaller might result in more (and smaller) circles

    def evaluate(self, img, target):
        """ Finds the position error by using the Hough transform function in opencv

        Args:
            img: 2d grayscale image list
            target: target coordinates (topleft pixel is 0,0)
        """
        if self.debug:
            # make a copy when debug mode is on, so that we can overlay the results on the original image
            original = img.copy()

        # Gaussian filter
        img = cv2.GaussianBlur(img, self.blur_kernelsize, self.blur_sigma)
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
        img = np.power(img, self.gamma)
        img = np.multiply(img, self.c * 255)
        np.putmask(img, img > 255, 255)
        img = img.astype(np.uint8)
        if self.debug:
            cv2.imshow('Gamma', img)

        # Hough transform
        circles = cv2.HoughCircles(img, cv2.HOUGH_GRADIENT, 1, self.min_distance, param1=self.param1,
                                   param2=self.param2, minRadius=self.min_radius, maxRadius=self.max_radius)

        # Calculate and return the offset
        # If 0 circles are found return none
        # If more than 1 circle is found, either assume the best match or return none (todo which is best?)
        offset = None
        if circles is None:
            if self.debug:
                cv2.imshow('Result', original)
        else:
            if self.debug:
                # Display found circles
                for i in circles[0, :]:
                    # outer circle
                    cv2.circle(original, (i[0], i[1]), i[2], (0, 255, 0), 2)
                    # center
                    cv2.circle(original, (i[0], i[1]), 2, (0, 0, 255), 3)
                    cv2.imshow('Result', original)
            # assume circle at index 0 is the best match
            centroid = (circles[0, 0, 0], circles[0, 0, 1])
            offset = tuple(np.subtract(target, centroid))
        return offset


class WellBottomFeaturesEvaluator(WellPositionEvaluator):
    update_blur = pyqtSignal(np.ndarray)
    update_gamma = pyqtSignal(np.ndarray)
    update_threshold = pyqtSignal(np.ndarray)
    update_scores = pyqtSignal(np.ndarray)
    update_result = pyqtSignal(np.ndarray)
    
    def __init__(self, resolution, debug=False):
        super().__init__()
        self.debug = debug
        self.img_width, self.img_height = resolution

        # blur
        self.blur_kernelsize = (25, 25)  # Has to be a square (for c implementation)
        self.blur_sigma = 1

        # manual threshold
        self.threshold = 20

        # gamma
        self.c = 0.5
        self.gamma = 8

        # morphology
        self.open_kernelsize = (10, 10)  # Has to be a square (for c implementation)
        self.close_kernelsize = (10, 10)
        # classification
        self.area_threshold = 5000

    def evaluate(self, img, target=(0, 0)):
        """ Finds the position error by finding the well bottom centroid.
        If self.debug = True, opencv is used instead of the c library

        Args:
            img: 2d grayscale image list
            target: target coordinates (topleft pixel is 0,0)

        Returns: offset tuple (x, y) position error
        """
        if not self.debug:
            # use custom vision library
            # convert img to 1d list
            cols = img.shape[1]
            rows = img.shape[0]
            data = list(img.flat)
            return wormvision.WBFE_evaluate(data, cols, rows, target, self.blur_kernelsize[0], self.blur_sigma, self.c, self.gamma,
                                            self.threshold, self.area_threshold)
        else:
            # use opencv library and show live images
            if self.debug:
                # Make a copy when debug mode is on so that we can overlay the results on it later.
                original = img.copy()

            # Gaussian filter
            img = cv2.GaussianBlur(img, self.blur_kernelsize, self.blur_sigma)
            if self.debug:
                self.update_blur.emit(cv2.resize(img, (int(self.img_width/2+0.5), int(self.img_height/2+0.5))))

            # Auto contrast
            _min = np.min(img)
            _max = np.max(img)
            img = np.subtract(img, _min)
            img = np.divide(img, (_max - _min))
            img = np.multiply(img, 255)
            cv2.normalize(img, img, 1, 0, cv2.NORM_MINMAX)

            # Gamma
            # NOTE: img is a float image at this point
            img = np.power(img, self.gamma)
            img = np.multiply(img, self.c * 255)
            np.putmask(img, img > 255, 255)
            img = img.astype(np.uint8)
            if self.debug:
                self.update_gamma.emit(cv2.resize(img, (int(self.img_width/2+0.5), int(self.img_height/2+0.5))))

            # manual threshold
            _, img = cv2.threshold(img, self.threshold, 255, cv2.THRESH_BINARY)
            # close to fill small holes
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, self.close_kernelsize)
            img = cv2.morphologyEx(img, cv2.MORPH_OPEN, kernel)
            if self.debug:
                self.update_threshold.emit(cv2.resize(img, (int(self.img_width/2+0.5), int(self.img_height/2+0.5))))

            # Select best match blob by looking at the mean score for
            # roundness and eccentricity, lower is better
            best_match = None
            best_score = 1000
            area_threshold = self.area_threshold
            im2, contours, hierarchy = cv2.findContours(img, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
            if self.debug:
                im3 = img.copy()  # Copy to overlay results in
                im3 = cv2.cvtColor(im3, cv2.COLOR_GRAY2BGR)
            for i, c in enumerate(contours):
                area = cv2.contourArea(c)
                if area < area_threshold:
                    continue
                perimeter = cv2.arcLength(c, True)
                roundness = 4 * np.pi * area / perimeter ** 2

                m = cv2.moments(c)
                eccentricity = ((m['nu20'] - m['nu02']) ** 2 + 4 * m['nu11'] ** 2) / (m['nu20'] + m['nu02']) ** 2
                score = (1-roundness + eccentricity) / 2

                if self.debug:
                    # Overlay scores on image in debug mode
                    cX = int(m["m10"] / m["m00"] + 0.5)
                    cY = int(m["m01"] / m["m00"] + 0.5)
                    cv2.putText(im3, '{0:.3f}'.format(score), (cX, cY), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1, cv2.LINE_AA)
                    self.update_scores.emit(cv2.resize(im3, (int(self.img_width/2+0.5), int(self.img_height/2+0.5))))

                if score < best_score:
                    best_score = score
                    best_match = c

            # calculate centroid for best match blob
            offset = None
            if best_match is None:
                if self.debug:
                    cv2.imshow('Result', original)
            else:
                M = cv2.moments(best_match)
                cX = int(M["m10"] / M["m00"] + 0.5)
                cY = int(M["m01"] / M["m00"] + 0.5)
                centroid = (cX, cY)
                offset = tuple(np.subtract(centroid, target))
                if self.debug:
                    # Overlay results on source image and display them
                    cv2.circle(original, target, 5, 0, 1)
                    cv2.circle(original, centroid, 4, 0, 5)
                    self.update_result.emit(cv2.resize(original, (int(self.img_width/2+0.5), int(self.img_height/2+0.5))))

            return offset


def test_wellbottomfeaturesevaluator():
    benchmarking = False

    if benchmarking:
        # Test WellBottomFeaturesEvaluator with a test image
        # imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_2_downscaled.png'
        # imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_6_downscaled.png'
        #imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\all_downscaled\\manualControl_v0.2.py_1538674924133_downscaled.png'
        # benchmark opencv vs c
        runtimes_c = []
        runtimes_opencv = []

        x = WellBottomFeaturesEvaluator((410, 308), False)
        for _ in range(10):
            start = timeit.default_timer()
            print(x.evaluate(cv2.imread(imgpath, cv2.CV_8UC1), (227, 144)))
            stop = timeit.default_timer()
            print('Time C ({}): {}'.format(_+1, stop-start))
            runtimes_c.append(stop - start)

        x.debug = True
        for _ in range(10):
            start = timeit.default_timer()
            print(x.evaluate(cv2.imread(imgpath, cv2.CV_8UC1), (227, 144), True))
            stop = timeit.default_timer()
            print('Time opencv ({}): {}'.format(_+1, stop-start))
            runtimes_opencv.append(stop - start)

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
    else:
        # single run to compare c vs opencv result
        imgpath = 'C:\\Users\\Daan\\Documents\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\all_downscaled\\manualControl_v0.2.py_1538675539656_downscaled.png'
        #imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\all_downscaled\\manualControl_v0.2.py_1538674924133_downscaled.png'

        x = WellBottomFeaturesEvaluator((410, 308), True)
        img = cv2.imread(imgpath, cv2.CV_8UC1)
        print('opencv: {}'.format(x.evaluate(img, (227, 144))))

        x.debug = False
        print('c: {}'.format(x.evaluate(img, (227, 144))))
        cv2.waitKey(0)
        # x.debug = False
        # print('c: {}'.format(x.evaluate(cv2.imread(imgpath, cv2.CV_8UC1), (227, 144), False)))


def test_houghtransformevaluator():
    # for this image matlab version finds circle center @ 225, 149
    # current params finds 226, 150
    imgpath = "D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\0_11_downscaled.png"
    x = HoughTransformEvaluator((410, 308), True)
    print('offset: {}'.format(x.evaluate(cv2.imread(imgpath, cv2.CV_8UC1), (227, 144))))
    print('centroid: {}'.format(x.centroid))

    cv2.waitKey(0)


if __name__ == '__main__':
    test_wellbottomfeaturesevaluator()
    #test_houghtransformevaluator()
