#include "../../TencentWiFi.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>

static int exec_prog(char **argv)
{
    pid_t   my_pid;
    int     status, timeout /* unused ifdef WAIT_FOR_COMPLETION */;

    if (0 == (my_pid = fork())) {
            if (-1 == execve(argv[0], (char **)argv , NULL)) {
                    perror("child process execve failed [%m]");
                    return -1;
            }
    }

#ifdef WAIT_FOR_COMPLETION
    timeout = 1000;

    while (0 == waitpid(my_pid , &status , WNOHANG)) {
            if ( --timeout < 0 ) {
                    perror("timeout");
                    return -1;
            }
            sleep(1);
    }

    printf("%s WEXITSTATUS %d WIFEXITED %d [status %d]\n",
            argv[0], WEXITSTATUS(status), WIFEXITED(status), status);

    if (1 != WIFEXITED(status) || 0 != WEXITSTATUS(status)) {
            perror("%s failed, halt system");
            return -1;
    }

#endif
    return 0;
}

int set_wmm_traffic(int wlan_interface,char *server_ip_address,int net_mask_length,int port,int ac_queue_tos )
{
    char net_mask_string[80];
    sprintf(net_mask_string,"%d",net_mask_length);

    char port_string[80];
    sprintf(port_string,"%d",port);

    char ac_queue_tos_string[80];
    sprintf(ac_queue_tos_string,"%d",ac_queue_tos);

    char* args[] = {"/bin/wifibase","-s","set_wme_ac_ip","wlan1",server_ip_address,net_mask_string,port_string,ac_queue_tos_string, NULL};
    exec_prog((char **)args);
}

int clear_wmm_traffic()  //TODO: the tos queue id should be maintain
{   
    int ac_queue_tos = 7; 
    char ac_queue_tos_string[80];
    sprintf(ac_queue_tos_string,"%d",ac_queue_tos);

    char* args[] = {"/bin/wifibase","-s","set_wme_ac_ip","wlan1","0.0.0.0","0","0",ac_queue_tos_string, NULL};
    exec_prog((char **)args);
}

int set_wmm_parameter(char* ac_queue_name,char* parameter_name,char* parameter_number) 
{   
    char* args[] = {"/bin/wl","-i","wl1","wme_ac","ap",ac_queue_name,parameter_name,parameter_number,NULL};
    exec_prog((char **)args);
}

