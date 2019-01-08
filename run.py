from well_position_controller import WellPositionController
from well_position_evaluators import WellBottomFeaturesEvaluator, HoughTransformEvaluator
from camera import PiVideoStream

# Run this file to start
if __name__ == '__main__':
    resolution = (640, 480)
    # Set up camera video stream
    vs = PiVideoStream(resolution=resolution, effect='denoise')
    vs.start()

    # Set up well position controller and evaluators
    setpoints_csv_file = "setpoints\\24.csv"
    target_coordinates = (0, 0)  # to be determined
    e = ((WellBottomFeaturesEvaluator(resolution, True), 1),
         (HoughTransformEvaluator(resolution, True), 1))
    wpc = WellPositionController(setpoints_csv_file, (16, 16), e, target_coordinates=target_coordinates)
    wpc.start()