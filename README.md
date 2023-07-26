# bridge_traffic_simulator
A C program that simulates traffic over a two-lane bridge.

Compilation steps:

1) Verify the file exists in the form of a .c
2) In the terminal, go to the directory that holds the file
3) Type the following into the terminal: gcc -pthread -o DESIREDFILENAME FILENAME.c -Wall -Werror
4) Following that command, type ./DESIREDFILENAME to execute the file

During the execution of the file, follow these steps:

*ALL INTEGERS MUST BE NON-NEGATIVE*
1) The first prompt will ask how many groups you're going to run. Type the integer and Enter.
2) The second prompt will repeat for each group:
	a) Type the amount of vehicles for the group.
	b) Type the probability of Northbound vehicles, from 0-100 inclusive. No decimals.
	c) Type the delay time. 
3) The terminal will now display information as the bridge simulations persist.
4) To run another schedule, repeat step 4 of compilation and the execution steps above.


If a decimal is entered during the input process, an input may be incorrectly read.
If a negative integer is entered, or any non-integer value, the program will not run.
