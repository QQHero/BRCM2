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

#include "bcmadsl.h"
#include "../softdsl/AdslCoreDefs.h"
#include "../BcmAdslCore.h"
#include "../AdslCore.h"
#include "../BcmOs.h"

#include "bcm_DrvXface.h"
#include "../softdsl/SoftDsl.h"
#include "../softdsl/AdslMib.h"
#include <linux/semaphore.h>

#if (kXDslSetConfigGfast != kDslSetConfigGfast)
#error Inconsistent kXDslSetConfigGfast definition
#endif
#if (kXDslSetRfiConfigGfast != kDslSetRfiConfigGfast)
#error Inconsistent kXDslSetRfiConfigGfast definition
#endif
#if (kXDslSetHmiCoreConfig != kDslSetHmiCoreConfig)
#error Inconsistent kXDslSetHmiCoreConfig definition
#endif
#if (kXDslSetHmiDtaConfig != kDslSetHmiLineSetDtaConfig)
#error Inconsistent kXDslSetHmiDtaConfig definition
#endif
#if (kXDslSetHmiChangeMds != kDslSetHmiLineChangeMds)
#error Inconsistent kXDslSetHmiChangeMds definition
#endif
#if (kXDslSetLineIntroduceErrors != kDslSetLineIntroduceErrors)
#error Inconsistent kXDslSetLineIntroduceErrors definition
#endif
#if (kXDslSetLogicalFrameConfig != kDslSetLogicalFrameConfig)
#error Inconsistent kXDslSetLogicalFrameConfig definition
#endif

extern int bigEndianByteSwap(void *data, void *layout);
extern void *XdslCoreGetDslVars(unsigned char lineId);

XdslCoreConfig xdslCoreCfg = { 2, 20, 28, 36, 8, 10, 0, 0};

void XdslDrvConnectionStart(unsigned char lineId)
{
	BcmAdslCoreConnectionStart(lineId);
}

void XdslDrvConnectionStop(unsigned char lineId)
{
	BcmAdslCoreConnectionStop(lineId);
}

void XdslDrvSetTestMode(unsigned char lineId, int testMode)
{
	BcmAdslCoreSetTestMode(lineId, testMode);
}

void XdslDrvSetL3Mode(unsigned char lineId)
{
	BcmAdslCoreSetTestMode(lineId, ADSL_TEST_L3);
}

void XdslDrvSendHmiCmd(unsigned char lineId, int cmd, int size, void *data, void *layout)
{
  void  *pData;

  gSharedMemAllocFromUserContext=1;
  pData = AdslCoreSharedMemAlloc(size);
  memcpy(pData, data, size);
#ifdef ADSLDRV_LITTLE_ENDIAN
  if (layout != NULL)
    bigEndianByteSwap(pData, layout);
#endif
  BcmXdslCoreSendHmiConfig(lineId, cmd, pData, size);
  gSharedMemAllocFromUserContext = 0;
}

unsigned long XdslDrvGetTime(void)
{
	unsigned long cTime;

	bcmOsGetTime(&cTime);
	return (cTime * BCMOS_MSEC_PER_TICK);
}

int XdslDrvIsXdsl2Mod(unsigned char lineId)
{
	return XdslMibIsXdsl2Mod(XdslCoreGetDslVars(lineId));
}

int XdslDrvIsGfastMod(unsigned char lineId)
{
	return XdslMibIsGfastMod(XdslCoreGetDslVars(lineId));
}

int XdslDrvIsVendorIdReceived(unsigned char lineId)
{
	return XdslMibIsVendorIdReceived(XdslCoreGetDslVars(lineId));
}

extern int printk(const char *fmt, ...);
extern uint gFastTestModeReq;

void XdslDrvSetGfastTestMode(
	unsigned char lineId,
	unsigned char rtxTestModeDs,
	unsigned char rtxTestModeUs,
	unsigned char tpsTcTestModeDs,
	unsigned char tpsTcTestModeUs)
{
	printk("%s->lineId %d: rtxTestModeDs=%d rtxTestModeUs=%d tpsTcTestModeDs=%d tpsTcTestModeUs=%d\n",
		__FUNCTION__, lineId, rtxTestModeDs, rtxTestModeUs, tpsTcTestModeDs, tpsTcTestModeUs);
	
	if(rtxTestModeDs)
		gFastTestModeReq |= GFAST_START_RTX_TESTMODE;
	if(rtxTestModeUs)
		BcmXdslCoreSendCmd(lineId, kDslGfastRTXTestModeCmd, kDslGfastRTXTestModeStart);
	
	if(tpsTcTestModeDs == tpsTcTestModeUs) {	/* Both directions has to be either enabled or disabled */
		if(tpsTcTestModeDs) {
			gFastTestModeReq |= GFAST_START_TPS_TESTMODE;
			BcmXdslCoreSendCmd(lineId, kDslGfastTPSTestModeCmd, kDslGfastTPSTestModeStart);
		}
	}
}

void XdslDrvIntroduceGfastErrors(	unsigned char lineId, unsigned int testMode)
{
	printk("%s->lineId %d: testMode 0x%08X gFastTestModeReq 0x%08X\n", __FUNCTION__, lineId, testMode, gFastTestModeReq);
	
	gFastTestModeReq |= testMode;
	switch(testMode) {
		case GFAST_START_RTX_TESTMODE:
			BcmXdslCoreSendCmd(lineId, kDslGfastRTXTestModeCmd, kDslGfastRTXTestModeStart);
			break;
		case GFAST_STOP_RTX_TESTMODE:
			BcmXdslCoreSendCmd(lineId, kDslGfastRTXTestModeCmd, kDslGfastRTXTestModeStop);
			break;
		case GFAST_START_TPS_TESTMODE:
			BcmXdslCoreSendCmd(lineId, kDslGfastTPSTestModeCmd, kDslGfastTPSTestModeStart);
			break;
		case GFAST_STOP_TPS_TESTMODE:
			BcmXdslCoreSendCmd(lineId, kDslGfastTPSTestModeCmd, kDslGfastTPSTestModeStop);
			break;
		default:
			gFastTestModeReq &= ~testMode;  /* Will never go here: only call with supported testMode */
			break;
	}
}

void XdslDrvSleep(unsigned int timeMs)
{
	bcmOsSleep(timeMs/BCMOS_MSEC_PER_TICK);
}

extern OS_SEMID	syncPhyMipsSemId;

void XdslDrvSleepWithWakeupEvent(unsigned int timeMs)
{
	OS_TICKS osTime;
	OS_STATUS nRet;
	
	sema_init((struct semaphore *)syncPhyMipsSemId, 0);
	bcmOsGetTime(&osTime);
	nRet = bcmOsSemTake(syncPhyMipsSemId, (timeMs/BCMOS_MSEC_PER_TICK));
	if(OS_STATUS_FAIL == nRet)
		bcmOsSleep(timeMs/BCMOS_MSEC_PER_TICK);
	DiagWriteString(0, DIAG_DSL_CLIENT, "%s: delayed %d ms, nRet(%d)\n", __FUNCTION__, (int)BcmAdslCoreOsTimeElapsedMs(osTime), (int)nRet);
}

