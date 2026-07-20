# CYPHAL on STM32Gx

This project is meant for exercise and learning of cyphal protocol over CAN with STM32Gxx series controller.

## The hardware:
- canable derived USB-to-CAN dongle - SLCAN firmware
- electronic speed controller - B-G431B-ESC1 with STM32G431CB MCU
- motor with hall sensor - maxon EC motor 386658

## Host PC os:
- Linux Mint 22.3

## motor control firmware
Fastest way to get the IFOC with b-g431b-esc1 is to use motor control workbench and cubeMX. Motor Control Workbench unfortunately exist only in windows, while canable dongle and yakut is native in linux. Project was generated in cubeMX with make toolchain.
Makefile was than updated manually to fit the changed project structure. MotorControlWorkbench and CubeMX files are located in tool/cube if changes are needed. Generated code in such case should be cherry picked.

## The environment for sending cyphal messages cyphal messages to esc via USB-to-CAN dongle following things are necessary:

### canable.io/updater/ 
	- boot switch ON
	- device manager - STM32 BOOTLOADER
	- SLCAN via canable.io/updater/ - try different firmwares until it works, wrong, dongle is unresponsive
  
### Install pycyphal, yakut
	- pip install pycyphal
	- pip install yakut

### check where is cansl device
	- ls -l /dev/serial/by-id/

### make in home new folder
	- mkdir -p ~/.cyphal
### clone in .cyphal
	- git clone https://github.com/OpenCyphal/public_regulated_data_types

### make enviroment variable that points there:
	- export CYPHAL_PATH="/home/x230/.cyphal/public_regulated_data_types"
	- export UAVCAN__CAN__IFACE="slcan:/dev/ttyACMx"
	- export UAVCAN__CAN__MTU="8"
	- export UAVCAN__CAN__BITRATE="1000000 1000000"
	- export UAVCAN__NODE__ID="37" 
these lines can be added to ~/.bashrc to make them permanent

### send heartbeat with yakut
	- yakut pub --period=1 uavcan.node.Heartbeat.1.0 '{uptime: 0, health: 0, mode: 0, vendor_specific_status_code: 0}'

### stm32 environment in wsl
	- gdb-multiarch 
			- gdb-multiarch --batch --eval-command="set architecture arm"
	- make
	- openocd
	
### build stm32 with make toolchain
	- make -j4
	- make -j$(nproc)

### flash the target
	- sudo openocd -f interface/stlink.cfg -f target/stm32g4x.cfg -c "program build/lillian.bin 0x08000000 verify reset exit"

### debug the target in cmd
	- sudo openocd -f interface/stlink.cfg -f target/stm32g4x.cfg 							(Terminal1)
	- gdb-multiarch build/lillian.elf				(Terminal2)
	(gdb) target remote :3333						(Terminal2)

### debug the target in gui
- install extension Cortex-Debug by marus25, launch.json is already configured in .vscode, just hit CTRL+SHIFT+D

- install clangd extension
	- sudo apt update && sudo apt install bear
	- make clean
	- bear -- make 

	-> building project this way creates compile_commands.json in root, which clangd can access to make whole code accessible in debugging
	



	
