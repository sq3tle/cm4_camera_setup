import random
import string
import sys
import time

from picamera2 import Picamera2
from picamera2.encoders import H264Encoder
from picamera2.outputs import Output

camera = int(sys.argv[1]) if sys.argv[1] else 0

path = "/home/pi/camera{}/".format(camera)
session_id = (''.join(random.choice(string.ascii_letters + string.digits) for i in range(8))).upper()
video_config = {"size": (1920, 1080)}

print("camera:", camera)
print("session_id:", session_id)


class SplitOutput(Output):

    def __init__(self, filename, fps=30):
        super().__init__()
        self._cfile = None
        self._filename = filename
        self._fps = fps
        self._frame = 0
        self._number = 0
        self._last_frame_time = time.time()

    def running(self):
        return time.time() - self._last_frame_time < 1

    def outputframe(self, frame, keyframe=True, timestamp=None):

        self._last_frame_time = time.time()

        if ((self._frame > self._fps * 10 and keyframe) or not self._cfile):

            self._number += 1
            self._frame = 0

            if self._cfile:
                self._cfile.close()

            self._cfile = open("{}rec-{}_{}.h264".format(path, self._filename, str(self._number).zfill(6)), "ab")

        self._cfile.write(frame)

        self._frame += 1


# CAMERA SETUP
try:
    cam = Picamera2(camera)
except:
    print("CATASTROPHIC FAILURE - NO CAMERA")
    import RPi.GPIO as GPIO
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(16, GPIO.OUT)
    exit(10)
       
cam.configure(cam.create_video_configuration(video_config))
cam.encoder = H264Encoder(4000000, repeat=True)

cam.encoder.output = SplitOutput(session_id)
cam.start_encoder()
cam.start()
while 1:
    if not cam.encoder.output.running():
        print("CATASTROPHIC FAILURE - TIMEOUT")
        import RPi.GPIO as GPIO
        GPIO.setmode(GPIO.BCM)
        GPIO.setup(16, GPIO.OUT)
        exit(10)
    time.sleep(1)

cam.stop()
cam.stop_encoder()
