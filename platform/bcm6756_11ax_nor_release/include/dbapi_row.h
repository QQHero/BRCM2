/******************************************************************************

  Copyright (C), 1999-2017, Tenda Tech Co., Ltd.

 ******************************************************************************
  File Name     : row.h
  Version       : 1.0
  Author        : zengfanfan
  Created       : 2017/6/3
  Last Modified :
  Description   : declaration of attributes and operations of sqlite3 row
  Function List :
  History       :

******************************************************************************/
#ifndef LIB_DBAPI_ROW_H
#define LIB_DBAPI_ROW_H

#include "sqlite3.h"
#include "list.h"
#include "dbapi_column.h"

typedef struct
{
    struct list_head link;
    int id;
    int refer;
    int col_num;
    db_val_t values[DB_MAX_COL_NUM];
    char data[0];
} db_row_t;

#define ival(idx) values[idx].i
#define lval(idx) values[idx].l
#define fval(idx) values[idx].f
#define dval(idx) values[idx].d
#define sval(idx) values[idx].s

typedef db_row_t *db_row_ptr_t;

#endif // LIB_DBAPI_ROW_H