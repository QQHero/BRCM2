
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


#include "bcm_BasicTypes.h"
#include "HpiMessage.h"
#include "CoreMessageDef.h"
#include "DiagDef.h"
#include "bcm_DrvXface.h"

#define MAX_CORE_SERVER (NUMBER_OF_CORES-1)
#define CORE_SERVICE_NUMBER 0x7d
int16 coreServices[CORE_SERVICE_NUMBER][2] =
{
  /* 0 */ { -1, 0},                       /* undefined */
  /* 1 */ {sizeof(SantoriniConfig),       MAX_CORE_SERVER}, /* SANT_SET_CONFIG */
  /* 2 */ {0,                             MAX_CORE_SERVER}, /* SANT_GET_CONFIG */
  /* 3 */ {0,                             0}, /* SANT_GET_LINE_ISR */ 
  /* 4 */ {sizeof(SantChangeLineIMR),     0}, /* SANT_CHANGE_LINE_IMR */
  /* 5 */ {0,                             MAX_CORE_SERVER}, /* SANT_RESET_DEVICE */
  /* 6 */ {-1,                            0}, /* SANT_DEV_MIN_EXCEPT: cannot be called */
  /* 7 */ {0,                             MAX_CORE_SERVER}, /* SANT_GET_SOFTWARE_VERSION */
  /* 8 */ {sizeof(GPoutData),             0}, /* SANT_WRITE_GP_OUT */
  /* 9 */ {0,                             0}, /* SANT_READ_GP_IN */
  /* A */ {-1, 0},                            /* Unused old SANT_READ_SMEM */
  /* B */ {-1,                            0}, /* undefined */
  /* C */ {sizeof(SetOAMperiodElapsedTime),  MAX_CORE_SERVER}, /* SANT_SET_OAM_PERIOD_ELAPSED_TIME */
  /* D */ {sizeof(OEMid),                    MAX_CORE_SERVER}, /* SANT_SET_OEM_ID */
  /* E */ {sizeof(PtpTime),               0}, /* SANT_SET_PTP_TIME */
  /* F */ {sizeof(DspToBoardId),          0}, /* SANT_MAP_DSP_TO_BOAD_ID */
  /* 10 */ {-1,                           0}, /* undefined */
  /* 11 */ {-1,                           0}, /* undefined */
  /* 12 */ {-1,                           0}, /* undefined */
  /* 13 */ {-1,                           0}, /* undefined */
  /* 14 */ {sizeof(uint64),               MAX_CORE_SERVER}, /* SANT_SET_INTER_DEVICE_TR_CONFIG */
  /* 15 */ {-1,                           0}, /* undefined */
  /* 16 */ {sizeof(SchedMonConfig),       MAX_CORE_SERVER}, /* SANT_SET_SCHEDULING_CONF */
  /* 17 */  {0,                           MAX_CORE_SERVER}, /* SANT_GET_SCHEDULING_PROF */
  /* 18 */ {sizeof(I2C_request),          MAX_CORE_SERVER}, /* ACCESS_I2C  */
  /* 19 */ {sizeof(VCXO_request),         MAX_CORE_SERVER}, /* Si57X_SETFREQ */
  /* 1A */ {sizeof(uint16),               MAX_CORE_SERVER}, /* SANT_GET_NTR_TOD_DELTA */
  /* 1B */ {-1,                           0}, /* undefined */
  /* 1C */ {-1,                           0}, /* undefined */
  /* 1D */ {-1,                           0}, /* undefined */
  /* 1E */ {-1,                           0}, /* undefined */
  /* 1F */ {-1,                           0}, /* undefined */
  /* 20 */ {-1,                           0}, /* undefined */
  /* 21 */ {-1,0},
  /* 22 */ {-1,0},
  /* 23 */ {sizeof(GlobalDtaConfig),0},
  /* 24 */ {0,0},
  /* 25 */ {-1,0},
  /* 26 */ {sizeof(LineDriverDependentConfigExt),  0}, /* SANT_SET_EXTRA_LD_CONFIG */
  /* 27 */ {sizeof(LedPattern),           0}, /* SANT_SET_LED_PATTERN */
  /* 28 */ {sizeof(TxFilterDef),          MAX_CORE_SERVER}, /* SANT_SET_TXFILTER */ 
  /* 29 */ {sizeof(AfeConnection),        MAX_CORE_SERVER}, /* SANT_SET_AFE_CONNECT */
  /* 2A */ {0,                            MAX_CORE_SERVER}, /* SANT_GET_EXPECTED_LINE_NUMBER */
  /* 2B */ {-1,                           0}, /* undefined*/
  /* 2C */ {sizeof(SetGpioLedPattern),    MAX_CORE_SERVER}, /* SANT_SET_GPIO_LED_PATTERN */
  /* 2D */ {-1,                           0}, /* undefined*/
  /* 2E */ {-1,0},
  /* 2F */ {-1,0},
  /* 30 */ {-1,0},
  /* 31 */ {-1,0},
  /* 32 */ {-1,0},
  /* 33 */ {-1,0},
  /* 34 */ {-1,0},
  /* 35 */ {-1,0},
  /* 36 */ {-1,0},
  /* 37 */ {sizeof(uint32), MAX_CORE_SERVER}, /* SANT_REPORT_PTMSYNC_ON_GPIO */
  /* 38 */ {-1,0},
  /* 39 */ {-1,0},
  /* 3A */ {-1,0}, /* undefined */
  /* 3B */ {-1,0}, /* undefined */
  /* 3C */ {-1,0}, /* undefined */
  /* 3D */ {-1,0}, /* undefined */
  /* 3E */ {-1,0}, /* undefined */
  /* 3F */ {-1,0}, /* undefined */
  /* 40 */ {-1,0}, /* undefined */
  /* 41 */ {-1,0}, /* undefined */
  /* 42 */ {-1,0}, /* undefined */
  /* 43 */ {-1,0}, /* undefined */
  /* 44 */ {-1,0}, /* undefined */
  /* 45 */ {-1,0}, /* undefined */
  /* 46 */ {-1,0}, /* undefined */
  /* 47 */ {-1,0}, /* undefined */
  /* 48 */ {-1,0}, /* undefined */
  /* 49 */ {-1,0}, /* undefined */
  /* 4a */ {-1,0}, /* undefined */
  /* 4b */ {-1,0}, /* undefined */
  /* 4c */ {-1,0}, /* undefined */
  /* 4d */ {1, MAX_CORE_SERVER},
  /* 4e */ {-1,0}, /* undefined */
  /* 4f */ {-1,0}, /* undefined */
  /* 50 */ {-1,0}, /* undefined */
  /* 51 */ {-1,0}, /* undefined */
  /* 52 */ {-1,0}, /* undefined */
  /* 53 */ {-1,0}, /* undefined */  
  /* 54 */ {-1,0}, /* undefined */  
  /* 55 */ {-1,0}, /* undefined */  
  /* 56 */ {-1,0}, /* undefined */
  /* 57 */ {-1,0}, /* undefined */
  /* 58 */ {-1,0}, /* undefined */
  /* 59 */ {-1,0}, /* undefined */
  /* 5a */ {-1,0}, /* undefined */
  /* 5b */ {-1,0}, /* undefined */
  /* 5c */ {-1,0}, /* undefined */
  /* 5d */ {-1,0}, /* undefined */
  /* 5e */ {-1,0}, /* undefined */
  /* 5f */ {-1,0}, /* undefined */
  /* 60 */ {-1,0}, /* undefined */
  /* 61 */ {-1,0}, /* undefined */
  /* 62 */ {-1,0}, /* undefined */
  /* 63 */ {-1,0}, /* undefined */
  /* 64 */ {-1,0}, /* undefined */
  /* 65 */ {16,0}, /* undefined */
  /* 66 */ {-1,0}, /* undefined */
  /* 67 */ {-1,0}, /* undefined */
  /* 68 */ {-1,0}, /* undefined */
  /* 69 */ {0,0},   /* SANT_GET_DLV_SYNCHRO_STATUS */
  /* 6A */ {-1,0},
  /* 6B */ {-1,0}, /* undefined */
  /* 6C */ {-1,0}, /* undefined */
  /* 6D */ {-1,0}, /* undefined */
  /* 6E */ {-1,0}, /* undefined */
  /* 6F */ {-1,0}, /* undefined */
  /* 70 */ {-1,0},
  /* 71 */ {-1,0},
  /* 72 */ {-1,0},
  /* 73 */ {-1,0},
  /* 74 */ {-1,0},
  /* 75 */ {-1,0},
  /* 76 */ {-1,0},
  /* 77 */ {-1,0},
  /* 78 */ {-1,0},
  /* 79 */ {-1,0},
  /* 7A */ {-1,0},
  /* 7B */ {-1,0},
  /* 7C */ {-1,0},
};

extern int printk(const char *fmt, ...);

void *releaseRequestAllocateReply(void *requestPtr, uint16 size)
{
#define MAX_HMI_PACKET_SIZE 1224

  HpiMsgHeader *ptr = requestPtr;
  HpiMsgHeader *replyPtr;
  
  ptr --; /* align with the header */

  if (requestPtr != NULL)
  {
    replyPtr = ptr;
    /* check whether new size is valid */
    if (size > (MAX_HMI_PACKET_SIZE - sizeof(HpiMsgHeader)))
    {
      //EHcall(ERR_NONREC,"application tries to allocate too big message",size,0);
      printk("%s: application tries to allocate too big message(%d)",__FUNCTION__,size);
      return NULL;
    }
  }
  else
  {
    //EHcall(ERR_NONREC,"try to release a NULL pointed msg",(uint32)ptr,0);
    printk("%s: try to release a NULL pointed msg",__FUNCTION__);
    return NULL;
  }
  return (replyPtr+1);
} 

/* {{{ SDK version */
# define BCM_SDK_MAJOR_VERSION  16
# define BCM_SDK_MINOR_VERSION   3
# define BCM_SDK_FIX_VERSION    -1
# define BCM_SDK_SUBFIX_VERSION -1
/* }}} */
extern void BcmXdslCoreGetVerInfoForHmi(uint8 *versionString, uint16 *fwMajorVersion, uint8 *fwMinorVersion, uint32 *afeId, uint32 *hwChipId);

typedef struct LineIRQ LineIRQ;
struct LineIRQ
{
  int           newAlarms;
  LineInterrupt lineInterrupts;
};

SantoriniConfig   santConfig;
static LineIRQ    lineIRQ;
static uint32     expectedLineNumber = LINES_PER_DEV;

#include <linux/string.h> /* for memset() */

void CoreMgr_init(void)
{
  int i;
  memset(&santConfig, 0, sizeof(santConfig));
  memset(&lineIRQ, 0, sizeof(lineIRQ));
  for(i = 0; i < LINES_PER_DEV; i++) {
    santConfig.lineConfig[i].nwsPort[0].mac = -1;  /* Not used */
    santConfig.lineConfig[i].nwsPort[1].mac = -1;  /* Not used */
  }
}

int Core_getSoftwareVersion(void *msgp,  void **repMsgpp, uint16 *sizep)
{
  uint32 afeId;
  int   result = HPI_SUCCESS;
  SantGetSoftwareVersion_MsgRep *rep;
  
  *sizep = sizeof(SantGetSoftwareVersion_MsgRep);
  rep = releaseRequestAllocateReply(msgp,*sizep);
  *repMsgpp = rep;

  BcmXdslCoreGetVerInfoForHmi(
    rep->data.versionString,
    &rep->data.fwMajorVersion,
    &rep->data.fwMinorVersion,
    &afeId,
    &rep->data.hwChipId);
  
  rep->data.fwFixVersion    = 0;
  rep->data.hmiMajorVersion = BCM_SDK_MAJOR_VERSION;
  rep->data.hmiMinorVersion = BCM_SDK_MINOR_VERSION;
  rep->data.nrOfLinesSupportedInFirmwareVersion = 1;
  rep->data.ldTypeMap = 0;
  rep->data.vcxoVersion = 0;    /* ATU_R only */
  rep->data.buildType = 5;      /* BCM_BUILD_TYPE_GFAST212 */
  rep->data.reserved[0] = (uint8)(afeId & 0xFF);
  rep->data.reserved[1] = (uint8)((afeId >> 8) & 0xFF);
  rep->data.reserved[2] = (uint8)((afeId >> 16) & 0xFF);
  rep->data.afeChipId   = (uint8)((afeId >> 24) & 0xFF);

  return result;
}

  
static int SantoriniGetConfigMessage(void *msgp, void **repMsgpp, uint16 *size)
{
  int result, i , offset;
  int deltaSize;
  int sizeLineConfig;
  SantoriniGetConfig_MsgRep *rep;

  deltaSize = (expectedLineNumber-LINES_PER_DEV)*sizeof(LineConfig);
  *size = sizeof(SantoriniGetConfig_MsgRep)+deltaSize;
  sizeLineConfig = expectedLineNumber*sizeof(LineConfig);

  *repMsgpp = releaseRequestAllocateReply(msgp,*size);

  rep = *repMsgpp;
  memset( &rep->data, 0, sizeof(rep->data) );
  
  for(i=0;i<expectedLineNumber;i++)
    if(i<LINES_PER_DEV)
    {
      rep->data.lineConfig[i].nwsPort[0].channel  = santConfig.lineConfig[i].nwsPort[0].channel;
      rep->data.lineConfig[i].nwsPort[0].mac      = santConfig.lineConfig[i].nwsPort[0].mac;
      rep->data.lineConfig[i].nwsPort[0].flags    = santConfig.lineConfig[i].nwsPort[0].flags;
      rep->data.lineConfig[i].nwsPort[1].channel  = santConfig.lineConfig[i].nwsPort[1].channel;
      rep->data.lineConfig[i].nwsPort[1].mac      = santConfig.lineConfig[i].nwsPort[1].mac;
      rep->data.lineConfig[i].nwsPort[1].flags    = santConfig.lineConfig[i].nwsPort[1].flags;
    }
    else
    {
      rep->data.lineConfig[i].nwsPort[0].channel  = 0;
      rep->data.lineConfig[i].nwsPort[0].mac      = -1;
      rep->data.lineConfig[i].nwsPort[0].flags    = 0;
      rep->data.lineConfig[i].nwsPort[1].channel  = 0;
      rep->data.lineConfig[i].nwsPort[1].mac      = -1;
      rep->data.lineConfig[i].nwsPort[1].flags    = 0;
    }

  /* compare before utopia control address */
  offset = sizeof(santConfig) - sizeof(santConfig.lineConfig);
  memcpy((uint8*) &rep->data + sizeLineConfig,
   &santConfig.lineConfig[LINES_PER_DEV],
   offset);

  /* reset reservedH */
  //memset((uint8*) &rep->data + sizeLineConfig + offset - (sizeof(santConfig) - OFFSET(SantoriniConfig,reservedH)), 0, 11);
  rep->data.repetitionRate = xdslCoreCfg.repetitionRate;
  rep->data.reInitTimeOut = xdslCoreCfg.reInitTimeOut;
  rep->data.gfastTddParamsMds = xdslCoreCfg.gfast_tdd_params_Mds;
  rep->data.gfastTddParamsMf = xdslCoreCfg.gfast_tdd_params_Mf;
  rep->data.gfastTddParamsMsf = xdslCoreCfg.gfast_tdd_params_Msf;
  rep->data.gfastCyclicExtensionM= xdslCoreCfg.gfast_cyclic_extension_m;
  rep->data.enableDtaFraming = xdslCoreCfg.enableDtaFraming;

  rep->data.overwriteConfig = 1;

  result = HPI_SUCCESS;

  return result;
}

extern int LineStateInTraining(unsigned char lineId);
extern void LineStateSet(unsigned char lineId, unsigned char state);
extern void LineInitOvhMsg(unsigned char lineId);
extern void LineCpyCurrent15MinCounter(unsigned char lineId);

void lineIRQ_setStatus(uint32 lineId, uint32 newAlarms)
{
  uint32 currentIsr;
  
  if((INT_SRA_RATE_CHANGE_MASK == newAlarms) && LineStateInTraining(lineId)) {
    LineInitOvhMsg(lineId);
    LineStateSet(lineId, LINE_SHOWTIME);
    newAlarms = INT_LINE_STATE_CHANGE_MASK;
  }
  else if(INT_15MIN_PERIOD_MASK == newAlarms)
    LineCpyCurrent15MinCounter(lineId);
  
  currentIsr = lineIRQ.lineInterrupts.interruptStatus[lineId];
  
  lineIRQ.lineInterrupts.interruptStatus[lineId] = currentIsr | newAlarms;
}

static void lineIRQ_changeMask(uint32 *concernedInterrupts32p, uint32 *newMask32p)
{
  uint32 currentImr;
  uint32 currentIsr;
  uint32 newMask;
  uint32 newImr;
  uint32 concerned;

  uint32 *isrp = &lineIRQ.lineInterrupts.interruptStatus[0];
  uint32 *imrp = &lineIRQ.lineInterrupts.interruptMask[0];
  uint32 *concernedInterruptsp = concernedInterrupts32p;
  uint32 *newMaskp = newMask32p;
  
  uint32 beingUnmasked;

  int i;
  
  /* we need to verify whether we still have some alarms after this call ... */
  
  /* TO DO: Tony - RX_LOCK Enter */
  for (i = 0; i < LINES_PER_DEV; i++)
  {
    currentImr = *(imrp);
    newMask    = *(newMaskp++);
    concerned  = *(concernedInterruptsp++);
    currentIsr = *(isrp);
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "lineIRQ_changeMask: concerned=0x%08x newMask=0x%08x currentIsr=0x%08x currentImr=0x%08x \n",
      concerned, newMask,
      lineIRQ.lineInterrupts.interruptStatus[i], lineIRQ.lineInterrupts.interruptMask[i]);
    
    newImr = (currentImr & ~concerned) | (newMask & concerned);

    /* unmasked interrupts are reset except the INT_IDLE_RESET interrupts */
    beingUnmasked = (currentImr & ~newImr);
    currentIsr = currentIsr & (~beingUnmasked | INT_IDLE_RESET_MASK);
    
    *(imrp++) = newImr;
    *(isrp++) = currentIsr;

    DiagStrPrintf(0, DIAG_DSL_CLIENT, "lineIRQ_changeMask: newIsr = 0x%08x newImr = 0x%08x \n",
      lineIRQ.lineInterrupts.interruptStatus[i], lineIRQ.lineInterrupts.interruptMask[i]);
  }
  
  /* TO DO: Tony - RX_LOCK Exit */
}

static void lineIRQ_getStatus(uint32 *irqStatus32p, uint32 *irqMask32p, uint64 *sumupp, int line_number)
{
  int i;
  int expectLineNumber = (line_number > LINES_PER_DEV) ? LINES_PER_DEV : line_number;
  
  uint32 *isrp = &lineIRQ.lineInterrupts.interruptStatus[0];
  uint32 *imrp = &lineIRQ.lineInterrupts.interruptMask[0];
  uint32 *irqStatusp = &irqStatus32p[0];
  uint32 *irqMaskp = &irqMask32p[0];
  uint64 sumup = 0;
  
  /* TO DO: Tony - RX LOCK Enter */
  /*Mask active interrupts */
  for (i = 0; i < expectLineNumber; i++)
  {
    uint32 currentIsr = *(isrp++);
    uint32 currentImr = *(imrp);
    uint32 seenInterrupts;
    
    *(irqStatusp++) = currentIsr;
    seenInterrupts = currentIsr & ~currentImr;
    if ( seenInterrupts > 0 )
      sumup |= (1 << i);
    *(irqMaskp++) = currentImr;
    *(imrp++) = currentImr | currentIsr;
    DiagStrPrintf(0, DIAG_DSL_CLIENT, "lineIRQ_getStatus: isr = 0x%08x imr = 0x%08x sumup = 0x%08x",
      lineIRQ.lineInterrupts.interruptStatus[i], lineIRQ.lineInterrupts.interruptMask[i], sumup);
  }
#if 0
  /* reset end of arrays if necessary */
  if (line_number > LINES_PER_DEV)
  {
    for (i=LINES_PER_DEV; i<(line_number);i++)
    {
      irqStatus32p[i] = 0;
      irqMask32p[i] = (uint32)-1;
    }
  }
#endif
  *sumupp = sumup;
  /* TO DO: Tony - RX LOCK Exit */
}

extern void LineMgr_changeState(unsigned char lineId, int state);
extern void XdslDrvHpiPost(void *replyPtr, uint16 usedSize, int result);
#define hpi_post  XdslDrvHpiPost

static void CoreHPIserver(void *msgp, uint16 size, uint16 commandId, uint16 hmiVersion)
{

  static const int isSlvVce = 0;
  void *repMsgp = NULL; /* in case the reply is empty the repMsg remains 0 and */
                        /* will be intitialised just before posting the reply */
  int   result = HPI_SUCCESS;
  int   coreId = 0;
  if ((commandId >= CORE_SERVICE_NUMBER) ||
      (coreServices[commandId][0] == -1) ||
      (coreId > coreServices[commandId][1])
     )
  {
    result = HPI_UNKNOWN_COMMAND_ID;
  }
  else
  {
    /* this is a valid msg so check its size */
    if ((size != coreServices[commandId][0])
        && (commandId != SANT_SET_CONFIG)
        && (commandId != SANT_CHANGE_LINE_IMR)
        && (commandId != SANT_SET_AFE_CONNECT)
        )
    {
      result = HPI_WRONG_HEADER_FORMAT;
    }
    else if(!isSlvVce)
    {
      switch (commandId)
      {
      case SANT_SET_CONFIG:
      {
        int restartLine = 0;
        SantoriniSetConfig_MsgReq *req = msgp;
        // TBD: to account for extra lineConfig[] in python (GUI Ok)
        req = (void *) ((char *)req + size - sizeof(SantoriniConfig));
        if((xdslCoreCfg.repetitionRate |= req->data.repetitionRate) ||
          (xdslCoreCfg.reInitTimeOut |= req->data.reInitTimeOut) ||
          (xdslCoreCfg.gfast_tdd_params_Mds != req->data.gfastTddParamsMds) ||
          (xdslCoreCfg.gfast_tdd_params_Mf != req->data.gfastTddParamsMf) ||
          (xdslCoreCfg.gfast_tdd_params_Msf != req->data.gfastTddParamsMsf) ||
          (xdslCoreCfg.gfast_cyclic_extension_m |= req->data.gfastCyclicExtensionM))
          restartLine = 1;
        xdslCoreCfg.repetitionRate = req->data.repetitionRate;
        xdslCoreCfg.reInitTimeOut = req->data.reInitTimeOut;
        xdslCoreCfg.gfast_tdd_params_Mds = req->data.gfastTddParamsMds;
        xdslCoreCfg.gfast_tdd_params_Mf = req->data.gfastTddParamsMf;
        xdslCoreCfg.gfast_tdd_params_Msf = req->data.gfastTddParamsMsf;
        xdslCoreCfg.gfast_cyclic_extension_m = req->data.gfastCyclicExtensionM;
        xdslCoreCfg.enableDtaFraming = req->data.enableDtaFraming;

        if(restartLine) {
          XdslDrvSendHmiCmd(0, kXDslSetHmiCoreConfig, sizeof(XdslCoreConfig), &xdslCoreCfg, NULL);
          XdslDrvConnectionStop(0);
          XdslDrvSleepWithWakeupEvent(3000);
          XdslDrvConnectionStart(0);
          LineMgr_changeState(0, LINE_ITU_HANDSHAKE);
        }
        break;
      }
      case SANT_GET_SOFTWARE_VERSION:
      {
        /* Initialize client HMI version */
        //clientHmiVersion = hmiVersion;
        result = Core_getSoftwareVersion(msgp, &repMsgp, &size);
        break;
      }

      case SANT_GET_CONFIG:
      {
        result = SantoriniGetConfigMessage(msgp,&repMsgp,&size);
        printk("%s: SANT_GET_CONFIG size=%d\n", __FUNCTION__, size);
        break;
      }

      case SANT_GET_LINE_ISR:
      {
        SantoriniGetLineISR_MsgRep *rep;
        uint32 *imrp;
        /* to be changed:
           - compute the expected size
           - compute the pointer to pass to the getStatus
           - pass the expected line number to the getStatus (correct sumup)
        */

        size = sizeof(LineInterrupt) + ((expectedLineNumber-LINES_PER_DEV)*2*sizeof(uint32));

        rep = releaseRequestAllocateReply(msgp,size);
        repMsgp = rep;

        imrp = &rep->data.interruptStatus[0] + expectedLineNumber;
        lineIRQ_getStatus(rep->data.interruptStatus, imrp, &rep->data.interruptSummary, expectedLineNumber);
        result = HPI_SUCCESS;

        break;
      }

      case SANT_CHANGE_LINE_IMR:
      {
        SantoriniChangeLineIMR_MsgReq *req = msgp;
        uint32 expectedSize = expectedLineNumber * 2*sizeof(uint32); /* 8 bytes per expected line_number */
        int i;

        /* add a check on size .... */
        if (size != expectedSize){
          result = HPI_WRONG_HEADER_FORMAT;
        }
        else 
        {
          SantChangeLineIMR newImr;
          memcpy(&newImr.concernedInterrupts[0],
                 &req->data.concernedInterrupts[0],
                 LINES_PER_DEV*sizeof(uint32));
          memcpy(&newImr.newMask[0],
                 (uint8 *)&req->data+(expectedLineNumber*sizeof(uint32)),
                 LINES_PER_DEV*sizeof(uint32));

          for (i=expectedLineNumber; i<LINES_PER_DEV;i++)
          {
            newImr.concernedInterrupts[i] = 0;
          }

          lineIRQ_changeMask(newImr.concernedInterrupts, newImr.newMask);
          result = HPI_SUCCESS;
        }
        break;
      }

      case SANT_RESET_DEVICE:
        //result = Core_handleHmiReset(msgp, &repMsgp, &size);
        break;
      case SANT_SET_OAM_PERIOD_ELAPSED_TIME:
      {
        SetOAMperiodElapsedTime_MsgReq *req = msgp;
        printk("elapsedTime=%u numberOfSecIn15min=%u numberOf15minIn24hour=%u periodInterruptOffset=%u\n",
          req->data.elapsedTime,
          req->data.numberOfSecIn15min,
          req->data.numberOf15minIn24hour,
          req->data.periodInterruptOffset);
        break;
     }
     case SANT_SET_NTP_TIME:
     {  
       SetPtpTime_MsgReq *req=msgp;
       SetPtpTime_MsgRep *rep;
#if defined(PTP_ENABLED) 
       PtpTime currentPtpTime;
       setPtpTime(&currentPtpTime,&req->data);
#endif
       size = sizeof(*rep);
       rep = releaseRequestAllocateReply(msgp,size);
       repMsgp = rep;
#if defined(PTP_ENABLED) 
       rep->data =  currentPtpTime;
#else
       memcpy(&rep->data, &req->data, size);
#endif
       result = HPI_SUCCESS;
     }
     break;  
      default:
        printk("%s: non supported message 0x%X\n", __FUNCTION__, commandId);
        result = HPI_UNKNOWN_COMMAND_ID;
      }
    }
  }

  /* post answer */
  /* in case the reply is an empty one, allocate the reply here */
  if (repMsgp == NULL) {
    size = 0;
    repMsgp = releaseRequestAllocateReply(msgp, size);
  }

  hpi_post(repMsgp, size, result);
#if 0
  DiagStrPrintf(0, DIAG_DSL_CLIENT, "CoreHPIserver: reply res=%d size=%d", result, size);
  if (size != 0)
    DiagDumpData(0, DIAG_DSL_CLIENT, repMsgp, size, kDiagDbgDataFormatHex | kDiagDbgDataSize8);
#endif
}

extern void LineHPIserver(void *ptr, uint16 size, uint16 commandId, uint16 hmiVersion);

void XdslDrvProcessHmiMsg(void *hmiMsgp, uint16 size)
{
  HpiDefaultMessage *ptr = (HpiDefaultMessage *)hmiMsgp;

  DiagStrPrintf(0, DIAG_DSL_CLIENT, "XdslDrvProcessHmiMsg: AppId 0x%X cmdId 0x%X size=%d hdrLen=%d buildId=0x%X", 
    ptr->header.portNumber, ptr->header.commandId, size, ptr->header.len, ptr->header.buildId);

#if 1
  if (size != 0)
    DiagDumpData(0, DIAG_DSL_CLIENT, &ptr->data[0], size, kDiagDbgDataFormatHex | kDiagDbgDataSize8);
#endif
#if 0
  if( (ptr->header.commandId != 0x10B) && /* Get Line Status */
      (ptr->header.commandId != 0x10A) && /* Get Traffic Counters */
      (ptr->header.commandId != 0x11B) && /* Get PTM Counters */
      (ptr->header.commandId != 0xE)   && /* SANT_SET_NTP_TIME */
      (ptr->header.commandId != 0x131) )  /* Get Traffic Period Counters */
    printk("%s: AppId 0x%X cmdId 0x%X\n", __FUNCTION__, ptr->header.portNumber, ptr->header.commandId);
#endif

  switch(ptr->header.portNumber) {
    case CORE_BASE_APP_ID:
      CoreHPIserver(&ptr->data[0], ptr->header.len, ptr->header.commandId, ptr->header.buildId);
      break;
    case 0:
      LineHPIserver(&ptr->data[0], ptr->header.len, ptr->header.commandId, ptr->header.buildId);
      break;
    default:
      DiagStrPrintf(0, DIAG_DSL_CLIENT, "XdslDrvProcessHmiMsg: Unsupported AppId 0x%X cmdId 0x%X\n", ptr->header.portNumber, ptr->header.commandId);
      printk("%s: Unsupported AppId 0x%X cmdId 0x%X\n", __FUNCTION__, ptr->header.portNumber, ptr->header.commandId);
      XdslDrvHpiPost(&ptr->data[0], 0, UNAVAILABLE_PORTNUMBER);
      break;
  }
}

