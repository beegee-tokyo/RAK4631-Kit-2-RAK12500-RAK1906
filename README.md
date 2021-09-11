----
----
# _WORK IN PROGRESS_
----
----

# RAK4631-Kit-2-RAK12500-RAK1906
| <img src="./assets/RAK-Whirls.png" alt="Modules" width="150"> | <img src="./assets/rakstar.jpg" alt="RAKstar" width="100"> |    
| :-: | :-: |     
This is an example code for WisBlock GNSS tracker with RAK12500 GNSS module and RAK1906 environment sensor

## _REMARK 1_
The WisBlock Kit 2 comes with the RAK1910, the RAK1904 and without the RAK1906. To use this example it is suggested to buy
- [WisBlock Starter Kit](https://store.rakwireless.com/collections/kits-bundles/products/wisblock-starter-kit)
- [RAK12500](https://store.rakwireless.com/collections/wisblock-sensor/products/wisblock-gnss-location-module-rak12500)
- [RAK1906](https://store.rakwireless.com/collections/wisblock-sensor/products/rak1906-bme680-environment-sensor)
- [RAKBox-B2](https://store.rakwireless.com/collections/wisblock-enclosure/products/rakbox-b2-enclosure-with-solar-panel)

to build the system instead.

## _REMARK 2_
It is based on my low power event driven example [RAK4631-Quick-Start-Examples](https://github.com/beegee-tokyo/RAK4631-Quick-Start-Examples)

----

# Hardware used
- [RAK4631](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK4631/Overview/) WisBlock Core module
- [RAK5005-O](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK5005-O/Overview/) WisBlock Base board
- [RAK12500](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK12500/Overview/) WisBlock Sensor GNSS module
- [RAK1906](https://docs.rakwireless.com/Product-Categories/WisBlock/RAK1906/Overview/) WisBlock Sensor environment module

## Power consumption
The application does switch off the GPS module and the MCU and LoRa transceiver go into sleep mode between measurement cycles to save power. I could measure a sleep current of 40uA of the whole system. 

----

# Software used
- [PlatformIO](https://platformio.org/install)
- [Adafruit nRF52 BSP](https://docs.platformio.org/en/latest/boards/nordicnrf52/adafruit_feather_nrf52832.html)
- [Patch to use RAK4631 with PlatformIO](https://github.com/RAKWireless/WisBlock/blob/master/PlatformIO/RAK4630/README.md)
- [SX126x-Arduino LoRaWAN library](https://github.com/beegee-tokyo/SX126x-Arduino)
- [SparkFun u-blox GNSS Arduino Library](https://platformio.org/lib/show/11715/SparkFun%20u-blox%20GNSS%20Arduino%20Library)
- [Adafruit BME680 Library](https://platformio.org/lib/show/1922/Adafruit%20BME680%20Library)

## _REMARK_
The libraries are all listed in the **`platformio.ini`** and are automatically installed when the project is compiled.

----

# Setting up LoRaWAN credentials
The LoRaWAN credentials are defined in [include/main.h](./include/main.h). But this code supports 2 other methods to change the LoRaWAN credentials on the fly:

## 1) Setup over BLE
Using the [My nRF52 Toolbox](https://play.google.com/store/apps/details?id=tk.giesecke.my_nrf52_tb) you can connect to the WisBlock over BLE and setup all LoRaWAN parameters like
- Region
- OTAA/ABP
- Confirmed/Unconfirmed message
- ...

More details can be found in the [My nRF52 Toolbox repo](https://github.com/beegee-tokyo/My-nRF52-Toolbox/blob/master/README.md)

The device is advertising over BLE only the first 30 seconds after power up and then again for 15 seconds after wakeup for measurements. The device is advertising as **`RAK-GNSS-xx`** where xx is the BLE MAC address of the device.

## 2) Setup over USB port
Using the AT command interface the WisBlock can be setup over the USB port.

A detailed manual for the AT commands are in [AT-Commands.md](./AT-Commands.md)

## _REMARK_
The AT command format used here is _**NOT**_ compatible with the RAK5205/RAK7205 AT commands.

----

# Packet data format
The packet data is made compatible with the [RAK5205/RAK7205](https://docs.rakwireless.com/Product-Categories/WisTrio/RAK7205-5205/Overview). A detailed explanation and encoders for TTN and Chirpstack can be found in the [RAK5205 Documentation](https://docs.rakwireless.com/Product-Categories/WisTrio/RAK7205-5205/Quickstart/#decoding-sensor-data-on-chirpstack-and-ttn)

## _REMARK_
This application does not include the RAK1904 acceleration sensor, so the data packet does not include the accelerometer part.

----

# Compiled output
The compiled files are located in the [./Generated](./Generated) folder. Each successful compiled version is named as      
**`WisBlock_GNSS_Vx.y.z_YYYYMMddhhmmss`**    
x.y.z is the version number. The version number is setup in the [./platformio.ini](./platformio.ini) file.    
YYYYMMddhhmmss is the timestamp of the compilation.

The generated **`.hex`** file can be used as well to update the device over BLE using either [My nRF52 Toolbox repo](https://github.com/beegee-tokyo/My-nRF52-Toolbox/blob/master/README.md) or [Nordic nRF Toolbox](https://play.google.com/store/apps/details?id=no.nordicsemi.android.nrftoolbox) or [nRF Connect](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp)

----

# Debug options 
Debug output can be controlled by defines in the **`platformio.ini`**    
_**LIB_DEBUG**_ controls debug output of the SX126x-Arduino LoRaWAN library
 - 0 -> No debug outpuy
 - 1 -> Library debug output (not recommended, can have influence on timing)    

_**MY_DEBUG**_ controls debug output of the application itself
 - 0 -> No debug outpuy
 - 1 -> Application debug output

_**CFG_DEBUG**_ controls the debug output of the nRF52 BSP. It is recommended to keep it off

## Example for no debug output and maximum power savings:

```ini
[env:wiscore_rak4631]
platform = nordicnrf52
board = wiscore_rak4631
framework = arduino
build_flags = 
    ; -DCFG_DEBUG=2
	-DSW_VERSION_1=1 ; major version increase on API change / not backwards compatible
	-DSW_VERSION_2=0 ; minor version increase on API change / backward compatible
	-DSW_VERSION_3=0 ; patch version increase on bugfix, no affect on API
	-DLIB_DEBUG=0    ; 0 Disable LoRaWAN debug output
	-DMY_DEBUG=0     ; 0 Disable application debug output
	-DNO_BLE_LED=1   ; 1 Disable blue LED as BLE notificator
lib_deps = 
	beegee-tokyo/SX126x-Arduino
	sparkfun/SparkFun u-blox GNSS Arduino Library@2.0.13
	adafruit/Adafruit BME680 Library
extra_scripts = pre:rename.py
```