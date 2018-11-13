RPI_INFO = "dummy"
RPI_REVISION = "dummy"

BOARD = 0
BCM = 2


def setmode(d):
    pass


def getmode():
    return BCM


def setwarnings(b):
    pass

IN = 0
OUT = 2
HIGH = 10
LOW = 11
PUD_DOWN = 0
RISING = 1
FALLING = 2
PUD_UP=1

def setup(d, *args, **kwargs):
    pass


def input(c):
    pass


def output(c, v):
    pass


def cleanup(*args):
    pass


def add_event_detect(*args, **kwargs):
    pass


def remove_event_detect(channel):
    pass


def PWM(*args, **kwargs):
    pass
