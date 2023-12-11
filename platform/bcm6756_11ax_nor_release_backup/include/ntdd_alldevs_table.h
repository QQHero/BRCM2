#ifndef __NTDD_ALLDEVS_TABLE_H__
#define __NTDD_ALLDEVS_TABLE_H__

#include "dbapi_table.h"

enum {
    NTDD_ALLDEVS_COL_MAC,
    NTDD_ALLDEVS_COL_IP,
    NTDD_ALLDEVS_COL_TYPE,
    NTDD_ALLDEVS_COL_PORT,
    NTDD_ALLDEVS_COL_SN,
    NTDD_ALLDEVS_COL_PROJECT_ID,
    NTDD_ALLDEVS_COL_IS_NEIGH,
    NTDD_ALLDEVS_COL_IS_ACTIVE,
    NTDD_ALLDEVS_COL_NUM
};

enum {
    NTDD_ALLDEVS_STATUS_OFFLINE = 0,
    NTDD_ALLDEVS_STATUS_ONLINE  = 1
};

#define NTDD_ALLDEVS_MAC_SIZE            32
#define NTDD_ALLDEVS_IP_SIZE             32
#define NTDD_ALLDEVS_TYPE_SIZE           64
#define NTDD_ALLDEVS_SN_SIZE             64
#define NTDD_ALLDEVS_PROJECT_ID_SIZE     64

#define DB_DELAY_SYNC_OFF     0
#define DB_DELAY_SYNC_ON      1

#define NET_TOPO_DB_NAME      "network_topo"

static db_table_t *ntdd_get_alldevs_table(void)
{
    static db_table_t table;
    static uint8_t delay_flag = 0;
    static db_col_t cols[] = {
        DB_COL_SET_STR_UNQ(NTDD_ALLDEVS_COL_MAC, "mac", NTDD_ALLDEVS_MAC_SIZE),
        DB_COL_SET_STR(NTDD_ALLDEVS_COL_IP, "ip", NTDD_ALLDEVS_IP_SIZE),
        DB_COL_SET_STR(NTDD_ALLDEVS_COL_TYPE, "type", NTDD_ALLDEVS_TYPE_SIZE),
        DB_COL_SET_INT(NTDD_ALLDEVS_COL_PORT, "port"),
        DB_COL_SET_STR(NTDD_ALLDEVS_COL_SN, "sn", NTDD_ALLDEVS_SN_SIZE),
        DB_COL_SET_STR(NTDD_ALLDEVS_COL_PROJECT_ID, "project_id", NTDD_ALLDEVS_PROJECT_ID_SIZE),
        DB_COL_SET_INT(NTDD_ALLDEVS_COL_IS_NEIGH, "is_neigh"),
        DB_COL_SET_INT(NTDD_ALLDEVS_COL_IS_ACTIVE, "is_active"),
    };

    if (!db_table_init(NET_TOPO_DB_NAME, &table, "alldevs", cols, NTDD_ALLDEVS_COL_NUM)) {
        return NULL;
    }
    if(!delay_flag) {
        db_table_set_delay_sync_memdb(&table , DB_DELAY_SYNC_ON);
        delay_flag = 1;
    }
    
    return &table;
}

#endif

