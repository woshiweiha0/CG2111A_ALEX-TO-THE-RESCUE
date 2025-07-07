#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdint.h>
#include <math.h>
#include "packet.h"
#include "serial.h"
#include "serialize.h"
#include "constants.h"
#include <termios.h>
#include <fcntl.h>

#define PORT_NAME			"/dev/ttyACM0"
#define BAUD_RATE			B9600

int exitFlag=0;
sem_t _xmitSema;

void handleError(TResult error)
{
	switch(error)
	{
		case PACKET_BAD:
			printf("ERROR: Bad Magic Number\n");
			break;

		case PACKET_CHECKSUM_BAD:
			printf("ERROR: Bad checksum\n");
			break;

		default:
			printf("ERROR: UNKNOWN ERROR\n");
	}
}

char getch() {
    struct termios oldt, newt;
    char ch;

    tcgetattr(STDIN_FILENO, &oldt);            // Save old terminal settings
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);          // Disable buffering and echo
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);   // Apply new settings

    ch = getchar();                            // Read one character

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);   // Restore old settings
    return ch;
}


void handleStatus(TPacket *packet)
{
	printf("\n ------- ALEX STATUS REPORT ------- \n\n");
	printf("Left Forward Ticks:\t\t%d\n", packet->params[0]);
	printf("Right Forward Ticks:\t\t%d\n", packet->params[1]);
	printf("Left Reverse Ticks:\t\t%d\n", packet->params[2]);
	printf("Right Reverse Ticks:\t\t%d\n", packet->params[3]);
	printf("Left Forward Ticks Turns:\t%d\n", packet->params[4]);
	printf("Right Forward Ticks Turns:\t%d\n", packet->params[5]);
	printf("Left Reverse Ticks Turns:\t%d\n", packet->params[6]);
	printf("Right Reverse Ticks Turns:\t%d\n", packet->params[7]);
	printf("Forward Distance:\t\t%d\n", packet->params[8]);
	printf("Reverse Distance:\t\t%d\n", packet->params[9]);
	printf("\n---------------------------------------\n\n");
}

void handleColor(TPacket *packet) {
    uint32_t red = packet->params[0];
	uint32_t green = packet->params[1];
	uint32_t blue = packet->params[2];
	
	printf("\n --------- ALEX COLOR SENSOR --------- \n\n");
	printf("Red (R) frequency:\t%d\n", red);
	printf("Green (G) frequency:\t%d\n", green);
	printf("Blue (B) frequency:\t%d\n", blue);
	
	// Re-run the same k-NN logic here to mirror Arduino's result
float distRed = sqrt((float)((red - 71) * (red - 71) +
                             (green - 235) * (green - 235) +
                             (blue - 179) * (blue - 179)));

float distGreen = sqrt((float)((red - 59) * (red - 59) +
                               (green - 51) * (green - 51) +
                               (blue - 54) * (blue - 54)));


    const char* color = "NO COLOR";
    if (distRed < distGreen && distRed < 100)
        color = "RED";
    else if (distGreen < distRed && distGreen < 100)
        color = "GREEN";

    printf("Detected Color: %s\n", color);
}


void handleResponse(TPacket *packet)
{
	// The response code is stored in command
	switch(packet->command)
	{
		case RESP_OK:
			printf("Command OK\n");
		break;

		case RESP_STATUS:
			handleStatus(packet);
		break;
		
		case RESP_COLOR:
		    handleColor(packet);
		break;
		

		default:
			printf("Arduino is confused\n");
	}
}

void handleErrorResponse(TPacket *packet)
{
	// The error code is returned in command
	switch(packet->command)
	{
		case RESP_BAD_PACKET:
			printf("Arduino received bad magic number\n");
		break;

		case RESP_BAD_CHECKSUM:
			printf("Arduino received bad checksum\n");
		break;

		case RESP_BAD_COMMAND:
			printf("Arduino received bad command\n");
		break;

		case RESP_BAD_RESPONSE:
			printf("Arduino received unexpected response\n");
		break;

		default:
			printf("Arduino reports a weird error\n");
	}
}




void handleMessage(TPacket *packet)
{
	printf("Message from Alex: %s\n", packet->data);
}

void handlePacket(TPacket *packet)
{
	switch(packet->packetType)
	{
		case PACKET_TYPE_COMMAND:
				// Only we send command packets, so ignore
			break;

		case PACKET_TYPE_RESPONSE:
				handleResponse(packet);
			break;

		case PACKET_TYPE_ERROR:
				handleErrorResponse(packet);
			break;

		case PACKET_TYPE_MESSAGE:
				handleMessage(packet);
			break;
	}
}

void sendPacket(TPacket *packet)
{
	char buffer[PACKET_SIZE];
	int len = serialize(buffer, packet, sizeof(TPacket));

	serialWrite(buffer, len);
}

void *receiveThread(void *p)
{
	char buffer[PACKET_SIZE];
	int len;
	TPacket packet;
	TResult result;
	int counter=0;

	while(1)
	{
		len = serialRead(buffer);
		counter+=len;
		if(len > 0)
		{
			result = deserialize(buffer, len, &packet);

			if(result == PACKET_OK)
			{
				counter=0;
				handlePacket(&packet);
			}
			else 
				if(result != PACKET_INCOMPLETE)
				{
					printf("PACKET ERROR\n");
					handleError(result);
				}
		}
	}
}

void flushInput()
{
	char c;

	while((c = getchar()) != '\n' && c != EOF);
}

void getParams(TPacket *commandPacket)
{
	printf("Enter distance/angle in cm/degrees (e.g. 50)\n");
	//scanf("%d", &commandPacket->params[0]);
	flushInput();
}

 

void sendCommand(char command)
{
	TPacket commandPacket;

	commandPacket.packetType = PACKET_TYPE_COMMAND;

	switch(command)
	{
		case 'w':
		case 'W':
			commandPacket.command = COMMAND_FORWARD;
			commandPacket.params[0] = 3;
			commandPacket.params[1] = 70;
			sendPacket(&commandPacket);
			break;
			
		case 'f':
		case 'F':
			commandPacket.command = COMMAND_FORWARD;
			commandPacket.params[0] = 25;
			commandPacket.params[1] = 70;
			sendPacket(&commandPacket);
			break;

		case 's':
		case 'S':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_REVERSE;
			commandPacket.params[0] = 3;
			commandPacket.params[1] = 70;
			sendPacket(&commandPacket);
			break;

		case 'b':
		case 'B':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_REVERSE;
			commandPacket.params[0] = 25;
			commandPacket.params[1] = 70;
			sendPacket(&commandPacket);
			break;
			
		case '1':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_LEFT;
			commandPacket.params[0] = 180;
			commandPacket.params[1] = 90;
			sendPacket(&commandPacket);
			break;
		

		case 'a':
		case 'A':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_LEFT;
			commandPacket.params[0] = 20;
			commandPacket.params[1] = 90;
			sendPacket(&commandPacket);
			break;
			
		case 'q':
		case 'Q':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_LEFT;
			commandPacket.params[0] = 90;
			commandPacket.params[1] = 90;
			sendPacket(&commandPacket);
			break;			

		case 'd':
		case 'D':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_RIGHT;
			commandPacket.params[0] = 20;
			commandPacket.params[1] = 90;
			sendPacket(&commandPacket);
			break;
			
		case 'e':
		case 'E':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_RIGHT;
			commandPacket.params[0] = 90;
			commandPacket.params[1] = 90;
			sendPacket(&commandPacket);
			break;			

		case '2':
			//getParams(&commandPacket);
			commandPacket.command = COMMAND_TURN_RIGHT;
			commandPacket.params[0] = 180;
			commandPacket.params[1] = 90;
			sendPacket(&commandPacket);
			break;	

		case 'h':
		case 'H':
			commandPacket.command = COMMAND_STOP;
			sendPacket(&commandPacket);
			break;

		case 'c':
		case 'C':
			commandPacket.command = COMMAND_CLEAR_STATS;
			commandPacket.params[0] = 0;
			sendPacket(&commandPacket);
			break;

		case 'g':
		case 'G':
			commandPacket.command = COMMAND_GET_STATS;
			sendPacket(&commandPacket);
			break;

		case 'x':
		case 'X':
			exitFlag=1;
			break;
			
		case 'o':
		case 'O':
		    commandPacket.command = COMMAND_OPEN;
		    sendPacket(&commandPacket);
		    break;
		    
		case 'p':
		case 'P':
		    commandPacket.command = COMMAND_CLOSE;
		    sendPacket(&commandPacket);
		    break;
		    
		case 'k':
		case 'K':
		    commandPacket.command = COMMAND_SCAN;
		    sendPacket(&commandPacket);
		    break;
		    
		case 'l':
		case 'L':
		    commandPacket.command = COMMAND_DROP;
		    sendPacket(&commandPacket);
		    break;
		    

		default:
			printf("Bad command\n");

	}
}



int main()
{
	// Connect to the Arduino
	startSerial(PORT_NAME, BAUD_RATE, 8, 'N', 1, 5);

	// Sleep for two seconds
	printf("WAITING TWO SECONDS FOR ARDUINO TO REBOOT\n");
	sleep(2);
	printf("DONE\n");

	// Spawn receiver thread
	pthread_t recv;

	pthread_create(&recv, NULL, receiveThread, NULL);

	// Send a hello packet
	TPacket helloPacket;

	helloPacket.packetType = PACKET_TYPE_HELLO;
	sendPacket(&helloPacket);

	while(!exitFlag)
	{
		char ch;
		printf("Command (w=forward, s=reverse, a=turn left, d=turn right, h=stop, c=clear stats, o=open, p=close, l=med, k=scan, g=get stats q=exit)\n");
		//scanf("%c", &ch);

		// Purge extraneous characters from input stream
		//flushInput();

		ch = getch();
		sendCommand(ch);
		usleep(500000); //delay 500ms
	}

	printf("Closing connection to Arduino.\n");
	endSerial();
}
