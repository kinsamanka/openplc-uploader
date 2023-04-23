## Installation

##### Download
- Pre-built executables can be downloaded [here](https://github.com/kinsamanka/openplc-uploader/releases).

##### Build executable from source
- Download the build script: [[windows](https://raw.githubusercontent.com/kinsamanka/openplc-uploader/master/scripts/build.bat)] [[linux](https://raw.githubusercontent.com/kinsamanka/openplc-uploader/master/scripts/build.sh)].
- Save and run the script on a dedicated folder.
- The resulting binary can be found under the `dist` directory.

##### Run from source
- Download the script: [[windows](https://raw.githubusercontent.com/kinsamanka/openplc-uploader/master/scripts/start.bat)] [[linux](https://raw.githubusercontent.com/kinsamanka/openplc-uploader/master/scripts/start.sh)].
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
- If the upload is succesful, the `RUN` led will blink rapidly to indicate it is in bootloader mode.

#### Normal Usage
- Select the generated openplc code using the ***Browse*** button.
- Select the correct board.
- Select the correct serial port.
- Set the PLC `RUN` switch to `STOP` position.
- Click ***Upload*** button to start uploading.
- Set the PLC `RUN` switch to `RUN` to start the PLC.

## FX3U PLC Clone troubleshooting

### Upload of bootloader does not work initially

- Likely the flash memory is locked-out by the PLC vendor.
- To unlock the memory:
  - Download and install [STM32CubeProg](https://www.st.com/en/development-tools/stm32cubeprog.html)
  - Follow the steps in [Enable DFU mode on PLC](#enable-dfu-mode-on-plc) to enable the bootloader
  - Run the unprotect sequence (example uses Windows and COM8 serial port):
    - `"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=COM8 br=57600 -rdu`
    - NOTE: The `-rdu` command at the end is the command to remove read-out protection
  - The unprotect sequence will likely show error messages. However, as long as `Activating device: OK` is seen, and multiple retry attempts are shown in the log, the sequence will have likely worked. You can tell by the fact that the next steps (and eventually, the bootloader upload) will now succeed.
  - Follow **again** the steps in [Enable DFU mode on PLC](#enable-dfu-mode-on-plc) to enable the bootloader
  - Perform a mass-erase of the MCU (example uses Windows and COM8 serial port):
    - `"C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" -c port=COM8 br=57600 -e all`
    - NOTE: The `-e all` command at the end is the command to mass-erase the entire MCU.
- Now you can start over at the beginning of the sequence: [Enable DFU mode on PLC](#enable-dfu-mode-on-plc), then [Install Bootloader](#install-bootloader).
