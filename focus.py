from picamera2 import Picamera2
import time
picam2 = Picamera2()
camera_config = picam2.create_still_configuration()
picam2.configure(camera_config)
picam2.start()

for x in range(2*4,4*4):
  picam2.set_controls({"AfMode": 0, "LensPosition": x/4.0})
  print(picam2.capture_metadata()['LensPosition'])
  picam2.capture_file("test_{}.jpg".format(x))
