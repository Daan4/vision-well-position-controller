from well_position_controller import *


def create_random_error(min_error, max_error, stepper_motor_x, stepper_motor_y):
    """
        Introduce a random error to the positioning of the given motor.
        Useful for testing purposes to see if the WellPositionController can correct the error introduced by this function.
        Error is specified in 1 direction. Totale error range will be 2x the given value.

    Args:
        min_error: min error value (in steps)
        max_error: max error value (in steps)
        stepper_motor_x: StepperMotor class instance to control the well plate position in x dimension
        stepper_motor_y: StepperMotor class instance to control the well plate position in y dimension
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


if __name__ == '__main__':
    pass
