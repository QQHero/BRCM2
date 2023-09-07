#include "TencentWiFi.h"


#define SLIDING_WINDOW_MAXSIZE 20
#define DECA_SUPPORTED 1
#define RTS_FAILED_RATE_THRESHOLD 0.3
#define WINDOW_SIZE 3

float tx_attempt_UL = 1.2;
float tx_attempt_LL = 1.02;
int rssi_UL = -55;
int rssi_LL = -63;
int BROKEN_THRESH = 1000;


float tx_attempt_window[SLIDING_WINDOW_MAXSIZE];
int rssi_window[SLIDING_WINDOW_MAXSIZE];
uint8_t window_index; 
uint8_t window_full;


int hal_deca_supported(){
   return DECA_SUPPORTED;
}


void deca_init(){

   for(int i= 0; i < WINDOW_SIZE; i++){
        tx_attempt_window[i] = 0;
        rssi_window[i] = 0;
   }
   window_index = 0;
   window_full = 0;
}

int check_over_ULFLOAT(float num[], float UL, int size){
   
   for(int i=0;i<size; i++){
      if(num[i] < UL)
         return 0;
   }
   return 1;
}

int check_under_LLFLOAT(float num[], float LL, int size){
   
   for(int i=0; i<size; i++){
      if(num[i] > LL)
         return 0;
   }
   return 1;
}

int check_over_ULINT(int num[], int UL, int size){
   
   for(int i=0;i<size; i++){
      if(num[i] < UL)
         return 0;
   }
   return 1;
}

int check_under_LLINT(int num[], int LL, int size){
   
   for(int i=0; i<size; i++){
      if(num[i] > LL)
         return 0;
   }
   return 1;
}

int deca_policy_refresh(wifi_info* info, int deca_enabled){

   tx_attempt_window[window_index] = info->tx_attempt_avg;
   rssi_window[window_index] = info->rssi;


   window_index += 1;
   if(window_index >= WINDOW_SIZE){
      window_index = 0;
      window_full = 1;
   }

   //printf("[deca_policy_refresh] check_under_LL_rssi:%d\n", check_under_LLINT(rssi_window, rssi_LL, WINDOW_SIZE));

   if(info->broken_signal_cnt > BROKEN_THRESH) {
      deca_enabled = 1;
   } else {
      //only consider deca_enabled refresh after window is fully occupied
      if(window_full){      
         if((deca_enabled == 0) && check_over_ULFLOAT(tx_attempt_window, tx_attempt_UL, WINDOW_SIZE) && check_under_LLINT(rssi_window, rssi_LL, WINDOW_SIZE)){
            deca_enabled = 1;
         }
         else if((deca_enabled == 1) && check_over_ULINT(rssi_window, rssi_UL, WINDOW_SIZE)){
            deca_enabled = 0;
         }
      }
   }
   return deca_enabled;
}

int deca_policy_refresh_richard(wifi_info* info, int deca_enabled){


   tx_attempt_window[window_index] = info->tx_attempt_avg;
   rssi_window[window_index] = info->rssi;


   window_index += 1;
   if(window_index >= WINDOW_SIZE){
      window_index = 0;
      window_full = 1;
   }


   //only consider deca_enabled refresh after window is fully occupied
   if(window_full){

   /*
      for(int i= 0; i < WINDOW_SIZE; i++){
         printf("tx_attempt_avg[%d]:%f\n", i, tx_attempt_window[i] );
         printf("rssi[%d]:%d\n", i, rssi_window[i]);
      }
   */


   //printf("[deca_policy_refresh] deca_enabled:%d\n", deca_enabled);
   //printf("[deca_policy_refresh] check_over_UL_txattempt:%d\n", check_over_UL(tx_attempt_window, tx_attempt_UL, WINDOW_SIZE) );
   //printf("[deca_policy_refresh] check_under_LL_rssi:%d\n", check_under_LLINT(rssi_window, rssi_LL, WINDOW_SIZE));
   //printf("[deca_policy_refresh] check_over_UL_rssi:%d\n", check_over_ULINT(rssi_window, rssi_UL, WINDOW_SIZE));


      
      if((deca_enabled == 0) && check_over_ULFLOAT(tx_attempt_window, tx_attempt_UL, WINDOW_SIZE) && check_under_LLINT(rssi_window, rssi_LL, WINDOW_SIZE)){
         //deca_start_probe(info->freq_band);
         deca_enabled = 1;
         //printf("deca_policy_refresh1:%d\n", deca_enabled);
         return deca_enabled;
      }

      else if((deca_enabled == 1) && check_under_LLFLOAT(tx_attempt_window, tx_attempt_LL, WINDOW_SIZE) && check_over_ULINT(rssi_window, rssi_UL, WINDOW_SIZE)){
         //deca_stop_probe(info->freq_band);
         deca_enabled = 0;
         //printf("deca_policy_refresh2:%d\n", deca_enabled);
         return deca_enabled;
      }

   }

   
   //printf("deca_policy_refresh3:%d\n", deca_enabled);

   return deca_enabled;

}

int deca_start_probe(int freq_band){


  // printf("[DECA]start probe\n");
   int result = turn_on_rts(freq_band);
   return result;

}

int deca_stop_probe(int freq_band){

   //printf("[DECA]stop probe\n");
   int result = turn_off_rts(freq_band);
   return result;

}

int deca_probe_failed_check(wifi_info* new_wifi_info){

  // printf("[DECA]deca_probe_failed_check\n");
   if((new_wifi_info->rts_tx_cnt > 0) && (new_wifi_info->rts_failed_cnt > 0)) {
      float failed_rate = (float)new_wifi_info->rts_failed_cnt / (float) new_wifi_info->rts_tx_cnt;
      //printf("rts failed rate:%f\n", failed_rate);
      if(failed_rate >  RTS_FAILED_RATE_THRESHOLD)
      {
         printf("rts failed rate over limit\n");
         return 1;

      }
   
   }

	return 0;

}
