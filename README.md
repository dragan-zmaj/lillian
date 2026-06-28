# CYPHAL on STM32Gx

This project is meant for exercise and learning of cyphal protocol over CAN with STM32Gxx series controller.

## The hardware:
- canable derived USB-to-CAN dongle - SLCAN firmware
- electronic speed controller - B-G431B-ESC1 with STM32G431CB MCU
- motor with hall sensor 

## The firmware for STM32G4 needs following toolchain in Windows environment:
- MotorControl Workbench
- STM32CubeMX
- STM32CubeIDE

## The environment for sending cyphal messages cyphal messages to esc via USB-to-CAN dongle following things are necessary:
### zadig-2.9.exe - install driver

### canable.io/updater/ 
	- boot switch ON
	- device manager - STM32 BOOTLOADER
	- SLCAN via canable.io/updater/ - try different firmwares until it works, wrong, dongle is unresponsive
  
### install wsl2

### Install pycyphal, yakut
	- pip install pycyphal
	- pip install yakut
	- pip install python-can

### install usbipd, this bridges usb ports from windows to wsl2
	- winget install --interactive --exact dorssel.usbipd-win
	- usbipd list
	- usbipd bind --busid <BUSID>  # For example: usbipd bind --busid 3-2, this stays even after reboot
	- usbipd attach --wsl --busid <BUSID> # non-admin powershell sesh, wsl2 must be started, command must be executed at every start
	- usbipd detach --busid <BUSID> # command to detach

### start wsl
	- lsusb
	- ls /dev/ttyUSB* /dev/ttyACM* 

### make in home new folder
	- mkdir -p ~/.cyphal
### clone in .cyphal
	- git clone https://github.com/OpenCyphal/public_regulated_data_types

### make enviroment variable that points there:
	- export CYPHAL_PATH="/home/x230/.cyphal/public_regulated_data_types"
	- export CYPHAL_PATH="/home/x230/.cyphal/public_regulated_data_types"
	- export UAVCAN__CAN__IFACE="slcan:/dev/ttyACM0"
	- export UAVCAN__CAN__MTU="8"
	- export UAVCAN__CAN__BITRATE="1000000 1000000"
	- export UAVCAN__NODE__ID="42" 
these lines can be added to ~/.bashrc to make them permanent
