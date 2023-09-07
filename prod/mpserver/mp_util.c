/* @author: Tencent slimehsiao */


#include <stdio.h>
#include <time.h>
#include <signal.h>
#include<stdlib.h>
#include "mp_udpclient.h"
#include "mp_util.h"
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>





session_node *head=NULL, *last=NULL;
uint8_t stream_id_array[8] = {0};

static int total_session_count = 0;



session_node* create_session_node(char* session_id, char* sta_ip_addr, char* proxy_ip_addr, int proxy_port, char* report_ip_addr, int report_port, char* app_id, int timer_ms, uint32_t version_num)
{
	if(is_stream_id_avaliable() == 0){
		debug_print("cannot get valid stream id\n");
		return NULL;
	}
	else{
		session_node* temp_node;
		temp_node = (session_node *) malloc(sizeof(session_node));
		memset(temp_node, 0, sizeof(session_node));
		strcpy(temp_node->session_id, session_id);
		strcpy(temp_node->sta_ip_addr, sta_ip_addr);
		strcpy(temp_node->proxy_ip_addr, proxy_ip_addr);
		strcpy(temp_node->report_ip_addr, report_ip_addr);
		temp_node->mp_version_num = version_num;
		temp_node->proxy_port = proxy_port;
		temp_node->report_port = report_port;
		strcpy(temp_node->app_id, app_id);
		temp_node->life_time = MAX_SESSION_LIFE_TIME;
		temp_node->timer_ms = timer_ms;
		temp_node->next = NULL;
		temp_node->sample_number = 0;
		temp_node->stream_id = -1;
		temp_node->stream_priority = 0;
		temp_node->proxy_ip_index = -1;
		temp_node->session_config.BQW_enabled = -1;
		temp_node->report_ip_index = -1;
		temp_node->last_rtt_ms = 0;
		temp_node->received_udp_echo = 0;
		temp_node->deca_probe_enabled = 0;
		temp_node->deca_probe_failed = 0;

		return temp_node;
	}
}

int assign_stream_id()
{	
	int i = 0;
	while (i < 8){
		//debug_print("stream_id list%d: %d\n",i , stream_id_array[i]);
		if(stream_id_array[i] == 0){
			stream_id_array[i] = 1;
			return i;
		}
		else
			i ++;
	}
	debug_print("stream_id assignment full!\n");
	return -1;	
}

int is_stream_id_avaliable()
{
	int i = 0;
	while (i < 8){
		if(stream_id_array[i] == 0){
			return 1;
		}
		else
			i ++;
	}
	debug_print("stream_id assignment full!\n");
	return 0;	
}

int release_stream_id(int id)
{	
	if(stream_id_array[id] == 1){
		stream_id_array[id] = 0;
		//debug_print("stream_id %d set to 0\n",id);
		return 0;
	}
	else{
		debug_print("release unassigned stream id!\n");
		return -1;
	}
}

int get_session_count() 
{
	return total_session_count;
}

int insert_session_node(session_node* new_node)
{

    //For the 1st element
    if(head == NULL)
    {
        head = new_node;
        last = new_node;
		total_session_count = 1;
		return total_session_count;
    }
    else
    {
    	if(!get_session_node(new_node->session_id)) //not existed in the list
		{
        	last->next=new_node;
        	last=new_node;
			total_session_count += 1;
			return total_session_count;
    	}
		else
		{
			//free(new_node);
			debug_print("node already existed!");
			return -1;
		}
    }
    return -1;
}

int delete_session_node(char* session_id)
{

    session_node *myNode = head, *previous=NULL;

    while(myNode!=NULL)
    {
        if(strcmp(myNode->session_id,session_id) == 0)
        {
            if(previous==NULL){
                head = myNode->next;
				if(myNode->next == NULL)
					last = head;
			}
            else{
                previous->next = myNode->next;
				if(myNode->next == NULL)
					last = previous;
			}
			
			total_session_count -= 1;
            free(myNode); //need to free up the memory to prevent memory leak
            return total_session_count;
        }
        previous = myNode;
        myNode = myNode->next;
    }
	debug_print("session id not found!\n");
	return -1;
}





session_node* get_session_node(char* session_id)
{

	//print_all_sessions();
	session_node *searchNode = head;
    while(searchNode!=NULL)
    {
        if(strcmp(searchNode->session_id,session_id) == 0)
        {
			return searchNode;
        }
        else
            searchNode = searchNode->next;
    }
	return NULL;

}

void print_all_sessions()
{

    debug_print("Session List\n");
	session_node *myList;
    myList = head;
    int i = 1;
    while(myList!=NULL)
    {
        debug_print("[%d] Session ID:%s\n", i, myList->session_id);
        myList = myList->next;
		i++;
    }
 
}

session_node*  get_first_session_node()
{
	session_node *searchNode = head;
	return searchNode;
 
}



const char* get_localtime_str()
{
    struct timespec ts;
    static char timestamp[100];
    static char timestring[30];
   
    int result;
    result = clock_gettime(CLOCK_REALTIME, &ts);
    if (result == 0) {
        struct tm *local_tm = localtime(&ts.tv_sec);
        strftime(timestring, 30, "%Y-%m-%d %H:%M:%S", local_tm);
		sprintf(timestamp, "%s.%03ld", timestring, ts.tv_nsec); 
    }
    return timestamp;
}


int start_timer(char* session_id, timer_t* timerid, int interval_ms, void(*handle_function)(int signo, siginfo_t *si, void *uc))
{
	return 0;

	struct itimerspec value;
	struct sigevent sigev;
	struct sigaction sa; 
    sigemptyset(&sa.sa_mask);

	
    sa.sa_flags = SA_SIGINFO;   /*call our handler*/
    sa.sa_sigaction = handle_function;/*Event handler to be called after timer expires*/ 
    if(sigaction(SIGRTMAX, &sa, NULL) < 0){
        debug_print("start_timr sigaction failed\n");
        return -1;
    }

	memset(&sigev, 0, sizeof(sigev));

    sigev.sigev_notify          = SIGEV_SIGNAL;
    sigev.sigev_signo           = SIGRTMAX;
    sigev.sigev_value.sival_ptr = session_id;

	value.it_value.tv_sec = 0 ;
	value.it_value.tv_nsec = 1000000 * interval_ms ;

	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = 1000000 * interval_ms;

	if(timer_create (CLOCK_REALTIME, &sigev, timerid) < 0){
		debug_print("timer_create failed\n");
        return -1;
	}
		

	if(timer_settime (*timerid, 0, &value, NULL) < 0){
		debug_print("timer_settime failed\n");
        return -1;
	}

	return 0;

}

int stop_timer(timer_t timerid)
{

	return 0;
	struct itimerspec value;

	value.it_value.tv_sec = 0;
	value.it_value.tv_nsec = 0;

	value.it_interval.tv_sec = 0;
	value.it_interval.tv_nsec = 0;

	if(timer_settime (timerid, 0, &value, NULL) < 0){
		debug_print("timer_settime failed\n");
        return -1;
	}
	return 0;
}

uintmax_t string_to_umax(const char *nptr, char **endptr, int base)
{
	const char *s;
	uintmax_t acc, cutoff;
	int c;
	int neg, any, cutlim;
	/*
	 * See strtoq for comments as to the logic used.
	 */
	s = nptr;
	do {
		c = (unsigned char) *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;
        /* BIONIC: avoid division and modulo for common cases */
#define  CASE_BASE(x)                            \
            case x: cutoff = UINTMAX_MAX / x;    \
	            cutlim = UINTMAX_MAX % x;    \
		    break
        switch (base) {
        CASE_BASE(8);
	CASE_BASE(10);
	CASE_BASE(16);
	default:
	    cutoff = UINTMAX_MAX / base;
	    cutlim = UINTMAX_MAX % base;
	}
	for (acc = 0, any = 0;; c = (unsigned char) *s++) {
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0)
			continue;
		if (acc > cutoff || (acc == cutoff && c > cutlim)) {
			any = -1;
			acc = UINTMAX_MAX;
			errno = ERANGE;
		} else {
			any = 1;
			acc *= (uintmax_t)base;
			acc += c;
		}
	}
	if (neg && any > 0)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *) (any ? s - 1 : nptr);
	return (acc);
}


uint64_t string_to_uint64(const char *nptr, char **endptr, int base)
{
    return (uint64_t)string_to_umax(nptr, endptr, base);
}


int get_current_timestamp(struct time_stamp *time1)
{
    struct timeval tv;
    int result;
    result = gettimeofday(&tv, NULL);
    time1->sec = tv.tv_sec;
    time1->usec = tv.tv_usec;
    return result;
}


uint64_t timestamp_in_usecs(struct time_stamp *time)
{
    return time->sec * 1000000LL + time->usec;
}


double timestamp_in_secs(struct time_stamp *time)
{
    return time->sec + time->usec / 1000000.0; 
}

double timestamp_in_msecs(struct time_stamp *time)
{
    return time->sec * 1000 + time->usec / 1000.0;
}

/* 
 * Returns -1 if time1 is earlier, 1 if time1 is later,
 * or 0 if the timestamps are equal.
 */
int timestamp_compare(struct time_stamp *time1, struct time_stamp *time2)
{
    if (time1->sec < time2->sec)
        return -1;
    if (time1->sec > time2->sec)
        return 1;
    if (time1->usec < time2->usec)
        return -1;
    if (time1->usec > time2->usec)
        return 1;
    return 0;
}

/* 
 * Calculates the time from time2 to time1, assuming time1 is later than time2.
 * The diff will always be positive, so the return value should be checked
 * to determine if time1 was earlier than time2.
 *
 * Returns 1 if the time1 is less than or equal to time2, otherwise 0.
 */
int timestamp_diff(struct time_stamp *time1, struct time_stamp *time2, struct time_stamp *diff)
{
    int past = 0;
    int cmp = 0;

    cmp = timestamp_compare(time1, time2);
    if (cmp == 0) {
        diff->sec = 0;
        diff->usec = 0;
        past = 1;
    } 
    else if (cmp == 1) {
        diff->sec = time1->sec - time2->sec;
        diff->usec = time1->usec;
        if (diff->usec < time2->usec) {
            diff->sec -= 1;
            diff->usec += 1000000;
        }
        diff->usec = diff->usec - time2->usec;
    } else {
        diff->sec = time2->sec - time1->sec;
        diff->usec = time2->usec;
        if (diff->usec < time1->usec) {
            diff->sec -= 1;
            diff->usec += 1000000;
        }
        diff->usec = diff->usec - time1->usec;
        past = 1;
    }

    return past;
}

void string_lower_to_upper(char string[]) 
{
 int i = 0;
 
 while (string[i] != '\0') 
 {
     if (string[i] >= 'a' && string[i] <= 'z') {
         string[i] = string[i] - 32;
     }
       i++;
 }
}



