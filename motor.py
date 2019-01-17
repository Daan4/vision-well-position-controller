#!/usr/bin/python3
# -*- coding: utf-8 -*-
"""
PyQT GUI for manual control of WormPlateScanner EPS_v2 (A4988 Stepper Motor Driver)
"""
import os
import sys
import time
from datetime import datetime
import numpy as np
import cv2
from PyQt5.QtCore import Qt, QObject, QThread, QTimer, pyqtSignal, pyqtSlot, QSettings
from PyQt5.QtWidgets import QWidget, QDesktopWidget, QApplication, QGridLayout, QHBoxLayout, QVBoxLayout
from PyQt5.QtWidgets import QLabel, QSpacerItem, QSizePolicy, QPushButton, QComboBox, QCheckBox, QTextEdit, QDoubleSpinBox, QSpinBox
from PyQt5.QtGui import QCloseEvent, QPixmap, QImage
from picamera import PiCamera
from picamera.array import PiRGBArray, PiYUVArray
import pigpio
import threading


class Stepper(QObject):
# A4988 Stepper Motor Driver Carriers
## inspired by https://github.com/laurb9/StepperDriver (https://github.com/laurb9/StepperDriver/blob/master/src/BasicStepperDriver.cpp)
# todo: make sure the standard direction drives the load towards the end-stop
# implement MS_pins
# do something with the limit switch: interrupt + call-back
# make homing function
    sigMsg = pyqtSignal(str)  # Message signal
    pio = None # Reference to PigPIO object
    motor_type = "A4988"
    NEN_pin  = None # not Enable
    DIR_pin = None
    STP_pin = None
    LIM_pin = None
    MS_pins = (None, None, None)
    running = False

    def __init__(self, pio, mm_per_step, NEN_pin=14, DIR_pin=15, STP_pin=18, LIM_pin=0, MS_pins=(1,2,3)):
        super().__init__()
        if not isinstance(pio, pigpio.pi):
            raise TypeError("Constructor attribute is not a pigpio.pi instance!")
        if not(0<=NEN_pin<=26) or not(0<=DIR_pin<=26) or not (0<=STP_pin<=26) or not (0<=LIM_pin<=26):
            raise Error("Constructor attributes are not GPIO pin numbers!")
        self.NEN_pin  = NEN_pin
        self.DIR_pin = DIR_pin
        self.STP_pin = STP_pin
        self.LIM_pin = LIM_pin
        self.MS_pins = MS_pins
        self.mm_per_step = mm_per_step
        self.pio = pio
        self.pio.set_mode(self.NEN_pin, pigpio.OUTPUT)
        self.pio.write(self.NEN_pin, True) # inverse logic
        self.pio.set_mode(self.DIR_pin, pigpio.OUTPUT)
        self.pio.write(self.DIR_pin, False)
##        for i in self.MS_pins:
##            self.pio.set_mode(i, pigpio.OUTPUT)
        self.pio.set_mode(self.STP_pin, pigpio.OUTPUT)
        self.pio.write(self.STP_pin, False)
        self.cbc = self.pio.callback(self.STP_pin)  # to create a simple "callback counter" on the pin where the PWM pulses are sent
        
    @pyqtSlot(float)
    def go(self, clockwise=False, steptype="Full"):
        try:
            if not(self.pio is None):
                if not self.running:
                    self.sigMsg.emit(self.__class__.__name__ + ": go on pins (" + str(self.NEN_pin)+","+ str(self.DIR_pin)+","+str(self.STP_pin)+")")
                    self.running = True
                    self.setResolution(steptype)
                    self.pio.write(self.NEN_pin, False) # inverse logic
                    self.pio.write(self.DIR_pin, clockwise) # DIR pin is sampled on rising STEP edge, so it is set first
                    N = 20
                    f = 1000
                    for i in range(1,N):  # ramp up frequency
                        self.pio.write(self.STP_pin, True)
                        time.sleep(N*3/(4*f*i))
                        self.pio.write(self.STP_pin, False)
                        time.sleep(N/(4*f*i))                       
                    self.pio.set_PWM_frequency(self.STP_pin, f)
                    self.pio.set_PWM_dutycycle(self.STP_pin, 192) # PWM 3/4 on
        except Exception as err:
            raise ValueError(self.__class__.__name__ + ": ValueError")
            
    @pyqtSlot(float)
    def go_once(self, steps=None, mm=None, clockwise=False, steptype="Full"):
        # Similar to self.go, except self.go_once creates a pulse wave with num_steps pulses
        # and the pulse wave is sent to the motor only once.
        # this function blocks until the full pulse wave has been consumed
        
        if steps is None and mm is None or steps is not None and mm is not None:
            raise ValueError("Either the steps or mm parameter needs to be supplied, but not both.")
        if mm is not None:
            steps = int(mm / self.mm_per_step + 0.5)  # 15 mm per step, moving 50 mm
        
        if self.pio is not None and not self.running:
            self.sigMsg.emit(self.__class__.__name__ + ": go on pins (" + str(self.NEN_pin)+","+ str(self.DIR_pin)+","+str(self.STP_pin)+")")
            self.running = True
            self.setResolution(steptype)
            self.pio.write(self.NEN_pin, False) # inverse logic
            self.pio.write(self.DIR_pin, clockwise) # DIR pin is sampled on rising STEP edge, so it is set first
            self.cbc.reset_tally()
            self.generate_ramp([
                                [7,  0],
                                [20, 0],
                                [57, 1],
                                [154,1],
                                [354,1],
                                [622,1],
                                [832,1],
                                [937,1],
                                [978,1],
                                [993,1]
                                ]) # sigmoid ramp up
            self.pio.set_PWM_frequency(self.STP_pin, 1000)
            self.pio.set_PWM_dutycycle(self.STP_pin, 128) # PWM 3/4 on
            
            while self.cbc.tally() < steps - 10:
                time.sleep(0.001)
                
            self.stop()
                             
    def cbf(self, gpio, level, tick):
       print(gpio, level, tick)
       self.stop()
       self.sigMsg.emit(self.__name__ + ": home!")
##               

    @pyqtSlot(float)
    def home(self, clockwise=False, steptype="Full"):
##        cb1 = self.pio.callback(user_gpio=self.LIM_pin, func=self.cbf)
        self.go(clockwise=clockwise, steps=int(1e4)) ## Run until limit switch is hit

    
    @pyqtSlot()
    def stop(self):
        if self.running:
            self.sigMsg.emit(self.__class__.__name__ + ": stop on pins (" + str(self.NEN_pin)+","+ str(self.DIR_pin)+","+str(self.STP_pin)+")")
            try:
                if not(self.pio is None):
                    self.pio.set_PWM_frequency(self.STP_pin,0)
                    self.generate_ramp([[993,1],
                                        [979,1],
                                        [937,1],
                                        [832,1],
                                        [622,1],
                                        [354,1],
                                        [154,1],
                                        [57, 1],
                                        [20, 0],
                                        [7,  0]
                                        ]) # sigmoid rampdown
                    self.pio.write(self.NEN_pin, True) # inverse logic
                    self.running = False            
                    self.sigMsg.emit(self.__class__.__name__ + ": Current number of steps is " + str(self.cbc.tally())) # to display number of pulses made                    
            except Exception as err:
                self.sigMsg.emit(self.__class__.__name__ + ": Error")

    def setResolution(self, steptype):
        """ method to calculate step resolution
        based on motor type and steptype"""
        if self.motor_type == "A4988":
            resolution = {'Full': (0, 0, 0),
                          'Half': (1, 0, 0),
                          '1/4': (0, 1, 0),
                          '1/8': (1, 1, 0),
                          '1/16': (1, 1, 1)}
        elif self.motor_type == "DRV8825":
            resolution = {'Full': (0, 0, 0),
                          'Half': (1, 0, 0),
                          '1/4': (0, 1, 0),
                          '1/8': (1, 1, 0),
                          '1/16': (0, 0, 1),
                          '1/32': (1, 0, 1)}
        else: 
            raise ValueError("Error invalid motor_type: {}".format(steptype))
##        self.pio.write(self.MS_pins, resolution[steptype])

    def generate_ramp(self, ramp):
            """Generate ramp wave forms.
            ramp:  List of [Frequency, Steps]
            """
            self.pio.wave_clear()     # clear existing waves
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
            while self.pio.wave_tx_busy(): # While transmitting.
                time.sleep(0.1)
            # delete all waves
            for i in range(length):
                self.pio.wave_delete(wid[i])
