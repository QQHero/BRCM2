#ifndef __PKT_RUNNER_H_INCLUDED__
#define __PKT_RUNNER_H_INCLUDED__

/*
<:copyright-BRCM:2013:proprietary:standard

   Copyright (c) 2013 Broadcom 
   All Rights Reserved

 This program is the proprietary software of Broadcom and/or its
 licensors, and may only be used, duplicated, modified or distributed pursuant
 to the terms and conditions of a separate, written license agreement executed
 between you and Broadcom (an "Authorized License").  Except as set forth in
 an Authorized License, Broadcom grants no license (express or implied), right
 to use, or waiver of any kind with respect to the Software, and Broadcom
 expressly reserves all rights in and to the Software and all intellectual
 property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
 NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
 BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.

 Except as expressly set forth in the Authorized License,

 1. This program, including its structure, sequence and organization,
    constitutes the valuable trade secrets of Broadcom, and you shall use
    all reasonable efforts to protect the confidentiality thereof, and to
    use this information only in connection with your use of Broadcom
    integrated circuit products.

 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
    PERFORMANCE OF THE SOFTWARE.

 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
    LIMITED REMEDY.
:> 
*/


#include "pktrunner_common.h"

#ifndef CONFIG_BCM_RUNNER_MAX_FLOWS
#define PKTRUNNER_MAX_FLOWS       (16384)
#else
#define PKTRUNNER_MAX_FLOWS       (CONFIG_BCM_RUNNER_MAX_FLOWS) 
#endif
#define PKTRUNNER_MAX_FLOWS_MCAST_WHITELIST 1024
#define PKTRUNNER_MAX_FLOWS_MCAST 8192


#define FC_ACTIONS_NOT_AFFECTING_L2 (FC_ACTIONS_NOT_AFFECTING_PAYLOAD | rdpa_fc_action_ttl | rdpa_fc_action_dscp_remark | \
    rdpa_fc_action_nat | rdpa_fc_action_gre_remark | rdpa_fc_action_spdsvc)

#define FC_ACTIONS_NOT_AFFECTING_PAYLOAD  (rdpa_fc_action_no_forward | rdpa_fc_action_policer | rdpa_fc_action_service_q | \
    rdpa_fc_action_spdt_gen | rdpa_fc_action_skip_l2_hdr_copy | rdpa_fc_action_skip_hdr_copy) 

int __init runnerHost_construct(void);
void __exit runnerHost_destruct(void);

int __init runnerProto_construct(void);
void __exit runnerProto_destruct(void);

#endif  /* defined(__PKT_RUNNER_H_INCLUDED__) */
