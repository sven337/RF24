This is library to use the NRF24L01 on the Raspberry Pi.

It's based on the arduino lib from J. Coliz <maniacbug@ymail.com>.
the library was berryfied by Purinda Gunasekara <purinda@gmail.com>.
then forked from github stanleyseow/RF24 by myself

I used the modified low level BCM2835 library to suit my need 
and I also added some features (check the file comments)

This NRF24 library use direct I/O access and hardware SPI to 
control and access the NRF24 module, this really speed up program
for example Round Trip Time of the pingtest and pongtest samples
from Pi to Pi is about 5ms instead of 15ms with the original library

Also the libray and sample code has been rewritten to use native 
Linux API instead of emulated Arduino macros/functions and I removed
all defined part and conditionnal compilation dedicated for Arduino 
to make the code easier to read

setup the library
=================

Clone or download this repo then go to folder
cd RF24/librf24-rpi/librf24-bcm/

then 

make ; make install

examples
========

go to examples subfolder then 
make ; make install

In my examples I used the NRF on ArduiPi Board 
http://hallard.me/arduipi

So on example file the instance is created as follow, change the pins according your connections

// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 4Mhz  
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_4MHZ);   


Pin on ArduiPi board are  
NRF24L01    RPI       P1 Connector  
nrf-vcc  = rpi-3v3        (01)  
nrf-gnd  = rpi-gnd        (06)  
nrf-ce   = rpi-ce1        (26)  
nrf-csn  = rpi-gpio22     (15)  
nrf-sck  = rpi-sckl       (23)  
nrf-mo   = rpi-mosi       (19)  
nrf-mi   = rpi-miso       (21)  

known issues
============
none

contact
=======
Charles-Henri Hallard http://hallard.me

