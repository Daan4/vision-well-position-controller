from abc import abstractmethod, ABC
import cv2
import numpy as np
import wormvision
import timeit
import sys


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


class HoughTransformEvaluator(WellPositionEvaluator):
    def __init__(self, resolution, debug=False):
        super().__init__(debug)
        # Set up debug windows if debug mode is on
        self.debug = debug
        self.centroid = None
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
            self.centroid = None
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
            self.centroid = (circles[0, 0, 0], circles[0, 0, 1])
            offset = tuple(np.subtract(target, self.centroid))
        return offset


class WellBottomFeaturesEvaluator(WellPositionEvaluator):
    def __init__(self, resolution, debug=False):
        super().__init__(debug)
        # Set up debug windows if debug mode is on
        self.debug = debug
        self.centroid = None
        self.img_width, self.img_height = resolution
        if self.debug:
            cv2.namedWindow('Blur', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Blur', int(self.img_width/2+0.5), int(self.img_height/2+0.5))
            cv2.moveWindow('Blur', 50, 100)

            cv2.namedWindow('Gamma', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Gamma', int(self.img_width/2+0.5), int(self.img_height/2+0.5))
            cv2.moveWindow('Gamma', 460, 100)

            cv2.namedWindow('Threshold', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Threshold', int(self.img_width/2+0.5), int(self.img_height/2+0.5))
            cv2.moveWindow('Threshold', 870, 100)

            cv2.namedWindow('Scores', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Scores', int(self.img_width/2+0.5), int(self.img_height/2+0.5))
            cv2.moveWindow('Scores', 1280, 100)

            # cv2.namedWindow('Morphology', cv2.WINDOW_NORMAL)
            # cv2.resizeWindow('Morphology', self.img_width, self.img_height)
            # cv2.moveWindow('Morphology', 1280, 100)

            cv2.namedWindow('Result', cv2.WINDOW_NORMAL)
            cv2.resizeWindow('Result', int(self.img_width/2+0.5), int(self.img_height/2+0.5))
            cv2.moveWindow('Result', 50, 500)

        # Evaluation function parameters, scaled by image width if needed
        # parameters were originally determined for resolution of 410x308
        # blur
        self.blur_kernelsize = (25, 25)  # Has to be a square (for c implementation)
        #self.blur_kernelsize = tuple(np.multiply(self.blur_kernelsize, self.img_width / 410 + 0.5).astype(int))
        self.blur_sigma = 1

        # manual threshold
        self.threshold = 30

        # gamma
        self.c = 0.5
        self.gamma = 8

        # morphology
        self.open_kernelsize = (10, 10)  # Has to be a square (for c implementation)
        self.close_kernelsize = (10, 10)

        # classification
        #self.metric_threshold = 0.8
        self.area_threshold = 5000

    def evaluate(self, img, target=(0, 0), benchmarking=False):
        """ Finds the position error by finding the well bottom centroid.
        If self.debug = True, opencv is used instead of the c library

        Args:
            img: 2d grayscale image list
            target: target coordinates (topleft pixel is 0,0)
            benchmarking: set to true to hide windows when using opencv

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
            if benchmarking:
                self.debug = False # dont show live images when benchmarking
            # use opencv library and show live images
            if self.debug:
                # Make a copy when debug mode is on so that we can overlay the results on it later.
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

            # Otsu threshold
            #img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            # For some reason img is not 8uc1 before, even though i cast to uint8 and cvt to gray initially???
            #_, img = cv2.threshold(img, 0, 255, cv2.THRESH_OTSU)
            #if self.debug:
                #cv2.imshow('Threshold', img)

            # manual threshold
            _, img = cv2.threshold(img, self.threshold, 255, cv2.THRESH_BINARY)
            # close to fill small holes
            kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, self.close_kernelsize)
            img = cv2.morphologyEx(img, cv2.MORPH_OPEN, kernel)
            if self.debug:
                cv2.imshow('Threshold', img)

            # Morphology
            # kernel = cv2.getStructuringElement(cv2.MORPH_ELLIPSE, self.open_kernelsize)
            # img = cv2.morphologyEx(img, cv2.MORPH_OPEN, kernel)
            # if self.debug:
            #     cv2.imshow('Morphology', img)

            # # Keep only the well bottom region
            # # By looking at region features
            # # step 1: Keep only blobs that score above roundness threshold
            # # step 2: Keep largest blob remaining after step 1
            # largest_area_above_threshold = -1
            # best_match = None
            # im2, contours, hierarchy = cv2.findContours(img, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
            # for c in contours:
            #     area = cv2.contourArea(c)
            #     perimeter = cv2.arcLength(c, True)
            #     metric = 4 * np.pi * area / perimeter ** 2
            #     if metric > self.metric_threshold and area > largest_area_above_threshold:
            #         largest_area_above_threshold = area
            #         best_match = c

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

                # Calculate eccentricity from central moments as such:
                # e = ((mu20 - mu02)^2 - 4*mu11^2)/(mu20+mu02)^2
                m = cv2.moments(c)
                # todo: eccentricity is supposed to be between 0 and 1, 0 being a perfect circle
                # // same
                # formula, different
                # results? both in opencv and c?
                # // https: // books.google.nl / books?id = qUeecNvfn0oC & pg = PA522 & lpg = PA522 & dq = eccentricity + moments + range & source = bl & ots = IQ3ankJQYI & sig = ACfU3U2ZywI0Ed557Wi8xd65cPViCwniQA & hl = en & sa = X & ved = 2
                # ahUKEwiQ_PaNxP3fAhVFJ1AKHVhdAVcQ6AEwC3oECAYQAQ  # v=onepage&q=eccentricity%20moments%20range&f=false
                # // http: // breckon.eu / toby / teaching / dip / opencv / SimpleImageAnalysisbyMoments.pdf

                # but for some reason it seems to be higher is better with random bounds? (but around -2...1 usually?)
                eccentricity = ((m['nu20'] - m['nu02']) ** 2 - 4 * m['nu11'] ** 2) / (m['nu20'] + m['nu02']) ** 2
                score = (1-roundness + eccentricity) / 2

                print("{} score {} roundness {} eccentricity {}".format(i, score, roundness, eccentricity))

                if self.debug:
                    # Overlay scores on image in debug mode
                    cX = int(m["m10"] / m["m00"] + 0.5)
                    cY = int(m["m01"] / m["m00"] + 0.5)
                    cv2.putText(im3, '{0:.3f}'.format(score), (cX, cY), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 1, cv2.LINE_AA)
                    cv2.imshow('Scores', im3)

                if score < best_score:
                    best_score = score
                    best_match = c

            # calculate centroid for best match blob
            offset = None
            if best_match is None:
                self.centroid = None
                if self.debug:
                    cv2.imshow('Result', original)
            else:
                M = cv2.moments(best_match)
                cX = int(M["m10"] / M["m00"] + 0.5)
                cY = int(M["m01"] / M["m00"] + 0.5)
                self.centroid = (cX, cY)
                offset = tuple(np.subtract(self.centroid, target))
                if self.debug:
                    # Overlay results on source image and display them
                    cv2.circle(original, target, 5, 0, 1)
                    cv2.circle(original, self.centroid, 4, 0, 5)
                    cv2.imshow('Result', cv2.resize(original, (int(self.img_width/2+0.5), int(self.img_height/2+0.5))))
            if benchmarking:
                self.debug = True


            return offset


def test_wellbottomfeaturesevaluator():
    benchmarking = False

    if benchmarking:
        # Test WellBottomFeaturesEvaluator with a test image
        # imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_2_downscaled.png'
        # imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\test set 1\\downscaled\\1_6_downscaled.png'
        imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\all_downscaled\\manualControl_v0.2.py_1538674924133_downscaled.png'
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
        # imgpath = 'C:\\Users\\Daan\\Documents\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\all_downscaled\\manualControl_v0.2.py_1538674924133_downscaled.png'
        imgpath = 'D:\\Libraries\\Documents\\svn\\EVD_PROJ\\99-0. Overig\\05. Images of C. Elegans (11-10-2018)\\all_downscaled\\manualControl_v0.2.py_1538674924133_downscaled.png'

        x = WellBottomFeaturesEvaluator((410, 308), True)
        img = cv2.imread(imgpath, cv2.CV_8UC1)
        print('opencv: {}'.format(x.evaluate(img, (227, 144), False)))

        x.debug = False
        print('c: {}'.format(x.evaluate(img, (227, 144), False)))
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
