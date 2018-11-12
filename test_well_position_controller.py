from well_position_controller import *


def create_random_error(max_error, stepper_motor):
    """
        Introduce a random error to the positioning of the given motor.
        Useful for testing purposes to see if the WellPositionController can correct the error introduced by this function.

    Args:
        max_error: max error value (in steps) (in 1 direction, so total error band will be 2*max_error)
        stepper_motor: StepperMotor class instance to control the motor position
    """
    pass


def create_error(error, direction, stepper_motor):
    """
        Similar to create_random_error, except this function creates an error based on the input.

    Args:
        error: error value (in steps)
        direction: motor direction (True or False)
        stepper_motor: StepperMotor class instance to control the motor position
    """
    pass


class CameraMock:
    """Mock class for testing"""
    pass


class StepperMotorMock:
    """Mock class for testing"""
    pass


if __name__ == '__main__':
    pass
