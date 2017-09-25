# ESP8266iTachEmulator

Send and learn infrared remote control signals via WLAN using an ESP8266

![irbox](https://cloud.githubusercontent.com/assets/2480569/17837757/ea087514-67bb-11e6-9638-3812f706d5da.JPG)

## Purpose

Implements sending and receiving/learning of infrared remote control commands using (subsets of)
* [iTach API Specification Version 1.5](http://www.globalcache.com/files/docs/API-iTach.pdf)

## Compatibility

Known to work with
* [Anymote Smart Remote](http://anymote.io),[Android](https://play.google.com/store/apps/details?id=com.remotefairy4), [Apple](https://itunes.apple.com/app/anymote-smart-remote/id881829455?mt=8) Anymote Smart Remote is very similar to iRule app in that it allows you to send ir comands form your smartphone, except that is has a built in editor, it has the ability to send to Global Cache devices built in (goto settings page in the app, tap Global Cache iTach, input IP & 1:1 for the module adress)
* [IrScrutinizer](https://github.com/bengtmartensson/harctoolboxbundle/releases), a very powerful software to work with infrared remote control codes, see the [IrScrutinizer Guide](http://www.hifi-remote.com/wiki/index.php?title=IrScrutinizer_Guide) to get an impression of what it can do
* Android/iOS [iRule app](http://iruleathome.com) with remote created with cloud codes online at http://iruleathome.com

## Changes form original
 * Created WifiManager version
 * added OTA Function back (because it got removed due to changing to WifiManager over EspManager)
 * Removed unneeded reciever code (May add this back eventually)
 * removed unneeded RCswitch (noclue why it was even used, since its used to cotrol RF 433mhz)
 * Removed unneeded LIRC
 * Removed debug telnet server, converted DebugSend back to serial
 * updated code to work with IRremoteESP8266 v2.1.1 
 *(This also removed the uneeded raw conversion, IRremoteESP8266 already has a send Global Cache function "irsend.sendGC" )
  
## TODO
* revert serial to DebugSend tor free up pins for esp-01
* add reciver code back (sent over Debug)
* add static IP, https://github.com/alexkirill/WiFiManager already has this but it dosn't work on reboot

## Example use case

Using my TRRS Idea i use it for home automation controled by the anymote app

## Building
**ESP8266iTachEmulatorGUI-wm** Now uses wifimanager partuctally kentaylor version [WifiManager](https://github.com/kentaylor/WiFiManager) 

* making it configurable with [ESP Connect](https://play.google.com/store/apps/details?id=au.com.umranium.espconnect&hl=en)


## libraries

* https://github.com/sebastienwarin/IRremoteESP8266.git
* https://github.com/kentaylor/WiFiManager.git
* https://github.com/datacute/DoubleResetDetector

place in Arduino\libraries

### Hardware

Minimal hardware needed for sending:

* NodeMCU 1.0 module (around USD 3 from China shipped) or, if it should fit a small enclosure as shown on the picture, 1 bare ESP12E module (around USD 2 from China shipped; in this case need a matching programming jig too in order to flash it initially, and you need to use the usual minimal circuit to pull up CH_PD and pull down GPIO15)
* Tested with ESP-01 works fine but keep inmind its limitations, GPIO0 & GPIO02 need to be pulled high during boot
* A basic IR LED circuit made up of: Infrared LED, 2N7000 N-channel transistor, Resistor to drive the transistor. 
* or could also uses a TRRS jack for pluging into a IR repeater (check IR repeater pinouts first), you only need to connect data & GND witch can be wired directly to the ESP8266 GPIO & GND without the need for aditional circuitry, (this circuit also meets the requirements for ESP-01 in that during boot GPIO is pulled High

Circut for sending:

* To get good range, attach a resistor to pin 12 of the ESP-12E and connect the resistor to the G pin of a 2N7000 transistor
* If you look at the flat side of the 2N7000 you have S, G, D pins.
* Connect S to GND, G to the resistor to the MCU, and D to the IR LED short pin.
* The long pin of the IR LED is connected to +3.3V.
* I picked the 2N7000 because unlike others it will not pull GPIO2 down which would prevent the chip from booting (if using an ESP-1 module; I prefer ESP-12 now)

Circut for receiving:
REMOVED

## Credits
 probonopd for the original code
