#include "sp.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <sys/time.h>
#include <vector>
#include <assert.h>

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
int msg_id = 1;  //the current msg id for the current process
int aru = 0;     //the total msg id received for all the messages

static  void	Bye();
long long diff_ms(timeval, timeval);
void get_performance(const struct timeval&, int);
void update_sending_buf(Message*, int, int);
void send_msg(Message*, int);
bool is_all_finished(const vector<bool>& v);
void p_v(const vector<bool>&);

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
    int sending_proc_num = num_proc;

    int ret = 0;

    bool all_joined = false;  //did all processes join?
    bool all_finished = false; //did all processes finished?
    bool all_sent = false; //did process send all messages?
    bool bursted = false;  // is first round of burst messages sent?
    int SENDING_QUOTA = 30;   //30,400,600 for baseline speed
    int GLOBAL_QUOTA = SENDING_QUOTA * num_proc;

    //buffer
    Message receive_buf;
    Message sending_buf;

    // for ending
    vector<bool> finished_member(num_proc+1, false);

    sp_time test_timeout;
    test_timeout.sec = 5;
    test_timeout.usec = 0;

    s1 >> num_mes;
    s2 >> p_id;
    s3 >> num_proc;
    sprintf( User, "userjiekim_4803" );
	sprintf( Spread_name, "4803");
    sprintf( group, "kimjie_group_4803");

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

    // open fileviv
    string filename = "/tmp/"+ to_string(p_id) + ".out";
//    string filename = to_string(p_id) + ".out";
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
                cout << "Current Aru = " << aru << endl;
                cout << "Current msg id = " << msg_id << endl;
                printf("\nBye.\n");
            }
            exit( 0 );
        }
        if( Is_regular_mess( service_type ) )
        {
//            cout << "Received: proc_id = "  << receive_buf.proc_id << ", msg_id = " << receive_buf.msg_id << endl;
            if((MSG_TYPE)mess_type == MSG_TYPE::NORMAL_DATA){
                fprintf(fp, "%2d, %8d, %8d\n", receive_buf.proc_id, receive_buf.msg_id, receive_buf.rand_num);
                aru++;
                if(receive_buf.proc_id == p_id && !all_sent) {
                    update_sending_buf(&sending_buf, p_id, msg_id);
                    send_msg(&sending_buf, num_mes);
                    if(msg_id == num_mes)
                        all_sent = true;
                    msg_id++;
                }
            }
            if((MSG_TYPE)mess_type == MSG_TYPE::LAST_DATA){
                finished_member[receive_buf.proc_id] = true;
                std::cout << receive_buf.proc_id << ": FINISHED!" << std::endl;
                sending_proc_num--;
                p_v(finished_member);
                if(receive_buf.proc_id != p_id && !all_sent) {
                    // new burst of messages because other got out
                    SENDING_QUOTA = GLOBAL_QUOTA / sending_proc_num;
                    for(int i = 0; i < SENDING_QUOTA; i++) {
                        update_sending_buf(&sending_buf, p_id, msg_id);
                        send_msg(&sending_buf, num_mes);
                        if(msg_id == num_mes) {
                            all_sent = true;
                            break;
                        }
                        msg_id++;

                    }
                }
            }
            if(is_all_finished(finished_member)){
                all_finished = true;
                break;
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
                    // send the first burst after every other process the join the group
                    if(!bursted && all_joined && !all_sent) {
                        if (num_mes == 0) {
                            update_sending_buf(&sending_buf, p_id, msg_id);
                            send_msg(&sending_buf, num_mes);
                            all_sent = true;
                        }
                        else if( num_mes <= SENDING_QUOTA ){
                            for (int i = 0; i < num_mes; i++) {
                                update_sending_buf(&sending_buf, p_id, msg_id);
                                send_msg(&sending_buf, num_mes);
                                msg_id++;
                            }
                            all_sent = true;
                        } else {
                            for(int i = 0; i < SENDING_QUOTA; i++) {
                                update_sending_buf(&sending_buf, p_id, msg_id);
                                send_msg(&sending_buf, num_mes);
                                msg_id++;
                            }
                        }
                        bursted = true;
                        cout << "First burst of message sent with init win size : " << msg_id - 1 << endl;
                    }
                }
                for(int i=0; i < num_groups; i++ )
                    printf("\t%s\n", &target_groups[i][0] );
                printf("grp id is %d %d %d\n",memb_info.gid.id[0], memb_info.gid.id[1], memb_info.gid.id[2] );
            }
        }else printf("received message of unknown message type 0x%x with ret %d\n", service_type, ret);
    }


    cout << "everything is received!" << endl;

    get_performance(started_timestamp, aru);
    //close file
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
    cout << "Current Aru = " << aru << endl;
    cout << "Current msg id = " << msg_id << endl;
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
    msg->msg_id = msg_id;
}

void send_msg(Message *snd_msg_buf, int total_num_of_packet_to_be_sent) {
    int ret;
    if( total_num_of_packet_to_be_sent == 0 ) {
        ret= SP_multicast( Mbox, AGREED_MESS, group, (short int)MSG_TYPE::LAST_DATA, sizeof(Message), (const char *)snd_msg_buf);
    } else if (snd_msg_buf->msg_id == total_num_of_packet_to_be_sent) {
        cout << "I have finished sending!!!!!!" << endl;
        ret= SP_multicast( Mbox, AGREED_MESS, group,  (short int)MSG_TYPE::NORMAL_DATA, sizeof(Message),(const char *)snd_msg_buf);
        if( ret < 0 )
        {
            SP_error( ret );
            cout << "Bye: NORMAL_DATA sending." << endl;
            Bye();
        }
        ret= SP_multicast( Mbox, AGREED_MESS, group, (short int)MSG_TYPE::LAST_DATA, sizeof(Message), (const char *)snd_msg_buf);
    } else {
        ret= SP_multicast( Mbox, AGREED_MESS, group,  (short int)MSG_TYPE::NORMAL_DATA, sizeof(Message),(const char *)snd_msg_buf);
    }
    if( ret < 0 )
    {
        SP_error( ret );
        cout << "Bye: DATA sending." << endl;

        Bye();
    }
}

void p_v(const vector<bool>& v){
    cout << "[";
    for(auto i : v) {
        cout << i << ", ";
    }
    cout << "]" << endl;
}

bool is_all_finished(const vector<bool>& v){
    for(int i = 1; i < v.size(); i++){
        if(!v[i])
            return false;
    }
    return true;
}


