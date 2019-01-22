import time
from PyQt5.QtCore import QObject, pyqtSignal, pyqtSlot
import pigpio


class Stepper(QObject):
    # A4988 Stepper Motor Driver Carriers
    # inspired by https://github.com/laurb9/StepperDriver (https://github.com/laurb9/StepperDriver/blob/master/src/BasicStepperDriver.cpp)
    # todo: make sure the standard direction drives the load towards the end-stop
    # implement MS_pins
    # do something with the limit switch: interrupt + call-back
    # make homing function
    sig_msg = pyqtSignal(str)  # message signal
    def __init__(self, pio, mm_per_step, NEN_pin=14, DIR_pin=15, STP_pin=18, LIM_pin=0, MS_pins=(1, 2, 3), motor_type="A4988"):
        """

        Args:
            pio: pigpio instance
            mm_per_step: how far the load moves with each motor step in mm
            NEN_pin: enable pin
            DIR_pin: direction pin
            STP_pin: step ping
            LIM_pin: limit switch pin
            MS_pins: ???
        """
        super().__init__()
        if not isinstance(pio, pigpio.pi):
            raise TypeError("Constructor attribute is not a pigpio.pi instance!")
        if not (0 <= NEN_pin <= 26) or not (0 <= DIR_pin <= 26) or not (0 <= STP_pin <= 26) or not (0 <= LIM_pin <= 26):
            raise ValueError("Constructor attributes are not GPIO pin numbers!")
        self.NEN_pin = NEN_pin
        self.DIR_pin = DIR_pin
        self.STP_pin = STP_pin
        self.LIM_pin = LIM_pin
        self.MS_pins = MS_pins
        self.mm_per_step = mm_per_step
        self.pio = pio
        self.pio.set_mode(self.NEN_pin, pigpio.OUTPUT)
        self.pio.write(self.NEN_pin, True)  # inverse logic
        self.pio.set_mode(self.DIR_pin, pigpio.OUTPUT)
        self.pio.write(self.DIR_pin, False)
        self.pio.set_mode(self.STP_pin, pigpio.OUTPUT)
        self.pio.write(self.STP_pin, False)
        self.cbc = self.pio.callback(
            self.STP_pin)  # to create a simple "callback counter" on the pin where the PWM pulses are sent
        self.running = False
        self.motor_type = motor_type

    @pyqtSlot(float)
    def go(self, clockwise=False, steptype="Full"):
        """Moves the motor in a given direction until self.stop is called.

        Args:
            clockwise: Motor direction
            steptype: Not implemented
        """
        try:
            if not (self.pio is None):
                if not self.running:
                    self.sig_msg.emit(self.__class__.__name__ + ": go on pins (" + str(self.NEN_pin) + "," + str(
                        self.DIR_pin) + "," + str(self.STP_pin) + ")")
                    self.running = True
                    self.pio.write(self.NEN_pin, False)  # inverse logic
                    self.pio.write(self.DIR_pin,
                                   clockwise)  # DIR pin is sampled on rising STEP edge, so it is set first
                    N = 20
                    f = 1000
                    for i in range(1, N):  # ramp up frequency
                        self.pio.write(self.STP_pin, True)
                        time.sleep(N * 3 / (4 * f * i))
                        self.pio.write(self.STP_pin, False)
                        time.sleep(N / (4 * f * i))
                    self.pio.set_PWM_frequency(self.STP_pin, f)
                    self.pio.set_PWM_dutycycle(self.STP_pin, 192)  # PWM 3/4 on
        except Exception as err:
            raise ValueError(self.__class__.__name__ + ": ValueError " + str(err))

    @pyqtSlot(float)
    def go_once(self, steps=None, mm=None, clockwise=False, steptype="Full"):
        """Moves the motor a given distance in a given direction. Blocks until the movement is completed.
        Either steps or mm has to be passed, but not both.

        Args:
            steps: The number of steps to move
            mm: The distance in mm to move
            clockwise: Motor direction
            steptype: Not implemented
        """
        if steps is None and mm is None or steps is not None and mm is not None:
            raise ValueError("Either the steps or mm parameter needs to be supplied, but not both.")
        if mm is not None:
            steps = int(mm / self.mm_per_step + 0.5)

        if self.pio is not None and not self.running:
            self.sig_msg.emit(
                self.__class__.__name__ + ": go on pins (" + str(self.NEN_pin) + "," + str(self.DIR_pin) + "," + str(
                    self.STP_pin) + ")")
            self.running = True
            self.pio.write(self.NEN_pin, False)  # inverse logic
            self.pio.write(self.DIR_pin, clockwise)  # DIR pin is sampled on rising STEP edge, so it is set first
            self.cbc.reset_tally()  # Reset the pulse counter
            self.generate_ramp([
                [7, 0],
                [20, 0],
                [57, 1],
                [154, 1],
                [354, 1],
                [622, 1],
                [832, 1],
                [937, 1],
                [978, 1],
                [993, 1]
            ])  # sigmoid ramp up
            self.pio.set_PWM_frequency(self.STP_pin, 1000)
            self.pio.set_PWM_dutycycle(self.STP_pin, 128)  # PWM 3/4 on

            # Count pulses until the movement is completed. Each pulse adds two to the tally, so tally() is halved.
            while int(self.cbc.tally() / 2 + 0.5) < steps - 10:
                time.sleep(0.001)

            self.stop()

    @pyqtSlot()
    def stop(self):
        """
        Stops the motor movement by ramping down the pwm frequency.
        """
        if self.running:
            self.sig_msg.emit(
                self.__class__.__name__ + ": stop on pins (" + str(self.NEN_pin) + "," + str(self.DIR_pin) + "," +
                                        str(self.STP_pin) + ")")
            try:
                if not (self.pio is None):
                    self.pio.set_PWM_frequency(self.STP_pin, 0)
                    self.generate_ramp([[993, 1],
                                        [979, 1],
                                        [937, 1],
                                        [832, 1],
                                        [622, 1],
                                        [354, 1],
                                        [154, 1],
                                        [57, 1],
                                        [20, 0],
                                        [7, 0]
                                        ])  # sigmoid rampdown
                    self.pio.write(self.NEN_pin, True)  # inverse logic
                    self.running = False
                    self.sig_msg.emit(self.__class__.__name__ + ": Current number of steps is " +
                                      str(self.cbc.tally()))  # to display number of pulses made
            except Exception as err:
                self.sig_msg.emit(self.__class__.__name__ + ": Error " + str(err))

    def generate_ramp(self, ramp):
        """Generate ramp wave forms.
        ramp:  List of [Frequency, Steps]
        """
        self.pio.wave_clear()  # clear existing waves
        length = len(ramp)  # number of ramp levels
        wid = [-1] * length

        # Generate a wave per ramp level
        for i in range(length):
            frequency = ramp[i][0]
            micros = int(500000 / frequency)
            wf = []
            wf.append(pigpio.pulse(1 << self.STP_pin, 0, micros))  # pulse on
            wf.append(pigpio.pulse(0, 1 << self.STP_pin, micros))  # pulse off
            self.pio.wave_add_generic(wf)
            wid[i] = self.pio.wave_create()

        # Generate a chain of waves
        chain = []
        for i in range(length):
            steps = ramp[i][1]
            x = steps & 255
            y = steps >> 8
            chain += [255, 0, wid[i], 255, 1, x, y]

        self.pio.wave_chain(chain)  # Transmit chain.
        # Is this required?
        while self.pio.wave_tx_busy():  # While transmitting.
            time.sleep(0.1)
        # delete all waves
        for i in range(length):
            self.pio.wave_delete(wid[i])
