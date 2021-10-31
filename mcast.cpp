#include "sp.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <sys/time.h>

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100

static	char	User[80];
static  char    Spread_name[80];
static  char    Private_group[MAX_GROUP_NAME];
static  char    group[80];
static  mailbox Mbox;
static	int	Num_sent;
static	unsigned int	Previous_len;
static  int     To_exit = 0;

static  void	Bye();
long long diff_ms(timeval, timeval);
void get_performance(const struct timeval&);

struct Message{
    
}

int main(int argc, char * argv[])
{
    std::stringstream s1(argv[1]);
    std::stringstream s2(argv[2]);
    std::stringstream s3(argv[3]);

    int num_mes = 0; //number of messages
    int p_id = 0; //process id
    int num_proc = 0; //number of processes

    s1>>num_mes;
    s2>>p_id;
    s3>>num_proc;

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
    sprintf( group, "chkjjl_group");

    // received message
    static	char	 mess[MAX_MESSLEN];
    char	 sender[MAX_GROUP_NAME];
    char	 target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
    membership_info  memb_info;
    vs_set_info      vssets[MAX_VSSETS];
    unsigned int     my_vsset_index;
    int      num_vs_sets;
    char     members[MAX_MEMBERS][MAX_GROUP_NAME];
    int		 num_groups;
    int		 service_type = 0;
    int16	 mess_type;
    int		 endian_mismatch;
    int		 i,j;
    struct timeval started_timestamp;

    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION ) 
	{
		SP_error( ret );
		Bye();
	 }
    std::cout << "Connected to Spread!" << std::endl;
    std::cout << "Mbox from Spread is : " << Mbox << std::endl;

    // join group
    ret = SP_join( Mbox, group );
    if( ret < 0 ) SP_error( ret );

    while(!all_finished) {

        // receive
        ret = SP_receive( Mbox, &service_type, sender, 100, &num_groups, target_groups,
                          &mess_type, &endian_mismatch, sizeof(mess), mess );
        if( ret < 0 )
        {
            if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
                service_type = DROP_RECV;
                printf("\n========Buffers or Groups too Short=======\n");
                ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,
                                  &mess_type, &endian_mismatch, sizeof(mess), mess );
            }
        }
        if (ret < 0 )
        {
            if( ! To_exit )
            {
                SP_error( ret );
                printf("\n============================\n");
                printf("\nBye.\n");
            }
            exit( 0 );
        }
        if( Is_regular_mess( service_type ) )
        {
            std::cout << "received regular messages." << std::endl;
            // TODO: complete this
        }else if( Is_membership_mess( service_type ) )
        {
            std::cout << "received membership message from group " << sender << std::endl;
            ret = SP_get_memb_info( mess, service_type, &memb_info );
            if (ret < 0) {
                printf("BUG: membership message does not have valid body\n");
                SP_error( ret );
                exit( 1 );
            }
            if ( Is_reg_memb_mess( service_type ) )
            {
                printf("Received REGULAR membership for group %s with %d members, where I am member %d:\n",
                       sender, num_groups, mess_type );
                if(num_proc == num_groups) {
                    std::cout << "everyone in the group has joined!" << std::endl;
                    all_joined = true;
                    // start performance counter is all joined, here we assume that nobody crushes before finish, therefore
                    // assume that no one is getting out of the group once joined until finished
                    gettimeofday(&started_timestamp, nullptr);
                }
                for( i=0; i < num_groups; i++ )
                    printf("\t%s\n", &target_groups[i][0] );
                printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
            }
        }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);

        // send after every other process the join the group
        if(all_joined) {
            //        while (all_joined) { //multicast happens
//            for sendmax && if not sent last msg
//            sp_multicast
//            handle receive, write to file
        }

//        break if num_last message == n
        }
    }

    cout << "everything is received!" << endl;

    get_performance(started_timestamp);
    return 0;
}

void get_performance(const struct timeval& started_timestamp){
    // TODO: complete this
//    struct timeval ended_timestamp;
//    gettimeofday(&ended_timestamp, nullptr);
//    auto msec = diff_ms(ended_timestamp, started_timestamp);
//    auto total_packet = aru;
//    auto pakcet_size_in_bytes = sizeof(Message);
}

static  void	Bye()
{
	To_exit = 1;

	printf("\nBye.\n");

	SP_disconnect( Mbox );

	exit( 0 );
}

long long diff_ms(timeval t1, timeval t2)
{
    struct timeval diff;
    timersub(&t1, &t2, &diff);
    return (diff.tv_sec * 1000 + diff.tv_usec / 1000);
}


