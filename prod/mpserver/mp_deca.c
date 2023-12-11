#include "TencentWiFi.h"


#define SLIDING_WINDOW_MAXSIZE 20
int window_size = 5;
float tx_attempt_UL = 1.1;
float tx_attempt_LL = 1.02;
int rssi_UL = -65;
int rssi_LL = -65;

uint8_t window_full = 0;

int tx_attempt_window[SLIDING_WINDOW_MAXSIZE];
float rssi_window[SLIDING_WINDOW_MAXSIZE];
uint8_t window_index; 

void deca_init(){

   for(int i= 0; i < window_size; i++){
        tx_attempt_window[i] = 0;
        rssi_window[i] = 0;
   }
   window_index = 0;
}

int check_over_UL(float num[], float UL, int size){
   
   for(int i=0;i<size; i++){
      if(num[i] < UL)
         return 0;
   }
   return 1;
}

int check_under_LL(float num[], float LL, int size){
   
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
   if(window_index >= window_size){
      window_index = 0;
      window_full = 1;
   }

   //only consider deca_enabled refresh after window is fully occupied
   if(window_full){

      if((!deca_enabled) && check_over_UL(tx_attempt_window, tx_attempt_UL, window_size) && check_under_LL(rssi_window, rssi_LL, window_size)){
         deca_start_probe(info->freq_band);
         return 1;
      }

      if((deca_enabled) && check_under_LL(tx_attempt_window, tx_attempt_LL, window_size) && check_over_UL(rssi_window, rssi_UL, window_size)){
         deca_stop_probe(info->freq_band);
         return 0;
      }

   }

   return deca_enabled;

}

int deca_start_probe(int freq_band){


   printf("[DECA]start probe\n");
   int result = turn_on_rts(freq_band);
   return result;

}

int deca_stop_probe(int freq_band){

   printf("[DECA]stop probe\n");
   int result = turn_off_rts(freq_band);
   return result;

}

