Documentation for Warmup Assignment 2
=====================================

+-------+
| BUILD |
+-------+

Comments: ?
The command to generate warmup2 executable
	make 
Tohe command to execute warmup2 executable
	./warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]	
The command to clean up *.o files and executable
	make clean
	
+------+
| SKIP |
+------+

Is there are any tests in the standard test suite that you know that it's not
working and you don't want the grader to run it at all so you won't get extra
deductions, please list them here.  (Of course, if the grader won't run these
tests, you will not get plus points for them.)

NONE.

+---------+
| GRADING |
+---------+

Basic running of the code : ? out of 100 pts
100 pts

Missing required section(s) in README file : (Comments?)
NONE.

Cannot compile : (Comments?)
NONE.

Compiler warnings : (Comments?)
NONE.

"make clean" : (Comments?)
DONE.

Segmentation faults : (Comments?)
NONE.

Separate compilation : (Comments?)
DONE.

Using busy-wait : (Comments?)
NONE.

Handling of commandline arguments:
    1) -n : (Comments?)       	DONE
    2) -lambda : (Comments?)  	DONE
    3) -mu : (Comments?)      	DONE
    4) -r : (Comments?)			DONE
    5) -B : (Comments?)			DONE
    6) -P : (Comments?)			DONE
Trace output :
    1) regular packets: (Comments?)	DONE
    2) dropped packets: (Comments?)	DONE
    3) removed packets: (Comments?)	DONE
    4) token arrival (dropped or not dropped): (Comments?)	DONE
Statistics output :
    1) inter-arrival time : (Comments?)					   	DONE   
    2) service time : (Comments?)                          	DONE
    3) number of customers in Q1 : (Comments?)             	DONE
    4) number of customers in Q2 : (Comments?)             	DONE
    5) number of customers at a server : (Comments?)       	DONE
    6) time in system : (Comments?)                        	DONE
    7) standard deviation for time in system : (Comments?)	DONE
    8) drop probability : (Comments?)						DONE
Output bad format : (Comments?)	DONE
Output wrong precision for statistics (should be 6-8 significant digits) : (Comments?) NONE
Large service time test : (Comments?)	DONE
Large inter-arrival time test : (Comments?)	DONE
Tiny inter-arrival time test : (Comments?)	DONE
Tiny service time test : (Comments?)	DONE
Large total number of customers test : (Comments?)	DONE
Large total number of customers with high arrival rate test : (Comments?)	DONE
Dropped tokens test : (Comments?)	DONE
Cannot handle <Cntrl+C> at all (ignored or no statistics) : (Comments?) NONE
Can handle <Cntrl+C> but statistics way off : (Comments?) NONE.
Not using condition variables and do some kind of busy-wait : (Comments?) NONE.
Synchronization check : (Comments?) DONE
Deadlocks : (Comments?) NONE

+------+
| BUGS |
+------+

Comments: ?
NOT THAT I AM AWARE OF
+-------+
| OTHER |
+-------+

Comments on design decisions: ?
1. TRY TO MINIMIZE THE LINES OF CODES IN MAIN FUNCTION IS LESS THAN 10
2. HANDLE CTRL+C IN AN EXTRA THREAD
3. TRY TO USE FUNCTION INSTEAD OF REPEATED SAME PART OF CODE EVERYWHERE
LIKE THOSE FUNTIONS ARE CATEGORIZED UNDER Utility Function COMMENTS
4. TRY TO PUT EVERYTHING COMMON USE IN GLOBAL VARIABLE LIKE tokenBucketSystem
5. TRY TO REUSE CODE FROM WARMUP1 FOR READ ARGUMENT 
6. TRY TO WRITE SELF DEFINED ATOI, ATOF, SQRT 
Comments on deviation from spec: ?
SEEMS REASONABLE BECAUSE DEVIATION IS MOSTLY SMALLER THAN AVERAGE VALUE

