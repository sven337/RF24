/*
 *
 *  Filename : sendto_hub.cpp
 *
 * This is the client for rpi-hub.cpp or use the RPi as a client to an Arduino as a hub
 * The first address in the pipe is for writing and the second address is for reading
 *
 *
 *  Author : Stanley Seow
 *  e-mail : stanleyseow@gmail.com
 *  date   : 4th Apr 2013
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
  // Get the last byte as node-id
  uint8_t nodeID = *((uint8_t *) &pipe_0);
	char receivePayload[32+1];
	char asciibuff[32+1];
  int counter,Data1,Data2,Data3,Data4 ;
  int timeout;

  counter=Data1=Data2=Data3=Data4=0;
	
	// Calculate time taken by a request
	struct timespec requestStart, requestEnd;
	
  printf("\nsendto_hub/\nPress any key to exit\n");

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
	radio.openWritingPipe(  pipe_0);
	radio.openReadingPipe(1,pipe_1);
	radio.openReadingPipe(2,pipe_2);
	radio.openReadingPipe(3,pipe_3);
	radio.openReadingPipe(4,pipe_4);
	radio.openReadingPipe(5,pipe_b);

  // display configuration of the rf
  radio.printDetails();
	
	// loop until key pressed
	while ( !kbhit() )
	{
    // Get readings from sensors, change codes below to read sensors
    Data1 = counter++;
    Data2 = rand() % 1000;
    Data3 = rand() % 1000;
    Data4 = rand() % 1000;
    
    if ( counter > 999 ) 
			counter = 0;

    // Append the hex nodeID to the beginning of the payload    
    // Convert int to strings and number smaller than 3 digits to 000 to 999
    sprintf(asciibuff,"%02X,%03d,%03d,%03d,%03d",nodeID, Data1, Data2, Data3, Data4);
       
    printf("Now sending[%02d] %s..", strlen(asciibuff), asciibuff );
    
		// get clock start value to calculate round trip time
		clock_gettime(CLOCK_REALTIME, &requestStart);
    
    // Send to hub
    if ( !radio.write( asciibuff, strlen(asciibuff)) ) 
		{
       printf(".Failed\n");
    }
    else 
		{
       printf(".Ok...."); 

			 // Ok listen for the response
			radio.startListening();

			timeout = 250; /* 250 ms */

			// Wait here until we get a response, or timeout 
			while ( !radio.available() && timeout-- > 0)
				usleep(1000);

			// Ok so why we are there ?
			if ( timeout <= 0 )
			{
				printf("Failed, response timed out.\n");
			}
			else
			{
				// get clock value to calculate round trip time
				clock_gettime(CLOCK_REALTIME, &requestEnd);

				// Calculate time it took to receive response (in second)
				double roundtrip = ( requestEnd.tv_sec - requestStart.tv_sec ) + ( requestEnd.tv_nsec - requestStart.tv_nsec ) / 1E9L;

				// Get payload size
				uint8_t len = radio.getDynamicPayloadSize();

				// Read payload 
				radio.read( receivePayload, len); 

				// convert to string (should be ok, we sent a string)
				receivePayload[len] = 0;
				
				// Display results
				printf("Got response[%d] %s round-trip delay: %d ms\n",len, strcmp(asciibuff, receivePayload) ? "NO match !":"OK matched", (int) (roundtrip * 1000) );
			} 
			
			// Ready to send next paylaod
			radio.stopListening();    
		}
     
    usleep(250000);
		
	} // While not kbhit()
	
	return 0;
}


