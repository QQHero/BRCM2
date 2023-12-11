#include "mp_server.h"
#include "mp_core.h"
#include "TencentWiFi.h"
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <string.h>
#include "router_server.h"
#include "router_log.h"



/* Function prototypes */

/* functions to handle connections */
void handle_tcp_client(int clntSocket); /* TCP client handling function */
int handle_set_WMM_traffic(cJSON *jobjReceived);
int handle_quit_cmd(cJSON *jobjReceived);
int handle_clear_WMM_traffic(cJSON *jobjReceived);
int handle_set_WMM_parameter(cJSON *jobjReceived);
int handle_handshake_cmd(int index,cJSON *jobjReceived);
int handle_heartbeat_cmd(int index,cJSON *jobjReceived);
int handle_finish_cmd(int index,cJSON *jobjReceived);
int handle_json(int index,char *objReceived);
int handle_tgpa_notify_cmd(int index,cJSON *jobjReceived);
int handle_start_set_tuples(int index,cJSON *jobjReceived);
int handle_start_get_tuples(int index,cJSON *jobjReceived);

//int handle_BQW_config(char* session_id,char *value_string,char *probability_string);

int convert_json_tuples_toArray(five_tuples_t *tuples,cJSON *jtuples);
void dump_tuples(five_tuples_t *tuples,int tuple_size);

void send_heartbeat_response(int index,int result,char *jdata_string);
void send_handshake_response(int index,int result,int mp_version_num,char *report);
void send_finish_response(int index,int result);
void send_tgpa_notify_response(int index,int result);
void send_start_set_tuples_response(int index,int result);
void send_start_get_tuples_response(int index,int result,five_tuples_t *tuples);

struct JSON_CMD json_cmds[MAX_WLAN_CMDS];
//uint32_t mp_version_num = 1008;
extern mp_version_num;
int timer_ms = 200;

int main(int argc, char *argv[])
{
    int port;

    sprintf(json_cmds[0].cmd_string,"start.handshake.req");
    json_cmds[0].handle_function = handle_handshake_cmd;
    sprintf(json_cmds[1].cmd_string,"start.heartbeat.req");
    json_cmds[1].handle_function = handle_heartbeat_cmd;
    sprintf(json_cmds[2].cmd_string,"start.finish.req");
    json_cmds[2].handle_function = handle_finish_cmd;
    sprintf(json_cmds[3].cmd_string,"tgpa.notify.req");
    json_cmds[3].handle_function = handle_tgpa_notify_cmd;
    sprintf(json_cmds[4].cmd_string,"start.settuples.req");
    json_cmds[4].handle_function = handle_start_set_tuples;
    sprintf(json_cmds[5].cmd_string,"start.gettuples.req");
    json_cmds[5].handle_function = handle_start_get_tuples;
    

    init_proxy_list();
    init_report_list();
    init_mp_core();
    init_hal();

    

    if (argc != 2) { /* Test for correct number of arguments */
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);
    if (port <= 1024 || port > 65534) {
        fprintf(stderr, "Fail to start router server with invalid port[%d], you should input valid param port\n",
               port);
        return -1;
    }

    //turn_off_rts();

    printf("===============Begin mp_server version:%d  Port:%d=============\n", mp_version_num, port);
    startRouterListen(port);
    printf("===============End mp_server=============\n");



  /* NOT REACHED */
}


int handle_json(int index,char *objReceived) 
{
  cJSON *jobjReceived;
  jobjReceived = cJSON_CreateObject();
  jobjReceived = cJSON_Parse(objReceived);
  char *cmd_string = cJSON_GetObjectItemCaseSensitive(jobjReceived, "cmd")->valuestring;

	if (cmd_string!=NULL) {
    DEBUG_LOG_I("JSON CMD:%s",cmd_string);
    for (int i =0;i<MAX_WLAN_CMDS;i++) {
      DEBUG_LOG_I("COMPARE CMD:%s",json_cmds[i].cmd_string);
      if(strcmp(cmd_string,json_cmds[i].cmd_string)==0) {
        DEBUG_LOG_I("FOUND CMD");
        json_cmds[i].handle_function(index,jobjReceived);
		    break;
      }
    }
  }
  cJSON_Delete(jobjReceived);
}


void send_start_get_tuples_response(int index,int result,five_tuples_t *tuples)
{

  cJSON *jobjToSend = cJSON_CreateObject();
  cJSON *jtuples = NULL;
  cJSON *item;
  char result_str[32];
  DEBUG_LOG_I("send_start_get_tuples_response");

  cJSON_AddItemToObject(jobjToSend, "cmd", cJSON_CreateString("start.gettuples.rsp"));
  jtuples = cJSON_AddArrayToObject(jobjToSend,"tuples");
  
  if(tuples!=NULL) {
    for (int i = 0 ; i < MAX_TUPLES ; i++) {
      if(strlen(tuples[i].src_addr)>0) {
        item = cJSON_CreateObject();
        cJSON_AddItemToObject(item,"server_ip",cJSON_CreateString(tuples[i].src_addr));
        cJSON_AddItemToObject(item,"server_port",cJSON_CreateString(tuples[i].src_port));
        cJSON_AddItemToObject(item,"client_ip",cJSON_CreateString(tuples[i].dst_addr));
        cJSON_AddItemToObject(item,"client_port",cJSON_CreateString(tuples[i].dst_port));
        cJSON_AddItemToObject(item,"protocol",cJSON_CreateString(tuples[i].protocol));
        cJSON_AddItemToArray(jtuples,item);        
      }
    }
    //DEBUG_LOG_I("add array to obj");    
    //cJSON_AddItemToObject(jobjToSend,"tuples",tuples);
    //cJSON_AddArrayToObject(jtuples,"tuples");
    result =0;
  } else {
    result = -1;
  }

  sprintf(result_str, "%d", result);
  cJSON_AddItemToObject(jobjToSend, "result", cJSON_CreateString(result_str));

  char *request = cJSON_PrintUnformatted(jobjToSend);
  DEBUG_LOG_I("send rsp");   
  handleSendData(index, request, strlen(request)+1);

  cJSON_free(request);
  cJSON_Delete(jobjToSend);
}


int handle_start_get_tuples(int index,cJSON *jobjReceived)
{
  char *session_id = NULL;
  int result = 0;
  five_tuples_t *tuples;
  DEBUG_LOG_I("handle get tuples");

  if(cJSON_HasObjectItem(jobjReceived,"instanceid")) {
    session_id = cJSON_GetObjectItemCaseSensitive(jobjReceived,"instanceid")->valuestring;
  } else {
    DEBUG_LOG_E("Cannot find instanceid in JSON");
    send_start_get_tuples_response(index,-1,NULL);
    return -1;
  }  

  tuples = mp_get_tuples(session_id);

  send_start_get_tuples_response(index,1,tuples);
  return 0;

}

void send_start_set_tuples_response(int index,int result) 
{
  cJSON *jobjToSend = cJSON_CreateObject();
  char result_str[32];
  sprintf(result_str, "%d", result);

  cJSON_AddItemToObject(jobjToSend, "cmd", cJSON_CreateString("start.settuples.rsp"));
  cJSON_AddItemToObject(jobjToSend, "result", cJSON_CreateString(result_str));

  char *request = cJSON_PrintUnformatted(jobjToSend);
  handleSendData(index, request, strlen(request)+1);

  cJSON_free(request);
  cJSON_Delete(jobjToSend);
}

int handle_start_set_tuples(int index,cJSON *jobjReceived)
{
  char *session_id = NULL;
  int result = 0;
  cJSON *jtuples=NULL;
  five_tuples_t tuples[MAX_TUPLES];
  memset(tuples,0,sizeof(five_tuples_t)*MAX_TUPLES);

  DEBUG_LOG_I("handle start set tuples\n");

  if(cJSON_HasObjectItem(jobjReceived,"instanceid")) {
    session_id = cJSON_GetObjectItemCaseSensitive(jobjReceived,"instanceid")->valuestring;
  } else {
    DEBUG_LOG_E("Cannot find instanceid in JSON");
    result = -1;
  }  

  if(cJSON_HasObjectItem(jobjReceived,"tuples")) {
    DEBUG_LOG_I("get tuples\n");
    jtuples = cJSON_GetObjectItemCaseSensitive(jobjReceived,"tuples");
    convert_json_tuples_toArray(tuples,jtuples);
  } else {
    DEBUG_LOG_E("empty tuples\n");
    result = -1;
  }
  dump_tuples(tuples,cJSON_GetArraySize(jtuples));
  if (result!=-1) {
    result = mp_set_tuples(session_id,tuples);
  }
  send_start_set_tuples_response(index,result);

}

int handle_tgpa_notify_cmd(int index,cJSON *jobjReceived) 
{
  char *session_id = NULL;
  int result=0;
  char* jevent_code_string = NULL;
  char* jevent_details_string = NULL;

  DEBUG_LOG_I("handle tgpa notify event\n");

  if(cJSON_HasObjectItem(jobjReceived,"instanceid")) {
    session_id = cJSON_GetObjectItemCaseSensitive(jobjReceived,"instanceid")->valuestring;
  } else {
    DEBUG_LOG_E("Cannot find instanceid in JSON");
    result = -1;
  }

  if(cJSON_HasObjectItem(jobjReceived,"event_code")) {
    jevent_code_string = cJSON_GetObjectItemCaseSensitive(jobjReceived,"event_code")->valuestring;
  } else {
    DEBUG_LOG_E("Cannot find event_code in JSON");
    result = -1;
  }

  if(cJSON_HasObjectItem(jobjReceived,"event_details")) {
    jevent_details_string = cJSON_GetObjectItemCaseSensitive(jobjReceived,"event_details")->valuestring;
  } else {
    DEBUG_LOG_E("Cannot find event_details in JSON");
    result = -1;
  }  

  if (result!=-1) {
    //process event code 

    //result = mp_handle_tgpa_event(session_id,jevent_code_string,jevent_details_string);
    result = 0; // for test
    	      
	  if(result == 0){
      //process send back
	    DEBUG_LOG_I("tgpa event execution success\n");
      send_tgpa_notify_response(index,result);
      //send_tgpa_notify_response(index,result,jdata_string);
	  }
    
  }   
}

void send_tgpa_notify_response(int index,int result) {
  cJSON *jobjToSend = cJSON_CreateObject();
  char result_str[32];
  sprintf(result_str, "%d", result);

  cJSON_AddItemToObject(jobjToSend, "cmd", cJSON_CreateString("tgpa.notify.rsp"));
  cJSON_AddItemToObject(jobjToSend, "result", cJSON_CreateString(result_str));

  char *request = cJSON_PrintUnformatted(jobjToSend);
  handleSendData(index, request, strlen(request)+1);

  cJSON_free(request);
  cJSON_Delete(jobjToSend);
}


int handle_finish_cmd(int index,cJSON *jobjReceived) {
  char *session_id = NULL;
  int result=0;

  DEBUG_LOG_I("handle_finish_cmd enter\n");

  if(cJSON_HasObjectItem(jobjReceived,"instanceid")) {
    session_id = cJSON_GetObjectItemCaseSensitive(jobjReceived,"instanceid")->valuestring;
  } else {
    DEBUG_LOG_E("Cannot find instanceid in JSON->data");
    send_finish_response(index,-1); 
    return -1;
  }

  result = mp_stop_session(session_id, SESSION_STOPREASON_NORMAL);
  if(result == 0){
	  DEBUG_LOG_I("finish execution success\n");
  }

  send_finish_response(index,result); 

}

void send_finish_response(int index,int result) {
  cJSON *jobjToSend = cJSON_CreateObject();
  char result_str[32];
  sprintf(result_str, "%d", result);

  cJSON_AddItemToObject(jobjToSend, "cmd", cJSON_CreateString("start.finish.rsp"));
  cJSON_AddItemToObject(jobjToSend, "result", cJSON_CreateString(result_str));

  char *request = cJSON_PrintUnformatted(jobjToSend);
  handleSendData(index, request, strlen(request)+1);

  //free(request);
  cJSON_free(request);
  cJSON_Delete(jobjToSend);
}


int handle_heartbeat_cmd(int index,cJSON *jobjReceived) {
  cJSON *jdata=NULL;
  char *session_id = NULL;
  int result=0;
  char* jdata_string = NULL;

  if(cJSON_HasObjectItem(jobjReceived,"instanceid")) {
    session_id = cJSON_GetObjectItemCaseSensitive(jobjReceived,"instanceid")->valuestring;
  } else {
    DEBUG_LOG_E("Cannot find instanceid in JSON");
    result = -1;
  }

  if(cJSON_HasObjectItem(jobjReceived,"data")) {
    jdata = cJSON_GetObjectItem(jobjReceived,"data");
  } else {
    DEBUG_LOG_E("Cannot find data in JSON");
    result = -1;
  }

  if (result!=-1) {
    result = mp_report_heartbeat(session_id);
    jdata_string = cJSON_Print(jdata);	      
	  if(result == 0){
	    DEBUG_LOG_I("heartbeat execution success\n");
      send_heartbeat_response(index,result,jdata_string);
	  }
    cJSON_free(jdata_string);
  } 
  
}


void send_heartbeat_response(int index,int result,char *jdata_string) {
  cJSON *jobjToSend = cJSON_CreateObject();
  char result_str[32];
  sprintf(result_str, "%d", result);


  cJSON_AddItemToObject(jobjToSend, "cmd", cJSON_CreateString("start.heartbeat.rsp"));
  cJSON_AddItemToObject(jobjToSend, "result", cJSON_CreateString(result_str));
  cJSON_AddItemToObject(jobjToSend, "data",  cJSON_Parse(jdata_string));
  

  char *request = cJSON_PrintUnformatted(jobjToSend);
  handleSendData(index, request, strlen(request)+1);

  cJSON_free(request);
  cJSON_Delete(jobjToSend);

}

void dump_tuples(five_tuples_t *tuples,int tuple_size)
{
  for (int i = 0 ; i < tuple_size ; i++) {
    DEBUG_LOG_I("server_ip:%s",tuples[i].src_addr);
    DEBUG_LOG_I("server_port:%s",tuples[i].src_port);
    DEBUG_LOG_I("client_ip:%s",tuples[i].dst_addr);
    DEBUG_LOG_I("client_port:%s",tuples[i].dst_port);
    DEBUG_LOG_I("server_ip:%s",tuples[i].protocol);
  }

}


int convert_json_tuples_toArray(five_tuples_t *tuples,cJSON *jtuples)
{
  int i,tuple_size;
  char *server_ip_str,*client_ip_str,*server_port_str,*client_port_str,*protocol_str;
  tuple_size = cJSON_GetArraySize(jtuples);
  for (i = 0 ; i < tuple_size ; i++) {
    cJSON * subitem_tuple = cJSON_GetArrayItem(jtuples, i);
    if (subitem_tuple!= NULL) {
        if(cJSON_HasObjectItem(subitem_tuple,"server_ip")) {
          server_ip_str = cJSON_GetObjectItemCaseSensitive(subitem_tuple,"server_ip")->valuestring;
          strcpy(tuples[i].src_addr, server_ip_str);
          DEBUG_LOG_I("JSON:Server IP:%s",server_ip_str);
        }
        if(cJSON_HasObjectItem(subitem_tuple,"client_ip")) {
          client_ip_str = cJSON_GetObjectItemCaseSensitive(subitem_tuple,"client_ip")->valuestring;
          strcpy(tuples[i].dst_addr, client_ip_str);
        }
        if(cJSON_HasObjectItem(subitem_tuple,"server_port")) {
          server_port_str = cJSON_GetObjectItemCaseSensitive(subitem_tuple,"server_port")->valuestring;
          strcpy(tuples[i].src_port, server_port_str);
        }
        if(cJSON_HasObjectItem(subitem_tuple,"client_port")) {
          client_port_str = cJSON_GetObjectItemCaseSensitive(subitem_tuple,"client_port")->valuestring;
          strcpy(tuples[i].dst_port, client_port_str);
        }
        if(cJSON_HasObjectItem(subitem_tuple,"protocol")) {
          protocol_str = cJSON_GetObjectItemCaseSensitive(subitem_tuple,"protocol")->valuestring;
          strcpy(tuples[i].protocol, protocol_str);
        }                    
    }
  }
}


int handle_handshake_cmd(int index,cJSON *jobjReceived) 
{
  cJSON *jhead=NULL;
  cJSON *jdata=NULL;
  cJSON *jconfig=NULL;
  cJSON *jtuples=NULL;
  char *session_id = NULL;
  char *acc_ip = NULL;
  char *port = NULL;
  char *report_ip = NULL;
  char *report_port = NULL;
  char default_report_port[] = "20009";
  char *app_id = NULL;
  char *bqw_str = NULL;
  char *interval_str = NULL;
  char *server_ip_str = NULL;
  char *client_ip_str = NULL;
  char *server_port_str = NULL;
  char *client_port_str = NULL; 
  char *protocol_str = NULL; 
  int result = 0;
  char report[MAX_STRING_LENGTH];
  int hit_flag = 0;
  five_tuples_t tuples[MAX_TUPLES];
  int tuple_size=0;
  int i,j;

  memset(tuples,0,sizeof(five_tuples_t)*MAX_TUPLES);

  if(cJSON_HasObjectItem(jobjReceived,"head")) {
    jhead = cJSON_GetObjectItemCaseSensitive(jobjReceived,"head")->child;
  } else {
    DEBUG_LOG_E("Cannot find head in JSON");
    sprintf(report,"error:json_head");
    result = -1;
    send_handshake_response(index,result,mp_version_num,report);
    return -1;
  }

  if(cJSON_HasObjectItem(jobjReceived,"data")) {
    jdata = cJSON_GetObjectItem(jobjReceived,"data");

    if(cJSON_HasObjectItem(jdata,"instanceid")) {
      session_id = cJSON_GetObjectItemCaseSensitive(jdata,"instanceid")->valuestring;
    } else {
      DEBUG_LOG_E("Cannot find instanceid in JSON->data");
      sprintf(report,"error:json_instanceid");
      result = -1;
    }

    if(cJSON_HasObjectItem(jdata,"accip")) {
      acc_ip = cJSON_GetObjectItemCaseSensitive(jdata,"accip")->valuestring;
    } else {
      DEBUG_LOG_E("Cannot find acc_ip in JSON->data");
      sprintf(report,"error:json_accip");
      result = -1;
    }

    if(cJSON_HasObjectItem(jdata,"port")) {
      port = cJSON_GetObjectItemCaseSensitive(jdata,"port")->valuestring;
    } else {
      DEBUG_LOG_E("Cannot find port in JSON->data");
      sprintf(report,"error:json_port");
      result = -1;
    } 

    if(cJSON_HasObjectItem(jdata,"reportdomain")) {
      report_ip = cJSON_GetObjectItemCaseSensitive(jdata,"reportdomain")->valuestring;
    } else {
      DEBUG_LOG_E("Cannot find report_ip in JSON->data");
      
      if (acc_ip!=NULL) {
        report_ip = acc_ip;
      } else {
        sprintf(report,"error:json_reportdomain");
        result =  -1;

      }
    }

    if(cJSON_HasObjectItem(jdata,"reportport")) {
      report_port = cJSON_GetObjectItemCaseSensitive(jdata,"reportport")->valuestring;
    } else {
      DEBUG_LOG_E("Cannot find report_port in JSON->data");
      report_port = default_report_port;
    } 

    if(cJSON_HasObjectItem(jdata,"appid")) {
      app_id = cJSON_GetObjectItemCaseSensitive(jdata,"appid")->valuestring;
    } else {
      DEBUG_LOG_E("Cannot find appid in JSON->data");
      sprintf(report,"error:json_appid");
      result = -1;
    }   

  } else {
    DEBUG_LOG_E("Cannot find data in JSON");
    sprintf(report,"error:json_data");
    result = -1;
    send_handshake_response(index,result,mp_version_num,report);
    return -1;
  }

  if(cJSON_HasObjectItem(jobjReceived,"tuples")) {
    jtuples = cJSON_GetObjectItemCaseSensitive(jobjReceived,"tuples");
    convert_json_tuples_toArray(tuples,jtuples);
    dump_tuples(tuples,cJSON_GetArraySize(jtuples));
    DEBUG_LOG_I("mp_start_session_without_report");
    result = mp_start_session_without_report(session_id,sockets[index].clientIp,app_id,timer_ms,mp_version_num);
    DEBUG_LOG_I("mp_set_tuples");
    result = mp_set_tuples(session_id,tuples);
    if(result==-1) {
      DEBUG_LOG_E("handshake set tuples error");
    }
    sprintf(report,"handshake tuples");
    send_handshake_response(index,result,mp_version_num,report);
    return 0;
  }
 
    
  if (result==0) {
    DEBUG_LOG_I("Session ID:%s,ACC IP:%s,PORT:%s,STA IP:%s,APP_ID:%s\n",session_id,acc_ip,port,sockets[index].clientIp,app_id);
    result = mp_start_session(session_id,sockets[index].clientIp,acc_ip,atoi(port),report_ip,atoi(report_port),app_id,timer_ms ,mp_version_num);

    if (result!=-1) {
      if(cJSON_HasObjectItem(jobjReceived,"config")) {
        jconfig = cJSON_GetObjectItemCaseSensitive(jobjReceived,"config");
        char *BQW_enabled_string = NULL;
        char *DECA_enabled_string = NULL;
        char *probability_string = NULL;
        struct apconfig config;
        config.BQW_enabled = 1;
        config.DECA_enabled = 1;

        //for test RS algorithm
        //config.RS_enabled = 1;

        int BQW_probability = 0;
        int DECA_probability = 0;
        srand((unsigned)(time(NULL)));
        int rand_num = rand() % 100;

        int BQW_hit = 0;
        int DECA_hit = 0;      

        DEBUG_LOG_I("[Config]rand_num:%d",rand_num);
      
        //processing config array
        //TODO: fix segmentation fault when get error config
        for (i = 0 ; i < cJSON_GetArraySize(jconfig) ; i++) {
          cJSON * subitem = cJSON_GetArrayItem(jconfig, i);
          BQW_enabled_string = NULL;
          probability_string = NULL;
          DECA_enabled_string = NULL;
          if (subitem!=NULL) {
            if(cJSON_HasObjectItem(subitem,"BQW_enabled")) {
                BQW_enabled_string = cJSON_GetObjectItemCaseSensitive(subitem, "BQW_enabled")->valuestring;
                DEBUG_LOG_I("[Config]Check BQW_enabled");
                if(cJSON_HasObjectItem(subitem,"probability")) {
                  probability_string = cJSON_GetObjectItemCaseSensitive(subitem, "probability")->valuestring;
                  BQW_probability += atoi(probability_string);
                  DEBUG_LOG_I("[Config]BQW_probability:%d",BQW_probability);
                  if ((BQW_hit != 1) && (BQW_probability >= rand_num)) {
                    config.BQW_enabled = atoi(BQW_enabled_string);
                    DEBUG_LOG_I("probability hit, BQW_Enabled:%d",config.BQW_enabled);
                    BQW_hit = 1; 
                  }
                } else {
                  DEBUG_LOG_E("Cannot find probability in config item");
                  continue;
                }
                //free(BQW_enabled_string);
                //free(probability_string);
                BQW_enabled_string = NULL;
                probability_string = NULL;
            } else if(cJSON_HasObjectItem(subitem,"DECA_enabled")) {
                DEBUG_LOG_I("[Config]Check DECA_enabled");
                DECA_enabled_string = cJSON_GetObjectItemCaseSensitive(subitem, "DECA_enabled")->valuestring;
                if(cJSON_HasObjectItem(subitem,"probability")) {
                  probability_string = cJSON_GetObjectItemCaseSensitive(subitem, "probability")->valuestring;
                  DECA_probability += atoi(probability_string);
                  DEBUG_LOG_I("[Config]DECA_probability:%d",DECA_probability);
                  if((DECA_hit!=1) && (DECA_probability>=rand_num)) {
                    config.DECA_enabled = atoi(DECA_enabled_string);
                    DEBUG_LOG_I("probability hit, DECA_Enabled:%d",config.DECA_enabled);
                    DECA_hit = 1; 
                  }
                } else {
                  DEBUG_LOG_E("Cannot find probability in config item");
                  continue;
                }
                //free(DECA_enabled_string);
                //free(probability_string);
                DECA_enabled_string = NULL;
                probability_string = NULL;
            } else {
              DEBUG_LOG_E("Cannot find BQW_enabled/DECA_enabled in config item");
              continue;
            }
          }
        }

        //check if config apply success, if failed , use BQW_enabled:0
        if (mp_apply_config(session_id,config,report)!=0) {
            DEBUG_LOG_E("mp_apply_config error");
            sprintf(report,"error:appply_failed_use_0");
        }

      } else {
        DEBUG_LOG_E("Cannot find config in JSON");
        sprintf(report,"error:json_config");
      }      
    } else {
      sprintf(report,"error:mp_start_session");
    }
  } else if (result==1) {
    sprintf(report,"error:instance id exists");
  } else {
    sprintf(report,"error:json_data_error");
  }
    
  if(result==0) {
    DEBUG_LOG_I("Session ID:%s Start Success,report:%s\n",session_id,report);
  } else {
    DEBUG_LOG_E("Session ID:%s Start Failed\n",session_id);
  }
  send_handshake_response(index,result,mp_version_num,report);

}

void send_handshake_response(int index,int result,int mp_version_num,char* report) 
{
    cJSON *jobjToSend = cJSON_CreateObject();
    char result_str[32];
    sprintf(result_str, "%d", result);
    char ap_version_str[32];
    char *request;
    
    sprintf(ap_version_str, "%d", mp_version_num); 

  cJSON_AddItemToObject(jobjToSend, "cmd", cJSON_CreateString("start.handshake.rsp"));
  cJSON_AddItemToObject(jobjToSend, "result", cJSON_CreateString(result_str));
  if (report!=NULL) {
    cJSON_AddItemToObject(jobjToSend, "report", cJSON_CreateString(report));
  } else {
    cJSON_AddItemToObject(jobjToSend, "report", cJSON_CreateString("empty_report"));
  }
  
  cJSON_AddItemToObject(jobjToSend, "apversion", cJSON_CreateString(ap_version_str));

  request = cJSON_PrintUnformatted(jobjToSend);
  handleSendData(index, request, strlen(request)+1);
  cJSON_free(request);
  cJSON_Delete(jobjToSend);
}

int handle_quit_cmd(cJSON *jobjReceived)
{
  return 0;
}

int handle_set_WMM_traffic(cJSON *jobjReceived)
{
  int wlan_interface = cJSON_GetObjectItemCaseSensitive(jobjReceived, "WLAN_Interface")->valueint;
  char *server_ip_address = cJSON_GetObjectItemCaseSensitive(jobjReceived, "Server_IP")->valuestring;
  int net_mask_length = cJSON_GetObjectItemCaseSensitive(jobjReceived, "Net_Mask_Length")->valueint;
  int server_port = cJSON_GetObjectItemCaseSensitive(jobjReceived, "Server_Port")->valueint;
  int ac_queue_tos = cJSON_GetObjectItemCaseSensitive(jobjReceived, "AC_Queue_TOS")->valueint;

  set_wmm_traffic(wlan_interface,server_ip_address,net_mask_length,server_port,ac_queue_tos );
  return 1; //TODO: use result code instead of 1
}

int handle_clear_WMM_traffic(cJSON *jobjReceived)
{
  clear_wmm_traffic(); 
  return 1; //TODO: use result code instead of 1
}

int handle_set_WMM_parameter(cJSON *jobjReceived)
{

  char *ac_queue_name = cJSON_GetObjectItemCaseSensitive(jobjReceived, "AC_QUEUE_NAME")->valuestring;
  char *parameter_name = cJSON_GetObjectItemCaseSensitive(jobjReceived, "AC_QUEUE_PARAMETER")->valuestring;
  char *parameter_number = cJSON_GetObjectItemCaseSensitive(jobjReceived, "AC_QUEUE_PARAMETER_VALUE")->valuestring;

  set_wmm_parameter(ac_queue_name,parameter_name,parameter_number);
  return 1; //TODO: use result code instead of 1
}
