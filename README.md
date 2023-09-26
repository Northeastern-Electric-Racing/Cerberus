# Cerberus
> FreeRTOS Application that runs on MPU22A

## Setting up Docker Environment
> For initial installation, visit here: https://nerdocs.atlassian.net/wiki/spaces/NER/pages/108888108/Setting+Up+STM32+Dev+Env

```
# TLDR:
# Pull Container:
docker pull nwdepatie/ner-gcc-arm

# Run Container
# macOS: 
docker run --rm -it --privileged -v "$PWD:/home/app" nwdepatie/ner-gcc-arm:latest bash

# Windows:
docker run --rm -it --privileged -v "%cd%:/home/app" nwdepatie/ner-gcc-arm:latest bash
# or
docker run --rm -it --privileged -v "$(PWD):/home/app" nwdepatie/ner-gcc-arm:latest bash

# Linux:
sudo docker run --rm -it --privileged -v "$PWD:/home/app" nwdepatie/ner-gcc-arm:latest bash
```

## Container Tools and Utilities
> This container is packed with tools that can be utilized by developers to give them more insight into their developed software
```
## Tools / Utils

# to build project
make all

# to run Renode emulation
renode
i @cerberus.resc
start

# to open a serial port with Rasberry Pi Probe (make sure /dev/tty0/ACM0 exists first)
minicom -b 115200 -o -D /dev/ttyACM0

# to flash STM board with Raspberry Pi Probe (WIP)
openocd -f interface/cmsis-dap.cfg -f target/stm32f4x.cfg -c "adapter speed 5000" -c "program ./build/cerberus.elf verify reset exit"
```
### Mounting Hardware to Docker Container in Windows
> Very specific use case but nonetheless needed, also documented in the above confluence page, on macOS and Linux this happens by default when running privileged docker container

**We now have a Python script to automatically mount hardware! Run:** `python3 mount.py`

For manually mounting, follow the process below:
```
# Connect probe and open two terminals

# Terminal 1:
wsl -d ubuntu

# Terminal 2
usbipd wsl list
usbipd wsl attach --distribution=ubuntu --busid=<BUSID> # Terminal might need to be elevated to admin privileges for this

# Close the other wsl window, the device should be mounted to any WSL instance
# Start docker container
```
## Building Docker Container
>  Typically this isn't needed unless making local changes to Dockerfile, please default to the first method via pulling the docker container
```
# if need to rebuild image
sudo docker image prune -a
sudo docker build -f ./Dockerfile -t ner-gcc-arm .
```
