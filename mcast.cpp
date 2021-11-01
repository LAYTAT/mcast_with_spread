#include "sp.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <vector>

using namespace std;

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100
#define PAYLOAD_SIZE    1300  // required size, do not change this

enum class MSG_TYPE{
    NORMAL_DATA = 1,
    LAST_DATA = 2
};

struct Message{
    int32_t proc_id;
    int32_t msg_id;
    int32_t rand_num;
    char payload[PAYLOAD_SIZE];
};

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
void get_performance(const struct timeval&, int);
void update_sending_buf(Message*, int, int);
void send_msg(Message *, int);
bool is_all_finished(const vector<bool>& v);


int main(int argc, char * argv[])
{
    stringstream s1(argv[1]);
    stringstream s2(argv[2]);
    stringstream s3(argv[3]);

    int num_mes = 0; //number of messages
    int p_id = 0; //process id
    int num_proc = 0; //number of processes

    s1>>num_mes;
    s2>>p_id;
    s3>>num_proc;

    int ret = 0; 

    int msg_id = 1;  //the current msg id for the current process
    int aru = 0;     //the total msg id received for all the messages
    bool all_joined = false;  //did all processes join?
    bool all_finished = false; //did all processes finished?
    bool can_send = false;     //for flow control
    bool all_sent = false; //did process send all messages?
    float OK_TO_SEND_PERCENT = 1.0f;
    int SENDING_QUOTA = 10;
    int received_count = 0;

    //buffer
    static Message receive_buf;
    static Message sending_buf;

    // for ending
    vector<bool> finished_member(num_proc, false);

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

    //random seed
    struct timeval timestamp_for_rand;
    gettimeofday(&timestamp_for_rand, NULL);
    srand(timestamp_for_rand.tv_usec * timestamp_for_rand.tv_sec);

    ret = SP_connect_timeout( Spread_name, User, 0, 1, &Mbox, Private_group, test_timeout );
	if( ret != ACCEPT_SESSION ) 
	{
		SP_error( ret );
		Bye();
	 }
    cout << "Connected to Spread!" << endl;
    cout << "Mbox from Spread is : " << Mbox << endl;

    // open file
    string filename = to_string(p_id) + ".out";
    auto fp = fopen(filename.c_str(), "w");
    if (fp == NULL) {
        cerr << "Error: file failed to open" << endl;
        exit(1);
    }

    // join group
    ret = SP_join( Mbox, group );
    if( ret < 0 ) SP_error( ret );

    while(!all_finished) {

        // receive
        ret = SP_receive( Mbox, &service_type, sender, 10, &num_groups, target_groups,
                          &mess_type, &endian_mismatch, sizeof(Message),(char *) &receive_buf );
        if( ret < 0 )
        {
            if ( (ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT) ) {
                service_type = DROP_RECV;
                printf("\n========Buffers or Groups too Short=======\n");
                ret = SP_receive( Mbox, &service_type, sender, MAX_MEMBERS, &num_groups, target_groups,
                                  &mess_type, &endian_mismatch, sizeof(Message),(char *) &receive_buf);
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
            cout << "received regular messages." << endl;
            fprintf(fp, "%2d, %8d, %8d\n", receive_buf.proc_id, receive_buf.msg_id, receive_buf.rand_num);
            if((MSG_TYPE)mess_type == MSG_TYPE::LAST_DATA){
                finished_member[receive_buf.proc_id] = true;
                can_send = true;
            }
            if(is_all_finished(finished_member)){
                all_finished = true;
                break;
            }

            aru++;
            received_count++;
            if(received_count >= OK_TO_SEND_PERCENT * num_proc * SENDING_QUOTA){
                can_send = true;
                received_count = 0;
            }

        }else if( Is_membership_mess( service_type ) )
        {
            cout << "received membership message from group " << sender << endl;
            ret = SP_get_memb_info( (const char *)&receive_buf, service_type, &memb_info );
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
                    cout << "everyone in the group has joined!" << endl;
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
            if(can_send & !all_sent) {
                // send a burst of new messages
                for(int i = 0; i < SENDING_QUOTA; i++) {
                    if (msg_id == num_mes + 1) {
                        all_sent = true;
                        break;
                    }
                    update_sending_buf(&sending_buf, p_id, msg_id);
                    send_msg(&sending_buf, num_mes);
                    msg_id++;
                }
                can_send = false;
            }
        }


        }


    cout << "everything is received!" << endl;

    get_performance(started_timestamp, aru);

    //close file
    fflush(fp);
    fclose(fp);
    return 0;
}

void get_performance(const struct timeval& started_timestamp, int total_packet){
    struct timeval ended_timestamp;
    gettimeofday(&ended_timestamp, nullptr);
    auto msec = diff_ms(ended_timestamp, started_timestamp);
    auto pakcet_size_in_bytes = sizeof(Message);

    cout << "============================Performance========================== " << endl;
    cout << "Transmission time:  " << msec << " ms." << endl;
    cout << "Total num of pcks:  " << total_packet << "." << endl;
    cout << "Transmission speed: " << static_cast<double>((pakcet_size_in_bytes * total_packet) * 8)  / static_cast<double>(1000 * msec)<< " Mbits per second. " << endl;
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

void update_sending_buf(Message* msg, int proc_id, int msg_id){
    msg->rand_num = rand() % 1000000 + 1;
    msg->proc_id = proc_id;
    msg->msg_id;
}

void send_msg(Message * snd_msg_buf, int total_num_of_packet_to_be_sent) {
    int ret;
    if(snd_msg_buf->msg_id == total_num_of_packet_to_be_sent) {
        ret= SP_multicast( Mbox, AGREED_MESS, group, (short int)MSG_TYPE::LAST_DATA, sizeof(Message),snd_msg_buf);
    } else {
        ret= SP_multicast( Mbox, AGREED_MESS, group, (short int)MSG_TYPE::NORMAL_DATA, sizeof(Message),snd_msg_buf);
    }
    if( ret < 0 )
    {
        SP_error( ret );
        Bye();
    }
}

bool is_all_finished(const vector<bool>& v){
    for(auto i : v) {
        if(!i)
            return false;
    }
    return true;
}
