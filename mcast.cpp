
#include "sp.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>


static	char	User[80];
static  char    Spread_name[80];

static  char    Private_group[MAX_GROUP_NAME];
static  mailbox Mbox;
static	int	Num_sent;
static	unsigned int	Previous_len;

static  int     To_exit = 0;

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100

static	void	Read_message();
static  void	Bye();



int main(int argc, char * argv[])
{
    std::stringstream s1(argv[1]);
    std::stringstream s2(argv[2]);
    std::stringstream s3(argv[3]);

    int num_mes = 0; //number of messages
    int p_id = 0; //process id
    int num_proc = 0; //number of processes

    int ret = 0; 

    bool all_joined = false;  //did all processes join?
    bool all_finished = false; //did all processes finished?

    sp_time test_timeout;
    test_timeout.sec = 5;
    test_timeout.usec = 0;

    s1 >> num_mes;
    s2 >> p_id;
    s3 >> num_proc;
    sprintf( User, "user" );
	sprintf( Spread_name, "4803");


    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION ) 
	{
		SP_error( ret );
		Bye();
	 }
     std::cout << "Connected to Spread!" << std::endl;

     return 0;
}





static  void	Bye()
{
	To_exit = 1;

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}






