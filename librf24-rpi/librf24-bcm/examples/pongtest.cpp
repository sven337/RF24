/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 
 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
              Modified to use with Arduipi board http://hallard.me/arduipi
						  Changed to use modified bcm2835 and RF24 library 

 */

/**
 * Example RF Radio Ping Pair
 *
 * This is an example of how to use the RF24 class.
 */

#include <stdio.h>
#include <sys/sysinfo.h>
#include <unistd.h> 
#include <time.h>
#include <cstdlib>
#include <termios.h>
#include <iostream>
#include <fcntl.h>
#include "../RF24.h"


// Hardware configuration
// CE Pin, CSN Pin, SPI Speed
// Setup for GPIO 22 CE and CE1 CSN with SPI Speed @ 4Mhz
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_26, BCM2835_SPI_SPEED_4MHZ);  

// Topology
// Radio pipe addresses for the nodes to communicate.
// I like string, it talk me more than uint64_t, so cast string to uint64_t
// take care that for pipe 2 to end only the last char can be changed, this
// is why I set for x before it will not be taken into account
const uint64_t pipe_0 = *(reinterpret_cast<const uint64_t *>(&"1000W"));
const uint64_t pipe_1 = *(reinterpret_cast<const uint64_t *>(&"1000R"));
//const uint64_t pipe_2 = *(reinterpret_cast<const uint64_t *>(&"2000R"));
//const uint64_t pipe_3 = *(reinterpret_cast<const uint64_t *>(&"3000R"));
//const uint64_t pipe_4 = *(reinterpret_cast<const uint64_t *>(&"4000R"));
//const uint64_t pipe_b = *(reinterpret_cast<const uint64_t *>(&"B000R"));

// Keyboard hit
int kbhit(void)
{
   struct termios oldt, newt;
   int ch, oldf;

   tcgetattr(STDIN_FILENO, &oldt);
   newt = oldt;
   newt.c_lflag &= ~(ICANON | ECHO);
   tcsetattr(STDIN_FILENO, TCSANOW, &newt);
   oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
   fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

   ch = getchar();

   tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
   fcntl(STDIN_FILENO, F_SETFL, oldf);

   if(ch != EOF)
   {
      ungetc(ch, stdin);
      return 1;
   }
   return 0;
} 

int main(int argc, char** argv)
{
	struct sysinfo info;

  printf("pongtest/\nPress any key to exit\n");

  // Setup and configure rf radio, no Debug Info
  radio.begin(DEBUG_LEVEL_NONE);

	// avoid listening when configuring
	radio.stopListening();

	// I recommend doing your full configuration again here
	// not relying on driver init, this will be better in
	// case of driver change, and you will be sure of what 
	// it is initialized !!!!

  // Increase the delay between retries & # of retries
	// 15 Retries 16 * 250 us
  radio.setRetries(15,15);

	// Do not worry about payload size
	radio.enableDynamicPayloads();

	// Set channel used 
	radio.setChannel(76);

	// Set power level to maximum
  radio.setPALevel(RF24_PA_MAX);
	
	// Then set the data rate to the slowest (and most reliable) speed 
  radio.setDataRate( RF24_250KBPS ) ;

  // Initialize CRC and request 1-byte (8bit) CRC
  radio.setCRCLength( RF24_CRC_8 ) ;

  // Auto ACK
  radio.setAutoAck( false ) ;

  // Open pipes to other nodes for communication
  radio.openWritingPipe(  pipe_1);
  radio.openReadingPipe(1,pipe_0);
	
  // Start listening
  radio.startListening();

  // Dump the configuration of the rf unit for debugging
  radio.printDetails();

	// loop until key pressed
	while ( !kbhit() )
	{
		// if there is data ready
		//printf("Check available...\n");
		if ( radio.available() )
		{
			// Dump the payloads until we've gotten everything
			bool done = false;
			
			while (!done)
			{
				// Fetch the payload, and see if this was the last one.
				done = radio.read( &info.uptime, sizeof(info.uptime) );

				// Spew it
				printf("Got payload(%d) %lu...", sizeof(info.uptime), info.uptime);
			}

			// First, stop listening so we can talk
			radio.stopListening();

			printf("Sending back %lu...\n", info.uptime);

			// Send the final one back.
			radio.write( &info.uptime, sizeof(info.uptime) );

			// let transmission done 
			// usleep(50000);

			// Now, resume listening so we catch the next packets.
			radio.startListening();
		}
		
		// May be a good idea not using all CPU into the loop
		usleep(1000); 

	} // forever loop

  return 0;
}

// vim:cin:ai:sts=2 sw=2 ft=cpp
