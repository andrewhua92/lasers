#include "gpiolib_addr.h"
#include "gpiolib_reg.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/watchdog.h> 	//needed for the watchdog specific constants
#include <unistd.h> 		//needed for sleep
#include <sys/ioctl.h> 		//needed for the ioctl function
#include <sys/time.h>           //for gettimeofday()

//Below is a macro that had been defined to output appropriate logging messages
//You can think of it as being similar to a function
//file        - will be the file pointer to the log file
//time        - will be the current time at which the message is being printed
//programName - will be the name of the program, in this case it will be Lab4Sample
//str         - will be a string that contains the message that will be printed to the file.
#define PRINT_MSG(file, time, programName, sev, str) \
	do{ \
         char* severity; \
         switch(sev){ \
            case 1: \
            severity = "Debug"; \
            break; \
            case 2: \
            severity = "Info"; \
            break; \
            case 3: \
            severity = "Warning"; \
            break; \
            case 4: \
            severity = "Error"; \
            break; \
            case 5: \
            severity = "Critical"; \
            break; \
         }\
			fprintf(file, "%s : %s : %s : %s", time, programName, severity, str); \
			fflush(file); \
	}while(0)
/*
#define PRINT_MSG(file, time, programName, sev, str) \
	do{ \
			fprintf(logFile, "%s : %s : %s", time, programName, str); \
			fflush(logFile); \
	}while(0)*/
//HARDWARE DEPENDENT CODE BELOW
#ifndef MARMOSET_TESTING

/* You may want to create helper functions for the Hardware Dependent functions*/

//This function accepts an errorCode. You can define what the corresponding error code
//will be for each type of error that may occur.
// Types of Errors:
// -1 - GPIO failed to initialized
// -2 - Invalid time entered
// -3 - No time entered
// -4 - Invalid diode number
void errorMessage(int errorCode)
{
   fprintf(stderr, "An error occured; the error code was %d \n", errorCode);
}


//This function should initialize the GPIO pins
GPIO_Handle initializeGPIO()
{
   GPIO_Handle gpio;
   gpio = gpiolib_init_gpio();
   if (gpio == NULL)
   {
      errorMessage(-1);
   }
   return gpio;
}

//This function will get the current time using the gettimeofday function
void getTime(char* buffer)
{
	//Create a timeval struct named tv
   struct timeval tv;

	//Create a time_t variable named curtime
   time_t curtime;


	//Get the current time and store it in the tv struct
   gettimeofday(&tv, NULL); 

	//Set curtime to be equal to the number of seconds in tv
   curtime=tv.tv_sec;

	//This will set buffer to be equal to a string that in
	//equivalent to the current date, in a month, day, year and
	//the current time in 24 hour notation.
   strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));

} 

// This will simply just make sure the inputted arrays has space values as opposed to letters
void cleanArray(char* dirty, int num){
   for (int i =0 ; i < num ; i++)
   {
      dirty[i] = ' ';
   }
}

// State machine for the config read file
// Different states for the start and end of the read-in line
// One for when to ignore a line
// One for when reading in spaces, next-line character, and equals
// One for reading in words (char after char or just strings)
// Ones for when reading in the strings for the file names of the log, stats, and the time
// Variable for the state it is on at the moment,
// Another for the 'current' status of the what is to be read-in (timeout, log file or stats file)
// Also another one for the previous state
enum READER {STARTS, END, IGNORE, SPACES, NEXT, EQUALS, WORDS, TIMEOUT, LOG_FILE, STATS_FILE};
enum READER r = STARTS;
enum READER curr;
enum READER prevs;

// Functions for reading in the file
void readConfig(FILE* config, int* timeout, char* logFile, char* statsFile){

   // Char arrays for the buffer for reading in the char
   // to store the keyword for reading in log file
   // to store the keyword for reading in time name
   // to store the keyword for reading in stats file
   char buffer[255];
   char logName[8] = "LOGFILE";
   char timeName[17]= "WATCHDOG_TIMEOUT";
   char statsName[10] = "STATSFILE";
   
   // Initializes as 0
   *timeout = 0;

   // Int booleans to determine which 'curr' or status of what is to be read-in
   int log;
   int stats;
   int timer;   

   // Log Count for reading in Log chars for the file name
   // Stats Count for reading in Stats chars for the file name
   // Current Count for counting the number of chars per string parsed by space or equals
   // Char array for reading in the current string
   int logCount = 0;
   int statsCount = 0;
   int currCount = 0;
   char current[100];

   // Int man for the counter for traversing the buffer
   int i = 0;
  
  // While loop to read in the buffered char if its a valid char
   while(fgets(buffer,255,config) != NULL)
   {
      // Resets counter for everything
      logCount = 0;
      statsCount = 0;
      currCount = 0;
      cleanArray(current,100);
      i= 0;
      r = STARTS;
      // While loop to traverse the array
      while(i < 255)
      {
         // Switch case structure
         switch(r)
         {
            // If state is STARTS...
            case STARTS:
               // if the next char is a space...
               if (buffer[i] == ' ')
               {
                  // state goes to SPACES
                  r = SPACES;
               }
               // if the next char is a new-line character
               else if (buffer[i] == '\n')
               {
                  // state goes to NEXT
                  r = NEXT;
               }
               // if the next char is an equals sign
               else if (buffer[i] == '=')
               {
                  // state goes to EQUALS
                  r = EQUALS;
               }
               // if the next char is a valid 'character' 
               else if (buffer[i] >= 32 && buffer[i] <= 126){
                  // states goes to WORDS
                  // store a character in the current array
                  // increases current counter
                  r = WORDS;
                  current[currCount] = buffer[i];
                  currCount++;
               }
               // if the char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // state goes to END
                  r = END;
               }  
               // store previous as STARTS
               prevs = STARTS;
               break;
         
            // If state is END...
            case END:
               // set the counter to 255 to cease loop
               i = 255;
               break;
         
            // If the state is IGNORE
            case IGNORE:
               // reset the current counter and current char array
               currCount = 0;
               cleanArray(current, 100);
               // if next char is a new-line character...
               if (buffer[i] == '\n')
               {
                  // state goes to NEXT
                  r = NEXT;
               }
               // if the next char is a space...
               else if (buffer[i] == ' ')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an equals sign
               else if (buffer[i] == '=')
               {
                  // state goes to EQAUSL
                  r = EQUALS;
               }
               // if the next is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126){
                  // state goes to IGNORE
                  r = IGNORE;      
               }
               // if the next char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // state goes to END
                  r = END;
               }  
               // store previous as IGNORE
               prevs = IGNORE;
               break;
         
            // If state is SPACES...
            case SPACES:
               // initializes the int bools for the current read-in status
               log = 1;
               stats = 1;
               timer = 1;
               // caps off the last char as an null-terminator for the current string
               current[currCount+1] = '\0';
               // checks if the last state was a word
               if (prevs == WORDS)
               {
                  // loops through the number of chars read in from the previous string
                  for (int j = 0; j < currCount; j++)
                  {
                     // For the following if structures, they will essentially compare
                     // if the read-in char array is the same as the char array (string to string)
                     // are the same, and if not, will set the int bool to false (or 0)
                     if (!(current[j] == logName[j]) && j < 9)
                     {
                        log = 0;
                     }
                     if (!(current[j] == statsName[j]) && j < 11)
                     {
                        stats = 0;
                     }
                     if (!(current[j] == timeName[j]) && j < 18)
                     {
                        timer = 0;
                     }
                  }
               }
               else
               {
                  // if the last state wasn't even WORDS, just set them to false
                  log = 0;
                  stats = 0;
                  timer = 0;
               }
               // If any of the previous 'current' states were true, set the
               // current 'status' of read-in string to be the corresponding
               // read-in status, so either log file, stats file, or timeout
               if (log)
               {
                  curr = LOG_FILE;
               }
               else if (stats)
               {
                  curr  = STATS_FILE;
               }
               else if (timer)
               {
                  curr = TIMEOUT;
               }
               // Wipe the array and reset the current counter
               cleanArray(current, 100);
               currCount = 0;
               // if the next char is a space...
               if (buffer[i] == ' ')
               {
                  // state goes to SPACES
                  r = SPACES;
               }
               // if the next char is a new-line character
               else if (buffer[i] == '\n')
               {  
                  // state goes to NEXT
                  r = NEXT;
               }
               // if the next char is an equals sign
               else if (buffer[i] == '=')
               {
                  // state goes to EQUALS
                  r = EQUALS;
               }
               // if the next char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // state goes to END
                  r = END;
               }
               // if the next char is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126){
                  // The following ifs check if the corresponding 'current status'
                  // is true and if so, sets the state to the corresponding state
                  // and also starts the reading in current status, and if it is
                  // not in a current status, simply just reads in normally
                  if (curr == TIMEOUT && buffer[i] >= '0' && buffer[i] <= '9')
                  {
                     *timeout = (*timeout*10) + (buffer[i] - '0');
                     r = TIMEOUT;
                     currCount++;
                  }
                  else if (curr == LOG_FILE)
                  {
                     current[currCount] = buffer[i];
                     r = LOG_FILE;
                     currCount++;
                  }
                  else if (curr == STATS_FILE)
                  {
                     current[currCount] = buffer[i];
                     r = STATS_FILE;
                     currCount++;
                  }
                  else
                  {
                     r = WORDS;
                     current[currCount] = buffer[i];
                     currCount++;      
                  }
               }
               // stores the previous state as SPACES
               prevs = SPACES;
               break;
         
            // If the state is NEXT...
            case NEXT:
               // resets the current counter and wipes the current char array
               currCount = 0;
               cleanArray(current,100);
               // if the next char is a space...
               if (buffer[i] == ' ')
               {
                  // state goes to SPACES
                  r = SPACES;
               }
               // if the next char is a new-line character
               else if (buffer[i] == '\n')
               {
                  // state goes to NEXT
                  r = NEXT;
               }
               // if the next char is an equasl sign
               else if (buffer[i] == '=')
               {
                  // state goes to EQUALS
                  r = EQUALS;
               }
               // if the next char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // state goes to END
                  r = END;
               }  
               // if the next char is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126){
                  // state goes to WORDS
                  // store a character in the current array
                  // increases current counter
                  r = WORDS;
                  current[currCount] = buffer[i];
                  currCount++;
               }
               // store previous state as NEXT
               prevs = NEXT;
               break;
            
            // If the state is EQUALS...
            case EQUALS:
               // resets the current counter
               currCount = 0;
               // if the next char is a space...
               if (buffer[i] == ' ')
               {
                  // state goes to SPACES
                  r = SPACES;
               }
               // if the next char is a new-line character
               else if (buffer[i] == '\n')
               {
                  // state goes to NEXT
                  r = NEXT;
               }
               // if the next char is an equals sign
               else if (buffer[i] == '=')
               {
                  // state goes to EQUALS
                  r = EQUALS;
               }
               // if the next char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // state goes to END
                  r = END;
               }  
               // if the next char is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126)
               {
                  // The following ifs check if the corresponding 'current status'
                  // is true and if so, sets the state to the corresponding state
                  // and also starts the reading in current status, and if it is
                  // not in a current status, simply just reads in normally
                  if (curr == TIMEOUT)
                  {
                     *timeout = (*timeout*10) + (buffer[i] - '0');
                     r = TIMEOUT;
                     currCount++;
                  }
                  else if (curr == LOG_FILE)
                  {
                     current[currCount] = buffer[i];
                     r = LOG_FILE;
                     currCount++;
                  }
                  else if (curr == STATS_FILE)
                  {
                     current[currCount] = buffer[i];
                     r = STATS_FILE;
                     currCount++;
                  }
                  else
                  {
                     r = WORDS;
                     current[currCount] = buffer[i];
                     currCount++;      
                  }
               }
               // stores previous state as EQUALS
               prevs = EQUALS;
               break;
                 
            // If the next state is WORDS
            case WORDS:
               // if the next char is a space 
               if (buffer[i] == ' ')
               {
                  // state goes to SPACES
                  r  = SPACES;
               }
               // if the next char is a new-line character
               else if (buffer[i] == '\n')
               {
                  // state goes to NEXT
                  r = NEXT;
               }
               // if the next char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // state goes to END
                  r = END;
               }  
               // if the next char is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126)
               {
                  // states goes to WORDS
                  // store a character in the current array
                  // increases current counter
                  current[currCount] = buffer[i];
                  currCount++;
                  r = WORDS;
               }
               // store the previous state as WORDS
               prevs = WORDS;
               break;
                  
            // If the current state is TIMEOUT...
            case TIMEOUT:
               // if the next char is a space
               if (buffer[i] == ' ')
               {
                  // make the current status as IGNORE to reset it
                  // state goes to SPACES
                  curr = IGNORE;
                  r = SPACES;
               }
               // if the next char is a new-line character
               else if (buffer[i] == '\n')
               {
                  // make the current status as IGNORE to reset it
                  // state goes to NEXT
                  curr = IGNORE;
                  r = NEXT;
               }
               // if the next char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // state goes to END
                  r = END;
               }  
               // if the next char is a number
               else if (buffer[i] >= '0' && buffer[i] <= '9')
               {
                  // increases the timeout int and state goes to TIMEOUT
                  *timeout  = (*timeout*10) + (buffer[i] - '0');
                  r = TIMEOUT;   
               }
               // if the next char is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126)
               {
                  // states goes to WORDS
                  // store a character in the current array
                  // increases current counter
                  r = WORDS;
                  current[currCount] = buffer[i];
                  currCount++;
               }
               // store previous as TIMEOUT
               prevs = TIMEOUT;
               break;
         
            // If the state is LOG_FILE
            case LOG_FILE:
               // increases the log char counter
               logCount++;
               // if the next char is a space
               if (buffer[i] == ' ')
               {
                  // this means the string has ended
                  // stores the current string into the log file name char array
                  // ends the array with a null-terminator
                  // makes the current status as IGNORE to reset it
                  // state goes to SPACES
                  for (int j = 0; j < logCount; j++)
                  {
                     logFile[j] = current[j];
                  }
                  logFile[logCount] = 0;
                  curr = IGNORE;
                  r = SPACES;
               }
               // if the next char is a space
               else if (buffer[i] == '\n')
               {  
                  // this means the string has ended
                  // stores the current string into the log file name char array
                  // ends the array with a null-terminator
                  // makes the current status as IGNORE to reset it
                  // state goes to SPACES
                  for (int j = 0; j < logCount; j++)
                  {
                     logFile[j] = current[j];
                  }
                  logFile[logCount] = 0;
                  curr = IGNORE;
                  r = NEXT;
               }
               // if the next char is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126)
               {
                  // state goes to WORDS
                  // store a character in a current array
                  // increases current counter
                  current[currCount] = buffer[i];
                  currCount++;
                  r = LOG_FILE;        
               }
               // if the next char is a hashtag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // this means the string has ended
                  // stores the current string into the log file name char array
                  // ends the array with a null-terminator
                  // makes the current status as IGNORE to reset it
                  // state goes to SPACES
                  for (int j = 0; j < logCount; j++)
                  {
                     logFile[j] = current[j];
                  }
                  logFile[logCount] = 0;
                  r = END;
               }  
               prevs = LOG_FILE;
               break;
         
            // If the state is STATS_FILE
            case STATS_FILE:
               // increase the stats counter
               statsCount++;
               // if the next char is a space
               if (buffer[i] == ' ')
               {
                  // this means the string has ended
                  // stores the current string into the stats file name char array
                  // ends the array with a null-terminator
                  // makes the current status as IGNORE to reset it
                  // state goes to SPACES
                  for (int j = 0; j < statsCount; j++)
                  {
                     statsFile[j] = current[j];
                  }
                  curr = IGNORE;
                  r = SPACES;
               }
               // if the next char is a next-line character
               else if (buffer[i] == '\n')
               {
                  // this means the string has ended
                  // stores the current string into the log file name char array
                  // ends the array with a null-terminator
                  // makes the current status as IGNORE to reset it
                  // state goes to SPACES
                  for (int j = 0; j < statsCount; j++)
                  {
                     statsFile[j] = current[j];
                  }
                  curr = IGNORE;
                  r = NEXT;
               }
               // if the next char is a valid 'character'
               else if (buffer[i] >= 32 && buffer[i] <= 126){
                  // states goes to WORDS
                  // store a character in the current array
                  // increases current counter
                  current[currCount] = buffer[i];
                  currCount++;
                  r = STATS_FILE;        
               }
               // if the next char is a hasthag
               else if (buffer[i] == '#')
               {
                  // state goes to IGNORE
                  r = IGNORE;
               }
               // if the next char is an EOF char
               else if (buffer[i] == EOF)
               {
                  // this means the string has ended
                  // stores the current string into the log file name char array
                  // ends the array with a null-terminator
                  // makes the current status as IGNORE to reset it
                  // state goes to SPACES
                  for (int j = 0; j < statsCount; j++)
                  {
                     statsFile[j] = current[j];
                  }
                  r = END;
               }  
               // store the previous state as STATS_FILE
               prevs = STATS_FILE;
               break;
         }
         i++;
      }
   }
}

//This function should accept the diode number (1 or 2) and output
//a 0 if the laser beam is not reaching the diode, a 1 if the laser
//beam is reaching the diode or -1 if an error occurs.
// Pins 4 and 10 initialized as pins ready for input from the photodiode output
#define LASER1_PIN_NUM 4
#define LASER2_PIN_NUM 10
int laserDiodeStatus(GPIO_Handle gpio, int diodeNumber)
{
   // Checks if the GPIO is initialized
   if(gpio == NULL)
   {
      return -1;
   }
   
   // Checks the current diode
   if(diodeNumber == 1)
   {
      // Initializes the level register for the number
      uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));
   
      // Checks if the pin state of the first bit in the current pin is on or off (1 or 0)
      // Returns the respective value
      if(level_reg & (1 << LASER1_PIN_NUM))
      {
         return 1;
      }
      else
      {
         return 0;
      }
   }
   else if (diodeNumber == 2)
   {
   
      uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));
      
      if(level_reg & (1 << LASER2_PIN_NUM))
      {
         return 1;
      }
      else
      {
         return 0;
      }
   }
   // If somehow the diodeNumber is not 1 or 2
   else
   {  
      // Returns an error code
      errorMessage(-4);
      return -1;
   }
}

#endif
//END OF HARDWARE DEPENDENT CODE

//This function will output the number of times each laser was broken
//and it will output how many objects have moved into and out of the room.

//laser1Count will be how many times laser 1 is broken (the left laser).
//laser2Count will be how many times laser 2 is broken (the right laser).
//numberIn will be the number  of objects that moved into the room.
//numberOut will be the number of objects that moved out of the room.
void outputMessage(int laser1Count, int laser2Count, int numberIn, int numberOut)
{
   printf("Laser 1 was broken %d times \n", laser1Count);
   printf("Laser 2 was broken %d times \n", laser2Count);
   printf("%d objects entered the room \n", numberIn);
   printf("%d objects exitted the room \n", numberOut);
}

#ifndef MARMOSET_TESTING

// Initializes the enumerated state machine
// Features the start, when the left laser is off, the right laser is off, when both are off, and when both are on again
// Creates a state variable to store current value
// Creates a state variable to store the previous value
enum STATE {START, LEFT_OFF, RIGHT_OFF, BOTH_OFF, BOTH_ON};
enum STATE s = START;
enum STATE prev = START;

// Main functions with functionality
int main(const int argc, const char* const argv[])
{
	//We want to accept a command line argument that will be the number
	//of seconds that the program will run for, so we need to ensure that
	//the user actually gives us a time to run the program for
   /*if(argc < 2)
   {
      errorMessage(-3);
      return -1;
   }*/
	
   // Create a string that contains the program name
   const char* argName = argv[0];
   
   int i = 0;
   int nameLength = 0;
   
   while(argName[i] != 0)
   {
      nameLength++;
      i++;
   }
   
   char programName[nameLength];
   i = 0;
   
   // Copy the name of the program without the ./ at the start of argv[0]
   while (argName[i+2] != 0)
   {
      programName[i] = argName[i+2];
      i++;
   }
   
   // Create a file pointer named configFile
   // Set it to point to the laserConfig.cfg, setting it to read it
   FILE* configFile = fopen("/home/pi/laserConfig.cfg", "r");
   
   // Outputs a warning message if file cannot be opened
   if (!configFile)
   {
      perror("The config file could not be opened");
      return -1;
   }
   
   // Declare variables to pass into the readConfig function
   int timeout = 0;
   char logFileName[50];
   char statsFileName[50];
   
   // Call the readConfig function to read from the config file
   readConfig(configFile, &timeout, logFileName, statsFileName);
   
   //Close the configFile after finished reading from the file
   fclose(configFile);
   
   // Create a file pointer to point to the log file
   // Set it to point to the file from the congfig file and append to whatever it writes to
   FILE* logFile = fopen(logFileName, "a");
   
   FILE* statsFile = fopen(statsFileName, "a");

   printf("Time out is %d \n", timeout);
   printf("Stats name is %s\n", statsFileName);
   printf("Log name is %s\n", logFileName);

   // Outputs a warning message if file cannot be opened
   if (!configFile)
   {
      perror("The config file could not be opened");
      return -1;
   }
   
   if(!logFile)
   {
      perror("The log file could not be opened");
      return -1;
   }

   if(!statsFile)
   {
      perror("The stats file could not be opened");
      return -1;
   }

   //Create a char array that will be used to hold the time values
   char timeVal[30];
   getTime(timeVal);
   
   //Initialize the GPIO pins
   GPIO_Handle gpio = initializeGPIO();
   //Get the current time
   getTime(timeVal);
   //Log that GPIO pins have been initialized
   PRINT_MSG(logFile, timeVal, programName, 2, "The GPIO pins have been initialized\n\n");
   
   //This variable will be used to access the /dev/watchdog file, similar to how the GPIO_Handle works
   int watchdog;
   
   //We use the open function here to open the /dev/watchdog file. If it does
   //not open, then we output an error message. We do not use fopen() 
   //because we not want to create a file it doesn't exist
   if ((watchdog = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0) {
      printf("Error: Couldn't open watchdog device! %d\n", watchdog);
      return -1;
   } 
   
   //Get the current time
   getTime(timeVal);
   //Log that the watchdog file has been opened
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog file has been opened\n\n");
   
   //This line uses the ioctl function to set the time limit of the watchdog
	//timer to 15 seconds. The time limit can not be set higher that 15 seconds
	//so please make a note of that when creating your own programs.
	//If we try to set it to any value greater than 15, then it will reject that
	//value and continue to use the previously set time limit
   ioctl(watchdog, WDIOC_SETTIMEOUT, &timeout);
	
	//Get the current time
   getTime(timeVal);
	//Log that the Watchdog time limit has been set
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog time limit has been set\n\n");

	//The value of timeout will be changed to whatever the current time limit of the
	//watchdog timer is
   ioctl(watchdog, WDIOC_GETTIMEOUT, &timeout);

	//This print statement will confirm to us if the time limit has been properly
	//changed. The \n will create a newline character similar to what endl does.
   printf("The watchdog timeout is %d seconds.\n\n", timeout);
   

   // These variables will represent the number of times the respective laser is broken
   // as well as indicate the number of object that are in and out of the room
   int laser1Count = 0;
   int laser2Count = 0;
   int numberIn = 0;
   int numberOut = 0;

   // Initialize the state of the laser diodes
   int laser_state1 = laserDiodeStatus(gpio, 1);
   int laser_state2 = laserDiodeStatus(gpio, 2);

   time_t curTime = time(NULL);

   int count = 0;
   
	//In the while condition, we check the current time minus the start time and
	//see if it is less than the number of seconds that was given from the 
	//command line.
   while(1)
   {
   	//The code in this while loop is identical to the code from buttonBlink
   	//in lab 2. You can think of the laser/photodiode setup as a type of switch
      laser_state1 = laserDiodeStatus(gpio, 1);
      laser_state2 = laserDiodeStatus(gpio, 2);
      
      switch(s)
      {  
         // If it just started...
         case START:
            // and the left laser is turned off...
            if (!laser_state1 && laser_state2)
            {
               // Log that the left laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The left laser turned off\n\n");
               // Current state is LEFT_OFF
               s = LEFT_OFF;
               prev = START;
               laser1Count++;
            }
            // and the right laser is turned off...
            if (!laser_state2 && laser_state1)
            {
               // Log that the right laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The right laser turned off\n\n");
               // Current state is RIGHT_OFF
               s = RIGHT_OFF;
               prev = START;
               laser2Count++;
            }
            break;
         
         // If both lasers are on ...
         case BOTH_ON:
            // and the left laser is turned off...
            if (!laser_state1 && laser_state2)
            {
               // Log that the left laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The left laser turned off\n\n");
               // Current state is LEFT_OFF
               s = LEFT_OFF;
               prev = BOTH_ON;
               laser1Count++; //
            }
            // and the right laser is turned off...
            if (!laser_state2 && laser_state1)
            {
               // Log that the right laser turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The right laser turned off\n\n");
               // Current state is RIGHT_OFF
               s = RIGHT_OFF;
               prev = BOTH_ON;
               laser2Count++; //
            }
            // Stores current state
            break;
         
         // If both lasers aren't on ...
         case BOTH_OFF:
            // and the left laser is now back on...
            if (laser_state1 && !laser_state2)
            {
               // Log that the right laser is turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The right laser is turned off\n\n");
               // Current state is RIGHT_OFF
               s = RIGHT_OFF;
               prev = BOTH_OFF;
            }
            // and the right laser is now back on...
            if (laser_state2 && !laser_state1)
            {
               // Log that the left laser is turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "The left laser is turned off\n\n");
               // Current state is LEFT_OFF
               s = LEFT_OFF;
               prev = BOTH_OFF;
            }
            // Stores current state
            break;
         
         // If left laser is off...
         case LEFT_OFF:
            // and is turned back on...
            if (laser_state1)
            {
               // and the right laser is off...
               if (!laser_state2)
               {  
                  // Log that the right laser is turned off
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "The right laser is turned off\n\n");
                  // Current state is RIGHT_OFF
                  // Successfully moves from left laser to right
                  s = RIGHT_OFF;
                  prev = LEFT_OFF;
                  laser2Count++;
               }
               // and the lasers are both on...
               else if (laser_state2)
               {
                  // Log that both lasers are on
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "Both lasers are turned on\n\n");
                  // Current state is BOTH_ON
                  s = BOTH_ON;
                  // and if previous state was when both were off...
                  if (prev == BOTH_OFF)
                  {
                     // Log that the object moved out
                     getTime(timeVal);
                     PRINT_MSG(logFile, timeVal, programName, 2, "An object has moved out\n\n");
                     // An object has gone from right to left
                     numberOut++;
                  }
                  prev = LEFT_OFF;
               }
            }
            // and the right laser is turned off...
            if (!laser_state2)
            {
               //Log that both lasers are off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "Both lasers are turned off\n\n");
               // Current state is BOTH_OFF
               s = BOTH_OFF;
               prev = LEFT_OFF;
               laser2Count++;
            }
            break;
         
         // If right laser is off...
         case RIGHT_OFF:
            // and is turned back on...
            if (laser_state2)
            {
               // and the left laser is off...
               if (!laser_state1)
               {
                  //Log that the left laser is off
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "The left laser is turned off\n\n");
                  // Current state is LEFT_OFF
                  // Successfully moves object from right laser to left
                  s = LEFT_OFF;
                  prev = RIGHT_OFF;
                  laser1Count++;
               }
               // and lasers are both on...
               else if (laser_state1)
               {
                  //Log that both lasers are on  
                  getTime(timeVal);
                  PRINT_MSG(logFile, timeVal, programName, 2, "Both lasers are turned on\n\n");
                  // Current state is BOTH_ON
                  s = BOTH_ON;
                  // and if previously both lasers were off...
                  if (prev == BOTH_OFF)
                  {
                     //Log that an object has moved in
                     getTime(timeVal);
                     PRINT_MSG(logFile, timeVal, programName, 2, "An object has moved in\n\n");
                     // An object has moved left to right
                     numberIn++;
                  }
                  prev = RIGHT_OFF;
               }
            }
            // and the left laser is turned off...
            if (!laser_state1)
            {
               //Log that the both lasers are turned off
               getTime(timeVal);
               PRINT_MSG(logFile, timeVal, programName, 2, "Both of the lasers are turned off\n\n");
               // Current state is BOTH_OFF
               s = BOTH_OFF;
               prev = RIGHT_OFF;
               laser1Count++;
            }
            break;
      }
      // Inputs a time delay 
      usleep(10000);
      if (count == 10)
      {
         count = 0;
	outputMessage(laser1Count,laser2Count,numberIn,numberOut);
      //This ioctl call will write to the watchdog file and prevent 
      //the system from rebooting. It does this every 2 seconds, so 
      //setting the watchdog timer lower than this will cause the timer
      //to reset the Pi after 1 second
         ioctl(watchdog, WDIOC_KEEPALIVE, 0);
         getTime(timeVal);
      //Log that the Watchdog was kicked
         PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog was kicked\n\n");
         // Updates the stats file 
         getTime(timeVal);
         fprintf(statsFile, "%s : %s : Laser 1 was broken %d times \n%s : %s : Laser 2 was broken %d times \n%s : %s : An object went in %d times \n%s : %s : An object went out %d times \n", timeVal, programName, laser1Count, time, programName, laser2Count, time, programName, numberIn, time, programName, numberOut); 
         fflush(statsFile);
      }
      else
      {
         count++;
      }
   }
   
   // Outputs the message that indicates the number of laser breaks, and number of objects in and out of the room
   outputMessage(laser1Count, laser2Count, numberIn, numberOut);
 
  
   //Writing a V to the watchdog file will disable to watchdog and prevent it from
	//resetting the system
   write(watchdog, "V", 1);
   getTime(timeVal);
	//Log that the Watchdog was disabled
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog was disabled\n\n");

	//Close the watchdog file so that it is not accidentally tampered with
   close(watchdog);
   getTime(timeVal);
	//Log that the Watchdog was closed
   PRINT_MSG(logFile, timeVal, programName, 2, "The Watchdog was closed\n\n");

   //Free the GPIO pins
   gpiolib_free_gpio(gpio);
   getTime(timeVal);
   
   //Log that the GPIO pins were freed
   PRINT_MSG(logFile, timeVal, programName, 2, "The GPIO pins have been freed\n\n");
   //Return to end program
   
   return 0;

}

#endif