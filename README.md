# Smart Theater

Making my projector and (now motorised) canvas work with Siri

## Requirements
- [mklittlefs](https://github.com/earlephilhower/mklittlefs)
- [arduino-cli](https://github.com/arduino/arduino-cli)
- [esptool](https://github.com/espressif/esptool)

# Commands to run
```bash
mklittlefs -b 4096 -s 1441792 -c data/ filesystem.littlefs
esptool.py --port /dev/tty.usbserial-0001 erase_flash
esptool.py --chip esp32 --port /dev/tty.usbserial-0001 --baud 921600 write_flash 0x290000 filesystem.littlefs
```
Then upload the sketch.