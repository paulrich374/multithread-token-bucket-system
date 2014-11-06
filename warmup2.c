#include <stdio.h>
#include<ctype.h>       /* classify and transform individual characters*/
#include <stdlib.h>
#include <pthread.h>
#include<sys/time.h>    /* struct timeval definition      */
#include <unistd.h> 	/* for sleep() */
#include "my402list.h"
#include "cs402.h"
#include <string.h>
#include<signal.h>		/* SIGINT */
#include <unistd.h>     /* declaration of gettimeofday()  */

#include <math.h>

 
#define N_TOTAL_PACKET_TO_ARRIVE    20  
#define LAMBA_PACKET_ARRIVAL_RATE   0.5	 
#define MU_PACKET_SERVICE_RATE      0.35
#define R_TOKEN_RATE                1.5   
#define B_TOKEN_BUCKET_DEPTH        10   
#define P_PACKET_NEEDED_TOKENS      3   


/* global variable */ 
typedef struct tokenBucketSystem {
    My402List list1, list2;           	/* Q1 and Q2 */
    int   packet_index, token_index;
    int   current_bucket_depth;
	int   current_served_packets;
    
    int   B_bucket_total_depth;		    /* bucket depth */
    int   P_packet_required_tokens;	    /* tokens that packet require  */
    int   n_total_number_of_packets;	/* total number of packets to arrive*/
    double lamba_packet_arrival_rate;	/* number of packet arrive per second */
    double mu_packet_service_rate;		/* number of packet served per second */
    double r_token_arrival_rate;			/* number of token arrive per second */
    char *tsfile;						/* trace specification file */
    FILE *file;						    /* trace specification file */
    int   mode;							/* Deterministic:0, Trace-driven:1 */
    
    double start_time, end_time;      /* in microseconds */
     
    double avg_packet_inter_arrival_time;	
	double avg_packet_service_time;
	double avg_number_of_packets_in_q1;
	double avg_number_of_packets_in_q2;	
	double avg_number_of_packets_at_s;
	double avg_time_packet_spent_in_system;
	double avg_standard_deviation_for_time_packet_spent_in_system;
	double avg_variance_for_time_packet_spent_in_system;
	double total_packet_spent_in_system;

	int removed_packets;		
	int dropped_packets;	
	int dropped_tokens;


} tokenBucketSystemParameters;


typedef struct packet {
    int   index;
    int   P_packet_required_tokens;	     
    int   lamba_packet_arrival_time;	
    int   mu_packet_service_time;		
    
    double  start_time, arrival_time;
} packetParameters;
 


pthread_t 		p_threads[4]; 
pthread_mutex_t request_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;

sigset_t new;             

tokenBucketSystemParameters *tbs;

int sigint_flag = 0;
int packet_thread_terminate_flag = 0;


/*--------------------------- Utility FunctionS ----------------------------*/
/* command line usage message*/
void Usage() {
	fprintf(stderr, "Usage: warmup2 [-lambda lambda] [-mu mu] [-r r] [-B B] [-P P] [-n num] [-t tsfile]\n");
   	exit(1);
}

/* print error message and exit out program */
void mal_format (char error_msg[]) {
	printf("error: Malformated input - %s\n ", error_msg);
	exit(1);
}

/* get current timestamp */
double getTime()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec*1000000 + tv.tv_usec;
}

/* struct tokenBucketSystemParameters constructor */
void tokenBucketSystemParametersInit(tokenBucketSystemParameters *tb1) {
	
    My402ListInit(&tb1->list1);
    My402ListInit(&tb1->list2);    
    tb1->packet_index 				= 0;
    tb1->token_index  				= 0;
    tb1->current_bucket_depth		= 0;
	tb1->current_served_packets 	= 0;

    tb1->n_total_number_of_packets 	= N_TOTAL_PACKET_TO_ARRIVE;
    tb1->lamba_packet_arrival_rate	= LAMBA_PACKET_ARRIVAL_RATE;	
    tb1->mu_packet_service_rate	    = MU_PACKET_SERVICE_RATE;		
    tb1->r_token_arrival_rate		= R_TOKEN_RATE;	         
    tb1->B_bucket_total_depth      	= B_TOKEN_BUCKET_DEPTH;
    tb1->P_packet_required_tokens  	= P_PACKET_NEEDED_TOKENS;
    
  
    
    
    tb1->tsfile						= NULL;						
    tb1->file						= 0;						    
    tb1->mode 						= 0;							
    
    tb1->start_time					= getTime(); 
    tb1->end_time				    = 0;
    
    tb1->avg_packet_inter_arrival_time 							= 0.0;
	tb1->avg_packet_service_time								= 0.0;
	tb1->avg_number_of_packets_in_q1							= 0.0;
	tb1->avg_number_of_packets_in_q2							= 0.0;	
	tb1->avg_number_of_packets_at_s								= 0.0;
	tb1->avg_time_packet_spent_in_system						= 0.0;
	tb1->avg_standard_deviation_for_time_packet_spent_in_system	= 0.0;	
	tb1->avg_variance_for_time_packet_spent_in_system			= 0.0;
	tb1->total_packet_spent_in_system                           = 0.0;
	
	tb1->removed_packets            = 0;       
	tb1->dropped_packets			= 0;	
	tb1->dropped_tokens				= 0;
	
}

/* check command line argument is positive real or not*/
int isPositiveReal(char *str){
	int inDecimal = 0;
	char *start;
    if(str == NULL){
        return 0;
    }
    start = str;
    if(*start == '-'){
		return 0;
    }	
    while(*start != '\0'){
		if (*start == '.') {
        	inDecimal += 1;
        } else if( !(*start >= '0' && *start <= '9') ){
        	return 0;
        }  
        start++;
    }	
    /* more than one decimal point */
	if (inDecimal > 1) return 0;    
	return 1;
}

/* check command line argument is positive integer or not*/
int isPositiveInteger(char *str){
    char *start;
    if(str == NULL){
        return 0;
    }
    start = str;	
    if(*start == '-'){
		return 0;
    }    
    while(*start != '\0'){
        if( !(*start >= '0' && *start <= '9') ) 
        	return 0;
        start++;
    }	
    return 1;
}

/* string to integer */
int tbs_atoi(char *str){
    int negative = 0;
    int value    = 0;
    char *start;
 
    if(str == NULL){
        return 0;
    }
    start = str;
    /* ignoring space */
    while(*start == ' ')
        start++;
    /* add - for neg */ 
    if(*start == '-'){
        negative = 1;
        start++;
    }
 
    while(*start != '\0'){
        if(*start >= '0' && *start <= '9'){
            value = value*10 + (int)*start - '0';
        }
        start++;
    }
    if(negative){
        value = -1 * value;
    }
    return value;
}

/* string to double */
double tbs_atof(char *str) {
	int negative   			= 0;
	int inDecimal 			= 0;
	int exponent 			= 1;
    double value    			= 0;
    char *start;
	
    if(str == NULL){
        return 0;
    }
    start = str;
    /* ignoring space */
    while(*start == ' ')
        start++;
    /* add - for neg */ 
    if(*start == '-'){
        negative = 1;
        start++;
    }	
    while(*start != '\0'){
        if(*start >= '0' && *start <= '9'){
            value = value*10 + (int)*start - '0';
            /* how many decimal digit */
            if (inDecimal) exponent*=10; 
        } else if (*start == '.') {
        	inDecimal = 1;
        }
        start++;
    }
    value = value / exponent;
    if(negative){
        value = -1 * value;
    }
    return value;	
}

/* returns the square root of value. Note that the function might have infinte loop so set up a threshold of loop count*/
double tbs_squareRoot(double value) {
  /*We are using n itself as initial approximation
   This can definitely be improved */
  /* in case infinite run, we set up 30 run as a limit*/
  int count = 0;
  double root1 = value;
  double root2 = 1;
  double e = 0.000001; /* e decides the accuracy level*/
  while( root1 - root2 > e && count < 30) {
  	count++;
    root1 = (root1 + root2) / 2;
    root2 = value / root1;
  }
  return root1;
} 

/* read command line arguments */
void readArgv(tokenBucketSystemParameters *tbs1, int argc, char *argv[]) {
	int i;
	/* for detecting duplicate argument */
	int bit = 0;
	/* only one argument - warmup2 */
	if (argc == 1) return;
	/* multiple arguments involved */		 
	for ( i = 1; i < argc; i+=2 ) {
		if(argv[i][0] != '-') Usage();	
		if ( strcmp(argv[i], "-lambda") == 0 ) {
			if(bit & 1) mal_format("duplicate -lambda.\n");
			bit |= 0x01;
			if( isPositiveReal(argv[i+1]) == 0 ) mal_format("lambda suppose to have a positive real number.\n");
			tbs1->lamba_packet_arrival_rate = tbs_atof(argv[i+1]);
		} else if ( strcmp(argv[i], "-mu") == 0  ) {
			if(bit & 2) mal_format("duplicate -mu.\n");
			bit |= 0x02;
			if( isPositiveReal(argv[i+1]) == 0 ) mal_format("mu suppose to have a positive real number.\n");
			tbs1->mu_packet_service_rate = tbs_atof(argv[i+1]);				
		} else if ( strcmp(argv[i], "-r") == 0  ) {	
			if(bit & 4) mal_format("duplicate -r.\n");
			bit |= 0x04;
			if( isPositiveReal(argv[i+1]) == 0 ) mal_format("r suppose to have a positive real number.\n");
			tbs1->r_token_arrival_rate = tbs_atof(argv[i+1]);			
		} else if ( strcmp(argv[i], "-B") == 0  ) {				
			if(bit & 8) mal_format("duplicate -B.\n");
			bit |= 0x08;
			if( isPositiveInteger(argv[i+1]) == 0 ) mal_format("B suppose have a positive integer.\n");
			tbs1->B_bucket_total_depth = tbs_atoi(argv[i+1]);
			if(tbs1->B_bucket_total_depth > 2147483647) mal_format("B maximum value 2147483647.\n");		
		} else if ( strcmp(argv[i], "-P") == 0  ) {
			if(bit & 16) mal_format("duplicate -P.\n");
			bit |= 0x10;
			if( isPositiveInteger(argv[i+1]) == 0 ) mal_format("P suppose have a positive integer.\n");
			tbs1->P_packet_required_tokens = tbs_atoi(argv[i+1]);
			if(tbs1->P_packet_required_tokens > 2147483647) mal_format("P maximum value 2147483647.\n");		
		} else if ( strcmp(argv[i], "-n") == 0  ) {				
			if(bit & 32) mal_format("duplicate -n.\n");
			bit |= 0x20;
			if( isPositiveInteger(argv[i+1]) == 0 ) mal_format("n suppose have a positive integer.\n");
			tbs1->n_total_number_of_packets = tbs_atoi(argv[i+1]);	
			if(tbs1->n_total_number_of_packets > 2147483647) mal_format("n maximum value 2147483647.\n");		
		} else if ( strcmp(argv[i], "-t") == 0  ) {
			tbs1->tsfile = argv[i+1];		
			tbs1->file = fopen(argv[i+1], "r"); 
			if ( !tbs1->file ) mal_format("Could not open file.\n"); 
			char line[80];
			/* get first line of file */
			fgets( line, sizeof(line), tbs1->file );
			line[strlen(line)-1] = '\0';
			/* check empty file */
			if ( line[0] < 32 ) mal_format("Empty File: Should have at least one transaction.");
			if( isPositiveInteger(line) == 0 ) mal_format("suppose to have a positive integer.\n");
			tbs1->n_total_number_of_packets = tbs_atoi(line);
			if(tbs1->n_total_number_of_packets > 2147483647) mal_format("n maximum value 2147483647.\n");	
			tbs1->mode = 1;				
		} else {
			Usage();	
		}
	}
}

/* get and set up struct packet parameters from each line of file */
void getPacketParametersFromLine (packetParameters *pkt, tokenBucketSystemParameters *tbs) {

	if (tbs->mode == 1) {

		char line[80];
		fgets( line, sizeof(line), tbs->file );
		/* init a char 2d array to store splitted string */
		char sub_string[3][13];
		int count_subString = 0;
		int count_line_characters = 0;
		int count_subString_characters = 0;

		while (count_subString < 3) {
			if(line[count_line_characters] != ' ' && line[count_line_characters] != '\t' && line[count_line_characters] != '\n') {	/* save characters one by one till hit break */
					sub_string[count_subString][count_subString_characters] = line[count_line_characters];
					count_line_characters++;
					count_subString_characters++;
			} else {
					/* add terminates character at the end of each sub string NOTE: when character start non-number */
					sub_string[count_subString][count_subString_characters] = '\0';								
					/* keep looping out of all non-nnumber characters*/
					while (	line[count_line_characters] == ' ' || line[count_line_characters] == '\t')
							count_line_characters++;
					/* start a new sub string read in*/		
					count_subString++;
					count_subString_characters = 0;
    		}
		}		
		
		if( isPositiveInteger(sub_string[0]) == 0 ) mal_format("1/lamba suppose have a positive integer.\n");
		tbs->lamba_packet_arrival_rate = tbs_atof(sub_string[0]);
		if(tbs->lamba_packet_arrival_rate > 2147483647) mal_format("1/lamba maximum value 2147483647.\n");		
		if( isPositiveInteger(sub_string[1]) == 0 ) mal_format("P suppose have a positive integer.\n");
		tbs->P_packet_required_tokens = tbs_atoi(sub_string[1]);
		if(tbs->P_packet_required_tokens > 2147483647) mal_format("P maximum value 2147483647.\n");			
		if( isPositiveInteger(sub_string[2]) == 0 ) mal_format("1/Mu suppose have a positive integer.\n");
		tbs->mu_packet_service_rate = tbs_atof(sub_string[2]);
		if(tbs->mu_packet_service_rate > 2147483647) mal_format("1/Mu maximum value 2147483647.\n");			
	
		pkt->lamba_packet_arrival_time = tbs->lamba_packet_arrival_rate;
		pkt->P_packet_required_tokens  = tbs->P_packet_required_tokens;
		pkt->mu_packet_service_time    = tbs->mu_packet_service_rate;		
	} else {
		pkt->lamba_packet_arrival_time = (1000.0f/tbs->lamba_packet_arrival_rate > 10000.0f) ?10000.0f:1000.0f/tbs->lamba_packet_arrival_rate;
		pkt->P_packet_required_tokens  = tbs->P_packet_required_tokens;
		pkt->mu_packet_service_time    = (1000.0f/tbs->mu_packet_service_rate > 10000.0f)?10000.0f:1000.0f/tbs->mu_packet_service_rate;
	}
}

/* print header start up message */
void printHeaderMessage (tokenBucketSystemParameters *tbs1) {
	

	if(tbs1->mode == 1){
		printf("Emulation Parameters: \n");
		printf("     number to arrive = %d\n"	,tbs1->n_total_number_of_packets);						
		printf("     r = %.6g\n"     			,tbs1->r_token_arrival_rate);
		printf("     B = %d\n"       			,tbs1->B_bucket_total_depth);
		printf("     tsfile = %s\n\n"			,tbs1->tsfile);		
	}else{
		printf("\nEmulation Parameters: \n");
		printf("     number to arrive = %d\n"	,tbs1->n_total_number_of_packets);				
		printf("     lambda = %.6g\n"			,tbs1->lamba_packet_arrival_rate);
		printf("     mu = %.6g\n"				,tbs1->mu_packet_service_rate);
		printf("     r = %.6g\n"				,tbs1->r_token_arrival_rate);
		printf("     B = %d\n"					,tbs1->B_bucket_total_depth);
		printf("     P = %d\n\n"				,tbs1->P_packet_required_tokens);
	}
    /* start the system */
    printf("%012.3fms: emulation begins\n", 0.0f);
}

/* print footer statistics message */
void printStatistics(tokenBucketSystemParameters *tbs) {
	
	double square_avg_time_packet_spent_in_system;

	
	printf("\nStatistics: \n\n");
	
	
	if ( tbs->packet_index == 0 ) 		
		printf("average packet inter-arrival time  = %s\n", " N.A. as no packet arrived");
	else 
		printf("average packet inter-arrival time = %.6gs\n",(tbs->avg_packet_inter_arrival_time / tbs->packet_index) / 1000000);
			
	if(tbs->current_served_packets == 0 || tbs->packet_index == 0)	
		printf("average packet service time  = %s\n\n", " N.A. as no packet at the server");
	else
		printf("average packet service time = %.6gs\n\n",(tbs->avg_packet_service_time / tbs->current_served_packets) / 1000000);

	if ( tbs->avg_number_of_packets_in_q1 == 0.0 ) 
		printf("average number of packets in Q1  = %s\n", " N.A. as no packet in Q1");				
	else
		printf("average number of packets in Q1 = %.8g\n",tbs->avg_number_of_packets_in_q1 / (tbs->end_time)); 
		
	if ( tbs->avg_number_of_packets_in_q2 == 0.0 ) 		
		printf("average number of packets in Q2  = %s\n", " N.A. as no packet in Q2");				
	else	
		printf("average number of packets in Q2 = %.6g\n",tbs->avg_number_of_packets_in_q2 / (tbs->end_time));
 	
		
	if(tbs->current_served_packets == 0 || tbs->packet_index == 0)		
		printf("average number of packets at S  = %s\n\n", " N.A. as no packet at the server");
	else
		printf("average number of packets at S  = %.6g\n\n",tbs->avg_packet_service_time / (tbs->end_time));	
	
	if(tbs->current_served_packets == 0 || tbs->packet_index == 0)	
		printf("average time a packet spent in system  = %s\n", " N.A. as no packet served in the system");
	
	else
		printf("average time a packet spent in system = %.6gs\n",(tbs->avg_time_packet_spent_in_system / tbs->current_served_packets) / 1000000);
	

	if(tbs->current_served_packets == 0 || tbs->packet_index == 0)	
		printf("standard deviation for time spent in system  = %s\n\n", " N.A. as no packet served in the system");
	else {			
		square_avg_time_packet_spent_in_system = (tbs->avg_time_packet_spent_in_system / tbs->current_served_packets)*(tbs->avg_time_packet_spent_in_system / tbs->current_served_packets);		
		tbs->total_packet_spent_in_system = tbs->total_packet_spent_in_system / tbs->current_served_packets;
		tbs->avg_standard_deviation_for_time_packet_spent_in_system = sqrt( tbs->total_packet_spent_in_system - square_avg_time_packet_spent_in_system ); 
		printf("standard deviation for time spent in system = %.6gs\n\n",(tbs->avg_standard_deviation_for_time_packet_spent_in_system / 1000000));
	}
	
	if ( tbs->token_index == 0 ) 		
		printf("token drop probability = %s\n", "N.A. as no token arrived" );
	else	
		printf("token drop probability = %.6g\n"   ,((double)tbs->dropped_tokens / tbs->token_index));
	if ( tbs->packet_index == 0 ) 	
		printf("packet drop probability = %s\n\n","N.A. as no packet arrived");				
	else	
		printf("packet drop probability = %.6g\n\n",((double)tbs->dropped_packets / tbs->packet_index));	
}





/*----------------- Token Bucket System 4 threads  --------------------*/


void *packet_arrival(void *tbs){
	


	double current_time_in_system;
	double last_packet_arrival_time = 0;
	
	tokenBucketSystemParameters *tbs1;
	packetParameters *pkt1;
	packetParameters *pkt_temp;
	
	My402ListElem *first_elem;
	void *curr_obj;
	int number_of_items_in_Q1;	
	int number_of_items_in_Q2;	
	
	tbs1 = (tokenBucketSystemParameters*)tbs;
	int i;

	for(i = 0;i < tbs1->n_total_number_of_packets;i++){
				
		/* catch ctrl+c */
		if (sigint_flag == 1) return 0;
		
		
		pkt1 = (struct packet*) malloc(sizeof(struct packet));
		
		/* assign lamba, P, mu to each single packet */ 
		getPacketParametersFromLine(pkt1, tbs1);
		
		
		
		
		/* sleep for a inter-arrival time */
		usleep( 1000.0f*pkt1->lamba_packet_arrival_time);



		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs1->start_time;
		
		/* lock mutex */
		pthread_mutex_lock(&request_mutex);
				
		
		/* enqueue to Q1 and increment packet count */
		tbs1->packet_index++;		
		pkt1->index = tbs1->packet_index;
		
		
		pkt1->arrival_time = current_time_in_system; 
		
		/* statitics - */
		tbs1->avg_packet_inter_arrival_time += pkt1->arrival_time - last_packet_arrival_time;
		
		/*check if total token numbers is over bucket depth */
		if ( pkt1->P_packet_required_tokens > tbs1->B_bucket_total_depth ) {
			tbs1->dropped_packets++;
        	printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms, dropped\n", (double)current_time_in_system/1000.0f, tbs1->packet_index, pkt1->P_packet_required_tokens, (double)(pkt1->arrival_time - last_packet_arrival_time)/1000.0f);			
					
					
			/*  packet thread terminate case */ 
			if ( ((int)tbs1->dropped_packets ) == tbs1->n_total_number_of_packets ) {
				pthread_mutex_unlock(&request_mutex);				
				packet_thread_terminate_flag = 1;
		    	return 0;
		    }
		    	
		    pthread_mutex_unlock(&request_mutex);			
			/* update last pakcet arrival time */
			last_packet_arrival_time = current_time_in_system;
						
			continue;
		} else {
			My402ListAppend(&tbs1->list1, (void*)pkt1);
			printf("%012.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n", (double)current_time_in_system/1000.0f, tbs1->packet_index, pkt1->P_packet_required_tokens, (double)(pkt1->arrival_time - last_packet_arrival_time)/1000.0f);		
			
			/* update last pakcet arrival time */
			last_packet_arrival_time = current_time_in_system;		
		}	
		

			

		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs1->start_time;
		pkt1->start_time = current_time_in_system;
		printf("%012.3fms: p%d enters Q1\n", (double)current_time_in_system/1000.0f, pkt1->index);
	
		
		/* check if we can move first packet from Q1 to Q2 */
		number_of_items_in_Q1 = My402ListLength(&tbs1->list1);
		if ( number_of_items_in_Q1 > 0 ) {
			first_elem = My402ListFirst(&tbs1->list1);
			curr_obj   = first_elem->obj;
			pkt_temp   = (packetParameters*)curr_obj;
			
			/* check if pkt required tokens is less than bucket number */
			if ( pkt_temp->P_packet_required_tokens  <= tbs1->current_bucket_depth) {
				
				/* move packet out from Q1 */
				My402ListUnlink(&tbs1->list1, first_elem);
				
				/* decrease token number */	
				tbs1->current_bucket_depth-=pkt_temp->P_packet_required_tokens;
			
				/* get current time in system - when action happens */
				current_time_in_system = getTime() - tbs1->start_time;
				printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d tokens\n", (double)current_time_in_system/1000.0f, pkt_temp->index, (double)(current_time_in_system - pkt_temp->start_time)/1000.0f, tbs1->current_bucket_depth);
                            
                /*statistics */            
                tbs1->avg_number_of_packets_in_q1 += current_time_in_system - pkt_temp->start_time;            
                            
				/* Insert pkt to the list2 (Q2) */
				My402ListAppend(&tbs1->list2, curr_obj);
	
				/* get current time in system - when action happens */
				current_time_in_system = getTime() - tbs1->start_time;
				/* re-timestamp time as start time in Q2 */
               	pkt_temp->start_time = current_time_in_system;					
                printf("%012.3fms: p%d enters Q2\n", (double)current_time_in_system/1000.0f, pkt_temp->index);
                
				/* length of Q2 */ 		
				number_of_items_in_Q2 = My402ListLength(&tbs1->list2);
								
				/* Q2 empty before, need to signal*/
				if ( number_of_items_in_Q2 == 1) {
					pthread_cond_signal(&got_request);
				}/* end of check number_of_items_in_Q2 */
											
			}/* end of check P_packet_required_tokens */
			
		}/* end of check number_of_items_in_Q1 */

		/* unlock mutex */
		pthread_mutex_unlock(&request_mutex);

	}/* end of for */
	
	return (void*)0;
}

void *token_depositing(void *tbs){


	double current_time_in_system;
	tokenBucketSystemParameters *tbs3;
	
	
	packetParameters *pkt3;
	My402ListElem *first_elem;
	void *curr_obj;
	int number_of_items_in_Q1;	
	int number_of_items_in_Q2;
	
	
	tbs3 = (tokenBucketSystemParameters*)tbs;
	
	
	
	while ( 1 ){
		/* catch ctrl+c */	
		if (sigint_flag) return 0;
		/* catch packet thread terminate case */ 
		if ( tbs3->packet_index == tbs3->n_total_number_of_packets && My402ListLength(&tbs3->list1) == 0 ) {
			
			packet_thread_terminate_flag = 1;
			pthread_cond_signal(&got_request);
			return 0;	
		}
		
		/* sleep for a inter-arrival time */
		usleep(1000000.0f/tbs3->r_token_arrival_rate);
		
		/* lock mutex */
		pthread_mutex_lock(&request_mutex);
		
		/* increment token count */
		tbs3->current_bucket_depth++;
		tbs3->token_index++;
		
		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs3->start_time;
		
		/*check if total token numbers is over bucket depth */
		if ( tbs3->current_bucket_depth > tbs3->B_bucket_total_depth ) {
			tbs3->current_bucket_depth = tbs3->B_bucket_total_depth;
			tbs3->dropped_tokens++;
			printf("%012.3fms: token t%d arrives, dropped\n", (double)current_time_in_system/1000.0f, tbs3->token_index);

		} else {
			if ( tbs3->current_bucket_depth == 1 ) {
				printf("%012.3fms: token t%d arrives, token bucket now has %d token\n", (double)current_time_in_system/1000.0f, tbs3->token_index, tbs3->current_bucket_depth);
			} else {
				printf("%012.3fms: token t%d arrives, token bucket now has %d tokens\n", (double)current_time_in_system/1000.0f, tbs3->token_index, tbs3->current_bucket_depth);
			}
		}


		
		/* check if we can move first packet from Q1 to Q2 */
		number_of_items_in_Q1 = My402ListLength(&tbs3->list1);
		if ( number_of_items_in_Q1 > 0 ) {
			
			
			
			first_elem = My402ListFirst(&tbs3->list1);
			curr_obj   = first_elem->obj;
			pkt3       = (packetParameters*)curr_obj;
			
			/* check if pkt required tokens is less than bucket number */
			if ( pkt3->P_packet_required_tokens  <= tbs3->current_bucket_depth) {
				
				
				
				/* move packet out from Q1 */
				My402ListUnlink(&tbs3->list1, first_elem);
				
				/* decrease token number */	
				tbs3->current_bucket_depth-=pkt3->P_packet_required_tokens;
			
				/* get current time in system - when action happens */
				current_time_in_system = getTime() - tbs3->start_time;
				printf("%012.3fms: p%d leaves Q1, time in Q1 = %.3fms, token bucket now has %d tokens\n", (double)current_time_in_system/1000.0f, pkt3->index, (double)(current_time_in_system - pkt3->start_time)/1000.0f, tbs3->current_bucket_depth);
                            
                /*statistics */            
                tbs3->avg_number_of_packets_in_q1 += current_time_in_system - pkt3->start_time;                                            
                            
				/* Insert pkt to the list2 (Q2) */
				My402ListAppend(&tbs3->list2, curr_obj);
	
				/* get current time in system - when action happens */
				current_time_in_system = getTime() - tbs3->start_time;
				/* re-timestamp time as start time in Q2 */
               	pkt3->start_time = current_time_in_system;				
                printf("%012.3fms: p%d enters Q2\n", (double)current_time_in_system/1000.0f, pkt3->index);

               			
				/* length of Q2 */ 		
				number_of_items_in_Q2 = My402ListLength(&tbs3->list2);
								
				/* Q2 empty before, need to signal*/
				if ( number_of_items_in_Q2 == 1) {
					pthread_cond_signal(&got_request);
				}/* end of check number_of_items_in_Q2 */
											
			}/* end of check P_packet_required_tokens */
			
		}/* end of check number_of_items_in_Q1 */

		
		/* unlock mutex */
		pthread_mutex_unlock(&request_mutex);
		
	}// end of while


	
	return (void*)0;
}

void *server(void *tbs){
	
	
	
	double current_time_in_system;
	tokenBucketSystemParameters *tbs2;
	packetParameters *pkt2;

	My402ListElem *first_elem_Q2;
	void *curr_obj;		
	int number_of_items_in_Q2;		
	double sleep_time;
	double total_packet_spent_in_system_precalculation;
	
			
	
	tbs2 = (tokenBucketSystemParameters*)tbs;
	int k;


		
	for(k = 0;k < tbs2->n_total_number_of_packets;k++){

		/* lock mutex */
		pthread_mutex_lock(&request_mutex);

		
		
		/* length of Q2 */ 		
		number_of_items_in_Q2 = My402ListLength(&tbs2->list2);
						
		/* Q2 empty, need to wait*/
		if ( number_of_items_in_Q2 == 0) {
			pthread_cond_wait(&got_request, &request_mutex);
		}	
		
		/* catch ctrl+c */
		if ( sigint_flag == 1 ) {
			/* unlock mutex */
			pthread_mutex_unlock(&request_mutex);		    
		    return 0;
    	}
    	
    	/* catch packet thread terminate case */
		if( packet_thread_terminate_flag == 1 ){
			if(My402ListEmpty(&tbs2->list2)){
				pthread_mutex_unlock(&request_mutex);
				return 0;
			}
		}    			
				
		/* fetch first packet in Q2 */
		first_elem_Q2 = My402ListFirst(&tbs2->list2);
		curr_obj   = first_elem_Q2->obj;
		pkt2   = (packetParameters*)curr_obj;


		
		/**/
		sleep_time = pkt2->mu_packet_service_time;
		

		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs2->start_time;		
        printf("%012.3fms: p%d leaves Q2, time in Q2 =  %.3fms\n", (double)current_time_in_system/1000.0f , pkt2->index, (double)(current_time_in_system - pkt2->start_time)/1000.0f);		
					
        /*statistics */            
       	tbs2->avg_number_of_packets_in_q2 += current_time_in_system - pkt2->start_time; 

		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs2->start_time;	
        pkt2->start_time = current_time_in_system;					
        printf("%012.3fms: p%d begins service as S, requesting %.3fms of service\n", (double)current_time_in_system/1000.0f , pkt2->index, (double)pkt2->mu_packet_service_time);		
					
		/* lock mutex */
		pthread_mutex_unlock(&request_mutex);
		
		
							
		/* sleep for a inter-arrival time */
		usleep( 1000.0f*sleep_time );			
		
		
		/* lock mutex */
		pthread_mutex_lock(&request_mutex);		
		
		
		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs2->start_time;		
        printf("%012.3fms: p%d departs from S, service time = %.3fms time in system = %.3fms\n", (double)current_time_in_system / 1000.0f , pkt2->index, (double)(current_time_in_system - pkt2->start_time)/1000.0f, (double)(current_time_in_system - pkt2->arrival_time)/1000.0f);
		
		/*statistics*/
		tbs2->avg_packet_service_time += current_time_in_system - pkt2->start_time;		
		tbs2->avg_time_packet_spent_in_system += current_time_in_system - pkt2->arrival_time;
		total_packet_spent_in_system_precalculation = ((double)(current_time_in_system - pkt2->arrival_time))*((double)(current_time_in_system - pkt2->arrival_time));
		tbs2->total_packet_spent_in_system += total_packet_spent_in_system_precalculation;		
		
		/* increase served packets */
		tbs2->current_served_packets++;
		
		/* remove pakcet from Q2 */
		My402ListUnlink(&tbs2->list2, My402ListFirst(&tbs2->list2));		
		
		/* end the system */
		if (pkt2->index == tbs2->n_total_number_of_packets || tbs2->dropped_packets == tbs2->n_total_number_of_packets || sigint_flag ) {
			

		    
			/* unlock mutex */
			pthread_mutex_unlock(&request_mutex);		    
		    
		    return 0;
    	}
		/* unlock mutex */
		pthread_mutex_unlock(&request_mutex);    	
	
	}
	
	return (void*)0;
}

void *handler(void *tbs) {
	
	sigwait(&new);	
	double current_time_in_system;
	tokenBucketSystemParameters *tbs_sigint;
	tbs_sigint = (tokenBucketSystemParameters*)tbs;


	sigint_flag = 1;


	pthread_cancel(p_threads[1]);						
	pthread_cancel(p_threads[2]);	
	

	/* lock mutex */
	pthread_mutex_lock(&request_mutex);	



	if ( My402ListLength(&tbs_sigint->list2) == 0) {
		pthread_cond_signal(&got_request);	
	}
	
	
	
	while ( My402ListLength(&tbs_sigint->list1) != 0 ) {		
		
		packetParameters *pkt;
		My402ListElem *first_elem_Q1;
		void *curr_obj;	
		
		/* fetch first packet in Q2 */
		first_elem_Q1 = My402ListFirst(&tbs_sigint->list1);
		curr_obj   = first_elem_Q1->obj;
		pkt   = (packetParameters*)curr_obj;
		
		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs_sigint->start_time;				
		
		printf("%012.3fms: p%d removed from Q1\n", (double)current_time_in_system / 1000.0f , pkt->index);
		
		My402ListUnlink(&tbs_sigint->list1, first_elem_Q1);	
		tbs_sigint->removed_packets++;					
	}

	/* unlock mutex */
	pthread_mutex_unlock(&request_mutex);	
		
	return (void*)0;

}

void *thread_managing (void *tbs){
	
	double current_time_in_system;
	tokenBucketSystemParameters *tbs_thread;
	tbs_thread = (tokenBucketSystemParameters*)tbs;
	
	
	/* catch cntrl+c  - parent thread */
	sigemptyset(&new);    									
	sigaddset(&new, SIGINT);								
	pthread_sigmask(SIG_BLOCK, &new, NULL);		
	pthread_create(&p_threads[0], NULL, handler, tbs_thread);	
	
	/* normal thread initialization*/
	pthread_create(&p_threads[1], NULL, packet_arrival, tbs_thread);	
	pthread_create(&p_threads[2], NULL, token_depositing, tbs_thread);		
	pthread_create(&p_threads[3], NULL, server, tbs_thread);	
	

	/* wait to each other thread complete */
	pthread_join(p_threads[1],0);
	pthread_join(p_threads[2],0);
	pthread_join(p_threads[3],0);		
		
	
	/* remove packet from Q2, which are not being served */	
	while ( My402ListLength(&tbs_thread->list2) != 0 ) {		
				
		packetParameters *pkt22;
		My402ListElem *first_elem_Q22;
		void *curr_obj2;	
		
		/* fetch first packet in Q2 */
		first_elem_Q22 = My402ListFirst(&tbs_thread->list2);
		curr_obj2   = first_elem_Q22->obj;
		pkt22   = (packetParameters*)curr_obj2;
		
		/* get current time in system - when action happens */
		current_time_in_system = getTime() - tbs_thread->start_time;				
		
		printf("%012.3fms: p%d removed from Q2\n", (double)current_time_in_system / 1000.0f , pkt22->index);
		
		My402ListUnlink(&tbs_thread->list2, first_elem_Q22);	
		tbs_thread->removed_packets++;					
	}		
	
	current_time_in_system = getTime() - tbs_thread->start_time;	
    tbs_thread->end_time = current_time_in_system;						
    
    printf("%012.3fms: emulation ends\n", (double)current_time_in_system/1000.0f );
    
    
    
    pthread_cancel(p_threads[1]);						/* no more packet service in Q2*/
	pthread_cancel(p_threads[2]);						/* no more token in*/
	pthread_cancel(p_threads[3]);						/* no more packet in*/	
	
	return (void*) 0;
}

int main(int argc, char* argv[]) {
	

	/* initialize tokenBucketSystemParameters instance */
	tbs = (struct tokenBucketSystem*) malloc(sizeof(struct tokenBucketSystem));
	tokenBucketSystemParametersInit(tbs);
	
	/* read command line argument*/
	readArgv(tbs, argc, argv);
	
	/* print start up message */
	printHeaderMessage(tbs);
	
	/* managing thread live and die */
	thread_managing(tbs);

	/* print statistics */
	printStatistics(tbs);
	
	return 0;
}

