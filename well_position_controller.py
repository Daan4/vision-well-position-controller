from abc import abstractmethod, ABC


class WellPositionEvaluator(ABC):
    def __init__(self):
        pass

    @abstractmethod
    def evaluate(self, img, debug_mode=False):
        """
            Vision algorithm implementation of the evaluator.

        Args:
            img: 2d Image matrix to evaluate
            debug_mode: Set to True to display live visual feedback for debugging purposes

        Returns:
            A vector (x, y) that represents the found offset between the well center and diaphragm center.
            Should return None if no vector is found.
        """
        pass


class WellBottomFeaturesEvaluator(WellPositionEvaluator):
    """ Scores an image based on illuminated well bottom features. """
    def evaluate(self, img, debug_mode=False):
        return None


class HoughTransformEvaluator(WellPositionEvaluator):
    """ Scores an image based on the circle center found by using the Hough circle transform. """
    def evaluate(self, img, debug_mode=False):
        return None


class SymmetryEvaluator(WellPositionEvaluator):
    """ Scores an image based on symmetry above the diaphragm. """
    def evaluate(self, img, debug_mode=False):
        return None


class WellPositionController:
    """
    Controls well positioning in the following manner:
    1) Move to the next well coordinates using preset setpoints (feedforward)
    2) Take image
    3) Evaluate well position from image
    4) Adjust well positioning if its wrong using a feedback control loop by repeating steps 2-4 until the image is scored as correct

    This class supports multiple scoring algorithms that are weighted differently
    """
    def __init__(self, setpoints, *evaluators):
        """

        Args:
            setpoints: List of tuples, each tuple of the format (x, y)
            *evaluators: List of tuples, each tuple of the format (WellPositionEvaluator, score_weight)
        """
        pass
