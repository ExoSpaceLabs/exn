
# IDC

Communication packets definition between devices, where the data is wrapped in CCSDS Packets.


The flow of communication is as follows
- MCU-RTOS -> TC -> PI-CAM -> ACK + TM / NACK -> MCU-RTOS
- MCU-RTOS -> TC -> FPGA-AI -> ACK + TM / NACK -> MCU-RTOS

These might overlap between devices.

## ACK / NACK

## Telecommands definition (TC)
Tele commands are instructions sent by the RTOS for each device to control their behaviour.

### PI-CAM TC

The telecommands directed to the camera is lested below:
- ***HK*** : House keeping / Health Check / Status.
- ***Capture*** : Triggers the camera to capture an image.
- ***Cam-Settings*** : Settings of the camera (Exposure, Focus ...).
    - Settter
    - Getter
- ***Coms-settings*** : The communiication of camera (e.g. direction of data transfer, Interface to use, FPGA, MCU ...).
    - Settter
    - Getter
- ***Data-Transfer*** : Start transfering data.

### FPGA-AI TC
- ***HK*** : House keeping / Health Check / Status.
- ***Coms-settings*** : The communiication of FPGA (e.g. direction of data transfer, Interface to use, PI, MCU ...).
    - Settter
    - Getter
- ***Proc-Settings*** : The Type of processing to perform (preprocess, inference, post process and params) ...
    - Settter
    - Getter
- ***Exec*** : Execute processing (manual execute processing - should be settable that if data transfer is complete, auto execute)
- ***Data-Transfer*** : This could be resultant image or any predictions / model data from MCU or PI.


## Telemetry definition (TM)

### PI-CAM TM
- ***ACK / NACK*** - Acknowledge that the TC has been understood.
- ***HK*** : House keeping / Health Check / Status.
- ***Cam-Settings*** : Returns the settings of the camera (Exposure, Focus ...) after Getter.
- ***Coms-settings*** : Returns the communiication of camera (e.g. direction of data transfer, Interface to use, FPGA, MCU ...) after Getter.

### FPGA-AI TM
- ***ACK / NACK*** - Acknowledge that the TC has been understood.
- ***HK*** : House keeping / Health Check / Status.
- ***Coms-settings*** : Returns the communiication of FPGA (e.g. direction of data transfer, Interface to use, PI, MCU ...) after Getter.
- ***Proc-Settings*** : Returns the Type of processing to perform (preprocess, inference, post process and params ...) after Getter.

## Data Transfer
This section defines how the data is packaged in the CCSDS packets, to successfully stream the data between the devices 
(e.g. PI-CAM -> FPGA-AI) in the first packet of the series some metadata will be sent describing the image.

Image metadata:
- Image ID:   uint16 (up to 65,535)
- Image Size:
  - height:   uint16
  - width:    uint16
  - channels: uint8  (up to 255)
- Pixel type: uint8
  - enum [RGB888, GRAY8, GRAY16 ... ]

