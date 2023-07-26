import random
import string
import sys
import time
import os
from concurrent.futures import ThreadPoolExecutor

from picamera2 import Picamera2
from picamera2.encoders import H264Encoder
from picamera2.outputs import Output

camera = int(sys.argv[1]) if sys.argv[1] else 0
number = 0
path = "/home/pi/camera{}/".format(camera)
session_id = (''.join(random.choice(string.ascii_letters + string.digits) for i in range(8))).upper()


print("camera:", camera)
print("session_id:", session_id)


# CAMERA SETUP
try:
    cam = Picamera2(camera)
except:
    print("CATASTROPHIC FAILURE - NO CAMERA")
    import RPi.GPIO as GPIO
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(16, GPIO.OUT)
    exit(10)
       
camera_config = cam.create_still_configuration()
cam.configure(camera_config)
cam.start()
cam.set_controls({"AfMode": 0, "LensPosition": 3.0})
time.sleep(1)

executor = ThreadPoolExecutor(max_workers=1)

while 1:
    file_name = "{}img-{}_{}.jpg".format(path, session_id, str(number).zfill(6))
    number += 1
    
    future = executor.submit(cam.capture_file, file_name)
    
    try:
        future.result(timeout=5)
    except:
        print("Operation took longer than 5 seconds. Exiting with code 10.")
        os._exit(10)

    time.sleep(5)

cam.stop()
