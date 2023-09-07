/* 
* <:copyright-BRCM:2002:proprietary:standard
* 
*    Copyright (c) 2002 Broadcom 
*    All Rights Reserved
* 
*  This program is the proprietary software of Broadcom and/or its
*  licensors, and may only be used, duplicated, modified or distributed pursuant
*  to the terms and conditions of a separate, written license agreement executed
*  between you and Broadcom (an "Authorized License").  Except as set forth in
*  an Authorized License, Broadcom grants no license (express or implied), right
*  to use, or waiver of any kind with respect to the Software, and Broadcom
*  expressly reserves all rights in and to the Software and all intellectual
*  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
*  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
*  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
* 
*  Except as expressly set forth in the Authorized License,
* 
*  1. This program, including its structure, sequence and organization,
*     constitutes the valuable trade secrets of Broadcom, and you shall use
*     all reasonable efforts to protect the confidentiality thereof, and to
*     use this information only in connection with your use of Broadcom
*     integrated circuit products.
* 
*  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
*     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
*     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
*     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
*     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
*     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
*     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
*     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
*     PERFORMANCE OF THE SOFTWARE.
* 
*  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
*     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
*     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
*     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
*     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
*     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
*     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
*     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
*     LIMITED REMEDY.
* :>
*/

/******************************************************************************
 *   FileName        : bcm_layout.c
 *   Purpose         : usefull functions for layout types
 *   Limitations     : None
 *   Creation Date   : 17-Sep-2001
 *   Current Release : $Name: $
 ******************************************************************************/

#include "bcm_BasicTypes.h"
#include "bcm_layout.h"

int sizeOfType(typeLayout *layout)
{
  int i = 0;
  int size = 0;
  
  while (layout[i].size != 0) 
  {
    if (layout[i].layout != NULL)
      size += layout[i].size * sizeOfType(layout[i].layout);
    else
      size += layout[i].size;
    i++;
  }
  
  return size;
}

int sizeOfRequestMsg(MsgLayout *layout)
{
  return (sizeOfType(layout->requestMsgLayout));
}

int sizeOfReplyMsg(MsgLayout *layout)
{
  return (sizeOfType(layout->replyMsgLayout));
}

/* empty layout structure for message having no request/reply payload */
typeLayout emptyLayout[]  = { { 0, NULL } };
typeLayout uint8Layout[]  = { { sizeof(uint8),  NULL}, { 0, NULL } };
typeLayout uint16Layout[] = { { sizeof(uint16), NULL}, { 0, NULL } };
typeLayout uint32Layout[] = { { sizeof(uint32), NULL}, { 0, NULL } };
#ifdef BCM_UINT64_SUPPORT_REQUIRED
typeLayout uint64Layout[] = { { 2, uint32Layout}, { 0, NULL } };
#else
typeLayout uint64Layout[] = { { sizeof(uint64), NULL}, { 0, NULL } };
#endif

