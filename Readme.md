## Installation

##### Download
- Pre-built executables can be downloaded [here](https://github.com/kinsamanka/openplc-uploader/releases).

##### Build executable from source
- Download the build script for windows [here](https://raw.githubusercontent.com/kinsamanka/openplc-uploader/master/scripts/build.bat).
- Save and run the script on a dedicated folder.
- The resulting binary can be found under the `dist` directory.

##### Run from source
- Download the script for windows [here](https://raw.githubusercontent.com/kinsamanka/openplc-uploader/master/scripts/start.bat).
- Save and run the batch file in a dedicated folder.

------------

## STM32 boards or FX3U PLC's Usage

#### Enable DFU mode on PLC
- Disconnect PLC power
- Figure out where `BOOT0` and `Vcc` pins are located.
- Set the `RUN` switch to  `STOP` position.
- Briefly short `BOOT0` and `Vcc` pins while applying power to the PLC to enable DFU.

#### Install Bootloader
- Connect PLC serial port to PC.
- Start `openplc-uploader`.
- Select the correct board.
- Select the correct serial port
- Click on ***Install Bootloader***
- Click ***Upload*** button to start installing the bootloader.
- If the upload is succesfull, the `RUN` led will blink rapidly to indicate it is in bootloader mode.

#### Normal Usage
- Select the generated openplc code using the ***Browse*** button.
- Select the correct board.
- Select the correct serial port.
- Set the PLC `RUN` switch to `STOP` position.
- Click ***Upload*** button to start uploading.
- Set the PLC `RUN` switch to `RUN` to start the PLC.
