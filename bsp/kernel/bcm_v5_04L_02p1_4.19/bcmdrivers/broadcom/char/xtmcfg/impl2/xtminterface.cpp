/*
    <:copyright-BRCM:2011:proprietary:standard
    
       Copyright (c) 2011 Broadcom 
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
/***************************************************************************
 * File Name  : xtminterface.cpp (impl2)
 *
 * Description: This file contains the implementation for the XTM interface
 *              class.  This class handles the processing that is associated
 *              with an ATM/PTM port.
 ***************************************************************************/

#include "bcmtypes.h"
#include "xtmcfgimpl.h"
#include "bcmadsl.h"

typedef int (*FN_ADSL_GET_OBJECT_VALUE) (unsigned char lineId, char *objId,
                                         int objIdLen, char *dataBuf, long *dataBufLen);

static adslMibInfo MibInfo ;

extern "C" {

extern FN_ADSL_GET_OBJECT_VALUE g_pfnAdslGetObjValue;
}

/***************************************************************************
 * Function Name: XTM_INTERFACE
 * Description  : Constructor for the XTM interface class.
 * Returns      : None.
 ***************************************************************************/
XTM_INTERFACE::XTM_INTERFACE( void )
{
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    m_ulPhysPort = 0;
    m_ulPhysBondingPort = MAX_INTERFACES ;
    m_ulDataStatus = DATA_STATUS_DISABLED ;
    m_ulErrTickCount = 0 ;
    m_ulIfInPacketsPtm = 0;
    m_ulAutoSenseATM = BC_ATM_AUTO_SENSE_ENABLE ;
    XtmOsMemSet(&m_Cfg, 0x00, sizeof(m_Cfg));
    XtmOsMemSet( &m_LinkInfo, 0x00, sizeof(m_LinkInfo));
    XtmOsMemSet( &m_LinkDelay, 0x00, sizeof(m_LinkDelay));
    XtmOsMemSet( &m_Stats, 0x00, sizeof(m_Stats));
    m_LinkInfo.ulLinkState = LINK_DOWN;
    m_Cfg.ulIfAdminStatus = ADMSTS_DOWN;

    // Log that we not initialized at this point
    m_bInitialized = false;

} /* XTM_INTERFACE */

void XTM_INTERFACE::PreInit ( void )
{
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    m_ulPhysPort = 0;
    m_ulPhysBondingPort = MAX_INTERFACES ;
    m_ulDataStatus = DATA_STATUS_DISABLED ;
    m_ulErrTickCount = 0 ;
    m_ulIfInPacketsPtm = 0;
    m_ulAutoSenseATM = BC_ATM_AUTO_SENSE_ENABLE ;
    XtmOsMemSet(&m_Cfg, 0x00, sizeof(m_Cfg));
    XtmOsMemSet( &m_LinkInfo, 0x00, sizeof(m_LinkInfo));
    XtmOsMemSet( &m_LinkDelay, 0x00, sizeof(m_LinkDelay));
    XtmOsMemSet( &m_Stats, 0x00, sizeof(m_Stats));
    m_LinkInfo.ulLinkState = LINK_DOWN;
    m_Cfg.ulIfAdminStatus = ADMSTS_DOWN;
    m_ulXTMLinkMode      = XTM_LINK_MODE_UNKNOWN;

#if defined(XTM_PORT_SHAPING)
    m_ulEnableShaping    = 0;
    m_ulShapeRate        = 0;
    m_usMbs              = 0;
    m_ulPortShapingRatio = 0;
#endif

    // Log that we not initialized at this point
    m_bInitialized = false;

} /* XTM_INTERFACE */

/***************************************************************************
 * Function Name: ~XTM_INTERFACE
 * Description  : Destructor for the XTM interface class.
 * Returns      : None.
 ***************************************************************************/
XTM_INTERFACE::~XTM_INTERFACE( void )
{
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    Uninitialize();
} /* ~XTM_INTERFACE */


/***************************************************************************
 * Function Name: Initialize
 * Description  : Initializes the object.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::Initialize( UINT32 ulPort, UINT32 ulInternalPort, UINT32 ulBondingPort,
                                         UINT32 autoSenseATM, FN_XTMRT_REQ pfnXtmrtReq)
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    // Initialize as needed
    if(m_bInitialized != true)
    {
        m_ulPhysPort = ulPort;
        m_ulInternalPort = ulInternalPort;
        m_ulPhysBondingPort = ulBondingPort;
        m_ulIfInPacketsPtm = 0;
        m_ulAutoSenseATM = autoSenseATM ;
        m_pfnXtmrtReq = pfnXtmrtReq;

        XP_REGS->ulTxSarCfg |= (1 << (m_ulPhysPort + TXSARA_CRC10_EN_SHIFT));
#if !defined(CONFIG_BCM963158)
        XP_REGS->ulRxSarCfg |= (1 << (m_ulPhysPort + RXSARA_CRC10_EN_SHIFT));
#else
        /* in 63158A0 case, all the incoming cells are marked in error when this
        ** is set. So, a temporary workaround is to keep it disabled.
        ** Revert this for 63158B0.
        **/
#if (CONFIG_BRCM_CHIP_REV!=0x63158A0)
        XP_REGS->ulRxSarCfg |= (1 << (m_ulPhysPort + RXSARA_CRC10_EN_SHIFT));
#endif
#endif

        XP_REGS->ulRxAtmCfg[m_ulPhysPort] |= RX_DOE_MASK ;

        SetRxUtopiaLinkInfo (LINK_UP) ;

        XP_REGS->ulRxPafWriteChanFlush |= (0x11 << m_ulPhysPort) ;

        m_Cfg.ulIfLastChange = XTMCFG_DIV_ROUND(XtmOsTickGet(),10);
    }

    // Log the sucessful initialization
    m_bInitialized = true;

    return( bxStatus );
} /* Initialize */

/***************************************************************************
 * Function Name: ReInitialize
 * Description  : ReInitializes the object in terms of updating the bonding
 * port member.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::ReInitialize( UINT32 ulBondingPort )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    
    m_ulPhysBondingPort = ulBondingPort;
    return( bxStatus );
} /* ReInitialize */

/***************************************************************************
 * Function Name: GetStats
 * Description  : Returns statistics for this interface.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::GetStats( PXTM_INTERFACE_STATS pStats,
    UINT32 ulReset )
{
    //XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    UINT32 ulCurrMibCtrl = XP_REGS->ulMibCtrl;

    XP_REGS->ulMibCtrl = (ulReset) ? 1 : 0;

    pStats->ulIfInOctets  += XP_REGS->ulRxPortPktOctCnt[m_ulPhysPort];
    pStats->ulIfOutOctets += XP_REGS->ulTxPortPktOctCnt[m_ulPhysPort];
    
    if( m_LinkInfo.ulLinkTrafficType == TRAFFIC_TYPE_ATM ||
        m_LinkInfo.ulLinkTrafficType == TRAFFIC_TYPE_ATM_BONDED )
    {
        pStats->ulIfInPackets += XP_REGS->ulRxPortPktCnt[m_ulPhysPort];
    }
    else
    {
        pStats->ulIfInPackets = m_ulIfInPacketsPtm;
        if (m_LinkInfo.ulLinkTrafficType == TRAFFIC_TYPE_PTM_BONDED)
        {
            pStats->ulIfInPackets += (XP_REGS->ulRxPafFragCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafFragCount[m_ulPhysPort+MAX_PHY_PORTS]);

            /* ulRxPafFragCount[] includes dropped frag. Therefore, we have to
            * deduct the dropped frag count.
            */
            pStats->ulIfInPackets -= (XP_REGS->ulRxPafDroppedFragCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafDroppedFragCount[m_ulPhysPort+MAX_PHY_PORTS]);
                                  
            /* For PTM bonding, ulRxBondDroppedFragCount and ulRxPafDroppedPktCount
            * need to be counted.
            */
            pStats->ulIfInPackets -= (XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort+MAX_PHY_PORTS]);
            pStats->ulIfInPackets -= (XP_REGS->ulRxBondDroppedFragCount[m_ulPhysPort] +
                                      XP_REGS->ulRxBondDroppedFragCount[m_ulPhysPort+MAX_PHY_PORTS]);
        }
        else
        {
            pStats->ulIfInPackets += (XP_REGS->ulRxPafPktCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafPktCount[m_ulPhysPort+MAX_PHY_PORTS]);

            /* ulRxPafPktCount[] includes dropped packets. Therefore, we have to
            * deduct the dropped packet count.
            */
            pStats->ulIfInPackets -= (XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort] +
                                      XP_REGS->ulRxPafDroppedPktCount[m_ulPhysPort+MAX_PHY_PORTS]);
                                  
        }
        m_ulIfInPacketsPtm = (ulReset) ? 0 : pStats->ulIfInPackets;
    }
    
    pStats->ulIfOutPackets += XP_REGS->ulTxPortPktCnt[m_ulPhysPort];
    pStats->ulIfInOamRmCells += (XP_REGS->ulTxRxPortOamCellCnt[m_ulPhysPort] &
        OAM_RX_CELL_COUNT_MASK) >> OAM_RX_CELL_COUNT_SHIFT;
    pStats->ulIfOutOamRmCells += (XP_REGS->ulTxRxPortOamCellCnt[m_ulPhysPort] &
        OAM_TX_CELL_COUNT_MASK) >> OAM_TX_CELL_COUNT_SHIFT;
    pStats->ulIfInAsmCells += (XP_REGS->ulTxRxPortAsmCellCnt[m_ulPhysPort] &
        ASM_RX_CELL_COUNT_MASK) >> ASM_RX_CELL_COUNT_SHIFT;
    pStats->ulIfOutAsmCells += (XP_REGS->ulTxRxPortAsmCellCnt[m_ulPhysPort] &
        ASM_TX_CELL_COUNT_MASK) >> ASM_TX_CELL_COUNT_SHIFT;
    pStats->ulIfInCellErrors +=
        (XP_REGS->ulRxPortErrorPktCellCnt[m_ulPhysPort] &
        ERROR_RX_CELL_COUNT_MASK) >> ERROR_RX_CELL_COUNT_SHIFT;
    pStats->ulIfInPacketErrors +=
        (XP_REGS->ulRxPortErrorPktCellCnt[m_ulPhysPort] &
        ERROR_RX_PKT_COUNT_MASK) >> ERROR_RX_PKT_COUNT_SHIFT;

    XP_REGS->ulMibCtrl = ulCurrMibCtrl;

    return( XTMSTS_SUCCESS );
} /* GetStats */


/***************************************************************************
 * Function Name: SetRxUtopiaLinkInfo
 * Description  : Enabled/Disabled Utopia Links on rx side.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::SetRxUtopiaLinkInfo ( UINT32 ulLinkState )
{
   XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
   UINT32 ulRxPortEnShift = 0, ulRxSlaveIntPortEnShift = 32 ;

   if( m_ulInternalPort )
      ulRxPortEnShift = RXUTO_INT_PORT_EN_SHIFT;

   if( ulLinkState == LINK_UP ) {
      XP_REGS->ulRxUtopiaCfg |= 1 << (m_ulPhysPort + ulRxPortEnShift);
      if (ulRxSlaveIntPortEnShift != 32)
         XP_REGS->ulRxUtopiaCfg |= 1 << (m_ulPhysPort + ulRxSlaveIntPortEnShift);
   }
   else {
      XP_REGS->ulRxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulRxPortEnShift));
      if (ulRxSlaveIntPortEnShift != 32)
         XP_REGS->ulRxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulRxSlaveIntPortEnShift));
   }

   return( XTMSTS_SUCCESS );
} /* SetRxUtopiaLinkInfo */


#if defined(XTM_PORT_SHAPING)

/***************************************************************************
 * Function Name: ConfigureMaxUTPortShaping
 * Description  : Enables Max Port Shaping on Utopia
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::ConfigureMaxUTPortShaping ( UINT32 ulTrafficType)
{
   UINT32 i ;
   volatile UINT32 *Addr ;

   /* Apply for all the VDSL modes due to SAR clk/UT clk differences and Zero
   ** padding issue.
   ** Ref HW Jira - HW63158-290
   **/
   if ((m_ulXTMLinkMode != XTM_LINK_MODE_GFAST) &&
         ((ulTrafficType == TRAFFIC_TYPE_PTM) || (ulTrafficType == TRAFFIC_TYPE_PTM_RAW) ||
          (ulTrafficType == TRAFFIC_TYPE_PTM_BONDED))) {

      XtmOsPrintf(XTM_LOG_NOT,"bcmxtmcfg: Configure Max UT port shaping %d", m_ulPhysPort) ;

      XP_REGS->ulTxLineRateTimer = XTMCFG_LINE_RATE_TIMER ;


      XP_REGS->ulSstSitValue     = XTMCFG_SST_SIT_VALUE ;


      Addr = (UINT32 *) &XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort] [TMUEXT_PCRINCR_INDEX] ;

      i = *Addr ;
      i &= SSTE_UTOPCRMW_PCR_PLVLWT_MASK ;
      i |= (XTMCFG_UTO_PCR_INCR_SHIFT << SSTE_UTOPCRMW_PCR_INCR_SHIFT) ;
      *Addr = i;

#if defined(CONFIG_BCM963146)
      i = XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort] [TMUEXT_CURACC_INDEX] ;
      i &= SSTE_UTOPCRMW_PCR_CURACC_MASK ;
      XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort][TMUEXT_CURACC_INDEX] = i;

      i = (XTMCFG_UTO_PCR_ACCLMT << SSTE_UTOPCRMW_PCR_ACCLMT_SHIFT) ;
      XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort][TMUEXT_ACCLMT_INDEX] = i;
#else
      i = XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort] [TMUEXT_CURACC_INDEX] ;
      i &= SSTE_UTOPCRMW_PCR_CURACC_MASK ;
      i |= (XTMCFG_UTO_PCR_ACCLMT << SSTE_UTOPCRMW_PCR_ACCLMT_SHIFT) ;
      XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort][TMUEXT_ACCLMT_INDEX] = i; /* Curacc/Acclmt share the same reg */
#endif

      i = XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort] [TMUEXT_SITLMT_CURSIT_INDEX] ;  
      i &= SSTE_UTOPCRMW_PCR_CURSIT_MASK ;
      i |= (XTMCFG_UTO_PCR_SITLMT << SSTE_UTOPCRMW_PCR_SITLMT_SHIFT) ;
      XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort] [TMUEXT_SITLMT_CURSIT_INDEX] = i;

      i = XP_REGS->ulSsteUtoGtsCfg[m_ulPhysPort] ;  
      i &= ~SSTE_UTOGTSCFG_MASK ;
      i |= (0x1 << SSTE_UTOGTSCFG_SHIFT) ;
      XP_REGS->ulSsteUtoGtsCfg[m_ulPhysPort] = i;

   } /* if (!Gfast && VDSL PTM/PTM RAW) */

   else {

      /* ATM, ATM bonded, GFAST single/bonded modes dont need the PortShaping */
      /* Disable Port shaping */

      UnConfigureMaxUTPortShaping();
   }

   return( XTMSTS_SUCCESS );

} /* ConfigureMaxUTPortShaping */

/***************************************************************************
 * Function Name: UnConfigureMaxUTPortShaping
 * Description  : Disables Max Port Shaping on Utopia
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::UnConfigureMaxUTPortShaping ()
{
   UINT32 i ;

   XtmOsPrintf(XTM_LOG_NOT,"bcmxtmcfg: UnConfigure Max UT port shaping %d", m_ulPhysPort) ;

   i = XP_REGS->ulSsteUtoGtsCfg[m_ulPhysPort] ;  
   i &= ~SSTE_UTOGTSCFG_MASK ;
   i &= ~(0x1 << SSTE_UTOGTSCFG_SHIFT) ;
   XP_REGS->ulSsteUtoGtsCfg[m_ulPhysPort] = i;

   return( XTMSTS_SUCCESS );

} /* UnConfigureMaxUTPortShaping */
#endif


void XTM_INTERFACE::UpdateLinkDelay()
{
   int ret ;
   long int size = sizeof (adslMibInfo) ;
   XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 

   MibInfo.maxBondingDelay = 0 ;

   if (g_pfnAdslGetObjValue != NULL) {
      size = sizeof (MibInfo) ;
      ret  = g_pfnAdslGetObjValue (m_ulPhysPort, NULL, 0, (char *) &MibInfo, &size) ;
      if (ret != 0)
         XtmOsPrintf(XTM_LOG_ERR,"Error g_pfnAdslGetObjValue port - %d return value - %d ", m_ulPhysPort, ret) ;
   }

   m_LinkDelay.ulLinkDsBondingDelay = MibInfo.maxBondingDelay ;
}

/***************************************************************************
 * Function Name: SetLinkInfo
 * Description  : Call when there is a change in link status.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::SetLinkInfo( PXTM_INTERFACE_LINK_INFO pLinkInfo, UINT8 rxConfig,
                                          UINT8 linkInfoConfig, UINT8 txConfig, UINT32 ulXTMLinkMode )
{
   int ret ;
   UINT32 ulTxPortEnShift=0, ulRxTeqPortShift=0, ulRxTeqPortMask=0, ulTxSlaveIntPortEnShift = 32 ;
   UINT32 ulTxRawPortEnShift ;
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 

   if( m_ulInternalPort )
   {
      ulTxPortEnShift  = TXUTO_INT_PORT_EN_SHIFT;
      ulTxRawPortEnShift = TXUTO_INT_RAW_PID_EN_SHIFT;
      ulRxTeqPortShift = RXUTO_INT_TEQ_PORT_SHIFT;
      ulRxTeqPortMask  = RXUTO_INT_TEQ_PORT_MASK;
   }

   m_ulXTMLinkMode = ulXTMLinkMode ;
   XtmOsPrintf(XTM_LOG_NOT, "bcmxtmcfg: DSL link mode is %d\n", m_ulXTMLinkMode);

   if( pLinkInfo->ulLinkState == LINK_UP )
   {
      if( m_Cfg.ulIfAdminStatus == ADMSTS_UP )
      {
         if (linkInfoConfig == XTM_CONFIGURE) {
            XtmOsMemCpy( &m_LinkInfo, pLinkInfo, sizeof(m_LinkInfo));
            m_Cfg.usIfTrafficType = pLinkInfo->ulLinkTrafficType;
         }

         if (txConfig == XTM_CONFIGURE) {
#if defined(HW_PTM_TX_BONDING)
            /* In case of HW PTM bonding, only one port (Primary port) of the
            ** bonding group is to be enabled with non-preemptive flow in
            ** focus. Only for PTM bonded traffic type.
            ** Load distribution occurs with txpaf
            ** Preemptive flows - pending.
            */
            if (m_Cfg.usIfTrafficType == TRAFFIC_TYPE_PTM_BONDED) {
               XP_REGS->ulTxSchedCfg = (m_ulPhysPort == PHY_PORTID_0) ?
                                    (XP_REGS->ulTxSchedCfg | (1 << (m_ulPhysPort + TXSCH_PORT_EN_SHIFT))) :
                                     XP_REGS->ulTxSchedCfg ;
            }
            else {
               XP_REGS->ulTxSchedCfg |=
                  1 << (m_ulPhysPort + TXSCH_PORT_EN_SHIFT);
            }
#else
            XP_REGS->ulTxSchedCfg |=
               1 << (m_ulPhysPort + TXSCH_PORT_EN_SHIFT);
#endif
            XP_REGS->ulTxUtopiaCfg |= 
               1 << (m_ulPhysPort + ulTxPortEnShift);
            if (ulTxSlaveIntPortEnShift != 32)
               XP_REGS->ulTxUtopiaCfg |= 1 << (m_ulPhysPort + ulTxSlaveIntPortEnShift);

            if (m_Cfg.usIfTrafficType == TRAFFIC_TYPE_PTM_RAW)
               XP_REGS->ulTxUtopiaCfg |=
                  1 << (m_ulPhysPort + ulTxRawPortEnShift);
         }

         if (rxConfig == XTM_CONFIGURE) {
            XP_REGS->ulRxAtmCfg[m_ulPhysPort] |= RX_PORT_EN;

            if (m_Cfg.usIfTrafficType == TRAFFIC_TYPE_ATM_BONDED)
               XP_REGS->ulRxAtmCfg[m_ulPhysPort] |= (RXA_BONDING_VP_MASK |
                     (m_ulPhysPort << RX_PORT_MASK_SHIFT) |
                     RXA_HEC_CRC_IGNORE |
                     RXA_GFC_ERROR_IGNORE) ;

            XtmOsPrintf(XTM_LOG_INF,"traffictype = %d ", m_Cfg.usIfTrafficType) ;

            XtmOsPrintf(XTM_LOG_INF,"UtopiaCfg = %x ", XP_REGS->ulRxUtopiaCfg) ;
            if( m_Cfg.usIfTrafficType == TRAFFIC_TYPE_PTM_RAW )
            {
               //m_Cfg.usIfTrafficType = TRAFFIC_TYPE_PTM;
               XtmOsPrintf(XTM_LOG_INF,"UtopiaTeqMask = %x ", ulRxTeqPortMask) ;
               XP_REGS->ulRxUtopiaCfg &= ~ulRxTeqPortMask;
               XP_REGS->ulRxUtopiaCfg |= (1 << (m_ulPhysPort + ulRxTeqPortShift));
            }
            XtmOsPrintf(XTM_LOG_INF,"UtopiaCfg = %x ", XP_REGS->ulRxUtopiaCfg) ;
         } /* rxConfig */
      }

      /* Read a current snapshot of the IF Error Counters */

      if (m_ulPhysBondingPort != MAX_INTERFACES) {
         long int size = sizeof (adslMibInfo) ;

         if (g_pfnAdslGetObjValue != NULL) {
            size = sizeof (MibInfo) ;
            ret  = g_pfnAdslGetObjValue (m_ulPhysPort, NULL, 0, (char *) &MibInfo, &size) ;
            if (ret != 0)
               XtmOsPrintf(XTM_LOG_ERR,"g_pfnAdslGetObjValue port - %d return value - %d ", m_ulPhysPort, ret) ;
         }

         m_LinkDelay.ulLinkDsBondingDelay = MibInfo.maxBondingDelay ;
         m_LinkDelay.ulLinkUsDelay = MibInfo.xdslInfo.dirInfo[1].lpInfo[0].delay ;

         m_PrevIfMonitorInfo.rx_loss_of_sync     = MibInfo.adslPerfData.perfTotal.adslLOSS ;
         m_PrevIfMonitorInfo.rx_SES_count_change = MibInfo.adslPerfData.perfTotal.adslSES ;
         m_PrevIfMonitorInfo.tx_SES_count_change = MibInfo.adslTxPerfTotal.adslSES ;
         m_PrevIfMonitorInfo.tx_LOS_change       = MibInfo.adslTxPerfTotal.adslLOSS ;
         m_PrevIfMonitorInfo.rx_uncorrected      = MibInfo.xdslStat[0].rcvStat.cntRSUncor ; /* Latency 0 */

         m_ulErrTickCount = 0 ;
      }

      while (XTM_IS_WRCHAN_FLUSH_BUSY)
         XtmOsDelay (100) ;

      XP_REGS->ulRxPafWriteChanFlush &= ~(0xFFFFFF11 << m_ulPhysPort) ;

      m_ulDataStatus = DATA_STATUS_ENABLED ;
   }
   else /* LINK_DOWN */
   {
      if (linkInfoConfig == XTM_CONFIGURE) {
         XtmOsMemCpy( &m_LinkInfo, pLinkInfo, sizeof(m_LinkInfo));
         m_Cfg.usIfTrafficType        = TRAFFIC_TYPE_NOT_CONNECTED;
         m_LinkInfo.ulLinkTrafficType = TRAFFIC_TYPE_NOT_CONNECTED;
         m_LinkDelay.ulLinkDsBondingDelay = 0 ;
         m_LinkDelay.ulLinkUsDelay = 0 ;
      }

      if (txConfig == XTM_CONFIGURE) {
         XP_REGS->ulTxUtopiaCfg &= 
            ~(1 << (m_ulPhysPort + ulTxPortEnShift));
         if (ulTxSlaveIntPortEnShift != 32)
            XP_REGS->ulTxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulTxSlaveIntPortEnShift));
         XP_REGS->ulTxSchedCfg &=
            ~(1 << (m_ulPhysPort + TXSCH_PORT_EN_SHIFT));
         XP_REGS->ulTxUtopiaCfg &=
            ~(1 << (m_ulPhysPort + ulTxRawPortEnShift));
      }
      else {
         /* Workaround for 68 atm bonding. Keep the scheduler enabled and the
          * utopia disabled (as the other bonding link may still be up.)
          */
         if( m_Cfg.usIfTrafficType == TRAFFIC_TYPE_ATM_BONDED ) {
            XP_REGS->ulTxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulTxPortEnShift));
            if (ulTxSlaveIntPortEnShift != 32)
               XP_REGS->ulTxUtopiaCfg &= ~(1 << (m_ulPhysPort + ulTxSlaveIntPortEnShift));
         }
      }

      if (rxConfig == XTM_CONFIGURE) {
         XP_REGS->ulRxUtopiaCfg &= ~ulRxTeqPortMask;
         XP_REGS->ulRxUtopiaCfg |= (XP_MAX_PORTS << ulRxTeqPortShift);
         XP_REGS->ulRxAtmCfg[m_ulPhysPort] &= ~RX_PORT_EN;
      }

      while (XTM_IS_WRCHAN_FLUSH_BUSY)
         XtmOsDelay (100) ;

      XP_REGS->ulRxPafWriteChanFlush |= (0x11 << m_ulPhysPort) ;

      m_ulDataStatus = DATA_STATUS_DISABLED ;
      m_ulErrTickCount = 0 ;
   }

#if defined(CONFIG_BCM963158)
   ConfigureTxPortShaping (m_LinkInfo.ulLinkTrafficType) ;
#endif

   m_Cfg.ulIfLastChange = XTMCFG_DIV_ROUND(XtmOsTickGet(),10) ;

   return( XTMSTS_SUCCESS );
} /* SetLinkInfo */

/***************************************************************************
 * Function Name: UpdateLinkInfo
 * Description  : Call when there is a change in link traffic type.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::UpdateLinkInfo( UINT32 ulTrafficType )
{
   //XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
   m_LinkInfo.ulLinkTrafficType = ulTrafficType ;
   m_Cfg.usIfTrafficType = ulTrafficType;
   XtmOsPrintf(XTM_LOG_NOT,"bcmxtmcfg: Auto-Sensed XTM Link Information, port = %d, State = %s, Service Support = %s ",
                m_ulPhysPort,
                (m_LinkInfo.ulLinkState == LINK_UP ? "UP" :
                  (m_LinkInfo.ulLinkState == LINK_DOWN ? "DOWN":
                   (m_LinkInfo.ulLinkState == LINK_START_TEQ_DATA ? "START_TEQ": "STOP_TEQ"))),
                (ulTrafficType == TRAFFIC_TYPE_ATM ? "ATM" :
                 (ulTrafficType == TRAFFIC_TYPE_PTM ? "PTM" :
                  (ulTrafficType == TRAFFIC_TYPE_PTM_BONDED ? "PTM_BONDED" :
                   (ulTrafficType == TRAFFIC_TYPE_ATM_BONDED ? "ATM_BONDED" : "RAW"))))) ;

   return( XTMSTS_SUCCESS );
} /* UpdateLinkInfo */

/***************************************************************************
 * Function Name: GetLinkErrorStatus (63268)
 * Description  : Call for detecting Link Errors. Only called for 63268 bonding
 * through run-time configurations.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::GetLinkErrorStatus (UINT32 *pUsStatus, UINT32 *pDsStatus )
{
   int ret ;

   BCMXTM_STATUS nRet = XTMSTS_SUCCESS ;

   if (m_ulPhysBondingPort != MAX_INTERFACES) {

      if (m_ulErrTickCount == 0) {

         long int size = sizeof (adslMibInfo) ;
         IfMonitorErrorCounters CurrIfMonitorInfo ;

         m_ulUsStatus = m_ulDsStatus = XTMSTS_SUCCESS ;

         if (g_pfnAdslGetObjValue != NULL) {
            size = sizeof (MibInfo) ;
            ret = g_pfnAdslGetObjValue (m_ulPhysPort, NULL, 0, (char *) &MibInfo, &size) ;
            if (ret != 0)
               XtmOsPrintf(XTM_LOG_ERR,"g_pfnAdslGetObjValue port - %d return value - %d ", m_ulPhysPort, ret) ;
         }

         CurrIfMonitorInfo.rx_loss_of_sync = MibInfo.adslPerfData.perfTotal.adslLOSS ;
         CurrIfMonitorInfo.rx_SES_count_change  = MibInfo.adslPerfData.perfTotal.adslSES ;
         CurrIfMonitorInfo.tx_SES_count_change = MibInfo.adslTxPerfTotal.adslSES ;
         CurrIfMonitorInfo.tx_LOS_change = MibInfo.adslTxPerfTotal.adslLOSS ;
         CurrIfMonitorInfo.rx_uncorrected = MibInfo.xdslStat[0].rcvStat.cntRSUncor ; /* Latency 0 */

         if ((m_PrevIfMonitorInfo.rx_loss_of_sync != CurrIfMonitorInfo.rx_loss_of_sync) ||
             (m_PrevIfMonitorInfo.rx_SES_count_change != CurrIfMonitorInfo.rx_SES_count_change) ||
             (m_PrevIfMonitorInfo.rx_uncorrected != CurrIfMonitorInfo.rx_uncorrected)) {

            XtmOsPrintf(XTM_LOG_INF,"rxLos=%ul, rxSES=%ul, rxUnC=%ul ",
                          CurrIfMonitorInfo.rx_loss_of_sync, CurrIfMonitorInfo.rx_SES_count_change,
                          CurrIfMonitorInfo.rx_uncorrected) ;
            nRet = XTMSTS_ERROR ;
            m_ulDsStatus = XTMSTS_ERROR ;
          }

         if ((m_PrevIfMonitorInfo.tx_SES_count_change != CurrIfMonitorInfo.tx_SES_count_change) ||
             (m_PrevIfMonitorInfo.tx_LOS_change != CurrIfMonitorInfo.tx_LOS_change)) {

            XtmOsPrintf(XTM_LOG_INF,"txSES=%ul, txLos=%ul ",
                          CurrIfMonitorInfo.tx_SES_count_change, CurrIfMonitorInfo.tx_LOS_change) ;
            nRet = XTMSTS_ERROR ;
            m_ulUsStatus = XTMSTS_ERROR ;
         }

         if (nRet != XTMSTS_SUCCESS) {
            XtmOsMemCpy (&m_PrevIfMonitorInfo, &CurrIfMonitorInfo, sizeof (IfMonitorErrorCounters)) ;
            m_ulErrTickCount = XTM_BOND_DSL_ERR_DURATION_TIMEOUT_MS ;
         }
      }
      else {
         m_ulErrTickCount -= XTM_BOND_DSL_MONITOR_TIMEOUT_MS ;
         nRet = XTMSTS_ERROR ;
      }
   } /* if (m_ulPhysBondingPort != MAX_INTERFACES) */

   *pUsStatus = m_ulUsStatus ;
   *pDsStatus = m_ulDsStatus ;
   return (nRet) ;
}

/***************************************************************************
 * Function Name: SetPortDataStatus (63x68)
 * Description  : Call for setting the port data status.
 *                Also effects the port status in the SAR Rx registers.
 *                This is necessary to avoid SAR lockup from undesired traffic
 *                in DS direction during line micro-interruption.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
void XTM_INTERFACE::SetPortDataStatus ( UINT32 status, UINT32 flush )
{
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
   /* if Phy does stop & start the flow, then we do not need this here in XTM.
    * Currently in experimentation phase
    */
   if ((m_Cfg.usIfTrafficType == TRAFFIC_TYPE_ATM_BONDED) ||
       (m_Cfg.usIfTrafficType == TRAFFIC_TYPE_PTM_BONDED)) {
      /* For PTM Bonding, XTM Network driver will take care in SW for Tx Path. This is for
       * HW control.
       * We only effect Rx direction, as there may be pending traffic for
       * concerned port in tx direction in the DMA queues, which should not be
       * blocked.
       */

      if (XTM_IS_WRCHAN_FLUSH_BUSY) {
         goto _End ;
      }

      if (status == DATA_STATUS_ENABLED)
         XP_REGS->ulRxPafWriteChanFlush &= ~(0xFFFFFF11 << m_ulPhysPort) ;
      else {
         if (flush == XTM_FLUSH)
              XP_REGS->ulRxPafWriteChanFlush |= (0x11 << m_ulPhysPort) ;
      }
#if 0
      XtmOsPrintf(XTM_LOG_DBG,"Paf Fl, P%d %x ", m_ulPhysPort, XP_REGS->ulRxPafWriteChanFlush) ;
#endif
   } /* if (bonded traffic) */

   m_ulDataStatus = status;

_End :
   return ;
}


/***************************************************************************
 * Function Name: Uninitialize
 * Description  : Undo Initialize.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::Uninitialize( void )
{
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    // Set link status as needed
    if(m_bInitialized)
        SetRxUtopiaLinkInfo (LINK_DOWN) ;

    m_pfnXtmrtReq = NULL ;
    // Log the sucessful uninitialization
    m_bInitialized = false;

    return( XTMSTS_SUCCESS );
} /* Uninitialize */


/***************************************************************************
 * Function Name: GetCfg
 * Description  : Returns the interface configuration record.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::GetCfg( PXTM_INTERFACE_CFG pCfg,
    XTM_CONNECTION_TABLE *pConnTbl )
{
    BCMXTM_STATUS bxStatus = XTMSTS_SUCCESS;
    XTM_CONNECTION *pConn;
    XTM_ADDR Addr;

    //XtmOsPrintf(XTM_LOG_DBG,"Entry") ;
    //XtmOsPrintf(XTM_LOG_DBG,"2bcmxtmcfg: TxSarCfg%x, RxSarCfg%x \n", XP_REGS->ulTxSarCfg, XP_REGS->ulRxSarCfg) ; 
    XtmOsMemCpy(pCfg, &m_Cfg, sizeof(XTM_INTERFACE_CFG));

    pCfg->ulIfOperStatus = (m_LinkInfo.ulLinkState == LINK_UP &&
        IsInterfaceUp()) ? OPRSTS_UP : OPRSTS_DOWN;

#if defined(CONFIG_BCM963138) || defined(CONFIG_BCM963148) || defined(CONFIG_BCM963158) || defined(CONFIG_BCM963146)
    if (m_ulPhysPort != XTM_RX_TEQ_PHY_PORT) {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED
             | SUPPORT_TRAFFIC_TYPE_TXPAF_PTM_BONDED) ;
    }
    else {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED
             | SUPPORT_TRAFFIC_TYPE_TXPAF_PTM_BONDED | SUPPORT_TRAFFIC_TYPE_TEQ) ;
    }
#else
    if (m_ulPhysPort != XTM_RX_TEQ_PHY_PORT) {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED) ;
    }
    else {
       pCfg->usIfSupportedTrafficTypes = (SUPPORT_TRAFFIC_TYPE_ATM |
             SUPPORT_TRAFFIC_TYPE_PTM | SUPPORT_TRAFFIC_TYPE_PTM_RAW | SUPPORT_TRAFFIC_TYPE_PTM_BONDED
             | SUPPORT_TRAFFIC_TYPE_TEQ) ;
    }
#endif

    if (m_ulAutoSenseATM == BC_ATM_AUTO_SENSE_ENABLE)
        pCfg->usIfSupportedTrafficTypes |= SUPPORT_TRAFFIC_TYPE_ATM_BONDED ;

    /* Calculate the number of configured VCCs for this interface. */
    pCfg->ulAtmInterfaceConfVccs = 0;
    UINT32 i = 0;
    while( (pConn = pConnTbl->Enum( &i )) != NULL )
    {
        pConn->GetAddr( &Addr );
        if( ((Addr.ulTrafficType&TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM) &&
            (Addr.u.Vcc.ulPortMask & PORT_TO_PORTID(m_ulPhysPort)) ==
             PORT_TO_PORTID(m_ulPhysPort) )
        {
            pCfg->ulAtmInterfaceConfVccs++;
        }
    }

#if defined(XTM_PORT_SHAPING)
    pCfg->ulPortShaping = m_ulEnableShaping;
    pCfg->ulShapeRate = m_ulShapeRate;
    pCfg->usMbs = m_usMbs;
#endif

    //XtmOsPrintf(XTM_LOG_DBG,"3bcmxtmcfg: TxSarCfg%x, RxSarCfg%x \n", XP_REGS->ulTxSarCfg, XP_REGS->ulRxSarCfg) ; 
    XtmOsPrintf(XTM_LOG_DBG,"Exit") ;

    return( bxStatus );
} /* GetCfg */


/***************************************************************************
 * Function Name: SetCfg
 * Description  : Sets the configuration record.
 * Returns      : XTMSTS_SUCCESS if successful or error status.
 ***************************************************************************/
BCMXTM_STATUS XTM_INTERFACE::SetCfg( PXTM_INTERFACE_CFG pCfg )
{
    XtmOsPrintf(XTM_LOG_DBG,"%s Enter",__FUNCTION__); 
    m_Cfg.ulIfAdminStatus = pCfg->ulIfAdminStatus;
#if defined(XTM_PORT_SHAPING)
    //Port Shaping
    if((pCfg->ulShapeRate != 0) && (pCfg->usMbs != 0) && (pCfg->ulPortShaping == PORT_Q_SHAPING_ON))
    {
       m_ulEnableShaping = PORT_Q_SHAPING_ON;
       m_ulShapeRate  = pCfg->ulShapeRate;
       m_usMbs        = pCfg->usMbs;
       XtmOsPrintf(XTM_LOG_INF,"m_ulShapeRate[%u bits] m_usMbs[%u]" ,m_ulShapeRate,m_usMbs);
       ConfigureTxPortShaping (m_LinkInfo.ulLinkTrafficType) ;
    }
    else if(pCfg->ulPortShaping == PORT_Q_SHAPING_OFF)
    {
       m_ulEnableShaping = PORT_Q_SHAPING_OFF;
       m_ulShapeRate  = 0;
       m_usMbs        = 0;
       DisableTxPortShaping();
    }
    else {
       XtmOsPrintf(XTM_LOG_INF,"m_ulShapeRate[%u bits] m_usMbs[%u]",m_ulShapeRate,m_usMbs); 
       return XTMSTS_PARAMETER_ERROR;
    }

#endif
    return( XTMSTS_SUCCESS );
} /* SetCfg */


#if defined(XTM_PORT_SHAPING)

void XTM_INTERFACE::ResetHWTxPortShaping (void)
{
   volatile UINT32 *pulUtoPortScr = &XP_REGS->ulSsteUtoPortScr[m_ulPhysPort][TMUEXT_SCRINCR_INDEX];
   volatile UINT32 *pulUtoPortPcr = &XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort][TMUEXT_SCRINCR_INDEX];
   volatile UINT32 *pulUtoPortGts = &XP_REGS->ulSsteUtoGtsCfg[m_ulPhysPort];
   pulUtoPortScr[TMUEXT_SCRINCR_INDEX] = pulUtoPortScr[TMUEXT_ACCLMT_INDEX] = pulUtoPortScr[TMUEXT_CURACC_INDEX] = 
                                         pulUtoPortScr[TMUEXT_SITLMT_CURSIT_INDEX] = 0 ;
   pulUtoPortPcr[TMUEXT_SCRINCR_INDEX] = pulUtoPortPcr[TMUEXT_ACCLMT_INDEX] = pulUtoPortPcr[TMUEXT_CURACC_INDEX] = 
                                         pulUtoPortPcr[TMUEXT_SITLMT_CURSIT_INDEX] = 0 ;
   *pulUtoPortGts = 0;
   XtmOsPrintf(XTM_LOG_INF,"pUtoPortScr[%d-%08x] pUtoPortScr[%d-%08x] \n",TMUEXT_SCRINCR_INDEX, pulUtoPortScr[TMUEXT_SCRINCR_INDEX], TMUEXT_ACCLMT_INDEX, pulUtoPortScr[TMUEXT_ACCLMT_INDEX]);
   XtmOsPrintf(XTM_LOG_INF,"pUtoPortScr[%d-%08x] pUtoPortScr[%d-%08x] \n",TMUEXT_CURACC_INDEX, pulUtoPortScr[TMUEXT_CURACC_INDEX], TMUEXT_SITLMT_CURSIT_INDEX, pulUtoPortScr[TMUEXT_SITLMT_CURSIT_INDEX]);
   XtmOsPrintf(XTM_LOG_INF,"pUtoPortGts[%08x] ",*pulUtoPortGts);
   return;
}

#define  GFAST_SCHED_DIV_FACTOR        1333

UINT32 XTM_INTERFACE::ConfigureTxPortShaping( UINT32 ulTrafficType)
{
#if defined(HW_SAR_BASED_PORT_SHAPING_SUPPORT)
   UINT16 usMbs;
   UINT32 ulShapeRate, ulShapeRateKbps;
   //UINT32 ulMbs;
   volatile UINT32 *pulUtoPortScr = &XP_REGS->ulSsteUtoPortScr[m_ulPhysPort][TMUEXT_SCRINCR_INDEX];
   volatile UINT32 *pulUtoPortPcr = &XP_REGS->ulSsteUtoPortPcr[m_ulPhysPort][TMUEXT_SCRINCR_INDEX];
   volatile UINT32 *pulUtoPortGts = &XP_REGS->ulSsteUtoGtsCfg[m_ulPhysPort];
#if !defined(CONFIG_BCM963158)
   UINT32 ulScr = 0;
#endif
   UINT32 ulGts = 0;
   UINT64 u64ScrSitLmt = 0;
   UINT32 ulScrSitLmt = 0;

#else
   XTMRT_PORT_SHAPER_INFO   portShaperInfo ;
#endif
   if((m_ulEnableShaping == 0)) {
      //No Shaping
      return PORT_Q_SHAPING_OFF;
   }

#if !defined(HW_SAR_BASED_PORT_SHAPING_SUPPORT)
   /* Send a message to XTMRT to control port shaping to enable mode across
   ** all the xtm channels corresponding to the port.
   ** First let us disable at SAR end to be sure.
   */
   ResetHWTxPortShaping() ;
   portShaperInfo.ulShapingRate      = m_ulShapeRate ;
   portShaperInfo.usShapingBurstSize = m_usMbs ;
   if (m_pfnXtmrtReq != NULL) {
      (*m_pfnXtmrtReq)(NULL, XTMRT_CMD_SET_TX_PORT_SHAPER_INFO, &portShaperInfo);
   }
   else {
      XtmOsPrintf(XTM_LOG_ERR,"ConfigureTxPortShaping: Reference XTMRT Null \n") ;
   }
#else
   *pulUtoPortPcr = 0;
   //Now apply the port shaping ratio.
   //First divide the value by 100 and then multiply by the factor so that we
   //may not overflow. 
   ulShapeRate  = XTMCFG_DIV_ROUND(m_ulShapeRate,XTM_PORT_SHAPING_RATIO_FULL) * m_ulPortShapingRatio ;

   /* Shape rate in bits/sec */
   ulShapeRateKbps = XTMCFG_DIV_ROUND(ulShapeRate,1000) ;

   XtmOsPrintf(XTM_LOG_INF,"m_ulPortShapingRatio[%u] PhysPort[%u] ShapeRate[%u] Mbs[%u]",m_ulPortShapingRatio,
                                                                    m_ulPhysPort,
                                                                    ulShapeRate,
                                                                    m_usMbs);
   
   /* Calculate the number of SIT pulses per sec.
    * Note that ms_ulSitUt is the number of SAR clocks per SIT pulse.
   */
   UINT32 ulSitsPerSec   = XTMCFG_DIV_ROUND(SAR_CLOCK,ms_ulSitUt) ;
   UINT32 ulSitsPerSecLo = XTMCFG_DIV_ROUND(SAR_CLOCK,ms_ulSitLoUt) ;

   if ((ulTrafficType & TRAFFIC_TYPE_ATM_MASK) == TRAFFIC_TYPE_ATM) {

#if defined(CONFIG_BCM963158)
      /* Disable due to non-support */
      XtmOsPrintf(XTM_LOG_ERR,"bcmxtmcfg: ATM Modes Port Shaping is NOT supported. \n");
      ResetHWTxPortShaping() ;
#else

      //ATM mode UBR with PCR
      usMbs = 8;  // 8 Cells
      ulScr = XTMCFG_DIV_ROUND(ulShapeRate,(53*8)) ;
      ulScrSitLmt = XTMCFG_DIV_ROUND(ulSitsPerSec,ulScr) ;
      XtmOsPrintf(XTM_LOG_INF,"Scr[%u] ScrSitLmt[%u] ",ulScr,ulScrSitLmt);
      if( ulScrSitLmt == 0 )
      {
         XtmOsPrintf(XTM_LOG_INF,"SCR_SITLMT=0x0 is invalid. No shaping.");
         return PORT_Q_SHAPING_OFF;
      }
      else if( ulScrSitLmt > 0x7FFF )
      {
         /* Try using SIT_LO_CNT */
         ulScrSitLmt = XTMCFG_DIV_ROUND(ulSitsPerSecLo,ulScr) ;
         if( ulScrSitLmt == 0 || ulScrSitLmt > 0x7FFF )
         {
            XtmOsPrintf(XTM_LOG_INF,"SCR_SITLMT=0x%x is invalid. No shaping.",
                        ulScrSitLmt);
            return PORT_Q_SHAPING_OFF;
         }
         ulGts |= SST_GTS_SCR_EN | SST_GTS_SITLO_SCR_EN;
      }
      else
      {
         ulGts |= SST_GTS_SCR_EN;
      }
      *pulUtoPortGts = 0;

      /* Preserve SCR priority which is used for arbitration. */
      pulUtoPortScr[TMUEXT_SCRINCR_INDEX] &= (SSTE_SCR_PLVLWT_MASK << SSTE_SCR_PLVLWT_SHIFT);
      pulUtoPortScr[TMUEXT_SCRINCR_INDEX] |= (1 << SSTE_SCR_INCR_SHIFT);
      pulUtoPortScr[TMUEXT_ACCLMT_INDEX]  = (8 << SSTE_SCR_ACCLMT_SHIFT);
      pulUtoPortScr[TMUEXT_SITLMT_CURSIT_INDEX]  = (ulScrSitLmt << SSTE_SCR_SITLMT_SHIFT);
      *pulUtoPortGts = ulGts;
#endif /* if (158) */
   } /* if (ATM) */
   else
   {
#if defined(CONFIG_BCM963158) && !defined(XTM_6315X_GFAST_PORT_SHAPING_SUPPORTED)
      /* Disable for G.fast mode(s) */
      if (m_ulXTMLinkMode == XTM_LINK_MODE_GFAST) {
         ResetHWTxPortShaping() ;
         XtmOsPrintf(XTM_LOG_ERR,"bcmxtmcfg: GFAST Modes Port Shaping is NOT supported. \n");
         goto _RetConfPortShaper ;
      }
      usMbs = m_usMbs ;
      //UBR with PBR Configuration
#else      
       if (m_ulXTMLinkMode == XTM_LINK_MODE_GFAST) {
           /* new Mbs = (minrate)/8/(10^9/750000). Explanation that this is suitable
            ** for asymmetric Gfast is in SWBCACPE-23733. Not applicable for
            ** single port cases. Hence reverting this to be applicable for all
            ** modes */
         //usMbs = (ulShapeRate/8)/GFAST_SCHED_DIV_FACTOR ; /* somehow this
         //stalls at higher port shaped rates. So an issue?????
         usMbs = m_usMbs ;
       }
       else { 

         //usMbs = (ulShapeRate/8)/GFAST_SCHED_DIV_FACTOR ; /* somehow this
         //stalls at higher port shaped rates. So an issue?????
         usMbs = m_usMbs ;
       }
#endif

      if (ulShapeRate && usMbs)
      {
         /* Calculate SCR.
         * ulScr is the number of token bucket refills per second to support the
         * the shaping rate in bytes per second.  ulShapeRate is in bits per sec.
         * The number of tokens per refill is PTM_TOKEN_BUCKET_INCREMENT and is
         * set to SCR_INCR.
         */ 
         //UINT32 ulScr = ulShapeRate / (PTM_TOKEN_BUCKET_INCREMENT * 8);

         /* Calculate SITLMT for SCR.
         * SITLMT is the number of SIT pulses between token bucket refills.
         */
         //UINT32 ulScrSitLmt = ulSitsPerSec / ulScr;
         u64ScrSitLmt = ulSitsPerSec * PTM_TOKEN_BUCKET_INCREMENT ;
	      u64ScrSitLmt = XTMCFG_DIV_ROUND(u64ScrSitLmt,1000) ;   /* In Kbps */
         ulScrSitLmt = XTMCFG_DIV_FACTOR_AND_ROUND(u64ScrSitLmt,8,ulShapeRateKbps);

     	   XtmOsPrintf(XTM_LOG_INF,"ScrSitLmt[%u] SitsPerSec[%u]\n",ulScrSitLmt, ulSitsPerSec);
         if (ulScrSitLmt == 0)
         {
            XtmOsPrintf(XTM_LOG_ERR,"ConfigureTxPortShaping: SCR_SITLMT=0x0 is invalid. No shaping.");
            return PORT_Q_SHAPING_OFF;
         }
         else if (ulScrSitLmt > 0x7FFF)
         {
           XtmOsPrintf(XTM_LOG_INF,"ConfigureTxPortShaping: SCR_SITLMT=0x%x", ulScrSitLmt);
           
           /* Try using SIT_LO_CNT */
             u64ScrSitLmt = ulSitsPerSecLo * PTM_TOKEN_BUCKET_INCREMENT ;
             u64ScrSitLmt = XTMCFG_DIV_ROUND(u64ScrSitLmt, 1000) ; /* In Kbps */
             ulScrSitLmt = XTMCFG_DIV_FACTOR_AND_ROUND(u64ScrSitLmt,8,ulShapeRateKbps) ;
             if (ulScrSitLmt == 0 || ulScrSitLmt > 0x7FFF)
             {
                XtmOsPrintf(XTM_LOG_ERR,"ConfigureTxPortShaping: SCR_SITLMT=0x%x is invalid. No shaping.",
                          ulScrSitLmt);
                return PORT_Q_SHAPING_OFF;
             }
             ulGts |= SST_GTS_SCR_EN | SST_GTS_SITLO_SCR_EN | SST_GTS_PKT_MODE_SHAPING_EN;
         }
         else
         {
            ulGts |= SST_GTS_SCR_EN | SST_GTS_PKT_MODE_SHAPING_EN;
         }
        
   	 XtmOsPrintf(XTM_LOG_INF,"pUtoPortScr[%08x] \n", &pulUtoPortScr[TMUEXT_SCRINCR_INDEX]) ;

         /* Preserve SCR priority which is used for arbitration. */
         pulUtoPortScr[TMUEXT_SCRINCR_INDEX] &= (SSTE_SCR_PLVLWT_MASK << SSTE_SCR_PLVLWT_SHIFT);
         pulUtoPortScr[TMUEXT_SCRINCR_INDEX] |= (PTM_TOKEN_BUCKET_INCREMENT << SSTE_SCR_INCR_SHIFT);
         //pulUtoPortScr[TMUEXT_SCRINCR_INDEX] |= (2286 << SSTE_SCR_INCR_SHIFT);
         pulUtoPortScr[TMUEXT_ACCLMT_INDEX]  = (usMbs << SSTE_SCR_ACCLMT_SHIFT);
         pulUtoPortScr[TMUEXT_SITLMT_CURSIT_INDEX]  = (ulScrSitLmt << SSTE_SCR_SITLMT_SHIFT);
      }
      *pulUtoPortGts = ulGts;

   } /* else (non ATM modes ) */

#if defined(CONFIG_BCM963158) && !defined(XTM_6315X_GFAST_PORT_SHAPING_SUPPORTED)
_RetConfPortShaper :
#endif

   XtmOsPrintf(XTM_LOG_INF,"pUtoPortScr[%d-%08x] pUtoPortScr[%d-%08x] \n",TMUEXT_SCRINCR_INDEX, pulUtoPortScr[TMUEXT_SCRINCR_INDEX], TMUEXT_ACCLMT_INDEX, pulUtoPortScr[TMUEXT_ACCLMT_INDEX]);
   XtmOsPrintf(XTM_LOG_INF,"pUtoPortScr[%d-%08x] pUtoPortScr[%d-%08x] \n",TMUEXT_CURACC_INDEX, pulUtoPortScr[TMUEXT_CURACC_INDEX], TMUEXT_SITLMT_CURSIT_INDEX, pulUtoPortScr[TMUEXT_SITLMT_CURSIT_INDEX]);
   XtmOsPrintf(XTM_LOG_INF,"pUtoPortGts[%08x] ",*pulUtoPortGts);
#endif /* if (HW support) */

   return PORT_Q_SHAPING_ON;
}

void XTM_INTERFACE::DisableTxPortShaping()
{
#if !defined(HW_SAR_BASED_PORT_SHAPING_SUPPORT)
   XTMRT_PORT_SHAPER_INFO   portShaperInfo ;
#endif

   ResetHWTxPortShaping() ;

#if !defined(HW_SAR_BASED_PORT_SHAPING_SUPPORT)
   /* Send a message to XTMRT to control port shaping to disable mode across
   ** all the xtm channels corresponding to the port.
   ** The above lines are to make sure it is controlled at the SAR end.
   ** Values of shaping should be 0 set in previous routines.
   */
   portShaperInfo.ulShapingRate      = m_ulShapeRate ;
   portShaperInfo.usShapingBurstSize = m_usMbs ;
   if (m_pfnXtmrtReq != NULL) {
      (*m_pfnXtmrtReq)(NULL, XTMRT_CMD_SET_TX_PORT_SHAPER_INFO, &portShaperInfo);
   }
   else {
      XtmOsPrintf(XTM_LOG_ERR,"ConfigureTxPortShaping: Reference XTMRT Null \n") ;
   }
#endif
}
#endif /* XTM_PORT_SHAPING */
