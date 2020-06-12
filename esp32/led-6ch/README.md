
# 6-channel Light Controller

## Out-of-the-box

1. Solder whatever connectors you want to use
1. Connect a LED strip, lamp or similar that you want to control
1. Connect a power supply, 5-28 VDC
    * The power supply used must be one that fits the LED strip whatever that you connected to the outputs
1. Push the button on the PCB for one second, lights should start flashing
    * Flashing means the board is now waiting for SmartConfig
1. Download [EspTouch](https://www.espressif.com/en/products/software/esp-touch/overview) to your phone
1. Start EspTouch, fill in your WiFi password and start SmartConfig
    * The app will tell you what IP-address the device got

Now you just need a [MQTT broker](https://mosquitto.org/) and maybe [Home Assistant](https://www.home-assistant.io/) or similar to control the light(s) connected to the board.

## Hardware

The device is designed to fit in a 16x16mm cable duct that you easily find from hardware stores (though you need to cut away the extras on the PCB).

![pcb-layout](images/pcb-layout.png)

* ESP32-PICO-D4 MCU
    * GPIO2: LED PWM channel 0
    * GPIO4: LED PWM channel 1
    * GPIO18: LED PWM channel 2
    * GPIO23: LED PWM channel 3
    * GPIO19: LED PWM channel 4
    * GPIO22: LED PWM channel 5
    * GPIO13: Push button
    * GPIO33/ADC1CH5: MCP9700A temperature sensor

### Programming

You need a separate TTL-level serial port for programming. There is no integrated USB-to-serial converter or similar to save space.

The programming connector is a 6-pin connector on the PCB with all the pins labeled.
You need to connect ```RxD``` on the board to ```TxD``` and ```TxD``` on the board to ```RxD``` on your serial port.

## Modifying software

### Prerequisites

* [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html#installation-step-by-step) (v4.1 used when writing this)
* [libe](https://github.com/aehparta/libe)
    * ```git clone https://github.com/aehparta/libe.git```
    * set ```LIBE_PATH``` environment value to point to libe root
        * as an example add ```export LIBE_PATH="$HOME/libe"``` to your ```~/.bashrc``` if you cloned libe to your home directory

### Compile

This applies if you installed esp-idf with defaults:

```sh
get_idf
make menuconfig
make flash monitor
```
