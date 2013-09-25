/* 
 *
 *  Filename : rpi-hub.cpp
 *
 *  This program makes the RPi as a hub listening to all six pipes from the remote sensor nodes ( usually Arduino )
 *  and will return the packet back to the sensor on pipe0 so that the sender can calculate the round trip delays
 *  when the payload matches.
 *  
 *  I encounter that at times, it also receive from pipe7 ( or pipe0 ) with content of FFFFFFFFF that I will not sent
 *  back to the sender
 *
 *  Refer to RF24/examples/rpi_hub_arduino/ for the corresponding Arduino sketches to work with this code.
 * 
 *  
 *  CE is not used and CSN is GPIO25 (not pinout)
 *
 *  Refer to RPi docs for GPIO numbers
 *
 *  Author : Stanley Seow
 *  e-mail : stanleyseow@gmail.com
 *  date   : 6th Mar 2013
 *
 * 03/17/2013 : Charles-Henri Hallard (http://hallard.me)
 *              Modified to use with Arduipi board http://hallard.me/arduipi
 *						  Changed to use modified bcm2835 and RF24 library 
 *
 *
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

// Radio pipe addresses for the nodes to communicate.
// I like string, it talk me more than uint64_t, so cast string to uint64_t
// take care that for pipe 2 to end only the last char can be changed, this
// is why I set for x before it will not be taken into account
const uint64_t pipe_0 = *(reinterpret_cast<const uint64_t *>(&"1000W"));
const uint64_t pipe_1 = *(reinterpret_cast<const uint64_t *>(&"1000R"));
const uint64_t pipe_2 = *(reinterpret_cast<const uint64_t *>(&"2000R"));
const uint64_t pipe_3 = *(reinterpret_cast<const uint64_t *>(&"3000R"));
const uint64_t pipe_4 = *(reinterpret_cast<const uint64_t *>(&"4000R"));
const uint64_t pipe_b = *(reinterpret_cast<const uint64_t *>(&"B000R"));

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


int main(void)
{
	char receivePayload[32+1];
	char hexbuff[64+1];
	char asciibuff[32+1];
	uint8_t pipe = 0;
		
  printf("rpi-hub/\nPress any key to exit\n");

  // Setup and configure rf radio, no Debug Info
  radio.begin(DEBUG_LEVEL_NONE);

	// avoid listening when configuring
	radio.stopListening();

	// I recommend doing your full configuration again here
	// not relying on driver init, this will be better in
	// case of driver change, and you will be sure of what 
	// it is initialized !!!!

	// enable dynamic payloads
	// take care, dynamic payload require Auto Ack !!!!
	radio.enableDynamicPayloads();

  // Auto ACK
  radio.setAutoAck( false ) ;

  // Increase the delay between retries & # of retries
	// 15 Retries 16 * 250 us
  radio.setRetries(15,15);

	// Set channel used 
	radio.setChannel(76);

	// Set power level to maximum
  radio.setPALevel(RF24_PA_MAX);

	// Then set the data rate to the slowest (and most reliable) speed 
  radio.setDataRate( RF24_250KBPS ) ;

  // Initialize CRC and request 1-byte (8bit) CRC
  radio.setCRCLength( RF24_CRC_8 ) ;

	// Open 6 pipes for readings ( 5 plus pipe0, also can be used for reading )
	// Revert pipe 0 and pipe 1 to be able to receive packet from pingtest
	radio.openWritingPipe(  pipe_1);
	radio.openReadingPipe(1,pipe_0);
	radio.openReadingPipe(2,pipe_2);
	radio.openReadingPipe(3,pipe_3);
	radio.openReadingPipe(4,pipe_4);
	radio.openReadingPipe(5,pipe_b);

	// Ok ready to listen
	radio.startListening();
	
  // display configuration of the rf
  radio.printDetails();
	
	// loop until key pressed
	while ( !kbhit() )
	{
		// Display it on screen
		//printf("Listening\n");

		while ( radio.available( &pipe ) ) 
		{
			// be sure all is fine 
			// usleep(5000);
			
			// Get packet payload size
			uint8_t len = radio.getDynamicPayloadSize();
			uint8_t i;
			char c;
			
			// Avoid buffer overflow
			if (len > 32)
				len = 32;
			
			// Read data received
			radio.read( receivePayload, len );

			// Prepare display in HEX and ASCII format of payload
			for (i=0 ; i<len; i++ )
			{
				c = receivePayload[i];
				sprintf(&hexbuff[i*2], "%02X", c);
				asciibuff[i] = isprint(c) ? c: '.';
			}

			// end our strings
			asciibuff[i] = hexbuff[i*2] = '\0';
			
			// Display it on screen
			printf("Recv[%02d] from pipe %i : payload=0x%s -> %s",len, pipe, hexbuff, asciibuff);
				
			// Send back payload to sender
			radio.stopListening();

			// if pipe is 7, do not send it back
			if ( pipe != 7 ) 
			{
				// Send back using the same pipe
				// radio.openWritingPipe(pipes[pipe]);
				radio.write(receivePayload,len);

				receivePayload[len]=0;
				printf("\t Sent back %d bytes to pipe %d\n\r", len, pipe);
			}
			else 
			{
				printf("\n\r");
			}

			// Enable start listening again
			radio.startListening();

			// Increase the pipe outside the while loop
			pipe++;
			
			// reset pipe to 0
			if ( pipe > 5 ) 
				pipe = 0;
		}

		// May be a good idea not using all CPU into the loop
		usleep(1000);
	}
	
	return 0 ;
}


