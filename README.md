Part of our ECE150 Final Project, where we implemented C code with our Raspberry Pi to perform basic motion-sensing with lasers.

Using a State Machine, configuration file, log file, and a Watchdog, we were able to monitor the current status of a couple of lasers, and detect whether objects successfully pass through it, going 'in' or 'out'. 

The project comes with a config.cfg, statsFile.cfg and a log.cfg. The config file will be the file controlled by the user in which the location of the log and stats file are inputted (or the path in the Pi) and also the timeout duration for the Watchdog. Note that the stats and log files should be created in the Pi itself.

The stats file will output how many times each laser is broken, and number of objects which have entered or exited. It will be documented when the program halts (which will only occur forcibly). 

The log file will be the file that will periodically appended with messages indicating any type of change. Examples of 'logs' that could occur are when the Watchdog is booted up, when a laser is broken, or when an object passes through successfully.

The Watchdog, however not completely implemented, will be 'kicked' (or otherwise pinged by the program) in order to keep the program running. If it did not receive a 'kick' in the prescribed amount of time as discussed in the config.cfg file, then it will deem the Raspberry Pi as non-functional and cease the program. Otherwise, it will allow the Pi to run indefinitely until something drastic occurs. Further intended purposes with the Watchdog are to reset it normally after it has been halted.

On the Raspberry Pi, it has been implemented that the program is to be run like a 'service', and therefore will also run on start up. 

-----------------------------------------------------------------------------------------------------------------------------------------------

The motion detecting system is simply to have lasers adjcacent to one another, such that their proximity is close enough that a normal object could 'block' both lasers simultaneously when passing. The lasers are paired each with a photodiode, which will send an electric signal to the dedicated pins to indicate that it is 'on', and otherwise off when not receving the light stimulus (i.e. being blocked by an object). 

The algorithm used is a State Machine that would determine any blocking is occurring, and correlate that with previous states to determine if an object has passed through. An example of this is, if the left laser is broken, then the right laser is broken, then the left laser is unbroken and then right laser is unbroken, this can be seen as the breaking of both lasers once, and an item as passed through the objects successfully once. 
