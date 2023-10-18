
typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;
typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
__extension__ typedef signed long long int __int64_t;
__extension__ typedef unsigned long long int __uint64_t;
typedef __int8_t __int_least8_t;
typedef __uint8_t __uint_least8_t;
typedef __int16_t __int_least16_t;
typedef __uint16_t __uint_least16_t;
typedef __int32_t __int_least32_t;
typedef __uint32_t __uint_least32_t;
typedef __int64_t __int_least64_t;
typedef __uint64_t __uint_least64_t;
__extension__ typedef long long int __quad_t;
__extension__ typedef unsigned long long int __u_quad_t;
__extension__ typedef long long int __intmax_t;
__extension__ typedef unsigned long long int __uintmax_t;
__extension__ typedef __uint64_t __dev_t;
__extension__ typedef unsigned int __uid_t;
__extension__ typedef unsigned int __gid_t;
__extension__ typedef unsigned long int __ino_t;
__extension__ typedef __uint64_t __ino64_t;
__extension__ typedef unsigned int __mode_t;
__extension__ typedef unsigned int __nlink_t;
__extension__ typedef long int __off_t;
__extension__ typedef __int64_t __off64_t;
__extension__ typedef int __pid_t;
__extension__ typedef struct { int __val[2]; } __fsid_t;
__extension__ typedef long int __clock_t;
__extension__ typedef unsigned long int __rlim_t;
__extension__ typedef __uint64_t __rlim64_t;
__extension__ typedef unsigned int __id_t;
__extension__ typedef long int __time_t;
__extension__ typedef unsigned int __useconds_t;
__extension__ typedef long int __suseconds_t;
__extension__ typedef int __daddr_t;
__extension__ typedef int __key_t;
__extension__ typedef int __clockid_t;
__extension__ typedef void * __timer_t;
__extension__ typedef long int __blksize_t;
__extension__ typedef long int __blkcnt_t;
__extension__ typedef __int64_t __blkcnt64_t;
__extension__ typedef unsigned long int __fsblkcnt_t;
__extension__ typedef __uint64_t __fsblkcnt64_t;
__extension__ typedef unsigned long int __fsfilcnt_t;
__extension__ typedef __uint64_t __fsfilcnt64_t;
__extension__ typedef int __fsword_t;
__extension__ typedef int __ssize_t;
__extension__ typedef long int __syscall_slong_t;
__extension__ typedef unsigned long int __syscall_ulong_t;
typedef __off64_t __loff_t;
typedef char *__caddr_t;
__extension__ typedef int __intptr_t;
__extension__ typedef unsigned int __socklen_t;
typedef int __sig_atomic_t;
__extension__ typedef __int64_t __time64_t;
typedef __u_char u_char;
typedef __u_short u_short;
typedef __u_int u_int;
typedef __u_long u_long;
typedef __quad_t quad_t;
typedef __u_quad_t u_quad_t;
typedef __fsid_t fsid_t;
typedef __loff_t loff_t;
typedef __ino_t ino_t;
typedef __dev_t dev_t;
typedef __gid_t gid_t;
typedef __mode_t mode_t;
typedef __nlink_t nlink_t;
typedef __uid_t uid_t;
typedef __off_t off_t;
typedef __pid_t pid_t;
typedef __id_t id_t;
typedef __ssize_t ssize_t;
typedef __daddr_t daddr_t;
typedef __caddr_t caddr_t;
typedef __key_t key_t;
typedef __clock_t clock_t;
typedef __clockid_t clockid_t;
typedef __time_t time_t;
typedef __timer_t timer_t;
typedef unsigned int size_t;
typedef unsigned long int ulong;
typedef unsigned short int ushort;
typedef unsigned int uint;
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;
typedef __uint8_t u_int8_t;
typedef __uint16_t u_int16_t;
typedef __uint32_t u_int32_t;
typedef __uint64_t u_int64_t;
typedef int register_t __attribute__ ((__mode__ (__word__)));
static __inline __uint16_t
__bswap_16 (__uint16_t __bsx)
{
  return __builtin_bswap16 (__bsx);
}
static __inline __uint32_t
__bswap_32 (__uint32_t __bsx)
{
  return __builtin_bswap32 (__bsx);
}
__extension__ static __inline __uint64_t
__bswap_64 (__uint64_t __bsx)
{
  return __builtin_bswap64 (__bsx);
}
static __inline __uint16_t
__uint16_identity (__uint16_t __x)
{
  return __x;
}
static __inline __uint32_t
__uint32_identity (__uint32_t __x)
{
  return __x;
}
static __inline __uint64_t
__uint64_identity (__uint64_t __x)
{
  return __x;
}
typedef struct
{
  unsigned long int __val[(1024 / (8 * sizeof (unsigned long int)))];
} __sigset_t;
typedef __sigset_t sigset_t;
struct timeval
{
  __time_t tv_sec;
  __suseconds_t tv_usec;
};
struct timespec
{
  __time_t tv_sec;
  __syscall_slong_t tv_nsec;
};
typedef __suseconds_t suseconds_t;
typedef long int __fd_mask;
typedef struct
  {
    __fd_mask __fds_bits[1024 / (8 * (int) sizeof (__fd_mask))];
  } fd_set;
typedef __fd_mask fd_mask;

extern int select (int __nfds, fd_set *__restrict __readfds,
     fd_set *__restrict __writefds,
     fd_set *__restrict __exceptfds,
     struct timeval *__restrict __timeout);
extern int pselect (int __nfds, fd_set *__restrict __readfds,
      fd_set *__restrict __writefds,
      fd_set *__restrict __exceptfds,
      const struct timespec *__restrict __timeout,
      const __sigset_t *__restrict __sigmask);

typedef __blksize_t blksize_t;
typedef __blkcnt_t blkcnt_t;
typedef __fsblkcnt_t fsblkcnt_t;
typedef __fsfilcnt_t fsfilcnt_t;
struct __pthread_rwlock_arch_t
{
  unsigned int __readers;
  unsigned int __writers;
  unsigned int __wrphase_futex;
  unsigned int __writers_futex;
  unsigned int __pad3;
  unsigned int __pad4;
  unsigned char __flags;
  unsigned char __shared;
  unsigned char __pad1;
  unsigned char __pad2;
  int __cur_writer;
};
typedef struct __pthread_internal_slist
{
  struct __pthread_internal_slist *__next;
} __pthread_slist_t;
struct __pthread_mutex_s
{
  int __lock ;
  unsigned int __count;
  int __owner;
  int __kind;
 
  unsigned int __nusers;
  __extension__ union
  {
    int __spins;
    __pthread_slist_t __list;
  };
 
};
struct __pthread_cond_s
{
  __extension__ union
  {
    __extension__ unsigned long long int __wseq;
    struct
    {
      unsigned int __low;
      unsigned int __high;
    } __wseq32;
  };
  __extension__ union
  {
    __extension__ unsigned long long int __g1_start;
    struct
    {
      unsigned int __low;
      unsigned int __high;
    } __g1_start32;
  };
  unsigned int __g_refs[2] ;
  unsigned int __g_size[2];
  unsigned int __g1_orig_size;
  unsigned int __wrefs;
  unsigned int __g_signals[2];
};
typedef unsigned long int pthread_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_mutexattr_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_condattr_t;
typedef unsigned int pthread_key_t;
typedef int pthread_once_t;
union pthread_attr_t
{
  char __size[36];
  long int __align;
};
typedef union pthread_attr_t pthread_attr_t;
typedef union
{
  struct __pthread_mutex_s __data;
  char __size[24];
  long int __align;
} pthread_mutex_t;
typedef union
{
  struct __pthread_cond_s __data;
  char __size[48];
  __extension__ long long int __align;
} pthread_cond_t;
typedef union
{
  struct __pthread_rwlock_arch_t __data;
  char __size[32];
  long int __align;
} pthread_rwlock_t;
typedef union
{
  char __size[8];
  long int __align;
} pthread_rwlockattr_t;
typedef volatile int pthread_spinlock_t;
typedef union
{
  char __size[20];
  long int __align;
} pthread_barrier_t;
typedef union
{
  char __size[4];
  int __align;
} pthread_barrierattr_t;

typedef unsigned char bool;
typedef unsigned char uchar;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef unsigned int uintptr;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
typedef signed long long int64;
typedef float float32;
typedef double float64;
typedef float64 float_t;
typedef union dma64addr {
 struct {
  uint32 lo;
  uint32 hi;
 };
 struct {
  uint32 low;
  uint32 high;
 };
 struct {
  uint32 loaddr;
  uint32 hiaddr;
 };
 struct {
  uint32 low_addr;
  uint32 high_addr;
 };
} dma64addr_t;
typedef unsigned long dmaaddr_t;
typedef struct {
 dmaaddr_t addr;
 uint32 length;
} hnddma_seg_t;
typedef struct {
 void *oshdmah;
 uint origsize;
 uint nsegs;
 hnddma_seg_t segs[8];
} hnddma_seg_map_t;
extern uint32 gFWID;
typedef struct {
 uint16 biststatus_ID;
 uint16 biststatus2_ID;
 uint16 gptimer_ID;
 uint16 usectimer_ID;
 uint16 intstat0_ID;
 uint16 alt_intmask0_ID;
 uint16 inddma_ID;
 uint16 indaqm_ID;
 uint16 indqsel_ID;
 uint16 suspreq_ID;
 uint16 flushreq_ID;
 uint16 chnflushstatus_ID;
 uint16 intrcvlzy0_ID;
 uint16 MACCONTROL_ID;
 uint16 MACCOMMAND_ID;
 uint16 MACINTSTATUS_ID;
 uint16 MACINTMASK_ID;
 uint16 XMT_TEMPLATE_RW_PTR_ID;
 uint16 XMT_TEMPLATE_RW_DATA_ID;
 uint16 PMQHOSTDATA_ID;
 uint16 PMQPATL_ID;
 uint16 PMQPATH_ID;
 uint16 CHNSTATUS_ID;
 uint16 PSM_DEBUG_ID;
 uint16 PHY_DEBUG_ID;
 uint16 MacCapability_ID;
 uint16 objaddr_ID;
 uint16 objdata_ID;
 uint16 ALT_MACINTSTATUS_ID;
 uint16 ALT_MACINTMASK_ID;
 uint16 FrmTxStatus_ID;
 uint16 FrmTxStatus2_ID;
 uint16 FrmTxStatus3_ID;
 uint16 FrmTxStatus4_ID;
 uint16 TSFTimerLow_ID;
 uint16 TSFTimerHigh_ID;
 uint16 CFPRep_ID;
 uint16 CFPStart_ID;
 uint16 CFPMaxDur_ID;
 uint16 AvbRxTimeStamp_ID;
 uint16 AvbTxTimeStamp_ID;
 uint16 MacControl1_ID;
 uint16 MacHWCap1_ID;
 uint16 MacPatchCtrl_ID;
 uint16 gptimer_x_ID;
 uint16 maccontrol_x_ID;
 uint16 maccontrol1_x_ID;
 uint16 maccommand_x_ID;
 uint16 macintstatus_x_ID;
 uint16 macintmask_x_ID;
 uint16 altmacintstatus_x_ID;
 uint16 altmacintmask_x_ID;
 uint16 psmdebug_x_ID;
 uint16 phydebug_x_ID;
 uint16 statctr2_ID;
 uint16 statctr3_ID;
 uint16 ClockCtlStatus_ID;
 uint16 Workaround_ID;
 uint16 POWERCTL_ID;
 uint16 xmt0ctl_ID;
 uint16 fifobase_ID;
 uint16 aggfifocnt_ID;
 uint16 aggfifodata_ID;
 uint16 DebugStoreMask_ID;
 uint16 DebugTriggerMask_ID;
 uint16 DebugTriggerValue_ID;
 uint16 radioregaddr_cross_ID;
 uint16 radioregdata_cross_ID;
 uint16 radioregaddr_ID;
 uint16 radioregdata_ID;
 uint16 rfdisabledly_ID;
 uint16 PHY_REG_0_ID;
 uint16 PHY_REG_1_ID;
 uint16 PHY_REG_2_ID;
 uint16 PHY_REG_3_ID;
 uint16 PHY_REG_4_ID;
 uint16 PHY_REG_5_ID;
 uint16 PHY_REG_7_ID;
 uint16 PHY_REG_6_ID;
 uint16 PHY_REG_8_ID;
 uint16 PHY_REG_A_ID;
 uint16 PHY_REG_B_ID;
 uint16 PHY_REG_C_ID;
 uint16 PHY_REG_D_ID;
 uint16 PHY_REG_ADDR_ID;
 uint16 PHY_REG_DATA_ID;
 uint16 RCV_FIFO_CTL_ID;
 uint16 RCV_CTL_ID;
 uint16 RCV_FRM_CNT_ID;
 uint16 RCV_STATUS_LEN_ID;
 uint16 RXE_PHYRS_1_ID;
 uint16 RXE_RXCNT_ID;
 uint16 RXE_STATUS1_ID;
 uint16 RXE_STATUS2_ID;
 uint16 RXE_PLCP_LEN_ID;
 uint16 rcm_ctl_ID;
 uint16 rcm_mat_data_ID;
 uint16 rcm_mat_mask_ID;
 uint16 rcm_mat_dly_ID;
 uint16 rcm_cond_mask_l_ID;
 uint16 rcm_cond_mask_h_ID;
 uint16 rcm_cond_dly_ID;
 uint16 EXT_IHR_ADDR_ID;
 uint16 EXT_IHR_DATA_ID;
 uint16 RXE_PHYRS_2_ID;
 uint16 RXE_PHYRS_3_ID;
 uint16 PHY_MODE_ID;
 uint16 rcmta_ctl_ID;
 uint16 rcmta_size_ID;
 uint16 rcmta_addr0_ID;
 uint16 rcmta_addr1_ID;
 uint16 rcmta_addr2_ID;
 uint16 ext_ihr_status_ID;
 uint16 radio_ihr_addr_ID;
 uint16 radio_ihr_data_ID;
 uint16 radio_ihr_status_ID;
 uint16 PSM_MAC_CTLH_ID;
 uint16 PSM_MAC_INTSTAT_L_ID;
 uint16 PSM_MAC_INTSTAT_H_ID;
 uint16 PSM_MAC_INTMASK_L_ID;
 uint16 PSM_MAC_INTMASK_H_ID;
 uint16 PSM_MACCOMMAND_ID;
 uint16 PSM_BRC_0_ID;
 uint16 PSM_PHY_CTL_ID;
 uint16 PSM_IHR_ERR_ID;
 uint16 DMP_OOB_AIN_MASK_ID;
 uint16 psm_int_sel_2_ID;
 uint16 PSM_GPIOIN_ID;
 uint16 PSM_GPIOOUT_ID;
 uint16 PSM_GPIOEN_ID;
 uint16 PSM_BRED_0_ID;
 uint16 PSM_BRED_1_ID;
 uint16 PSM_BRED_2_ID;
 uint16 PSM_BRED_3_ID;
 uint16 PSM_BRCL_0_ID;
 uint16 PSM_BRCL_1_ID;
 uint16 PSM_BRCL_2_ID;
 uint16 PSM_BRCL_3_ID;
 uint16 PSM_BRPO_0_ID;
 uint16 PSM_BRPO_1_ID;
 uint16 PSM_BRPO_2_ID;
 uint16 PSM_BRPO_3_ID;
 uint16 PSM_BRWK_0_ID;
 uint16 PSM_BRWK_1_ID;
 uint16 PSM_BRWK_2_ID;
 uint16 PSM_BRWK_3_ID;
 uint16 PSM_INTSEL_0_ID;
 uint16 PSMPowerReqStatus_ID;
 uint16 psm_ihr_err_ID;
 uint16 SubrStkStatus_ID;
 uint16 SubrStkRdPtr_ID;
 uint16 SubrStkRdData_ID;
 uint16 psm_pc_reg_3_ID;
 uint16 PSM_BRC_1_ID;
 uint16 SBRegAddr_ID;
 uint16 SBRegDataL_ID;
 uint16 SBRegDataH_ID;
 uint16 PSMCoreCtlStat_ID;
 uint16 SbAddrLL_ID;
 uint16 SbAddrL_ID;
 uint16 SbAddrH_ID;
 uint16 SbAddrHH_ID;
 uint16 TXE_CTL_ID;
 uint16 TXE_AUX_ID;
 uint16 TXE_TS_LOC_ID;
 uint16 TXE_TIME_OUT_ID;
 uint16 TXE_WM_0_ID;
 uint16 TXE_WM_1_ID;
 uint16 TXE_PHYCTL_ID;
 uint16 TXE_STATUS_ID;
 uint16 TXE_MMPLCP0_ID;
 uint16 TXE_MMPLCP1_ID;
 uint16 TXE_PHYCTL1_ID;
 uint16 TXE_PHYCTL2_ID;
 uint16 xmtfifodef_ID;
 uint16 XmtFifoFrameCnt_ID;
 uint16 xmtfifo_byte_cnt_ID;
 uint16 xmtfifo_head_ID;
 uint16 xmtfifo_rd_ptr_ID;
 uint16 xmtfifo_wr_ptr_ID;
 uint16 xmtfifodef1_ID;
 uint16 aggfifo_cmd_ID;
 uint16 aggfifo_stat_ID;
 uint16 aggfifo_cfgctl_ID;
 uint16 aggfifo_cfgdata_ID;
 uint16 aggfifo_mpdunum_ID;
 uint16 aggfifo_len_ID;
 uint16 aggfifo_bmp_ID;
 uint16 aggfifo_ackedcnt_ID;
 uint16 aggfifo_sel_ID;
 uint16 xmtfifocmd_ID;
 uint16 xmtfifoflush_ID;
 uint16 xmtfifothresh_ID;
 uint16 xmtfifordy_ID;
 uint16 xmtfifoprirdy_ID;
 uint16 xmtfiforqpri_ID;
 uint16 xmttplatetxptr_ID;
 uint16 xmttplateptr_ID;
 uint16 SampleCollectStartPtr_ID;
 uint16 SampleCollectStopPtr_ID;
 uint16 SampleCollectCurPtr_ID;
 uint16 aggfifo_data_ID;
 uint16 XmtTemplateDataLo_ID;
 uint16 XmtTemplateDataHi_ID;
 uint16 xmtsel_ID;
 uint16 xmttxcnt_ID;
 uint16 xmttxshmaddr_ID;
 uint16 xmt_ampdu_ctl_ID;
 uint16 TSF_CFP_STRT_L_ID;
 uint16 TSF_CFP_STRT_H_ID;
 uint16 TSF_CFP_PRE_TBTT_ID;
 uint16 TSF_CLK_FRAC_L_ID;
 uint16 TSF_CLK_FRAC_H_ID;
 uint16 TSF_RANDOM_ID;
 uint16 TSF_GPT_2_STAT_ID;
 uint16 TSF_GPT_2_CTR_L_ID;
 uint16 TSF_GPT_2_CTR_H_ID;
 uint16 TSF_GPT_2_VAL_L_ID;
 uint16 TSF_GPT_2_VAL_H_ID;
 uint16 TSF_GPT_ALL_STAT_ID;
 uint16 IFS_SIFS_RX_TX_TX_ID;
 uint16 IFS_SIFS_NAV_TX_ID;
 uint16 IFS_SLOT_ID;
 uint16 IFS_CTL_ID;
 uint16 IFS_BOFF_CTR_ID;
 uint16 IFS_STAT_ID;
 uint16 IFS_MEDBUSY_CTR_ID;
 uint16 IFS_TX_DUR_ID;
 uint16 IFS_STAT1_ID;
 uint16 IFS_EDCAPRI_ID;
 uint16 IFS_AIFSN_ID;
 uint16 IFS_CTL1_ID;
 uint16 SLOW_CTL_ID;
 uint16 SLOW_TIMER_L_ID;
 uint16 SLOW_TIMER_H_ID;
 uint16 SLOW_FRAC_ID;
 uint16 FAST_PWRUP_DLY_ID;
 uint16 SLOW_PER_ID;
 uint16 SLOW_PER_FRAC_ID;
 uint16 SLOW_CALTIMER_L_ID;
 uint16 SLOW_CALTIMER_H_ID;
 uint16 BTCX_CTL_ID;
 uint16 BTCX_STAT_ID;
 uint16 BTCX_TRANSCTL_ID;
 uint16 BTCXPriorityWin_ID;
 uint16 BTCXConfTimer_ID;
 uint16 BTCXPriSelTimer_ID;
 uint16 BTCXPrvRfActTimer_ID;
 uint16 BTCXCurRfActTimer_ID;
 uint16 BTCXActDurTimer_ID;
 uint16 IFS_CTL_SEL_PRICRS_ID;
 uint16 IfsCtlSelSecCrs_ID;
 uint16 BTCXEciAddr_ID;
 uint16 BTCXEciData_ID;
 uint16 BTCXEciMaskAddr_ID;
 uint16 BTCXEciMaskData_ID;
 uint16 CoexIOMask_ID;
 uint16 btcx_eci_event_addr_ID;
 uint16 btcx_eci_event_data_ID;
 uint16 NAV_CTL_ID;
 uint16 NAV_STAT_ID;
 uint16 WEP_CTL_ID;
 uint16 WEP_STAT_ID;
 uint16 WEP_HDRLOC_ID;
 uint16 WEP_PSDULEN_ID;
 uint16 pcmctl_ID;
 uint16 pcmstat_ID;
 uint16 PMQ_CTL_ID;
 uint16 PMQ_STATUS_ID;
 uint16 PMQ_PAT_0_ID;
 uint16 PMQ_PAT_1_ID;
 uint16 PMQ_PAT_2_ID;
 uint16 PMQ_DAT_ID;
 uint16 PMQ_DAT_OR_MAT_ID;
 uint16 PMQ_DAT_OR_ALL_ID;
 uint16 CLK_GATE_REQ0_ID;
 uint16 CLK_GATE_REQ1_ID;
 uint16 CLK_GATE_REQ2_ID;
 uint16 CLK_GATE_UCODE_REQ0_ID;
 uint16 CLK_GATE_UCODE_REQ1_ID;
 uint16 CLK_GATE_UCODE_REQ2_ID;
 uint16 CLK_GATE_STRETCH0_ID;
 uint16 CLK_GATE_STRETCH1_ID;
 uint16 CLK_GATE_MISC_ID;
 uint16 CLK_GATE_DIV_CTRL_ID;
 uint16 CLK_GATE_PHY_CLK_CTRL_ID;
 uint16 CLK_GATE_STS_ID;
 uint16 CLK_GATE_EXT_REQ0_ID;
 uint16 CLK_GATE_EXT_REQ1_ID;
 uint16 CLK_GATE_EXT_REQ2_ID;
 uint16 CLK_GATE_UCODE_PHY_CLK_CTRL_ID;
} d11regs_regs_common_lt40_t;
typedef struct {
 uint16 RCV_FRM_CNT_Q0_ID;
 uint16 RCV_FRM_CNT_Q1_ID;
 uint16 RCV_WRD_CNT_Q0_ID;
 uint16 RCV_WRD_CNT_Q1_ID;
 uint16 RCV_BM_STARTPTR_Q0_ID;
 uint16 RCV_BM_ENDPTR_Q0_ID;
 uint16 RCV_BM_STARTPTR_Q1_ID;
 uint16 RCV_BM_ENDPTR_Q1_ID;
 uint16 RCV_COPYCNT_Q1_ID;
 uint16 rxe_errval_ID;
 uint16 rxe_status3_ID;
 uint16 XmtFIFOFullThreshold_ID;
 uint16 BMCReadReq_ID;
 uint16 BMCReadOffset_ID;
 uint16 BMCReadLength_ID;
 uint16 BMCReadStatus_ID;
 uint16 XmtShmAddr_ID;
 uint16 PsmMSDUAccess_ID;
 uint16 MSDUEntryBufCnt_ID;
 uint16 MSDUEntryStartIdx_ID;
 uint16 MSDUEntryEndIdx_ID;
 uint16 SampleCollectPlayPtrHigh_ID;
 uint16 SampleCollectCurPtrHigh_ID;
 uint16 BMCCmd1_ID;
 uint16 BMCDynAllocStatus_ID;
 uint16 BMCCTL_ID;
 uint16 BMCConfig_ID;
 uint16 BMCStartAddr_ID;
 uint16 BMCSize_ID;
 uint16 BMCCmd_ID;
 uint16 BMCMaxBuffers_ID;
 uint16 BMCMinBuffers_ID;
 uint16 BMCAllocCtl_ID;
 uint16 BMCDescrLen_ID;
 uint16 SaveRestoreStartPtr_ID;
 uint16 SamplePlayStartPtr_ID;
 uint16 SamplePlayStopPtr_ID;
 uint16 XmtDMABusy_ID;
 uint16 XmtTemplatePtr_ID;
 uint16 XmtSuspFlush_ID;
 uint16 XmtFifoRqPrio_ID;
 uint16 BMCStatCtl_ID;
 uint16 BMCStatData_ID;
 uint16 BMCMSDUFifoStat_ID;
 uint16 TXE_STATUS1_ID;
 uint16 pmqdataor_mat1_ID;
 uint16 pmqdataor_mat2_ID;
 uint16 pmqdataor_mat3_ID;
 uint16 pmq_auxsts_ID;
 uint16 pmq_ctl1_ID;
 uint16 pmq_status1_ID;
 uint16 pmq_addthr_ID;
 uint16 AQMConfig_ID;
 uint16 AQMFifoDef_ID;
 uint16 AQMMaxIdx_ID;
 uint16 AQMRcvdBA0_ID;
 uint16 AQMRcvdBA1_ID;
 uint16 AQMRcvdBA2_ID;
 uint16 AQMRcvdBA3_ID;
 uint16 AQMBaSSN_ID;
 uint16 AQMRefSN_ID;
 uint16 AQMMaxAggLenLow_ID;
 uint16 AQMMaxAggLenHi_ID;
 uint16 AQMAggParams_ID;
 uint16 AQMMinMpduLen_ID;
 uint16 AQMMacAdjLen_ID;
 uint16 DebugBusCtrl_ID;
 uint16 AQMAggStats_ID;
 uint16 AQMAggLenLow_ID;
 uint16 AQMAggLenHi_ID;
 uint16 AQMIdx_ID;
 uint16 AQMMpduLen_ID;
 uint16 AQMTxCnt_ID;
 uint16 AQMUpdBA0_ID;
 uint16 AQMUpdBA1_ID;
 uint16 AQMUpdBA2_ID;
 uint16 AQMUpdBA3_ID;
 uint16 AQMAckCnt_ID;
 uint16 AQMConsCnt_ID;
 uint16 AQMFifoReady_ID;
 uint16 AQMStartLoc_ID;
 uint16 AQMAggRptr_ID;
 uint16 AQMTxcntRptr_ID;
 uint16 TDCCTL_ID;
 uint16 TDC_Plcp0_ID;
 uint16 TDC_Plcp1_ID;
 uint16 TDC_FrmLen0_ID;
 uint16 TDC_FrmLen1_ID;
 uint16 TDC_Txtime_ID;
 uint16 TDC_VhtSigB0_ID;
 uint16 TDC_VhtSigB1_ID;
 uint16 TDC_LSigLen_ID;
 uint16 TDC_NSym0_ID;
 uint16 TDC_NSym1_ID;
 uint16 TDC_VhtPsduLen0_ID;
 uint16 TDC_VhtPsduLen1_ID;
 uint16 TDC_VhtMacPad_ID;
 uint16 AQMCurTxcnt_ID;
 uint16 ShmDma_Ctl_ID;
 uint16 ShmDma_TxdcAddr_ID;
 uint16 ShmDma_ShmAddr_ID;
 uint16 ShmDma_XferCnt_ID;
 uint16 Txdc_Addr_ID;
 uint16 Txdc_Data_ID;
 uint16 MHP_Status_ID;
 uint16 MHP_FC_ID;
 uint16 MHP_DUR_ID;
 uint16 MHP_SC_ID;
 uint16 MHP_QOS_ID;
 uint16 MHP_HTC_H_ID;
 uint16 MHP_HTC_L_ID;
 uint16 MHP_Addr1_H_ID;
 uint16 MHP_Addr1_M_ID;
 uint16 MHP_Addr1_L_ID;
 uint16 MHP_Addr2_H_ID;
 uint16 MHP_Addr2_M_ID;
 uint16 MHP_Addr2_L_ID;
 uint16 MHP_Addr3_H_ID;
 uint16 MHP_Addr3_M_ID;
 uint16 MHP_Addr3_L_ID;
 uint16 MHP_Addr4_H_ID;
 uint16 MHP_Addr4_M_ID;
 uint16 MHP_Addr4_L_ID;
 uint16 MHP_CFC_ID;
 uint16 DAGG_CTL2_ID;
 uint16 DAGG_BYTESLEFT_ID;
 uint16 DAGG_SH_OFFSET_ID;
 uint16 DAGG_STAT_ID;
 uint16 DAGG_LEN_ID;
 uint16 TXBA_CTL_ID;
 uint16 TXBA_DataSel_ID;
 uint16 TXBA_Data_ID;
 uint16 DAGG_LEN_THR_ID;
 uint16 AMT_CTL_ID;
 uint16 AMT_Status_ID;
 uint16 AMT_Limit_ID;
 uint16 AMT_Attr_ID;
 uint16 AMT_Match1_ID;
 uint16 AMT_Match2_ID;
 uint16 AMT_Table_Addr_ID;
 uint16 AMT_Table_Data_ID;
 uint16 AMT_Table_Val_ID;
 uint16 AMT_DBG_SEL_ID;
 uint16 RoeCtrl_ID;
 uint16 RoeStatus_ID;
 uint16 RoeIPChkSum_ID;
 uint16 RoeTCPUDPChkSum_ID;
 uint16 PSOCtl_ID;
 uint16 PSORxWordsWatermark_ID;
 uint16 PSORxCntWatermark_ID;
 uint16 OBFFCtl_ID;
 uint16 OBFFRxWordsWatermark_ID;
 uint16 OBFFRxCntWatermark_ID;
 uint16 RcvHdrConvCtrlSts_ID;
 uint16 RcvHdrConvSts_ID;
 uint16 RcvHdrConvSts1_ID;
 uint16 RCVLB_DAGG_CTL_ID;
 uint16 RcvFifo0Len_ID;
 uint16 RcvFifo1Len_ID;
 uint16 ToECTL_ID;
 uint16 ToERst_ID;
 uint16 ToECSumNZ_ID;
 uint16 TxSerialCtl_ID;
 uint16 TxPlcpLSig0_ID;
 uint16 TxPlcpLSig1_ID;
 uint16 TxPlcpHtSig0_ID;
 uint16 TxPlcpHtSig1_ID;
 uint16 TxPlcpHtSig2_ID;
 uint16 TxPlcpVhtSigB0_ID;
 uint16 TxPlcpVhtSigB1_ID;
 uint16 MacHdrFromShmLen_ID;
 uint16 TxPlcpLen_ID;
 uint16 TxBFRptLen_ID;
 uint16 BytCntInTxFrmLo_ID;
 uint16 BytCntInTxFrmHi_ID;
 uint16 TXBFCtl_ID;
 uint16 BfmRptOffset_ID;
 uint16 BfmRptLen_ID;
 uint16 TXBFBfeRptRdCnt_ID;
 uint16 PsmMboxOutSts_ID;
 uint16 PSM_BASE_0_ID;
 uint16 psm_base_x_ID;
 uint16 RXMapFifoSize_ID;
 uint16 RXMapStatus_ID;
 uint16 MsduThreshold_ID;
 uint16 BMCCore0TXAllMaxBuffers_ID;
 uint16 BMCCore1TXAllMaxBuffers_ID;
 uint16 BMCDynAllocStatus1_ID;
 uint16 DMAMaxOutStBuffers_ID;
 uint16 SampleCollectStoreMaskLo_ID;
 uint16 SampleCollectStoreMaskHi_ID;
 uint16 SampleCollectMatchMaskLo_ID;
 uint16 SampleCollectMatchMaskHi_ID;
 uint16 SampleCollectMatchValueLo_ID;
 uint16 SampleCollectMatchValueHi_ID;
 uint16 SampleCollectTriggerMaskLo_ID;
 uint16 SampleCollectTriggerMaskHi_ID;
 uint16 SampleCollectTriggerValueLo_ID;
 uint16 SampleCollectTriggerValueHi_ID;
 uint16 SampleCollectTransMaskLo_ID;
 uint16 SampleCollectTransMaskHi_ID;
 uint16 SampleCollectPlayCtrl_ID;
 uint16 Core0BMCAllocStatusTID7_ID;
 uint16 Core1BMCAllocStatusTID7_ID;
 uint16 XmtTemplatePtrOffset_ID;
 uint16 BMVpConfig_ID;
} d11regs_regs_common_ge40_lt64_t;
typedef struct {
 uint16 syncSlowTimeStamp_ID;
 uint16 IndXmtCtl_ID;
 uint16 IndAQMctl_ID;
 uint16 IndAQMQSel_ID;
 uint16 SUSPREQ_ID;
 uint16 FLUSHREQ_ID;
 uint16 CHNFLUSH_STATUS_ID;
 uint16 gptimer_psmx_ID;
 uint16 MACCONTROL_psmx_ID;
 uint16 MacControl1_psmx_ID;
 uint16 MACCOMMAND_psmx_ID;
 uint16 MACINTSTATUS_psmx_ID;
 uint16 MACINTMASK_psmx_ID;
 uint16 ALT_MACINTSTATUS_psmx_ID;
 uint16 ALT_MACINTMASK_psmx_ID;
 uint16 PSM_DEBUG_psmx_ID;
 uint16 PHY_DEBUG_psmx_ID;
 uint16 RXE_ERRVAL_ID;
 uint16 RXE_ERRMASK_ID;
 uint16 RXE_STATUS3_ID;
 uint16 PSM_STAT_CTR0_L_ID;
 uint16 PSM_STAT_CTR1_H_ID;
 uint16 PSM_STAT_SEL_ID;
 uint16 TXE_MMPLCP_0_ID;
 uint16 TXE_MMPLCP_1_ID;
 uint16 TXE_PHYCTL_1_ID;
 uint16 TXE_PHYCTL_2_ID;
 uint16 SMP_PTR_H_ID;
 uint16 SCP_CURPTR_H_ID;
 uint16 SCP_STRTPTR_ID;
 uint16 SCP_STOPPTR_ID;
 uint16 SCP_CURPTR_ID;
 uint16 SPP_STRTPTR_ID;
 uint16 SPP_STOPPTR_ID;
 uint16 WEP_HWKEY_ADDR_ID;
 uint16 WEP_HWKEY_LEN_ID;
 uint16 PMQ_DAT_OR_MAT_MU1_ID;
 uint16 PMQ_DAT_OR_MAT_MU2_ID;
 uint16 PMQ_DAT_OR_MAT_MU3_ID;
 uint16 PMQ_AUXPMQ_STATUS_ID;
 uint16 PMQ_CTL1_ID;
 uint16 PMQ_STATUS1_ID;
 uint16 PMQ_ADDTHR_ID;
 uint16 CTMode_ID;
 uint16 SCM_HVAL_L_ID;
 uint16 SCM_HVAL_H_ID;
 uint16 SCT_HMASK_L_ID;
 uint16 SCT_HMASK_H_ID;
 uint16 SCT_HVAL_L_ID;
 uint16 SCT_HVAL_H_ID;
 uint16 SCX_HMASK_L_ID;
 uint16 SCX_HMASK_H_ID;
 uint16 BMC_ReadQID_ID;
 uint16 BMC_BQFrmCnt_ID;
 uint16 BMC_BQByteCnt_L_ID;
 uint16 BMC_BQByteCnt_H_ID;
 uint16 AQM_BQFrmCnt_ID;
 uint16 AQM_BQByteCnt_L_ID;
 uint16 AQM_BQByteCnt_H_ID;
 uint16 AQM_BQPrelWM_ID;
 uint16 AQM_BQPrelStatus_ID;
 uint16 AQM_BQStatus_ID;
 uint16 BMC_MUDebugConfig_ID;
 uint16 BMC_MUDebugStatus_ID;
 uint16 BMCBQCutThruSt0_ID;
 uint16 BMCBQCutThruSt1_ID;
 uint16 BMCBQCutThruSt2_ID;
 uint16 TDC_VhtMacPad0_ID;
 uint16 TDC_VhtMacPad1_ID;
 uint16 TDC_MuVhtMCS_ID;
 uint16 ShmDma_XferWCnt_ID;
 uint16 AMT_MATCH1_ID;
 uint16 AMT_MATCH2_ID;
 uint16 PSM_REG_MUX_ID;
 uint16 PSM_BASE_PSMX_ID;
 uint16 SCS_MASK_L_ID;
 uint16 SCS_MASK_H_ID;
 uint16 SCM_MASK_L_ID;
 uint16 SCM_MASK_H_ID;
 uint16 SCM_VAL_L_ID;
 uint16 SCM_VAL_H_ID;
 uint16 SCT_MASK_L_ID;
 uint16 SCT_MASK_H_ID;
 uint16 SCT_VAL_L_ID;
 uint16 SCT_VAL_H_ID;
 uint16 SCX_MASK_L_ID;
 uint16 SCX_MASK_H_ID;
 uint16 SMP_CTRL_ID;
 uint16 SysMStartAddrHi_ID;
 uint16 SysMStartAddrLo_ID;
 uint16 AQM_REG_SEL_ID;
 uint16 AQMQMAP_ID;
 uint16 AQMCmd_ID;
 uint16 AQMConsMsdu_ID;
 uint16 AQMDMACTL_ID;
 uint16 AQMMinCons_ID;
 uint16 MsduMinCons_ID;
 uint16 AQMAggNum_ID;
 uint16 AQMAggEntry_ID;
 uint16 AQMAggIdx_ID;
 uint16 AQMFiFoRptr_ID;
 uint16 AQMFIFO_SOFDPtr_ID;
 uint16 AQMFIFO_SWDCnt_ID;
 uint16 AQMFIFO_TXDPtr_L_ID;
 uint16 AQMFIFO_TXDPtr_ML_ID;
 uint16 AQMFIFO_TXDPtr_MU_ID;
 uint16 AQMFIFO_TXDPtr_H_ID;
 uint16 AQMFifoRdy_L_ID;
 uint16 AQMFifoRdy_H_ID;
 uint16 AQMFifo_Status_ID;
 uint16 AQMFifoFull_L_ID;
 uint16 AQMFifoFull_H_ID;
 uint16 AQMTBCP_Busy_L_ID;
 uint16 AQMTBCP_Busy_H_ID;
 uint16 AQMDMA_SuspFlush_ID;
 uint16 AQMFIFOSusp_L_ID;
 uint16 AQMFIFOSusp_H_ID;
 uint16 AQMFIFO_SuspPend_L_ID;
 uint16 AQMFIFO_SuspPend_H_ID;
 uint16 AQMFIFOFlush_L_ID;
 uint16 AQMFIFOFlush_H_ID;
 uint16 AQMTXD_CTL_ID;
 uint16 AQMTXD_RdOffset_ID;
 uint16 AQMTXD_RdLen_ID;
 uint16 AQMTXD_DestAddr_ID;
 uint16 AQMTBCP_QSEL_ID;
 uint16 AQMTBCP_Prio_ID;
 uint16 AQMTBCP_PrioFifo_ID;
 uint16 AQMFIFO_MPDULen_ID;
 uint16 AQMTBCP_Max_ReqEntry_ID;
 uint16 AQMTBCP_FCNT_ID;
 uint16 AQMSTATUS_ID;
 uint16 AQMDBG_CTL_ID;
 uint16 AQMDBG_DATA_ID;
 uint16 CHNSUSP_STATUS_ID;
} d11regs_regs_common_ge64_lt80_t;
typedef struct {
 uint16 RxFilterEn_ID;
 uint16 RxHwaCtrl_ID;
 uint16 FlowAgeTimer_ID;
 uint16 RcvAMPDUCtl1_ID;
 uint16 RXE_PHYRS_ADDR_ID;
 uint16 RXE_PHYRS_DATA_ID;
 uint16 AMT_MATCH_ID;
 uint16 AMT_ATTR_A1_ID;
 uint16 AMT_ATTR_A2_ID;
 uint16 AMT_ATTR_A3_ID;
 uint16 AMT_ATTR_BSSID_ID;
 uint16 RXE_TXBA_CTL2_ID;
 uint16 MULTITID_STATUS_ID;
 uint16 FRAG_INFO_ID;
 uint16 FP_SHM_OFFSET_ID;
 uint16 FP_CTL_ID;
 uint16 BA_INFO_ACCESS_ID;
 uint16 BA_INFO_DATA_ID;
 uint16 FP_CONFIG_ID;
 uint16 FP_PATTERN1_ID;
 uint16 FP_MASK_ID;
 uint16 FP_STATUS_ID;
 uint16 FP_PATTERN2_ID;
 uint16 FP_PATTERN3_ID;
 uint16 RoeStatus1_ID;
 uint16 PSOCurRxFramePtrs_ID;
 uint16 PSOOBFFStatus_ID;
 uint16 AVB_RXTIMESTAMP_L_ID;
 uint16 AVB_RXTIMESTAMP_H_ID;
 uint16 IFS_STAT2_ID;
 uint16 IFS_CTL_SEL_SECCRS_ID;
 uint16 WEP_KEY_DATA_ID;
 uint16 WEP_DATA_ID;
 uint16 BTCX_PRIORITYWIN_ID;
 uint16 BTCX_TXCONFTIMER_ID;
 uint16 BTCX_PRISELTIMER_ID;
 uint16 BTCX_PRV_RFACT_TIMER_ID;
 uint16 BTCX_CUR_RFACT_TIMER_ID;
 uint16 BTCX_RFACT_DUR_TIMER_ID;
 uint16 BTCX_ECI_ADDR_ID;
 uint16 BTCX_ECI_DATA_ID;
 uint16 BTCX_ECI_MASK_ADDR_ID;
 uint16 BTCX_ECI_MASK_DATA_ID;
 uint16 COEX_IO_MASK_ID;
 uint16 BTCX_ECI_EVENT_ADDR_ID;
 uint16 BTCX_ECI_EVENT_DATA_ID;
 uint16 PSM_INTSEL_1_ID;
 uint16 PSM_INTSEL_2_ID;
 uint16 PSM_INTSEL_3_ID;
 uint16 TXE_BV_31_ID;
 uint16 TXE_STATUS2_ID;
 uint16 XmtSusp_ID;
 uint16 TXE_PHYCTL8_ID;
 uint16 TXE_PHYCTL9_ID;
 uint16 TXE_PHYCTL_3_ID;
 uint16 TXE_PHYCTL_4_ID;
 uint16 TXE_PHYCTL_5_ID;
 uint16 TXE_PHYCTL_6_ID;
 uint16 TXE_PHYCTL_7_ID;
 uint16 DebugTxFlowCtl_ID;
 uint16 TDC_PLCP0_ID;
 uint16 TDC_PLCP1_ID;
 uint16 TDC_FRMLEN0_ID;
 uint16 TDC_FRMLEN1_ID;
 uint16 TDC_TXTIME_ID;
 uint16 TDC_PLCP2_ID;
 uint16 TDC_PLCP3_ID;
 uint16 TDC_LSIGLEN_ID;
 uint16 TDC_NSYM0_ID;
 uint16 TDC_NSYM1_ID;
 uint16 SampleCollectPlayCtrl2_ID;
 uint16 XmtFlush_ID;
 uint16 AQMAGGR_ID;
 uint16 AQMMultBAFL_ID;
 uint16 AQM_DFAdjB_ID;
 uint16 AQM_MFAdjB_ID;
 uint16 AQM_CFAdjB_ID;
 uint16 AQMminfragB_ID;
 uint16 AQMcagburst_ID;
 uint16 MQAQMMaxAggLenLow_ID;
 uint16 MQAQMMaxAggLenHi_ID;
 uint16 MQMAXMPDU_ID;
 uint16 AMPDUXMT_ID;
 uint16 AQMCaggNum_ID;
 uint16 AQMCaggLenLow_ID;
 uint16 AQMCAggLenHi_ID;
 uint16 XMT_AMPDU_DLIM_H_ID;
 uint16 AQMFIFO_FLAG_ID;
 uint16 AQMFSIE_OP_ID;
 uint16 AQMFSIE_RWD0_ID;
 uint16 AQMFSIE_RWD1_ID;
 uint16 AQMFSIE_RWD2_ID;
 uint16 BMCHdrOffset_ID;
 uint16 BMCHdrLength_ID;
 uint16 Core0BMCAllocStatusTplate_ID;
 uint16 Core1BMCAllocStatusTplate_ID;
} d11regs_regs_common_ge80_lt128_t;
typedef struct {
 uint16 xmt_dma_chsel_ID;
 uint16 xmt_dma_intmap0_ID;
 uint16 xmt_dma_intmap1_ID;
 uint16 indintstat_ID;
 uint16 indintmask_ID;
 uint16 SUSPREQ1_ID;
 uint16 FLUSHREQ1_ID;
 uint16 CHNFLUSH_STATUS1_ID;
 uint16 CHNSUSP_STATUS1_ID;
 uint16 SUSPREQ2_ID;
 uint16 FLUSHREQ2_ID;
 uint16 CHNFLUSH_STATUS2_ID;
 uint16 CHNSUSP_STATUS2_ID;
 uint16 TxDMAIndXmtStat0_ID;
 uint16 TxDMAIndXmtStat1_ID;
 uint16 PHYPLUS1CTL_ID;
 uint16 PSM_CHK0_ERR_ID;
 uint16 PSM_CHK1_ERR_ID;
 uint16 gptimer_R1_ID;
 uint16 MACCONTROL_r1_ID;
 uint16 MacControl1_R1_ID;
 uint16 MACCOMMAND_R1_ID;
 uint16 MACINTSTATUS_R1_ID;
 uint16 MACINTMASK_R1_ID;
 uint16 ALT_MACINTSTATUS_R1_ID;
 uint16 ALT_MACINTMASK_R1_ID;
 uint16 PSM_DEBUG_R1_ID;
 uint16 TSFTimerLow_R1_ID;
 uint16 TSFTimerHigh_R1_ID;
 uint16 HostFCBSCmdPtr_ID;
 uint16 HostFCBSDataPtr_ID;
 uint16 HostCtrlStsReg_ID;
 uint16 RXE_CBR_CTL_ID;
 uint16 RXE_CBR_STAT_ID;
 uint16 RXE_DMA_STCNT_ID;
 uint16 OMAC_HWSTS_L_ID;
 uint16 OMAC_HWSTS_H_ID;
 uint16 RXBM_DBG_SEL_ID;
 uint16 RXBM_DBG_DATA_ID;
 uint16 RCV_BM_OVFL_DBGSEL_ID;
 uint16 MHP_FCTP_ID;
 uint16 MHP_FCTST_ID;
 uint16 MHP_DFCTST_ID;
 uint16 MHP_EXT0_ID;
 uint16 MHP_EXT1_ID;
 uint16 MHP_EXT2_ID;
 uint16 MHP_EXT3_ID;
 uint16 MHP_PLCP0_ID;
 uint16 MHP_PLCP1_ID;
 uint16 MHP_PLCP2_ID;
 uint16 MHP_PLCP3_ID;
 uint16 MHP_PLCP4_ID;
 uint16 MHP_PLCP5_ID;
 uint16 MHP_PLCP6_ID;
 uint16 MHP_SEL_ID;
 uint16 MHP_DATA_ID;
 uint16 MHP_CFG_ID;
 uint16 RXE_RCF_CTL_ID;
 uint16 RXE_RCF_ADDR_ID;
 uint16 RXE_RCF_WDATA_ID;
 uint16 RXE_RCF_RDATA_ID;
 uint16 RXE_RCF_NP_STATS_ID;
 uint16 RXE_RCF_HP_STATS_ID;
 uint16 RXE_HDRC_CTL_ID;
 uint16 RXE_HDRC_STATUS_ID;
 uint16 RXE_WM_0_ID;
 uint16 RXE_WM_1_ID;
 uint16 RXE_BM_ADDR_ID;
 uint16 RXE_BM_DATA_ID;
 uint16 RXE_BV_ADDR_ID;
 uint16 RXE_BV_DATA_ID;
 uint16 PERUSER_DBG_SEL_ID;
 uint16 RXQ_DBG_CTL_ID;
 uint16 RXQ_DBG_STS_ID;
 uint16 FP_PAT0_ID;
 uint16 FP_PAT1_ID;
 uint16 FP_PAT2_ID;
 uint16 FP_SUPAT_ID;
 uint16 RXE_FP_SHMADDR_ID;
 uint16 RXE_TXBA_CTL1_ID;
 uint16 MTID_STATUS_ID;
 uint16 BA_LEN_ID;
 uint16 BA_LEN_ENC_ID;
 uint16 MULTITID_STATUS2_ID;
 uint16 BAINFO_SEL_ID;
 uint16 BAINFO_DATA_ID;
 uint16 TXBA_UBMP_ID;
 uint16 TXBA_UCNT_ID;
 uint16 TXBA_XFRBMP_ID;
 uint16 TXBA_XFRCTL_ID;
 uint16 TXBA_XFRCNT_ID;
 uint16 TXBA_XFROFFS_ID;
 uint16 RDFBD_CTL0_ID;
 uint16 RDF_CTL0_ID;
 uint16 RDF_CTL1_ID;
 uint16 RDF_CTL2_ID;
 uint16 RDF_CTL3_ID;
 uint16 RDF_CTL4_ID;
 uint16 RDF_CTL5_ID;
 uint16 RDF_STAT1_ID;
 uint16 RDF_STAT2_ID;
 uint16 RDF_STAT3_ID;
 uint16 RDF_STAT4_ID;
 uint16 RDF_STAT5_ID;
 uint16 RDF_STAT6_ID;
 uint16 RDF_STAT7_ID;
 uint16 RDF_STAT8_ID;
 uint16 RXE_PHYSTS_SHMADDR_ID;
 uint16 RXE_PHYSTS_BMP_SEL_ID;
 uint16 RXE_PHYSTS_BMP_DATA_ID;
 uint16 RXE_PHYSTS_TIMEOUT_ID;
 uint16 RXE_OOB_CFG_ID;
 uint16 RXE_OOB_BMP0_ID;
 uint16 RXE_OOB_BMP1_ID;
 uint16 RXE_OOB_BMP2_ID;
 uint16 RXE_OOB_BMP3_ID;
 uint16 RXE_OOB_DSCR_ADDR_ID;
 uint16 RXE_OOB_DSCR_SIZE_ID;
 uint16 RXE_OOB_BUFA_ADDR_ID;
 uint16 RXE_OOB_BUFA_SIZE_ID;
 uint16 RXE_OOB_BUFB_ADDR_ID;
 uint16 RXE_OOB_BUFB_SIZE_ID;
 uint16 RXE_OOB_STATUS_ID;
 uint16 RXE_OOB_DESC_PTR_ID;
 uint16 RXE_OOB_BUFA_PTR_ID;
 uint16 RXE_OOB_BUFB_PTR_ID;
 uint16 RXE_BFDRPT_CTL_ID;
 uint16 RXE_BFDRPT_RST_ID;
 uint16 RXE_BFDRPT_LEN_ID;
 uint16 RXE_BFDRPT_OFFSET_ID;
 uint16 RXE_BFDRPT_MEND_ID;
 uint16 RXE_BFDRPT_XFER_ID;
 uint16 RXE_BFDRPT_SUCC_ID;
 uint16 RXE_BFDRPT_DONE_ID;
 uint16 RXE_BFM_HDRSEL_ID;
 uint16 RXE_BFM_HDRDATA_ID;
 uint16 CMDPTR_L_ID;
 uint16 CMDPTR_H_ID;
 uint16 DATA_PTR_L_ID;
 uint16 DATA_PTR_H_ID;
 uint16 CTRL_STS_ID;
 uint16 PHY_ADDR_ID;
 uint16 PHY_DATA_ID;
 uint16 RADIO_ADDR_ID;
 uint16 RADIO_DATA_ID;
 uint16 RUNTIME_CNT_ID;
 uint16 TSF_GPT_0_CTR_L_ID;
 uint16 TSF_GPT_0_CTR_H_ID;
 uint16 TSF_GPT_0_VAL_L_ID;
 uint16 TSF_GPT_0_VAL_H_ID;
 uint16 TSF_GPT_1_STAT_ID;
 uint16 TSF_GPT_1_CTL_L_ID;
 uint16 TSF_GPT_1_CTL_H_ID;
 uint16 TSF_GPT_1_VAL_L_ID;
 uint16 HWA_MACIF_CTL_ID;
 uint16 PSM_DIV_REM_L_ID;
 uint16 PSM_DIV_REM_H_ID;
 uint16 PSM_MACCTL_ID;
 uint16 PSM_SBADDR_ID;
 uint16 PSM_BRC_SEL_1_ID;
 uint16 PSM_BASE_14_ID;
 uint16 PSM_BASE_15_ID;
 uint16 PSM_BASE_16_ID;
 uint16 PSM_BASE_17_ID;
 uint16 PSM_BASE_18_ID;
 uint16 PSM_BASE_19_ID;
 uint16 PSM_BASE_20_ID;
 uint16 PSM_BASE_21_ID;
 uint16 PSM_BASE_22_ID;
 uint16 PSM_BASE_23_ID;
 uint16 PSM_RATEMEM_DBG_ID;
 uint16 PSM_RATEMEM_CTL_ID;
 uint16 PSM_RATEBLK_SIZE_ID;
 uint16 PSM_RATEXFER_SIZE_ID;
 uint16 PSM_LINKMEM_CTL_ID;
 uint16 PSM_LINKBLK_SIZE_ID;
 uint16 PSM_LINKXFER_SIZE_ID;
 uint16 MAC_GPIOOUT_L_ID;
 uint16 MAC_GPIOOUT_H_ID;
 uint16 PMQ_CNT_ID;
 uint16 PMQ_SRCH_USREN_ID;
 uint16 PMQ_USR_SEL_ID;
 uint16 PMQ_DAT_OR_MAT_SEL_ID;
 uint16 PMQ_DAT_OR_MAT_DATA_ID;
 uint16 PMQ_MATCH_ID;
 uint16 APMQ_MATCH_ID;
 uint16 PMQ_USER_BMP_ID;
 uint16 TXE_FCS_CTL_ID;
 uint16 AMPDU_CTL_ID;
 uint16 AMPDU_CRC_ID;
 uint16 TXE_WM_LEG0_ID;
 uint16 TXE_WM_LEG1_ID;
 uint16 TXE_CTL3_ID;
 uint16 TXE_TXE_EARLYTXMEND_CNT_ID;
 uint16 TXE_TXE_EARLYTXMEND_BSUB_CNT_ID;
 uint16 TXE_TXE_TXMEND_CNT_ID;
 uint16 TXE_TXE_TXMEND_NCONS_ID;
 uint16 TXE_TXE_TXMEND_PEND_ID;
 uint16 TXE_TXE_TXMEND_USR2GO_ID;
 uint16 TXE_TXE_TXMEND_CONSCNT_ID;
 uint16 TXE_BVBM_INIT_ID;
 uint16 ToEChannelState_ID;
 uint16 TXE_BV_REG0_ID;
 uint16 TXE_BM_REG0_ID;
 uint16 TXE_BV_REG1_ID;
 uint16 TXE_BM_REG1_ID;
 uint16 TXE_SHMDMA_MPMADDR_ID;
 uint16 TXE_BITSUB_IDX_ID;
 uint16 TXE_BM_ADDR_ID;
 uint16 TXE_BM_DATA_ID;
 uint16 TXE_BV_ADDR_ID;
 uint16 TXE_BV_DATA_ID;
 uint16 TXE_BV_10_ID;
 uint16 TXE_BV_11_ID;
 uint16 TXE_BV_12_ID;
 uint16 TXE_BV_13_ID;
 uint16 TXE_BV_14_ID;
 uint16 TXE_BV_15_ID;
 uint16 TXE_BMC_CONFIG_ID;
 uint16 TXE_BMC_RXMAPFIFOSIZE_ID;
 uint16 TXE_BMC_RXMAPSTATUS_ID;
 uint16 TXE_BMC_DYNSTATUS1_ID;
 uint16 TXE_BMC_MAXOUTSTBUFS_ID;
 uint16 TXE_BMC_CONFIG1_ID;
 uint16 TXE_LLM_CONFIG_ID;
 uint16 TXE_LOCALM_SADDR_L_ID;
 uint16 TXE_LOCALM_SADDR_H_ID;
 uint16 TXE_LOCALM_EADDR_L_ID;
 uint16 TXE_LOCALM_EADDR_H_ID;
 uint16 TXE_BMC_ALLOCCTL1_ID;
 uint16 TXE_BMC_MALLOCREQ_QB0_ID;
 uint16 TXE_BMC_MALLOCREQ_QB1_ID;
 uint16 TXE_BMC_MALLOCREQ_QB2_ID;
 uint16 TXE_BMC_MALLOCREQ_QB3_ID;
 uint16 TXE_BMC_MALLOCREQ_QB4_ID;
 uint16 PHYCTL_LEN_ID;
 uint16 TXE_PHYCTL_10_ID;
 uint16 PLCP_LSIG0_ID;
 uint16 PLCP_LSIG1_ID;
 uint16 PLCP_HTSIG0_ID;
 uint16 PLCP_HTSIG1_ID;
 uint16 PLCP_HTSIG2_ID;
 uint16 TXE_PLCP_VHTSIGB0_ID;
 uint16 TXE_PLCP_VHTSIGB1_ID;
 uint16 PLCP_EXT2_ID;
 uint16 PLCP_CC1_LEN_ID;
 uint16 PLCP_CC2_LEN_ID;
 uint16 TXE_MPMSIZE_SEL_ID;
 uint16 TXE_MPMSIZE_VAL_ID;
 uint16 TXE_PLCPEXT_ADDR_ID;
 uint16 TXE_PLCPEXT_DATA_ID;
 uint16 TXE_PHYCTLEXT_BASE_ID;
 uint16 TXE_PLCPEXT_BASE_ID;
 uint16 TXE_SIGB_BASE_ID;
 uint16 TXE_PLCP_LEN_ID;
 uint16 TXE_TXDBG_SEL_ID;
 uint16 TXE_TXDBG_DATA_ID;
 uint16 TXE_BFMRPT_MEMSEL_ID;
 uint16 TXE_BFMRPT_ADDR_ID;
 uint16 TXE_BFMRPT_DATA_ID;
 uint16 TXE_MEND_STATUS_ID;
 uint16 TXE_UNFLOW_STATUS_ID;
 uint16 TXE_TXERROR_STATUS_ID;
 uint16 TXE_SNDFRM_STATUS_ID;
 uint16 TXE_TXERROR_USR_ID;
 uint16 TXE_MACPAD_PAT_L_ID;
 uint16 TXE_MACPAD_PAT_H_ID;
 uint16 TXE_PHYTXREQ_TMOUT_ID;
 uint16 TXE_XMTSUSP_QB0_ID;
 uint16 TXE_XMTSUSP_QB1_ID;
 uint16 TXE_XMTSUSP_QB2_ID;
 uint16 TXE_XMTSUSP_QB3_ID;
 uint16 TXE_XMTSUSP_QB4_ID;
 uint16 TXE_XMTFLUSH_QB0_ID;
 uint16 TXE_XMTFLUSH_QB1_ID;
 uint16 TXE_XMTFLUSH_QB2_ID;
 uint16 TXE_XMTFLUSH_QB3_ID;
 uint16 TXE_XMTFLUSH_QB4_ID;
 uint16 TXE_XMT_SHMADDR_ID;
 uint16 TXE_BMC_READQID_ID;
 uint16 TXE_BMC_READIDX_ID;
 uint16 BMC_AQMBQID_ID;
 uint16 BMC_PSMCMD_THRESH_ID;
 uint16 BMC_PSMCMD_LOWVEC_ID;
 uint16 BMC_PSMCMD_EMPTYVEC_ID;
 uint16 BMC_PSMCMD_OFLOW_ID;
 uint16 BMC_CMD2SCHED_PEND_ID;
 uint16 BMC_AQM_TXDRD_PEND_ID;
 uint16 BMC_UC_TXDRD_PEND_ID;
 uint16 BMC_CMDQ_FREECNT_ID;
 uint16 BMC_CMDQ_FREESTS_ID;
 uint16 BMC_CMDQ_FRMCNT_ID;
 uint16 BMC_FRM2SER_PEND_ID;
 uint16 BMC_FRM2SER_PRGR_ID;
 uint16 BMC_FRM2MPM_PEND_ID;
 uint16 BMC_FRM2MPM_PENDSTS_ID;
 uint16 BMC_FRM2MPM_PRGR_ID;
 uint16 BMC_BITSUB_FREECNT_ID;
 uint16 BMC_BITSUB_FREESTS_ID;
 uint16 BMC_CMDQ_USR2GO_ID;
 uint16 BMC_BITSUB_USR2GO_ID;
 uint16 TXE_BRC_CMDQ_FREEUP_ID;
 uint16 TXE_BRC_FRM2MPM_PEND_ID;
 uint16 BMC_PSMCMD0_ID;
 uint16 BMC_PSMCMD1_ID;
 uint16 BMC_PSMCMD2_ID;
 uint16 BMC_PSMCMD3_ID;
 uint16 BMC_PSMCMD4_ID;
 uint16 BMC_PSMCMD5_ID;
 uint16 BMC_PSMCMD6_ID;
 uint16 BMC_PSMCMD7_ID;
 uint16 BMC_RDCLIENT_CTL_ID;
 uint16 BMC_RDMGR_USR_RST_ID;
 uint16 BMC_BMRD_INFLIGHT_THRESH_ID;
 uint16 BMC_BITSUB_CAP_ID;
 uint16 BMC_ERR_ID;
 uint16 TXE_SCHED_USR_RST_ID;
 uint16 TXE_SCHED_SET_UFL_ID;
 uint16 TXE_SCHED_UFL_WAIT_ID;
 uint16 BMC_SCHED_UFL_NOCMD_ID;
 uint16 BMC_CMD2SCHED_PENDSTS_ID;
 uint16 BMC_PSMCMD_RST_ID;
 uint16 TXE_SCHED_SENT_L_ID;
 uint16 TXE_SCHED_SENT_H_ID;
 uint16 TXE_SCHED_CTL_ID;
 uint16 TXE_MSCHED_USR_EN_ID;
 uint16 TXE_MSCHED_SYMB_CYCS_ID;
 uint16 TXE_SCHED_UFL_STS_ID;
 uint16 TXE_MSCHED_BURSTPH_TOTSZ_ID;
 uint16 TXE_MSCHED_BURSTPH_BURSTSZ_ID;
 uint16 TXE_MSCHED_NDPBS_L_ID;
 uint16 TXE_MSCHED_NDPBS_H_ID;
 uint16 TXE_MSCHED_STATE_ID;
 uint16 TXE_FRMINPROG_QB0_ID;
 uint16 TXE_FRMINPROG_QB1_ID;
 uint16 TXE_FRMINPROG_QB2_ID;
 uint16 TXE_FRMINPROG_QB3_ID;
 uint16 TXE_FRMINPROG_QB4_ID;
 uint16 TXE_XMT_DMABUSY_QB0_ID;
 uint16 TXE_XMT_DMABUSY_QB1_ID;
 uint16 TXE_XMT_DMABUSY_QB2_ID;
 uint16 TXE_XMT_DMABUSY_QB3_ID;
 uint16 TXE_XMT_DMABUSY_QB4_ID;
 uint16 TXE_BMC_FIFOFULL_QB0_ID;
 uint16 TXE_BMC_FIFOFULL_QB1_ID;
 uint16 TXE_BMC_FIFOFULL_QB2_ID;
 uint16 TXE_BMC_FIFOFULL_QB3_ID;
 uint16 TXE_BMC_FIFOFULL_QB4_ID;
 uint16 TXE_XMT_DPQSL_QB0_ID;
 uint16 TXE_XMT_DPQSL_QB1_ID;
 uint16 TXE_XMT_DPQSL_QB2_ID;
 uint16 TXE_XMT_DPQSL_QB3_ID;
 uint16 TXE_XMT_DPQSL_QB4_ID;
 uint16 TXE_AQM_BQSTATUS_ID;
 uint16 TXE_DBG_BMC_STATUS_ID;
 uint16 TXE_HDR_PDLIM_ID;
 uint16 TXE_MAC_PADBYTES_ID;
 uint16 TXE_MPDU_MINLEN_ID;
 uint16 TXE_AMP_EOFPD_L_ID;
 uint16 TXE_AMP_EOFPD_H_ID;
 uint16 TXE_AMP_EOFPADBYTES_ID;
 uint16 TXE_PSDULEN_L_ID;
 uint16 TXE_PSDULEN_H_ID;
 uint16 XMTDMA_CTL_ID;
 uint16 TXE_XMTDMA_PRELD_RANGE_ID;
 uint16 TXE_XMTDMA_ACT_RANGE_ID;
 uint16 TXE_XMTDMA_RUWT_QSEL_ID;
 uint16 TXE_XMTDMA_RUWT_DATA_ID;
 uint16 TXE_CTDMA_CTL_ID;
 uint16 TXE_XMTDMA_DBG_CTL_ID;
 uint16 XMTDMA_AQMIF_STS_ID;
 uint16 XMTDMA_PRELD_STS_ID;
 uint16 XMTDMA_ACT_STS_ID;
 uint16 XMTDMA_QUEUE_STS_ID;
 uint16 XMTDMA_ENGINE_STS_ID;
 uint16 XMTDMA_DMATX_STS_ID;
 uint16 XMTDMA_THRUPUTCTL_STS_ID;
 uint16 TXE_SCHED_MPMFC_STS_ID;
 uint16 TXE_SCHED_FORCE_BAD_ID;
 uint16 TXE_BMC_RDMGR_STATE_ID;
 uint16 TXE_SCHED_ERR_ID;
 uint16 TXE_BMC_PFF_CFG_ID;
 uint16 TXE_BMC_PFFSTART_DEF_ID;
 uint16 TXE_BMC_PFFSZ_DEF_ID;
 uint16 XmtDMABusyOtherQ0to15_ID;
 uint16 XmtDMABusyOtherQ16to31_ID;
 uint16 XmtDMABusyOtherQ32to47_ID;
 uint16 XmtDMABusyOtherQ48to63_ID;
 uint16 XmtDMABusyOtherQ64to69_ID;
 uint16 AQM_CFG_ID;
 uint16 AQM_FIFODEF0_ID;
 uint16 AQM_FIFODEF1_ID;
 uint16 AQM_TXCNTDEF0_ID;
 uint16 AQM_TXCNTDEF1_ID;
 uint16 AQM_INITREQ_ID;
 uint16 AQM_REGSEL_ID;
 uint16 AQM_QMAP_ID;
 uint16 AQM_MAXAGGRLEN_H_ID;
 uint16 AQM_MAXAGGRLEN_L_ID;
 uint16 AQM_MAXAGGRNUM_ID;
 uint16 AQM_MINMLEN_ID;
 uint16 AQM_MAXOSFRAG_ID;
 uint16 AQM_AGGPARM_ID;
 uint16 AQM_FRPRAM_ID;
 uint16 AQM_MINFRLEN_ID;
 uint16 AQM_MQBURST_ID;
 uint16 AQM_DMLEN_ID;
 uint16 AQM_MMLEN_ID;
 uint16 AQM_CMLEN_ID;
 uint16 AQM_VQENTRY0_ID;
 uint16 AQM_VQENTRY1_ID;
 uint16 AQM_VQADJ_ID;
 uint16 AQM_VQPADADJ_ID;
 uint16 AQM_AGGREN_ID;
 uint16 AQM_AGGREQ_ID;
 uint16 AQM_CAGGLEN_L_ID;
 uint16 AQM_CAGGLEN_H_ID;
 uint16 AQM_CAGGNUM_ID;
 uint16 AQM_QAGGSTATS_ID;
 uint16 AQM_QAGGNUM_ID;
 uint16 AQM_QAGGINFO_ID;
 uint16 AQM_AGGRPTR_ID;
 uint16 AQM_QAGGLEN_L_ID;
 uint16 AQM_QAGGLEN_H_ID;
 uint16 AQM_QMPDULEN_ID;
 uint16 AQM_QFRAGOS_ID;
 uint16 AQM_QTXCNT_ID;
 uint16 AQM_QMPDUINFO0_ID;
 uint16 AQM_QMPDUINFO1_ID;
 uint16 AQM_TXCNTEN_ID;
 uint16 AQM_TXCNTUPD_ID;
 uint16 AQM_QTXCNTRPTR_ID;
 uint16 AQM_QCURTXCNT_ID;
 uint16 AQM_BASEL_ID;
 uint16 AQM_RCVDBA0_ID;
 uint16 AQM_RCVDBA1_ID;
 uint16 AQM_RCVDBA2_ID;
 uint16 AQM_RCVDBA3_ID;
 uint16 AQM_BASSN_ID;
 uint16 AQM_REFSN_ID;
 uint16 AQM_QMAXBAIDX_ID;
 uint16 AQM_BAEN_ID;
 uint16 AQM_BAREQ_ID;
 uint16 AQM_UPDBARD_ID;
 uint16 AQM_UPDBA0_ID;
 uint16 AQM_UPDBA1_ID;
 uint16 AQM_UPDBA2_ID;
 uint16 AQM_UPDBA3_ID;
 uint16 AQM_ACKCNT_ID;
 uint16 AQM_CONSTYPE_ID;
 uint16 AQM_CONSEN_ID;
 uint16 AQM_CONSREQ_ID;
 uint16 AQM_CONSCNT_ID;
 uint16 AQM_XUPDEN_ID;
 uint16 AQM_XUPDREQ_ID;
 uint16 AQM_TXDMAEN_ID;
 uint16 AQM_TXDMAREQ_ID;
 uint16 AQMF_READY0_ID;
 uint16 AQMF_READY1_ID;
 uint16 AQMF_READY2_ID;
 uint16 AQMF_READY3_ID;
 uint16 AQMF_READY4_ID;
 uint16 AQMF_MTE0_ID;
 uint16 AQMF_MTE1_ID;
 uint16 AQMF_MTE2_ID;
 uint16 AQMF_MTE3_ID;
 uint16 AQMF_MTE4_ID;
 uint16 AQMF_PLREADY0_ID;
 uint16 AQMF_PLREADY1_ID;
 uint16 AQMF_PLREADY2_ID;
 uint16 AQMF_PLREADY3_ID;
 uint16 AQMF_PLREADY4_ID;
 uint16 AQMF_FULL0_ID;
 uint16 AQMF_FULL1_ID;
 uint16 AQMF_FULL2_ID;
 uint16 AQMF_FULL3_ID;
 uint16 AQMF_FULL4_ID;
 uint16 AQMF_STATUS_ID;
 uint16 MQF_EMPTY0_ID;
 uint16 MQF_EMPTY1_ID;
 uint16 MQF_EMPTY2_ID;
 uint16 MQF_EMPTY3_ID;
 uint16 MQF_EMPTY4_ID;
 uint16 MQF_STATUS_ID;
 uint16 TXQ_STATUS_ID;
 uint16 AQM_BQEN_ID;
 uint16 AQM_BQSUSP_ID;
 uint16 AQM_BQFLUSH_ID;
 uint16 AQMF_HSUSP0_ID;
 uint16 AQMF_HSUSP1_ID;
 uint16 AQMF_HSUSP2_ID;
 uint16 AQMF_HSUSP3_ID;
 uint16 AQMF_HSUSP4_ID;
 uint16 AQMF_HSUSP_PRGR0_ID;
 uint16 AQMF_HSUSP_PRGR1_ID;
 uint16 AQMF_HSUSP_PRGR2_ID;
 uint16 AQMF_HSUSP_PRGR3_ID;
 uint16 AQMF_HSUSP_PRGR4_ID;
 uint16 AQMF_HFLUSH0_ID;
 uint16 AQMF_HFLUSH1_ID;
 uint16 AQMF_HFLUSH2_ID;
 uint16 AQMF_HFLUSH3_ID;
 uint16 AQMF_HFLUSH4_ID;
 uint16 AQMTXD_READ_ID;
 uint16 AQMTXD_RDOFFSET_ID;
 uint16 AQMTXD_RDLEN_ID;
 uint16 AQMTXD_DSTADDR_ID;
 uint16 AQMTXD_AUTOPF_ID;
 uint16 AQMTXD_APFOFFSET_ID;
 uint16 AQMTXD_APFDSTADDR_ID;
 uint16 AQMTXD_PFREADY0_ID;
 uint16 AQMTXD_PFREADY1_ID;
 uint16 AQMTXD_PFREADY2_ID;
 uint16 AQMTXD_PFREADY3_ID;
 uint16 AQMTXD_PFREADY4_ID;
 uint16 AQMCT_BUSY0_ID;
 uint16 AQMCT_BUSY1_ID;
 uint16 AQMCT_BUSY2_ID;
 uint16 AQMCT_BUSY3_ID;
 uint16 AQMCT_BUSY4_ID;
 uint16 AQMCT_QSEL_ID;
 uint16 AQMCT_PRI_ID;
 uint16 AQMCT_PRIFIFO_ID;
 uint16 AQMCT_MAXREQNUM_ID;
 uint16 AQMCT_FREECNT_ID;
 uint16 AQMCT_CHDIS_ID;
 uint16 AQMPL_CFG_ID;
 uint16 AQMPL_QSEL_ID;
 uint16 AQMPL_THRESHOLD_ID;
 uint16 AQMPL_EPOCHMASK_ID;
 uint16 AQMPL_MAXMPDU_ID;
 uint16 AQMPL_DIS_ID;
 uint16 AQMPL_FORCE_ID;
 uint16 AQMTXPL_THRESH_ID;
 uint16 AQMTXPL_MASK_ID;
 uint16 AQMTXPL_RDY_ID;
 uint16 AQMCSB_REQ_ID;
 uint16 AQMF_RPTR_ID;
 uint16 AQMF_MPDULEN_ID;
 uint16 AQMF_SOFDPTR_ID;
 uint16 AQMF_SWDCNT_ID;
 uint16 AQMF_FLAG_ID;
 uint16 AQMF_TXDPTR_L_ID;
 uint16 AQMF_TXDPTR_ML_ID;
 uint16 AQMF_TXDPTR_MU_ID;
 uint16 AQMF_TXDPTR_H_ID;
 uint16 AQMF_SRTIDX_ID;
 uint16 AQMF_ENDIDX_ID;
 uint16 AQMF_NUMBUF_ID;
 uint16 AQMF_BUFLEN_ID;
 uint16 AQMFR_RWD0_ID;
 uint16 AQMFR_RWD1_ID;
 uint16 AQMFR_RWD2_ID;
 uint16 AQMFR_RWD3_ID;
 uint16 AQMCSB_RPTR_ID;
 uint16 AQMCSB_WPTR_ID;
 uint16 AQMCSB_BASEL_ID;
 uint16 AQMCSB_BA0_ID;
 uint16 AQMCSB_BA1_ID;
 uint16 AQMCSB_BA2_ID;
 uint16 AQMCSB_BA3_ID;
 uint16 AQMCSB_QAGGLEN_L_ID;
 uint16 AQMCSB_QAGGLEN_H_ID;
 uint16 AQMCSB_QAGGNUM_ID;
 uint16 AQMCSB_QAGGSTATS_ID;
 uint16 AQMCSB_QAGGINFO_ID;
 uint16 AQMCSB_CONSCNT_ID;
 uint16 AQMCSB_TOTCONSCNT_ID;
 uint16 AQMCSB_CFGSTRADDR_ID;
 uint16 AQMCSB_CFGENDADDR_ID;
 uint16 AQM_AGGERR_ID;
 uint16 AQM_QAGGERR_ID;
 uint16 AQM_BRSTATUS_ID;
 uint16 AQM_QBRSTATUS_ID;
 uint16 AQM_DBGCTL_ID;
 uint16 AQM_DBGDATA0_ID;
 uint16 AQM_DBGDATA1_ID;
 uint16 AQM_DBGDATA2_ID;
 uint16 AQM_DBGDATA3_ID;
 uint16 AQM_ERRSEL_ID;
 uint16 AQM_ERRSTS_ID;
 uint16 AQM_SWRESET_ID;
 uint16 TDC_CTL_ID;
 uint16 TDC_USRCFG_ID;
 uint16 TDC_RUNCMD_ID;
 uint16 TDC_RUNSTS_ID;
 uint16 TDC_STATUS_ID;
 uint16 TDC_NUSR_ID;
 uint16 TDC_ORIGBW_ID;
 uint16 TDC_FRMLEN_L_ID;
 uint16 TDC_FRMLEN_H_ID;
 uint16 TDC_USRINFO_0_ID;
 uint16 TDC_USRINFO_1_ID;
 uint16 TDC_USRAID_ID;
 uint16 TDC_PPET0_ID;
 uint16 TDC_PPET1_ID;
 uint16 TDC_MAX_LENEXP_ID;
 uint16 TDC_MAXDUR_ID;
 uint16 TDC_MINMDUR_ID;
 uint16 TDC_HDRDUR_ID;
 uint16 TDC_TRIG_PADDUR_ID;
 uint16 TDC_PSDULEN_L_ID;
 uint16 TDC_PSDULEN_H_ID;
 uint16 TDC_HDR_PDLIM_ID;
 uint16 TDC_MAC_PADLEN_ID;
 uint16 TDC_EOFPDLIM_L_ID;
 uint16 TDC_EOFPDLIM_H_ID;
 uint16 TDC_MAXPSDULEN_L_ID;
 uint16 TDC_MAXPSDULEN_H_ID;
 uint16 TDC_MPDU_MINLEN_ID;
 uint16 TDC_CCLEN_ID;
 uint16 TDC_RATE_L_ID;
 uint16 TDC_RATE_H_ID;
 uint16 TDC_NSYMINIT_L_ID;
 uint16 TDC_NSYMINIT_H_ID;
 uint16 TDC_NDBPS_L_ID;
 uint16 TDC_NDBPS_H_ID;
 uint16 TDC_NDBPS_S_ID;
 uint16 TDC_T_PRE_ID;
 uint16 TDC_TDATA_ID;
 uint16 TDC_TDATA_MIN_ID;
 uint16 TDC_TDATA_MAX_ID;
 uint16 TDC_TDATA_AVG_ID;
 uint16 TDC_TDATA_TOT_ID;
 uint16 TDC_HESIGB_NSYM_ID;
 uint16 TDC_AGGLEN_L_ID;
 uint16 TDC_AGGLEN_H_ID;
 uint16 TDC_TIME_IN_ID;
 uint16 TDC_INITREQ_ID;
 uint16 TDC_AGG0STS_ID;
 uint16 TDC_TRIGENDLOG_ID;
 uint16 TDC_RUSTS_ID;
 uint16 TDC_DAID_ID;
 uint16 TDC_FBWCTL_ID;
 uint16 TDC_PPCTL_ID;
 uint16 TDC_FBWSTS_ID;
 uint16 TDC_PPSTS_ID;
 uint16 TDC_CTL1_ID;
 uint16 TDC_PDBG_ID;
 uint16 TDC_PSDU_ACKSTS_ID;
 uint16 TDC_AGG_RDYSTS_ID;
 uint16 TDC_AGGLEN_OVR_L_ID;
 uint16 TDC_AGGLEN_OVR_H_ID;
 uint16 TDC_DROPUSR_ID;
 uint16 TDC_DURMARGIN_ID;
 uint16 TDC_DURTHRESH_ID;
 uint16 TDC_MAXTXOP_ID;
 uint16 TDC_RXPPET_ID;
 uint16 TDC_AGG0TXOP_ID;
} d11regs_regs_common_ge128_lt129_t;
typedef struct {
 uint16 SCTSTP_MASK_L_ID;
 uint16 SCTSTP_MASK_H_ID;
 uint16 SCTSTP_VAL_L_ID;
 uint16 SCTSTP_VAL_H_ID;
 uint16 SCTSTP_HMASK_L_ID;
 uint16 SCTSTP_HMASK_H_ID;
 uint16 SCTSTP_HVAL_L_ID;
 uint16 SCTSTP_HVAL_H_ID;
 uint16 SMP_CTRL2_ID;
 uint16 PSM_GpioMonMemPtr_ID;
 uint16 PSM_GpioMonMemData_ID;
 uint16 PSM_XMT_TPLDATA_L_ID;
 uint16 PSM_XMT_TPLDATA_H_ID;
 uint16 PSM_XMT_TPLPTR_ID;
 uint16 PSM_SMP_CTRL_ID;
 uint16 PSM_SCT_MASK_L_ID;
 uint16 PSM_SCT_MASK_H_ID;
 uint16 PSM_SCT_VAL_L_ID;
 uint16 PSM_SCT_VAL_H_ID;
 uint16 PSM_SCTSTP_MASK_L_ID;
 uint16 PSM_SCTSTP_MASK_H_ID;
 uint16 PSM_SCTSTP_VAL_L_ID;
 uint16 PSM_SCTSTP_VAL_H_ID;
 uint16 PSM_SCX_MASK_L_ID;
 uint16 PSM_SCX_MASK_H_ID;
 uint16 PSM_SCM_MASK_L_ID;
 uint16 PSM_SCM_MASK_H_ID;
 uint16 PSM_SCM_VAL_L_ID;
 uint16 PSM_SCM_VAL_H_ID;
 uint16 PSM_SCS_MASK_L_ID;
 uint16 PSM_SCS_MASK_H_ID;
 uint16 PSM_SCP_CURPTR_ID;
 uint16 PSM_SCP_STRTPTR_ID;
 uint16 PSM_SCP_STOPPTR_ID;
 uint16 PSM_SMP_CTRL2_ID;
 uint16 PSM_BRWK_4_ID;
 uint16 IFS_STAT_CRS_ID;
} d11regs_regs_common_ge129_t;
typedef struct {
 uint16 TXE_HTC_LOC_ID;
 uint16 TXE_HTC_L_ID;
 uint16 TXE_HTC_H_ID;
 uint16 BMCHWC_CNT_SEL_ID;
 uint16 BMCHWC_CNT_ID;
 uint16 BMCHWC_CTL_ID;
 uint16 BMC_FRM2MPM_PRGRSTS_ID;
 uint16 BMC_PSMCMD8_ID;
 uint16 TXE_MEND_SIG_ID;
 uint16 TXE_MENDLAST_STATUS_ID;
 uint16 TXE_BMC_PSMCMDQ_CTL_ID;
 uint16 TXE_BMC_ERRSTSEN_ID;
 uint16 TX_PREBM_FATAL_ERRVAL_ID;
 uint16 TX_PREBM_FATAL_ERRMASK_ID;
 uint16 SCS_TAIL_CNT_ID;
 uint16 TXE_BMC_ERRSTS_ID;
 uint16 XMTDMA_FATAL_ERR_RPTEN_ID;
 uint16 XMTDMA_FATAL_ERR_ID;
} d11regs_regs_common_ge130_t;
typedef struct {
 uint16 XMTDMA_PROGERR_ID;
} d11regs_regs_common_ge131_t;
typedef struct {
 uint16 ctdma_ctl_ID;
 uint16 MAC_BOOT_CTRL_ID;
 uint16 MacCapability2_ID;
 uint16 RxDmaXferCnt_ID;
 uint16 RxDmaXferCntMax_ID;
 uint16 RxFIFOCnt_ID;
 uint16 AMT_START_ID;
 uint16 AMT_END_ID;
 uint16 AMT_MATCH_A1_ID;
 uint16 AMT_MATCH_A2_ID;
 uint16 AMT_MATCH_A3_ID;
 uint16 AMT_MATCH_BSSID_ID;
 uint16 FP_PAT1A_ID;
 uint16 PSM_USR_SEL_ID;
 uint16 PSM_FATAL_STS_ID;
 uint16 PSM_FATAL_MASK_ID;
 uint16 HWA_MACIF_CTL1_ID;
 uint16 PSM_BASE_24_ID;
 uint16 PSM_BASE_25_ID;
 uint16 PSM_BASE_26_ID;
 uint16 PSM_BASE_27_ID;
 uint16 PSM_M2DMA_ADDR_ID;
 uint16 PSM_M2DMA_DATA_ID;
 uint16 PSM_M2DMA_FREE_ID;
 uint16 PSM_M2DMA_CFG_ID;
 uint16 PSM_M2DMA_DATA_L_ID;
 uint16 PSM_M2DMA_DATA_H_ID;
 uint16 TXE_SHMDMA_PHYSTSADDR_ID;
 uint16 TXE_SHMDMA_MACFIFOADDR_ID;
 uint16 PLCP_CC34_LEN_ID;
 uint16 TXE_XMTDMA_SW_RSTCTL_ID;
 uint16 XMTDMA_ACTUSR_DBGCTL_ID;
 uint16 XMTDMA_ACTUSR_VLDSTS_ID;
 uint16 XMTDMA_ACTUSR_QID_ID;
 uint16 XMTDMA_AQM_ACT_CNT_ID;
 uint16 AQM_MSDUDEF0_ID;
 uint16 AQM_MSDUDEF1_ID;
 uint16 AQM_PSMTXDSZ_ID;
 uint16 AQM_NDLIM_ID;
 uint16 AQM_MAXNDLIMLEN_ID;
 uint16 AQM_FRLENLIMIT_ID;
 uint16 AQM_FRFIXLEN_ID;
 uint16 AQM_2TDCCTL_ID;
 uint16 AQM_QFRTXCNT_ID;
 uint16 AQM_TOTACKCNT_ID;
 uint16 AQM_AGGR_PLDRDY_ID;
 uint16 AQM_AUTOBQC_EN_ID;
 uint16 AQM_AUTOBQC_ID;
 uint16 AQM_AUTOBQC_TO_ID;
 uint16 AQM_AUTOBQC_TOSTS_ID;
 uint16 AQM_AUTOBQC_TSCALE_ID;
 uint16 AQM_AUTOBQC_MAXTCNT_ID;
 uint16 AQMHWC_INPROG_ID;
 uint16 AQMHWC_STATUS_ID;
 uint16 AQM_DBGCTL1_ID;
 uint16 AQM_ERRSTSEN_ID;
 uint16 AQMFTM_CTL_ID;
 uint16 AQMFTM_MPDULEN_ID;
 uint16 AQMFTM_FLAG_ID;
 uint16 AQM_AUTOBQC_CONSDLY_ID;
 uint16 AQM_MUENGCTL_ID;
 uint16 TDC_NDLIM_ID;
 uint16 TDC_MAXNDLIMLEN_ID;
 uint16 TDC_CC34LEN_ID;
 uint16 TDC_NMURU_ID;
 uint16 TDC_MIMOSTS_ID;
 uint16 PSM_SCS_TAIL_CNT_ID;
 uint16 TXE_BMC_CMDPUSH_CNT_ID;
 uint16 TXE_BMC_CMDPOP_CNT_ID;
 uint16 TXE_RST_TXMEND_ID;
 uint16 TXE_BMC_BQSEL_ID;
 uint16 TXE_BMC_WBCNT0_ID;
 uint16 TXE_BMC_WBCNT1_ID;
 uint16 TXE_XMTDMA_CNTSTS_CTL_ID;
 uint16 XMTDMA_QREQBYTE_CNT0_ID;
 uint16 XMTDMA_QREQBYTE_CNT1_ID;
 uint16 XMTDMA_QREQ_MPDU_CNT_ID;
 uint16 XMTDMA_UREQBYTE_CNT0_ID;
 uint16 XMTDMA_UREQBYTE_CNT1_ID;
 uint16 XMTDMA_QRESPBYTE_CNT0_ID;
 uint16 XMTDMA_QRESPBYTE_CNT1_ID;
 uint16 XMTDMA_QRESP_MPDU_CNT_ID;
 uint16 XMTDMA_DATAPRI_STS0_ID;
 uint16 XMTDMA_DATAPRI_STS1_ID;
 uint16 TXE_MEND_CTL_ID;
 uint16 TXE_MENDLAST_SIG_ID;
 uint16 TXE_BMC_WFCNT_ID;
 uint16 TXE_SIGBCC3_BASE_ID;
 uint16 TXE_SIGBCC4_BASE_ID;
 uint16 TXE_BMC_EOFCNT_ID;
 uint16 TXE_PPR_CFG0_ID;
 uint16 TXE_PPR_CFG1_ID;
 uint16 TXE_PPR_CFG2_ID;
 uint16 TXE_PPR_CFG3_ID;
 uint16 TXE_PPR_CFG4_ID;
 uint16 TXE_PPR_CFG5_ID;
 uint16 TXE_PPR_CFG6_ID;
 uint16 TXE_PPR_CFG7_ID;
 uint16 TXE_PPR_CFG8_ID;
 uint16 TXE_PPR_CFG9_ID;
 uint16 TXE_PPR_CFG10_ID;
 uint16 TXE_PPR_CFG11_ID;
 uint16 TXE_PPR_CFG12_ID;
 uint16 TXE_PPR_CFG13_ID;
 uint16 TXE_PPR_CFG14_ID;
 uint16 TXE_PPR_CFG15_ID;
 uint16 TXE_PPR_CFG16_ID;
 uint16 TXE_PPR_CTL0_ID;
 uint16 TXE_PPR_CTL1_ID;
 uint16 TXE_PPR_CTL2_ID;
 uint16 TXE_PPR_STAT0_ID;
 uint16 TXE_PPR_STAT1_ID;
 uint16 TXE_PPR_STAT2_ID;
 uint16 TXE_PPR_STAT3_ID;
 uint16 RXE_IFEVENTDBG_CTL_ID;
 uint16 RXE_IFEVENTDBG_STAT_ID;
 uint16 RXE_MEND_FLAT_ID;
 uint16 RXE_XFERACT_FLAT_ID;
 uint16 RXE_PHYFIFO_NOT_EMPTY_FLAT_ID;
 uint16 RXE_PFPLCP_WRDCNT_ID;
 uint16 RXE_RXDATA_NOT_EMPTY_FLAT_ID;
 uint16 RXE_PHYSTS_SHM_CTL_ID;
 uint16 RXE_RXBM_FATAL_ERRVAL_ID;
 uint16 RXE_RXBM_FATAL_ERRMASK_ID;
 uint16 RXE_PREBM_FATAL_ERRVAL_ID;
 uint16 RXE_PREBM_FATAL_ERRMASK_ID;
 uint16 RXE_CTXSTSFIFO_FATAL_ERRVAL_ID;
 uint16 RXE_CTXSTSFIFO_FATAL_ERRMASK_ID;
 uint16 RXE_POSTBM_FATAL_ERRVAL_ID;
 uint16 RXE_POSTBM_FATAL_ERRMASK_ID;
 uint16 RXQ_FATAL_ERR_RPTEN_ID;
 uint16 RXQ_FATAL_ERR_ID;
 uint16 RXE_RXFRMUP_TSF_L_ID;
 uint16 RXE_RXFRMUP_TSF_H_ID;
 uint16 RXE_RXFRMDN_TSF_L_ID;
 uint16 RXE_RXFRMDN_TSF_H_ID;
 uint16 RXE_RSF_NP_STATS_ID;
 uint16 RXE_RSF_HP_STATS_ID;
 uint16 RXE_POSTBM_TIMEOUT_ID;
 uint16 RDF_CTL8_ID;
 uint16 FP_DEBUG_ID;
 uint16 RXE_RXE2WEP_BYTES_ID;
 uint16 RXE_WEP2FP_BYTES_ID;
 uint16 RXE_RXE2BM_BYTES_ID;
 uint16 RXE_WCS_HDR_THRESHOLD_ID;
 uint16 RXE_WCS_MIN_THRESHOLD_ID;
 uint16 RXE_WCS_MAX_THRESHOLD_ID;
 uint16 RXE_DAGG_DEBUG_ID;
 uint16 RXE_BFDRPT_XFERPEND_ID;
 uint16 RXE_BFDRPT_XFERRD_ID;
 uint16 RXE_BFDRPT_XFERSTS_ID;
 uint16 RXE_BFDRPT_CAPSTS_ID;
 uint16 RXE_WCS_TOUT_THRESHOLD_ID;
 uint16 RXE_WCS_TOUT_STATUS_ID;
 uint16 RXE_WCS_CTL_ID;
 uint16 RXE_BMCLOOPBACK_DISCARD_ID;
 uint16 RXE_STRMRD_THRESHOLD_ID;
 uint16 RXE_RCM_XFERBMP_ID;
 uint16 RXE_WCS_COUNTERS_ID;
 uint16 RXE_WCS_DEBUG_ID;
 uint16 FP_STATUS6_ID;
 uint16 FP_ACKTYPETID_BITMAP_ID;
 uint16 RXE_PHYSTS_DEBUG1_ID;
 uint16 RXE_PHYSTS_DEBUG2_ID;
 uint16 RXE_PHYSTS_ADDR_ID;
 uint16 RXE_PHYSTS_DATA_ID;
 uint16 RXE_PHYSTS_FREE_ID;
 uint16 RXE_PHYSTS_CFG_ID;
 uint16 RXE_PHYSTS_DATA_L_ID;
 uint16 RXE_PHYSTS_DATA_H_ID;
 uint16 CBR_DBG_DATA_ID;
 uint16 PHYFIFO_SIZE_CFG_ID;
 uint16 PHYFIFO_SIZE_CFG1_ID;
 uint16 FP_MUBAR_CONFIG_ID;
 uint16 FP_MUBAR_TYPE_0_ID;
 uint16 FP_MUBAR_TYPE_1_ID;
 uint16 FP_MUBAR_TYPE_2_ID;
 uint16 FP_MUBAR_TYPE_3_ID;
 uint16 RXE_EOFPD_CNT_ID;
 uint16 RXE_EOFPD_THRSH_ID;
 uint16 RXE_AGGLEN_EST_ID;
 uint16 RXE_PFWRDCNT_CTL_ID;
 uint16 RXE_OPMODE_CFG_ID;
 uint16 RXE_DBG_CTL_ID;
 uint16 POSTBM_DBG_STS_ID;
 uint16 PREBM_DBG_STS_ID;
 uint16 RXE_RCVCTL2_ID;
 uint16 PSM_CHK0_EADDR_ID;
 uint16 PSM_CHK1_EADDR_ID;
} d11regs_regs_common_ge132_t;
typedef struct d11regdefs_struct {
 const d11regs_regs_common_lt40_t *regs_common_lt40;
 const d11regs_regs_common_ge40_lt64_t *regs_common_ge40_lt64;
 const d11regs_regs_common_ge64_lt80_t *regs_common_ge64_lt80;
 const d11regs_regs_common_ge80_lt128_t *regs_common_ge80_lt128;
 const d11regs_regs_common_ge128_lt129_t *regs_common_ge128_lt129;
 const d11regs_regs_common_ge129_t *regs_common_ge129;
 const d11regs_regs_common_ge130_t *regs_common_ge130;
 const d11regs_regs_common_ge131_t *regs_common_ge131;
 const d11regs_regs_common_ge132_t *regs_common_ge132;
} d11regdefs_t;
static const d11regs_regs_common_lt40_t regs_common_lt40_132 = {
 .biststatus_ID = (0x0000000c),
 .biststatus2_ID = INVALID,
 .gptimer_ID = (0x00000018),
 .usectimer_ID = (0x0000001C),
 .intstat0_ID = (0x00000020),
 .alt_intmask0_ID = (0x00000060),
 .inddma_ID = (0x00000080),
 .indaqm_ID = (0x000000A0),
 .indqsel_ID = INVALID,
 .suspreq_ID = INVALID,
 .flushreq_ID = INVALID,
 .chnflushstatus_ID = INVALID,
 .intrcvlzy0_ID = (0x00000100),
 .MACCONTROL_ID = (0x00000120),
 .MACCOMMAND_ID = (0x00000124),
 .MACINTSTATUS_ID = (0x00000128),
 .MACINTMASK_ID = (0x0000012C),
 .XMT_TEMPLATE_RW_PTR_ID = (0x00000130),
 .XMT_TEMPLATE_RW_DATA_ID = (0x00000134),
 .PMQHOSTDATA_ID = (0x00000140),
 .PMQPATL_ID = (0x00000144),
 .PMQPATH_ID = (0x00000148),
 .CHNSTATUS_ID = (0x00000150),
 .PSM_DEBUG_ID = (0x00000154),
 .PHY_DEBUG_ID = (0x00000158),
 .MacCapability_ID = (0x0000015C),
 .objaddr_ID = (0x00000160),
 .objdata_ID = (0x00000164),
 .ALT_MACINTSTATUS_ID = (0x00000168),
 .ALT_MACINTMASK_ID = (0x0000016C),
 .FrmTxStatus_ID = (0x00000170),
 .FrmTxStatus2_ID = (0x00000174),
 .FrmTxStatus3_ID = (0x00000178),
 .FrmTxStatus4_ID = (0x0000017C),
 .TSFTimerLow_ID = (0x00000180),
 .TSFTimerHigh_ID = (0x00000184),
 .CFPRep_ID = (0x00000188),
 .CFPStart_ID = (0x0000018C),
 .CFPMaxDur_ID = (0x00000190),
 .AvbRxTimeStamp_ID = (0x00000194),
 .AvbTxTimeStamp_ID = (0x00000198),
 .MacControl1_ID = (0x000001A0),
 .MacHWCap1_ID = (0x000001A4),
 .MacPatchCtrl_ID = INVALID,
 .gptimer_x_ID = INVALID,
 .maccontrol_x_ID = INVALID,
 .maccontrol1_x_ID = INVALID,
 .maccommand_x_ID = INVALID,
 .macintstatus_x_ID = INVALID,
 .macintmask_x_ID = INVALID,
 .altmacintstatus_x_ID = INVALID,
 .altmacintmask_x_ID = INVALID,
 .psmdebug_x_ID = INVALID,
 .phydebug_x_ID = INVALID,
 .statctr2_ID = INVALID,
 .statctr3_ID = INVALID,
 .ClockCtlStatus_ID = (0x000001E0),
 .Workaround_ID = (0x000001E4),
 .POWERCTL_ID = (0x000001E8),
 .xmt0ctl_ID = (0x00000200),
 .fifobase_ID = (0x00000380),
 .aggfifocnt_ID = INVALID,
 .aggfifodata_ID = INVALID,
 .DebugStoreMask_ID = (0x00000398),
 .DebugTriggerMask_ID = (0x0000039C),
 .DebugTriggerValue_ID = (0x000003A0),
 .radioregaddr_cross_ID = (0x000003C8),
 .radioregdata_cross_ID = (0x000003CA),
 .radioregaddr_ID = (0x000003D8),
 .radioregdata_ID = (0x000003DA),
 .rfdisabledly_ID = (0x000003DC),
 .PHY_REG_0_ID = (0x000003E0),
 .PHY_REG_1_ID = (0x000003E2),
 .PHY_REG_2_ID = (0x000003E4),
 .PHY_REG_3_ID = (0x000003E6),
 .PHY_REG_4_ID = (0x000003E8),
 .PHY_REG_5_ID = (0x000003EA),
 .PHY_REG_7_ID = (0x000003EC),
 .PHY_REG_6_ID = (0x000003EE),
 .PHY_REG_8_ID = (0x000003F0),
 .PHY_REG_A_ID = (0x000003F4),
 .PHY_REG_B_ID = (0x000003F6),
 .PHY_REG_C_ID = (0x000003F8),
 .PHY_REG_D_ID = (0x000003FA),
 .PHY_REG_ADDR_ID = (0x000003FC),
 .PHY_REG_DATA_ID = (0x000003FE),
 .RCV_FIFO_CTL_ID = (0x0000040A),
 .RCV_CTL_ID = (0x00000402),
 .RCV_FRM_CNT_ID = (0x0000044E),
 .RCV_STATUS_LEN_ID = (0x00000438),
 .RXE_PHYRS_1_ID = (0x000005A6),
 .RXE_RXCNT_ID = (0x0000040E),
 .RXE_STATUS1_ID = (0x00000422),
 .RXE_STATUS2_ID = (0x00000424),
 .RXE_PLCP_LEN_ID = (0x00000420),
 .rcm_ctl_ID = INVALID,
 .rcm_mat_data_ID = INVALID,
 .rcm_mat_mask_ID = INVALID,
 .rcm_mat_dly_ID = INVALID,
 .rcm_cond_mask_l_ID = INVALID,
 .rcm_cond_mask_h_ID = INVALID,
 .rcm_cond_dly_ID = INVALID,
 .EXT_IHR_ADDR_ID = (0x00000580),
 .EXT_IHR_DATA_ID = (0x00000582),
 .RXE_PHYRS_2_ID = (0x000005A8),
 .RXE_PHYRS_3_ID = (0x000005AA),
 .PHY_MODE_ID = INVALID,
 .rcmta_ctl_ID = INVALID,
 .rcmta_size_ID = INVALID,
 .rcmta_addr0_ID = INVALID,
 .rcmta_addr1_ID = INVALID,
 .rcmta_addr2_ID = INVALID,
 .ext_ihr_status_ID = INVALID,
 .radio_ihr_addr_ID = INVALID,
 .radio_ihr_data_ID = INVALID,
 .radio_ihr_status_ID = INVALID,
 .PSM_MAC_CTLH_ID = (0x00000782),
 .PSM_MAC_INTSTAT_L_ID = (0x00000784),
 .PSM_MAC_INTSTAT_H_ID = (0x00000786),
 .PSM_MAC_INTMASK_L_ID = (0x00000788),
 .PSM_MAC_INTMASK_H_ID = (0x0000078A),
 .PSM_MACCOMMAND_ID = (0x0000078C),
 .PSM_BRC_0_ID = (0x00000830),
 .PSM_PHY_CTL_ID = (0x000007C0),
 .PSM_IHR_ERR_ID = (0x000007C6),
 .DMP_OOB_AIN_MASK_ID = INVALID,
 .psm_int_sel_2_ID = INVALID,
 .PSM_GPIOIN_ID = (0x00000880),
 .PSM_GPIOOUT_ID = (0x00000882),
 .PSM_GPIOEN_ID = (0x00000884),
 .PSM_BRED_0_ID = (0x00000800),
 .PSM_BRED_1_ID = (0x00000802),
 .PSM_BRED_2_ID = (0x00000804),
 .PSM_BRED_3_ID = (0x00000806),
 .PSM_BRCL_0_ID = (0x00000808),
 .PSM_BRCL_1_ID = (0x0000080A),
 .PSM_BRCL_2_ID = (0x0000080C),
 .PSM_BRCL_3_ID = (0x0000080E),
 .PSM_BRPO_0_ID = (0x00000810),
 .PSM_BRPO_1_ID = (0x00000812),
 .PSM_BRPO_2_ID = (0x00000814),
 .PSM_BRPO_3_ID = (0x00000816),
 .PSM_BRWK_0_ID = (0x00000818),
 .PSM_BRWK_1_ID = (0x0000081A),
 .PSM_BRWK_2_ID = (0x0000081C),
 .PSM_BRWK_3_ID = (0x0000081E),
 .PSM_INTSEL_0_ID = (0x00000820),
 .PSMPowerReqStatus_ID = INVALID,
 .psm_ihr_err_ID = INVALID,
 .SubrStkStatus_ID = (0x000007D2),
 .SubrStkRdPtr_ID = (0x000007D4),
 .SubrStkRdData_ID = (0x000007D6),
 .psm_pc_reg_3_ID = INVALID,
 .PSM_BRC_1_ID = (0x00000832),
 .SBRegAddr_ID = INVALID,
 .SBRegDataL_ID = (0x000007E4),
 .SBRegDataH_ID = (0x000007E6),
 .PSMCoreCtlStat_ID = (0x000007E8),
 .SbAddrLL_ID = (0x000007EC),
 .SbAddrL_ID = (0x000007EE),
 .SbAddrH_ID = (0x000007F0),
 .SbAddrHH_ID = (0x000007F2),
 .TXE_CTL_ID = (0x00000900),
 .TXE_AUX_ID = (0x00000906),
 .TXE_TS_LOC_ID = (0x00000908),
 .TXE_TIME_OUT_ID = (0x0000090A),
 .TXE_WM_0_ID = (0x0000090C),
 .TXE_WM_1_ID = (0x0000090E),
 .TXE_PHYCTL_ID = (0x00000A42),
 .TXE_STATUS_ID = (0x0000092C),
 .TXE_MMPLCP0_ID = INVALID,
 .TXE_MMPLCP1_ID = INVALID,
 .TXE_PHYCTL1_ID = INVALID,
 .TXE_PHYCTL2_ID = INVALID,
 .xmtfifodef_ID = INVALID,
 .XmtFifoFrameCnt_ID = INVALID,
 .xmtfifo_byte_cnt_ID = INVALID,
 .xmtfifo_head_ID = INVALID,
 .xmtfifo_rd_ptr_ID = INVALID,
 .xmtfifo_wr_ptr_ID = INVALID,
 .xmtfifodef1_ID = INVALID,
 .aggfifo_cmd_ID = INVALID,
 .aggfifo_stat_ID = INVALID,
 .aggfifo_cfgctl_ID = INVALID,
 .aggfifo_cfgdata_ID = INVALID,
 .aggfifo_mpdunum_ID = INVALID,
 .aggfifo_len_ID = INVALID,
 .aggfifo_bmp_ID = INVALID,
 .aggfifo_ackedcnt_ID = INVALID,
 .aggfifo_sel_ID = INVALID,
 .xmtfifocmd_ID = INVALID,
 .xmtfifoflush_ID = INVALID,
 .xmtfifothresh_ID = INVALID,
 .xmtfifordy_ID = INVALID,
 .xmtfifoprirdy_ID = INVALID,
 .xmtfiforqpri_ID = INVALID,
 .xmttplatetxptr_ID = INVALID,
 .xmttplateptr_ID = INVALID,
 .SampleCollectStartPtr_ID = INVALID,
 .SampleCollectStopPtr_ID = INVALID,
 .SampleCollectCurPtr_ID = INVALID,
 .aggfifo_data_ID = INVALID,
 .XmtTemplateDataLo_ID = (0x00000AE0),
 .XmtTemplateDataHi_ID = (0x00000AE2),
 .xmtsel_ID = INVALID,
 .xmttxcnt_ID = INVALID,
 .xmttxshmaddr_ID = INVALID,
 .xmt_ampdu_ctl_ID = INVALID,
 .TSF_CFP_STRT_L_ID = (0x00000704),
 .TSF_CFP_STRT_H_ID = (0x00000706),
 .TSF_CFP_PRE_TBTT_ID = (0x00000712),
 .TSF_CLK_FRAC_L_ID = (0x0000072E),
 .TSF_CLK_FRAC_H_ID = (0x00000730),
 .TSF_RANDOM_ID = (0x00000746),
 .TSF_GPT_2_STAT_ID = (0x00000774),
 .TSF_GPT_2_CTR_L_ID = (0x00000776),
 .TSF_GPT_2_CTR_H_ID = (0x00000778),
 .TSF_GPT_2_VAL_L_ID = (0x0000077A),
 .TSF_GPT_2_VAL_H_ID = (0x0000077C),
 .TSF_GPT_ALL_STAT_ID = (0x0000077E),
 .IFS_SIFS_RX_TX_TX_ID = (0x00000680),
 .IFS_SIFS_NAV_TX_ID = (0x00000682),
 .IFS_SLOT_ID = (0x00000684),
 .IFS_CTL_ID = (0x00000688),
 .IFS_BOFF_CTR_ID = (0x0000068A),
 .IFS_STAT_ID = (0x00000690),
 .IFS_MEDBUSY_CTR_ID = (0x00000692),
 .IFS_TX_DUR_ID = (0x000006C0),
 .IFS_STAT1_ID = (0x00000698),
 .IFS_EDCAPRI_ID = (0x0000069A),
 .IFS_AIFSN_ID = (0x0000069C),
 .IFS_CTL1_ID = (0x0000069E),
 .SLOW_CTL_ID = (0x000006CA),
 .SLOW_TIMER_L_ID = (0x000006CC),
 .SLOW_TIMER_H_ID = (0x000006CE),
 .SLOW_FRAC_ID = (0x000006D0),
 .FAST_PWRUP_DLY_ID = (0x000006C8),
 .SLOW_PER_ID = (0x000006D2),
 .SLOW_PER_FRAC_ID = (0x000006D4),
 .SLOW_CALTIMER_L_ID = (0x000006D6),
 .SLOW_CALTIMER_H_ID = (0x000006D8),
 .BTCX_CTL_ID = (0x00000600),
 .BTCX_STAT_ID = (0x00000602),
 .BTCX_TRANSCTL_ID = (0x00000604),
 .BTCXPriorityWin_ID = INVALID,
 .BTCXConfTimer_ID = INVALID,
 .BTCXPriSelTimer_ID = INVALID,
 .BTCXPrvRfActTimer_ID = INVALID,
 .BTCXCurRfActTimer_ID = INVALID,
 .BTCXActDurTimer_ID = INVALID,
 .IFS_CTL_SEL_PRICRS_ID = (0x000006A6),
 .IfsCtlSelSecCrs_ID = INVALID,
 .BTCXEciAddr_ID = INVALID,
 .BTCXEciData_ID = INVALID,
 .BTCXEciMaskAddr_ID = INVALID,
 .BTCXEciMaskData_ID = INVALID,
 .CoexIOMask_ID = INVALID,
 .btcx_eci_event_addr_ID = INVALID,
 .btcx_eci_event_data_ID = INVALID,
 .NAV_CTL_ID = (0x00000640),
 .NAV_STAT_ID = (0x00000642),
 .WEP_CTL_ID = (0x000008A0),
 .WEP_STAT_ID = (0x000008A2),
 .WEP_HDRLOC_ID = (0x000008A4),
 .WEP_PSDULEN_ID = (0x000008A6),
 .pcmctl_ID = INVALID,
 .pcmstat_ID = INVALID,
 .PMQ_CTL_ID = (0x000008C0),
 .PMQ_STATUS_ID = (0x000008C8),
 .PMQ_PAT_0_ID = (0x000008CA),
 .PMQ_PAT_1_ID = (0x000008CC),
 .PMQ_PAT_2_ID = (0x000008CE),
 .PMQ_DAT_ID = (0x000008D0),
 .PMQ_DAT_OR_MAT_ID = (0x000008D2),
 .PMQ_DAT_OR_ALL_ID = (0x000008D4),
 .CLK_GATE_REQ0_ID = INVALID,
 .CLK_GATE_REQ1_ID = INVALID,
 .CLK_GATE_REQ2_ID = INVALID,
 .CLK_GATE_UCODE_REQ0_ID = INVALID,
 .CLK_GATE_UCODE_REQ1_ID = INVALID,
 .CLK_GATE_UCODE_REQ2_ID = INVALID,
 .CLK_GATE_STRETCH0_ID = INVALID,
 .CLK_GATE_STRETCH1_ID = INVALID,
 .CLK_GATE_MISC_ID = INVALID,
 .CLK_GATE_DIV_CTRL_ID = INVALID,
 .CLK_GATE_PHY_CLK_CTRL_ID = INVALID,
 .CLK_GATE_STS_ID = INVALID,
 .CLK_GATE_EXT_REQ0_ID = INVALID,
 .CLK_GATE_EXT_REQ1_ID = INVALID,
 .CLK_GATE_EXT_REQ2_ID = INVALID,
 .CLK_GATE_UCODE_PHY_CLK_CTRL_ID = INVALID,
};
static const d11regs_regs_common_ge40_lt64_t regs_common_ge40_lt64_132 = {
 .RCV_FRM_CNT_Q0_ID = (0x00000450),
 .RCV_FRM_CNT_Q1_ID = (0x00000452),
 .RCV_WRD_CNT_Q0_ID = (0x00000442),
 .RCV_WRD_CNT_Q1_ID = (0x00000444),
 .RCV_BM_STARTPTR_Q0_ID = INVALID,
 .RCV_BM_ENDPTR_Q0_ID = (0x0000047A),
 .RCV_BM_STARTPTR_Q1_ID = (0x0000047C),
 .RCV_BM_ENDPTR_Q1_ID = (0x0000047E),
 .RCV_COPYCNT_Q1_ID = (0x0000041E),
 .rxe_errval_ID = INVALID,
 .rxe_status3_ID = INVALID,
 .XmtFIFOFullThreshold_ID = (0x000009E0),
 .BMCReadReq_ID = (0x00000B00),
 .BMCReadOffset_ID = (0x00000B04),
 .BMCReadLength_ID = (0x00000B06),
 .BMCReadStatus_ID = (0x00000B08),
 .XmtShmAddr_ID = INVALID,
 .PsmMSDUAccess_ID = (0x000009C0),
 .MSDUEntryBufCnt_ID = (0x000009C2),
 .MSDUEntryStartIdx_ID = (0x000009C4),
 .MSDUEntryEndIdx_ID = (0x000009C6),
 .SampleCollectPlayPtrHigh_ID = INVALID,
 .SampleCollectCurPtrHigh_ID = INVALID,
 .BMCCmd1_ID = (0x000009C8),
 .BMCDynAllocStatus_ID = (0x000009CA),
 .BMCCTL_ID = (0x000009CC),
 .BMCConfig_ID = INVALID,
 .BMCStartAddr_ID = (0x000009D0),
 .BMCSize_ID = INVALID,
 .BMCCmd_ID = (0x000009D2),
 .BMCMaxBuffers_ID = (0x000009D4),
 .BMCMinBuffers_ID = (0x000009D6),
 .BMCAllocCtl_ID = (0x000009D8),
 .BMCDescrLen_ID = (0x000009DA),
 .SaveRestoreStartPtr_ID = INVALID,
 .SamplePlayStartPtr_ID = INVALID,
 .SamplePlayStopPtr_ID = INVALID,
 .XmtDMABusy_ID = INVALID,
 .XmtTemplatePtr_ID = (0x00000AE4),
 .XmtSuspFlush_ID = INVALID,
 .XmtFifoRqPrio_ID = (0x000009F0),
 .BMCStatCtl_ID = (0x00000B9E),
 .BMCStatData_ID = (0x00000BA0),
 .BMCMSDUFifoStat_ID = INVALID,
 .TXE_STATUS1_ID = (0x00000930),
 .pmqdataor_mat1_ID = INVALID,
 .pmqdataor_mat2_ID = INVALID,
 .pmqdataor_mat3_ID = INVALID,
 .pmq_auxsts_ID = INVALID,
 .pmq_ctl1_ID = INVALID,
 .pmq_status1_ID = INVALID,
 .pmq_addthr_ID = INVALID,
 .AQMConfig_ID = INVALID,
 .AQMFifoDef_ID = INVALID,
 .AQMMaxIdx_ID = INVALID,
 .AQMRcvdBA0_ID = INVALID,
 .AQMRcvdBA1_ID = INVALID,
 .AQMRcvdBA2_ID = INVALID,
 .AQMRcvdBA3_ID = INVALID,
 .AQMBaSSN_ID = INVALID,
 .AQMRefSN_ID = INVALID,
 .AQMMaxAggLenLow_ID = INVALID,
 .AQMMaxAggLenHi_ID = INVALID,
 .AQMAggParams_ID = INVALID,
 .AQMMinMpduLen_ID = INVALID,
 .AQMMacAdjLen_ID = INVALID,
 .DebugBusCtrl_ID = (0x00000BB2),
 .AQMAggStats_ID = INVALID,
 .AQMAggLenLow_ID = INVALID,
 .AQMAggLenHi_ID = INVALID,
 .AQMIdx_ID = INVALID,
 .AQMMpduLen_ID = INVALID,
 .AQMTxCnt_ID = INVALID,
 .AQMUpdBA0_ID = INVALID,
 .AQMUpdBA1_ID = INVALID,
 .AQMUpdBA2_ID = INVALID,
 .AQMUpdBA3_ID = INVALID,
 .AQMAckCnt_ID = INVALID,
 .AQMConsCnt_ID = INVALID,
 .AQMFifoReady_ID = INVALID,
 .AQMStartLoc_ID = INVALID,
 .AQMAggRptr_ID = INVALID,
 .AQMTxcntRptr_ID = INVALID,
 .TDCCTL_ID = INVALID,
 .TDC_Plcp0_ID = (0x00000E0C),
 .TDC_Plcp1_ID = (0x00000E0E),
 .TDC_FrmLen0_ID = INVALID,
 .TDC_FrmLen1_ID = INVALID,
 .TDC_Txtime_ID = (0x00000E30),
 .TDC_VhtSigB0_ID = INVALID,
 .TDC_VhtSigB1_ID = INVALID,
 .TDC_LSigLen_ID = INVALID,
 .TDC_NSym0_ID = (0x00000E34),
 .TDC_NSym1_ID = (0x00000E36),
 .TDC_VhtPsduLen0_ID = INVALID,
 .TDC_VhtPsduLen1_ID = INVALID,
 .TDC_VhtMacPad_ID = INVALID,
 .AQMCurTxcnt_ID = INVALID,
 .ShmDma_Ctl_ID = (0x00000960),
 .ShmDma_TxdcAddr_ID = (0x00000962),
 .ShmDma_ShmAddr_ID = (0x00000964),
 .ShmDma_XferCnt_ID = INVALID,
 .Txdc_Addr_ID = INVALID,
 .Txdc_Data_ID = INVALID,
 .MHP_Status_ID = (0x00000480),
 .MHP_FC_ID = (0x00000482),
 .MHP_DUR_ID = (0x00000484),
 .MHP_SC_ID = (0x00000486),
 .MHP_QOS_ID = (0x00000488),
 .MHP_HTC_H_ID = (0x0000048C),
 .MHP_HTC_L_ID = (0x0000048A),
 .MHP_Addr1_H_ID = (0x00000492),
 .MHP_Addr1_M_ID = (0x00000490),
 .MHP_Addr1_L_ID = (0x0000048E),
 .MHP_Addr2_H_ID = (0x00000498),
 .MHP_Addr2_M_ID = (0x00000496),
 .MHP_Addr2_L_ID = (0x00000494),
 .MHP_Addr3_H_ID = (0x0000049E),
 .MHP_Addr3_M_ID = (0x0000049C),
 .MHP_Addr3_L_ID = (0x0000049A),
 .MHP_Addr4_H_ID = (0x000004A4),
 .MHP_Addr4_M_ID = (0x000004A2),
 .MHP_Addr4_L_ID = (0x000004A0),
 .MHP_CFC_ID = (0x000004A6),
 .DAGG_CTL2_ID = (0x000005F0),
 .DAGG_BYTESLEFT_ID = (0x000005F2),
 .DAGG_SH_OFFSET_ID = (0x000005F4),
 .DAGG_STAT_ID = (0x000005F6),
 .DAGG_LEN_ID = (0x000005F8),
 .TXBA_CTL_ID = (0x00000540),
 .TXBA_DataSel_ID = (0x00000544),
 .TXBA_Data_ID = (0x00000546),
 .DAGG_LEN_THR_ID = (0x0000041C),
 .AMT_CTL_ID = (0x000004E0),
 .AMT_Status_ID = (0x000004E2),
 .AMT_Limit_ID = INVALID,
 .AMT_Attr_ID = (0x000004E8),
 .AMT_Match1_ID = INVALID,
 .AMT_Match2_ID = INVALID,
 .AMT_Table_Addr_ID = (0x000004F4),
 .AMT_Table_Data_ID = (0x000004F6),
 .AMT_Table_Val_ID = (0x000004F8),
 .AMT_DBG_SEL_ID = (0x00000502),
 .RoeCtrl_ID = (0x0000046C),
 .RoeStatus_ID = (0x0000046E),
 .RoeIPChkSum_ID = (0x00000470),
 .RoeTCPUDPChkSum_ID = (0x00000472),
 .PSOCtl_ID = INVALID,
 .PSORxWordsWatermark_ID = INVALID,
 .PSORxCntWatermark_ID = INVALID,
 .OBFFCtl_ID = (0x00000460),
 .OBFFRxWordsWatermark_ID = (0x00000462),
 .OBFFRxCntWatermark_ID = (0x00000464),
 .RcvHdrConvCtrlSts_ID = (0x0000041A),
 .RcvHdrConvSts_ID = (0x0000044A),
 .RcvHdrConvSts1_ID = (0x0000044C),
 .RCVLB_DAGG_CTL_ID = (0x000005FA),
 .RcvFifo0Len_ID = (0x00000446),
 .RcvFifo1Len_ID = (0x00000448),
 .ToECTL_ID = (0x00000942),
 .ToERst_ID = (0x00000944),
 .ToECSumNZ_ID = (0x00000946),
 .TxSerialCtl_ID = INVALID,
 .TxPlcpLSig0_ID = INVALID,
 .TxPlcpLSig1_ID = INVALID,
 .TxPlcpHtSig0_ID = INVALID,
 .TxPlcpHtSig1_ID = INVALID,
 .TxPlcpHtSig2_ID = INVALID,
 .TxPlcpVhtSigB0_ID = INVALID,
 .TxPlcpVhtSigB1_ID = INVALID,
 .MacHdrFromShmLen_ID = (0x00000912),
 .TxPlcpLen_ID = INVALID,
 .TxBFRptLen_ID = (0x00000A82),
 .BytCntInTxFrmLo_ID = (0x00000A7C),
 .BytCntInTxFrmHi_ID = (0x00000A7E),
 .TXBFCtl_ID = (0x00000A80),
 .BfmRptOffset_ID = INVALID,
 .BfmRptLen_ID = INVALID,
 .TXBFBfeRptRdCnt_ID = (0x00000A84),
 .PsmMboxOutSts_ID = INVALID,
 .PSM_BASE_0_ID = (0x00000840),
 .psm_base_x_ID = INVALID,
 .RXMapFifoSize_ID = INVALID,
 .RXMapStatus_ID = INVALID,
 .MsduThreshold_ID = (0x00000CD6),
 .BMCCore0TXAllMaxBuffers_ID = (0x000009E6),
 .BMCCore1TXAllMaxBuffers_ID = INVALID,
 .BMCDynAllocStatus1_ID = INVALID,
 .DMAMaxOutStBuffers_ID = INVALID,
 .SampleCollectStoreMaskLo_ID = INVALID,
 .SampleCollectStoreMaskHi_ID = INVALID,
 .SampleCollectMatchMaskLo_ID = INVALID,
 .SampleCollectMatchMaskHi_ID = INVALID,
 .SampleCollectMatchValueLo_ID = INVALID,
 .SampleCollectMatchValueHi_ID = INVALID,
 .SampleCollectTriggerMaskLo_ID = INVALID,
 .SampleCollectTriggerMaskHi_ID = INVALID,
 .SampleCollectTriggerValueLo_ID = INVALID,
 .SampleCollectTriggerValueHi_ID = INVALID,
 .SampleCollectTransMaskLo_ID = INVALID,
 .SampleCollectTransMaskHi_ID = INVALID,
 .SampleCollectPlayCtrl_ID = INVALID,
 .Core0BMCAllocStatusTID7_ID = INVALID,
 .Core1BMCAllocStatusTID7_ID = INVALID,
 .XmtTemplatePtrOffset_ID = (0x00000AE6),
 .BMVpConfig_ID = (0x000009F8),
};
static const d11regs_regs_common_ge64_lt80_t regs_common_ge64_lt80_132 = {
 .syncSlowTimeStamp_ID = (0x00000010),
 .IndXmtCtl_ID = INVALID,
 .IndAQMctl_ID = INVALID,
 .IndAQMQSel_ID = (0x000000C0),
 .SUSPREQ_ID = (0x000000C4),
 .FLUSHREQ_ID = (0x000000C8),
 .CHNFLUSH_STATUS_ID = (0x000000CC),
 .gptimer_psmx_ID = (0x000001B0),
 .MACCONTROL_psmx_ID = (0x000001B4),
 .MacControl1_psmx_ID = (0x000001B8),
 .MACCOMMAND_psmx_ID = (0x000001BC),
 .MACINTSTATUS_psmx_ID = (0x000001C0),
 .MACINTMASK_psmx_ID = (0x000001C4),
 .ALT_MACINTSTATUS_psmx_ID = (0x000001C8),
 .ALT_MACINTMASK_psmx_ID = (0x000001CC),
 .PSM_DEBUG_psmx_ID = (0x000001D0),
 .PHY_DEBUG_psmx_ID = (0x000001D4),
 .RXE_ERRVAL_ID = (0x00000416),
 .RXE_ERRMASK_ID = (0x00000418),
 .RXE_STATUS3_ID = (0x00000426),
 .PSM_STAT_CTR0_L_ID = (0x000007C8),
 .PSM_STAT_CTR1_H_ID = (0x000007CE),
 .PSM_STAT_SEL_ID = (0x000007D0),
 .TXE_MMPLCP_0_ID = INVALID,
 .TXE_MMPLCP_1_ID = INVALID,
 .TXE_PHYCTL_1_ID = (0x00000A44),
 .TXE_PHYCTL_2_ID = (0x00000A46),
 .SMP_PTR_H_ID = (0x00000AA0),
 .SCP_CURPTR_H_ID = (0x00000AA4),
 .SCP_STRTPTR_ID = (0x00000AA6),
 .SCP_STOPPTR_ID = (0x00000AA8),
 .SCP_CURPTR_ID = (0x00000AA2),
 .SPP_STRTPTR_ID = (0x00000AAA),
 .SPP_STOPPTR_ID = (0x00000AAC),
 .WEP_HWKEY_ADDR_ID = (0x000008B0),
 .WEP_HWKEY_LEN_ID = (0x000008B2),
 .PMQ_DAT_OR_MAT_MU1_ID = INVALID,
 .PMQ_DAT_OR_MAT_MU2_ID = INVALID,
 .PMQ_DAT_OR_MAT_MU3_ID = INVALID,
 .PMQ_AUXPMQ_STATUS_ID = (0x000008DA),
 .PMQ_CTL1_ID = INVALID,
 .PMQ_STATUS1_ID = (0x000008DC),
 .PMQ_ADDTHR_ID = (0x000008DE),
 .CTMode_ID = (0x00000940),
 .SCM_HVAL_L_ID = (0x00000ACE),
 .SCM_HVAL_H_ID = (0x00000AD0),
 .SCT_HMASK_L_ID = (0x00000AD2),
 .SCT_HMASK_H_ID = (0x00000AD4),
 .SCT_HVAL_L_ID = (0x00000AD6),
 .SCT_HVAL_H_ID = (0x00000AD8),
 .SCX_HMASK_L_ID = (0x00000ADA),
 .SCX_HMASK_H_ID = (0x00000ADC),
 .BMC_ReadQID_ID = INVALID,
 .BMC_BQFrmCnt_ID = INVALID,
 .BMC_BQByteCnt_L_ID = INVALID,
 .BMC_BQByteCnt_H_ID = INVALID,
 .AQM_BQFrmCnt_ID = INVALID,
 .AQM_BQByteCnt_L_ID = INVALID,
 .AQM_BQByteCnt_H_ID = INVALID,
 .AQM_BQPrelWM_ID = INVALID,
 .AQM_BQPrelStatus_ID = INVALID,
 .AQM_BQStatus_ID = INVALID,
 .BMC_MUDebugConfig_ID = (0x00000BA2),
 .BMC_MUDebugStatus_ID = (0x00000BA4),
 .BMCBQCutThruSt0_ID = (0x00000BA6),
 .BMCBQCutThruSt1_ID = (0x00000BA8),
 .BMCBQCutThruSt2_ID = (0x00000BAA),
 .TDC_VhtMacPad0_ID = INVALID,
 .TDC_VhtMacPad1_ID = INVALID,
 .TDC_MuVhtMCS_ID = INVALID,
 .ShmDma_XferWCnt_ID = (0x00000966),
 .AMT_MATCH1_ID = INVALID,
 .AMT_MATCH2_ID = INVALID,
 .PSM_REG_MUX_ID = (0x000007B0),
 .PSM_BASE_PSMX_ID = (0x00000878),
 .SCS_MASK_L_ID = (0x00000AAE),
 .SCS_MASK_H_ID = (0x00000AB0),
 .SCM_MASK_L_ID = (0x00000AB2),
 .SCM_MASK_H_ID = (0x00000AB4),
 .SCM_VAL_L_ID = (0x00000AB6),
 .SCM_VAL_H_ID = (0x00000AB8),
 .SCT_MASK_L_ID = (0x00000ABA),
 .SCT_MASK_H_ID = (0x00000ABC),
 .SCT_VAL_L_ID = (0x00000ABE),
 .SCT_VAL_H_ID = (0x00000AC0),
 .SCX_MASK_L_ID = (0x00000AC2),
 .SCX_MASK_H_ID = (0x00000AC4),
 .SMP_CTRL_ID = (0x00000ADE),
 .SysMStartAddrHi_ID = (0x00000A08),
 .SysMStartAddrLo_ID = (0x00000A06),
 .AQM_REG_SEL_ID = INVALID,
 .AQMQMAP_ID = INVALID,
 .AQMCmd_ID = INVALID,
 .AQMConsMsdu_ID = INVALID,
 .AQMDMACTL_ID = INVALID,
 .AQMMinCons_ID = INVALID,
 .MsduMinCons_ID = INVALID,
 .AQMAggNum_ID = INVALID,
 .AQMAggEntry_ID = INVALID,
 .AQMAggIdx_ID = INVALID,
 .AQMFiFoRptr_ID = INVALID,
 .AQMFIFO_SOFDPtr_ID = INVALID,
 .AQMFIFO_SWDCnt_ID = INVALID,
 .AQMFIFO_TXDPtr_L_ID = INVALID,
 .AQMFIFO_TXDPtr_ML_ID = INVALID,
 .AQMFIFO_TXDPtr_MU_ID = INVALID,
 .AQMFIFO_TXDPtr_H_ID = INVALID,
 .AQMFifoRdy_L_ID = INVALID,
 .AQMFifoRdy_H_ID = INVALID,
 .AQMFifo_Status_ID = INVALID,
 .AQMFifoFull_L_ID = INVALID,
 .AQMFifoFull_H_ID = INVALID,
 .AQMTBCP_Busy_L_ID = INVALID,
 .AQMTBCP_Busy_H_ID = INVALID,
 .AQMDMA_SuspFlush_ID = INVALID,
 .AQMFIFOSusp_L_ID = INVALID,
 .AQMFIFOSusp_H_ID = INVALID,
 .AQMFIFO_SuspPend_L_ID = INVALID,
 .AQMFIFO_SuspPend_H_ID = INVALID,
 .AQMFIFOFlush_L_ID = INVALID,
 .AQMFIFOFlush_H_ID = INVALID,
 .AQMTXD_CTL_ID = INVALID,
 .AQMTXD_RdOffset_ID = INVALID,
 .AQMTXD_RdLen_ID = INVALID,
 .AQMTXD_DestAddr_ID = INVALID,
 .AQMTBCP_QSEL_ID = INVALID,
 .AQMTBCP_Prio_ID = INVALID,
 .AQMTBCP_PrioFifo_ID = INVALID,
 .AQMFIFO_MPDULen_ID = INVALID,
 .AQMTBCP_Max_ReqEntry_ID = INVALID,
 .AQMTBCP_FCNT_ID = INVALID,
 .AQMSTATUS_ID = INVALID,
 .AQMDBG_CTL_ID = INVALID,
 .AQMDBG_DATA_ID = INVALID,
 .CHNSUSP_STATUS_ID = (0x000000D0),
};
static const d11regs_regs_common_ge80_lt128_t regs_common_ge80_lt128_132 = {
 .RxFilterEn_ID = (0x000003C0),
 .RxHwaCtrl_ID = (0x000003C4),
 .FlowAgeTimer_ID = INVALID,
 .RcvAMPDUCtl1_ID = INVALID,
 .RXE_PHYRS_ADDR_ID = (0x000005A0),
 .RXE_PHYRS_DATA_ID = (0x000005A2),
 .AMT_MATCH_ID = (0x000004EA),
 .AMT_ATTR_A1_ID = (0x000004FA),
 .AMT_ATTR_A2_ID = (0x000004FC),
 .AMT_ATTR_A3_ID = (0x000004FE),
 .AMT_ATTR_BSSID_ID = (0x00000500),
 .RXE_TXBA_CTL2_ID = INVALID,
 .MULTITID_STATUS_ID = INVALID,
 .FRAG_INFO_ID = INVALID,
 .FP_SHM_OFFSET_ID = INVALID,
 .FP_CTL_ID = (0x00000520),
 .BA_INFO_ACCESS_ID = INVALID,
 .BA_INFO_DATA_ID = INVALID,
 .FP_CONFIG_ID = (0x00000522),
 .FP_PATTERN1_ID = INVALID,
 .FP_MASK_ID = (0x00000526),
 .FP_STATUS_ID = (0x00000524),
 .FP_PATTERN2_ID = INVALID,
 .FP_PATTERN3_ID = INVALID,
 .RoeStatus1_ID = (0x00000474),
 .PSOCurRxFramePtrs_ID = INVALID,
 .PSOOBFFStatus_ID = INVALID,
 .AVB_RXTIMESTAMP_L_ID = (0x0000042A),
 .AVB_RXTIMESTAMP_H_ID = INVALID,
 .IFS_STAT2_ID = (0x000006AA),
 .IFS_CTL_SEL_SECCRS_ID = INVALID,
 .WEP_KEY_DATA_ID = (0x000008AA),
 .WEP_DATA_ID = (0x000008AE),
 .BTCX_PRIORITYWIN_ID = (0x00000606),
 .BTCX_TXCONFTIMER_ID = (0x00000608),
 .BTCX_PRISELTIMER_ID = (0x0000060A),
 .BTCX_PRV_RFACT_TIMER_ID = (0x0000060C),
 .BTCX_CUR_RFACT_TIMER_ID = (0x0000060E),
 .BTCX_RFACT_DUR_TIMER_ID = (0x00000610),
 .BTCX_ECI_ADDR_ID = (0x00000624),
 .BTCX_ECI_DATA_ID = (0x00000626),
 .BTCX_ECI_MASK_ADDR_ID = (0x00000628),
 .BTCX_ECI_MASK_DATA_ID = (0x0000062A),
 .COEX_IO_MASK_ID = (0x0000062C),
 .BTCX_ECI_EVENT_ADDR_ID = INVALID,
 .BTCX_ECI_EVENT_DATA_ID = INVALID,
 .PSM_INTSEL_1_ID = (0x00000822),
 .PSM_INTSEL_2_ID = (0x00000824),
 .PSM_INTSEL_3_ID = (0x00000826),
 .TXE_BV_31_ID = INVALID,
 .TXE_STATUS2_ID = (0x0000092E),
 .XmtSusp_ID = INVALID,
 .TXE_PHYCTL8_ID = INVALID,
 .TXE_PHYCTL9_ID = INVALID,
 .TXE_PHYCTL_3_ID = (0x00000A48),
 .TXE_PHYCTL_4_ID = (0x00000A4A),
 .TXE_PHYCTL_5_ID = (0x00000A4C),
 .TXE_PHYCTL_6_ID = (0x00000A4E),
 .TXE_PHYCTL_7_ID = (0x00000A50),
 .DebugTxFlowCtl_ID = (0x00000BB4),
 .TDC_PLCP0_ID = INVALID,
 .TDC_PLCP1_ID = INVALID,
 .TDC_FRMLEN0_ID = INVALID,
 .TDC_FRMLEN1_ID = INVALID,
 .TDC_TXTIME_ID = INVALID,
 .TDC_PLCP2_ID = (0x00000E10),
 .TDC_PLCP3_ID = (0x00000E12),
 .TDC_LSIGLEN_ID = (0x00000E32),
 .TDC_NSYM0_ID = INVALID,
 .TDC_NSYM1_ID = INVALID,
 .SampleCollectPlayCtrl2_ID = INVALID,
 .XmtFlush_ID = INVALID,
 .AQMAGGR_ID = INVALID,
 .AQMMultBAFL_ID = INVALID,
 .AQM_DFAdjB_ID = INVALID,
 .AQM_MFAdjB_ID = INVALID,
 .AQM_CFAdjB_ID = INVALID,
 .AQMminfragB_ID = INVALID,
 .AQMcagburst_ID = INVALID,
 .MQAQMMaxAggLenLow_ID = INVALID,
 .MQAQMMaxAggLenHi_ID = INVALID,
 .MQMAXMPDU_ID = INVALID,
 .AMPDUXMT_ID = INVALID,
 .AQMCaggNum_ID = INVALID,
 .AQMCaggLenLow_ID = INVALID,
 .AQMCAggLenHi_ID = INVALID,
 .XMT_AMPDU_DLIM_H_ID = INVALID,
 .AQMFIFO_FLAG_ID = INVALID,
 .AQMFSIE_OP_ID = INVALID,
 .AQMFSIE_RWD0_ID = INVALID,
 .AQMFSIE_RWD1_ID = INVALID,
 .AQMFSIE_RWD2_ID = INVALID,
 .BMCHdrOffset_ID = INVALID,
 .BMCHdrLength_ID = INVALID,
 .Core0BMCAllocStatusTplate_ID = (0x000009F2),
 .Core1BMCAllocStatusTplate_ID = INVALID,
};
static const d11regs_regs_common_ge128_lt129_t regs_common_ge128_lt129_132 = {
 .xmt_dma_chsel_ID = (0x00000050),
 .xmt_dma_intmap0_ID = (0x00000054),
 .xmt_dma_intmap1_ID = INVALID,
 .indintstat_ID = (0x00000078),
 .indintmask_ID = (0x0000007C),
 .SUSPREQ1_ID = (0x000000D4),
 .FLUSHREQ1_ID = (0x000000D8),
 .CHNFLUSH_STATUS1_ID = (0x000000DC),
 .CHNSUSP_STATUS1_ID = (0x000000E0),
 .SUSPREQ2_ID = (0x000000E4),
 .FLUSHREQ2_ID = (0x000000E8),
 .CHNFLUSH_STATUS2_ID = (0x000000EC),
 .CHNSUSP_STATUS2_ID = (0x000000F0),
 .TxDMAIndXmtStat0_ID = (0x000000F8),
 .TxDMAIndXmtStat1_ID = (0x000000FC),
 .PHYPLUS1CTL_ID = (0x000001AC),
 .PSM_CHK0_ERR_ID = (0x00000890),
 .PSM_CHK1_ERR_ID = (0x0000089C),
 .gptimer_R1_ID = (0x00000320),
 .MACCONTROL_r1_ID = (0x00000324),
 .MacControl1_R1_ID = (0x00000328),
 .MACCOMMAND_R1_ID = (0x0000032C),
 .MACINTSTATUS_R1_ID = (0x00000330),
 .MACINTMASK_R1_ID = (0x00000334),
 .ALT_MACINTSTATUS_R1_ID = (0x00000338),
 .ALT_MACINTMASK_R1_ID = (0x0000033C),
 .PSM_DEBUG_R1_ID = (0x00000360),
 .TSFTimerLow_R1_ID = (0x00000364),
 .TSFTimerHigh_R1_ID = (0x00000368),
 .HostFCBSCmdPtr_ID = (0x000003B4),
 .HostFCBSDataPtr_ID = (0x000003B8),
 .HostCtrlStsReg_ID = (0x000003BC),
 .RXE_CBR_CTL_ID = (0x00000406),
 .RXE_CBR_STAT_ID = (0x00000408),
 .RXE_DMA_STCNT_ID = (0x00000436),
 .OMAC_HWSTS_L_ID = (0x00000458),
 .OMAC_HWSTS_H_ID = (0x0000045A),
 .RXBM_DBG_SEL_ID = (0x0000045C),
 .RXBM_DBG_DATA_ID = (0x0000045E),
 .RCV_BM_OVFL_DBGSEL_ID = (0x00000478),
 .MHP_FCTP_ID = (0x000004A8),
 .MHP_FCTST_ID = (0x000004AA),
 .MHP_DFCTST_ID = (0x000004AC),
 .MHP_EXT0_ID = (0x000004AE),
 .MHP_EXT1_ID = (0x000004B0),
 .MHP_EXT2_ID = (0x000004B2),
 .MHP_EXT3_ID = (0x000004B4),
 .MHP_PLCP0_ID = (0x000004B6),
 .MHP_PLCP1_ID = (0x000004B8),
 .MHP_PLCP2_ID = (0x000004BA),
 .MHP_PLCP3_ID = (0x000004BC),
 .MHP_PLCP4_ID = (0x000004BE),
 .MHP_PLCP5_ID = (0x000004C0),
 .MHP_PLCP6_ID = (0x000004C2),
 .MHP_SEL_ID = (0x000004C4),
 .MHP_DATA_ID = (0x000004C6),
 .MHP_CFG_ID = (0x000004C8),
 .RXE_RCF_CTL_ID = (0x000004D0),
 .RXE_RCF_ADDR_ID = (0x000004D2),
 .RXE_RCF_WDATA_ID = (0x000004D4),
 .RXE_RCF_RDATA_ID = (0x000004D6),
 .RXE_RCF_NP_STATS_ID = (0x000004D8),
 .RXE_RCF_HP_STATS_ID = (0x000004DA),
 .RXE_HDRC_CTL_ID = (0x00001300),
 .RXE_HDRC_STATUS_ID = (0x00001302),
 .RXE_WM_0_ID = (0x00001304),
 .RXE_WM_1_ID = (0x00001306),
 .RXE_BM_ADDR_ID = (0x00001308),
 .RXE_BM_DATA_ID = (0x0000130A),
 .RXE_BV_ADDR_ID = (0x0000130C),
 .RXE_BV_DATA_ID = (0x0000130E),
 .PERUSER_DBG_SEL_ID = (0x00001310),
 .RXQ_DBG_CTL_ID = (0x00000514),
 .RXQ_DBG_STS_ID = (0x00000516),
 .FP_PAT0_ID = (0x00000528),
 .FP_PAT1_ID = (0x0000052A),
 .FP_PAT2_ID = (0x0000052C),
 .FP_SUPAT_ID = (0x0000052E),
 .RXE_FP_SHMADDR_ID = (0x00000530),
 .RXE_TXBA_CTL1_ID = (0x00000542),
 .MTID_STATUS_ID = (0x00000548),
 .BA_LEN_ID = (0x0000054A),
 .BA_LEN_ENC_ID = (0x0000054C),
 .MULTITID_STATUS2_ID = (0x0000054E),
 .BAINFO_SEL_ID = (0x00000550),
 .BAINFO_DATA_ID = (0x00000552),
 .TXBA_UBMP_ID = (0x00000554),
 .TXBA_UCNT_ID = (0x00000556),
 .TXBA_XFRBMP_ID = (0x00000558),
 .TXBA_XFRCTL_ID = (0x0000055A),
 .TXBA_XFRCNT_ID = (0x0000055C),
 .TXBA_XFROFFS_ID = (0x0000055E),
 .RDFBD_CTL0_ID = (0x00000560),
 .RDF_CTL0_ID = (0x00000562),
 .RDF_CTL1_ID = (0x00000564),
 .RDF_CTL2_ID = (0x00000566),
 .RDF_CTL3_ID = (0x00000568),
 .RDF_CTL4_ID = (0x0000056A),
 .RDF_CTL5_ID = (0x0000056C),
 .RDF_STAT1_ID = (0x0000056E),
 .RDF_STAT2_ID = (0x00000570),
 .RDF_STAT3_ID = (0x00000572),
 .RDF_STAT4_ID = (0x00000574),
 .RDF_STAT5_ID = (0x00000576),
 .RDF_STAT6_ID = (0x00000578),
 .RDF_STAT7_ID = (0x0000057A),
 .RDF_STAT8_ID = (0x0000057C),
 .RXE_PHYSTS_SHMADDR_ID = (0x00000592),
 .RXE_PHYSTS_BMP_SEL_ID = (0x00000594),
 .RXE_PHYSTS_BMP_DATA_ID = (0x00000596),
 .RXE_PHYSTS_TIMEOUT_ID = (0x00000598),
 .RXE_OOB_CFG_ID = (0x000005B0),
 .RXE_OOB_BMP0_ID = (0x000005B2),
 .RXE_OOB_BMP1_ID = (0x000005B4),
 .RXE_OOB_BMP2_ID = (0x000005B6),
 .RXE_OOB_BMP3_ID = (0x000005B8),
 .RXE_OOB_DSCR_ADDR_ID = (0x000005BA),
 .RXE_OOB_DSCR_SIZE_ID = (0x000005BC),
 .RXE_OOB_BUFA_ADDR_ID = (0x000005BE),
 .RXE_OOB_BUFA_SIZE_ID = (0x000005C0),
 .RXE_OOB_BUFB_ADDR_ID = (0x000005C2),
 .RXE_OOB_BUFB_SIZE_ID = (0x000005C4),
 .RXE_OOB_STATUS_ID = (0x000005C6),
 .RXE_OOB_DESC_PTR_ID = (0x000005C8),
 .RXE_OOB_BUFA_PTR_ID = (0x000005CA),
 .RXE_OOB_BUFB_PTR_ID = (0x000005CC),
 .RXE_BFDRPT_CTL_ID = (0x000005CE),
 .RXE_BFDRPT_RST_ID = (0x000005D0),
 .RXE_BFDRPT_LEN_ID = (0x000005D2),
 .RXE_BFDRPT_OFFSET_ID = (0x000005D4),
 .RXE_BFDRPT_MEND_ID = (0x000005D6),
 .RXE_BFDRPT_XFER_ID = (0x000005D8),
 .RXE_BFDRPT_SUCC_ID = (0x000005DA),
 .RXE_BFDRPT_DONE_ID = (0x000005DC),
 .RXE_BFM_HDRSEL_ID = (0x000005DE),
 .RXE_BFM_HDRDATA_ID = (0x000005E0),
 .CMDPTR_L_ID = (0x000006E0),
 .CMDPTR_H_ID = (0x000006E2),
 .DATA_PTR_L_ID = (0x000006E4),
 .DATA_PTR_H_ID = (0x000006E6),
 .CTRL_STS_ID = (0x000006E8),
 .PHY_ADDR_ID = (0x000006EA),
 .PHY_DATA_ID = (0x000006EC),
 .RADIO_ADDR_ID = (0x000006EE),
 .RADIO_DATA_ID = (0x000006F0),
 .RUNTIME_CNT_ID = (0x000006F2),
 .TSF_GPT_0_CTR_L_ID = (0x00000762),
 .TSF_GPT_0_CTR_H_ID = (0x00000764),
 .TSF_GPT_0_VAL_L_ID = (0x00000766),
 .TSF_GPT_0_VAL_H_ID = (0x00000768),
 .TSF_GPT_1_STAT_ID = (0x0000076A),
 .TSF_GPT_1_CTL_L_ID = (0x0000076C),
 .TSF_GPT_1_CTL_H_ID = (0x0000076E),
 .TSF_GPT_1_VAL_L_ID = (0x00000770),
 .HWA_MACIF_CTL_ID = (0x000007B2),
 .PSM_DIV_REM_L_ID = (0x000007BC),
 .PSM_DIV_REM_H_ID = (0x000007BE),
 .PSM_MACCTL_ID = (0x000007C2),
 .PSM_SBADDR_ID = (0x000007E2),
 .PSM_BRC_SEL_1_ID = (0x0000082A),
 .PSM_BASE_14_ID = (0x0000085C),
 .PSM_BASE_15_ID = (0x0000085E),
 .PSM_BASE_16_ID = (0x00000860),
 .PSM_BASE_17_ID = (0x00000862),
 .PSM_BASE_18_ID = (0x00000864),
 .PSM_BASE_19_ID = (0x00000866),
 .PSM_BASE_20_ID = (0x00000868),
 .PSM_BASE_21_ID = (0x0000086A),
 .PSM_BASE_22_ID = (0x0000086C),
 .PSM_BASE_23_ID = (0x0000086E),
 .PSM_RATEMEM_DBG_ID = (0x000007F8),
 .PSM_RATEMEM_CTL_ID = (0x000007FA),
 .PSM_RATEBLK_SIZE_ID = (0x000007FC),
 .PSM_RATEXFER_SIZE_ID = (0x000007FE),
 .PSM_LINKMEM_CTL_ID = (0x0000087A),
 .PSM_LINKBLK_SIZE_ID = (0x0000087C),
 .PSM_LINKXFER_SIZE_ID = (0x0000087E),
 .MAC_GPIOOUT_L_ID = (0x00000886),
 .MAC_GPIOOUT_H_ID = (0x00000888),
 .PMQ_CNT_ID = (0x000008C2),
 .PMQ_SRCH_USREN_ID = (0x000008C4),
 .PMQ_USR_SEL_ID = (0x000008C6),
 .PMQ_DAT_OR_MAT_SEL_ID = INVALID,
 .PMQ_DAT_OR_MAT_DATA_ID = INVALID,
 .PMQ_MATCH_ID = (0x000008D6),
 .APMQ_MATCH_ID = (0x000008D8),
 .PMQ_USER_BMP_ID = INVALID,
 .TXE_FCS_CTL_ID = (0x00000910),
 .AMPDU_CTL_ID = (0x00000914),
 .AMPDU_CRC_ID = (0x00000916),
 .TXE_WM_LEG0_ID = (0x00000918),
 .TXE_WM_LEG1_ID = (0x0000091A),
 .TXE_CTL3_ID = (0x0000091C),
 .TXE_TXE_EARLYTXMEND_CNT_ID = INVALID,
 .TXE_TXE_EARLYTXMEND_BSUB_CNT_ID = INVALID,
 .TXE_TXE_TXMEND_CNT_ID = INVALID,
 .TXE_TXE_TXMEND_NCONS_ID = INVALID,
 .TXE_TXE_TXMEND_PEND_ID = INVALID,
 .TXE_TXE_TXMEND_USR2GO_ID = INVALID,
 .TXE_TXE_TXMEND_CONSCNT_ID = INVALID,
 .TXE_BVBM_INIT_ID = (0x0000093E),
 .ToEChannelState_ID = (0x00000948),
 .TXE_BV_REG0_ID = (0x00000952),
 .TXE_BM_REG0_ID = (0x00000954),
 .TXE_BV_REG1_ID = (0x00000956),
 .TXE_BM_REG1_ID = (0x00000958),
 .TXE_SHMDMA_MPMADDR_ID = (0x00000970),
 .TXE_BITSUB_IDX_ID = (0x00000976),
 .TXE_BM_ADDR_ID = (0x00000978),
 .TXE_BM_DATA_ID = (0x0000097A),
 .TXE_BV_ADDR_ID = (0x0000097C),
 .TXE_BV_DATA_ID = (0x0000097E),
 .TXE_BV_10_ID = (0x000009B4),
 .TXE_BV_11_ID = (0x000009B6),
 .TXE_BV_12_ID = (0x000009B8),
 .TXE_BV_13_ID = (0x000009BA),
 .TXE_BV_14_ID = (0x000009BC),
 .TXE_BV_15_ID = (0x000009BE),
 .TXE_BMC_CONFIG_ID = (0x000009CE),
 .TXE_BMC_RXMAPFIFOSIZE_ID = INVALID,
 .TXE_BMC_RXMAPSTATUS_ID = INVALID,
 .TXE_BMC_DYNSTATUS1_ID = INVALID,
 .TXE_BMC_MAXOUTSTBUFS_ID = (0x000009EC),
 .TXE_BMC_CONFIG1_ID = (0x000009EE),
 .TXE_LLM_CONFIG_ID = (0x000009F6),
 .TXE_LOCALM_SADDR_L_ID = (0x00000A0A),
 .TXE_LOCALM_SADDR_H_ID = (0x00000A0C),
 .TXE_LOCALM_EADDR_L_ID = (0x00000A0E),
 .TXE_LOCALM_EADDR_H_ID = (0x00000A10),
 .TXE_BMC_ALLOCCTL1_ID = (0x00000A12),
 .TXE_BMC_MALLOCREQ_QB0_ID = (0x00000A14),
 .TXE_BMC_MALLOCREQ_QB1_ID = (0x00000A16),
 .TXE_BMC_MALLOCREQ_QB2_ID = (0x00000A18),
 .TXE_BMC_MALLOCREQ_QB3_ID = (0x00000A1A),
 .TXE_BMC_MALLOCREQ_QB4_ID = (0x00000A1C),
 .PHYCTL_LEN_ID = (0x00000A40),
 .TXE_PHYCTL_10_ID = (0x00000A56),
 .PLCP_LSIG0_ID = (0x00000A58),
 .PLCP_LSIG1_ID = (0x00000A5A),
 .PLCP_HTSIG0_ID = (0x00000A5C),
 .PLCP_HTSIG1_ID = (0x00000A5E),
 .PLCP_HTSIG2_ID = (0x00000A60),
 .TXE_PLCP_VHTSIGB0_ID = (0x00000A62),
 .TXE_PLCP_VHTSIGB1_ID = (0x00000A64),
 .PLCP_EXT2_ID = (0x00000A66),
 .PLCP_CC1_LEN_ID = (0x00000A68),
 .PLCP_CC2_LEN_ID = (0x00000A6A),
 .TXE_MPMSIZE_SEL_ID = (0x00000A6C),
 .TXE_MPMSIZE_VAL_ID = (0x00000A6E),
 .TXE_PLCPEXT_ADDR_ID = (0x00000A70),
 .TXE_PLCPEXT_DATA_ID = (0x00000A72),
 .TXE_PHYCTLEXT_BASE_ID = (0x00000A74),
 .TXE_PLCPEXT_BASE_ID = (0x00000A76),
 .TXE_SIGB_BASE_ID = (0x00000A78),
 .TXE_PLCP_LEN_ID = INVALID,
 .TXE_TXDBG_SEL_ID = (0x00000A86),
 .TXE_TXDBG_DATA_ID = (0x00000A88),
 .TXE_BFMRPT_MEMSEL_ID = (0x00000A8A),
 .TXE_BFMRPT_ADDR_ID = (0x00000A8C),
 .TXE_BFMRPT_DATA_ID = (0x00000A8E),
 .TXE_MEND_STATUS_ID = (0x00000A90),
 .TXE_UNFLOW_STATUS_ID = (0x00000A92),
 .TXE_TXERROR_STATUS_ID = (0x00000A94),
 .TXE_SNDFRM_STATUS_ID = (0x00000A96),
 .TXE_TXERROR_USR_ID = (0x00000A98),
 .TXE_MACPAD_PAT_L_ID = (0x00000A9A),
 .TXE_MACPAD_PAT_H_ID = (0x00000A9C),
 .TXE_PHYTXREQ_TMOUT_ID = (0x00000A9E),
 .TXE_XMTSUSP_QB0_ID = (0x00000AE8),
 .TXE_XMTSUSP_QB1_ID = (0x00000AEA),
 .TXE_XMTSUSP_QB2_ID = (0x00000AEC),
 .TXE_XMTSUSP_QB3_ID = (0x00000AEE),
 .TXE_XMTSUSP_QB4_ID = (0x00000AF0),
 .TXE_XMTFLUSH_QB0_ID = (0x00000AF2),
 .TXE_XMTFLUSH_QB1_ID = (0x00000AF4),
 .TXE_XMTFLUSH_QB2_ID = (0x00000AF6),
 .TXE_XMTFLUSH_QB3_ID = (0x00000AF8),
 .TXE_XMTFLUSH_QB4_ID = (0x00000AFA),
 .TXE_XMT_SHMADDR_ID = (0x00000AFC),
 .TXE_BMC_READQID_ID = (0x00000AFE),
 .TXE_BMC_READIDX_ID = (0x00000B02),
 .BMC_AQMBQID_ID = (0x00000B0A),
 .BMC_PSMCMD_THRESH_ID = (0x00000B0C),
 .BMC_PSMCMD_LOWVEC_ID = (0x00000B0E),
 .BMC_PSMCMD_EMPTYVEC_ID = (0x00000B10),
 .BMC_PSMCMD_OFLOW_ID = (0x00000B12),
 .BMC_CMD2SCHED_PEND_ID = (0x00000B14),
 .BMC_AQM_TXDRD_PEND_ID = (0x00000B16),
 .BMC_UC_TXDRD_PEND_ID = (0x00000B18),
 .BMC_CMDQ_FREECNT_ID = (0x00000B1A),
 .BMC_CMDQ_FREESTS_ID = (0x00000B1C),
 .BMC_CMDQ_FRMCNT_ID = (0x00000B1E),
 .BMC_FRM2SER_PEND_ID = (0x00000B20),
 .BMC_FRM2SER_PRGR_ID = (0x00000B22),
 .BMC_FRM2MPM_PEND_ID = (0x00000B24),
 .BMC_FRM2MPM_PENDSTS_ID = (0x00000B26),
 .BMC_FRM2MPM_PRGR_ID = (0x00000B28),
 .BMC_BITSUB_FREECNT_ID = (0x00000B2A),
 .BMC_BITSUB_FREESTS_ID = (0x00000B2C),
 .BMC_CMDQ_USR2GO_ID = (0x00000B2E),
 .BMC_BITSUB_USR2GO_ID = (0x00000B30),
 .TXE_BRC_CMDQ_FREEUP_ID = (0x00000B32),
 .TXE_BRC_FRM2MPM_PEND_ID = (0x00000B34),
 .BMC_PSMCMD0_ID = (0x00000B36),
 .BMC_PSMCMD1_ID = (0x00000B38),
 .BMC_PSMCMD2_ID = (0x00000B3A),
 .BMC_PSMCMD3_ID = (0x00000B3C),
 .BMC_PSMCMD4_ID = (0x00000B3E),
 .BMC_PSMCMD5_ID = (0x00000B40),
 .BMC_PSMCMD6_ID = (0x00000B42),
 .BMC_PSMCMD7_ID = (0x00000B44),
 .BMC_RDCLIENT_CTL_ID = (0x00000B46),
 .BMC_RDMGR_USR_RST_ID = (0x00000B48),
 .BMC_BMRD_INFLIGHT_THRESH_ID = (0x00000B4A),
 .BMC_BITSUB_CAP_ID = (0x00000B4C),
 .BMC_ERR_ID = (0x00000B4E),
 .TXE_SCHED_USR_RST_ID = (0x00000B50),
 .TXE_SCHED_SET_UFL_ID = (0x00000B52),
 .TXE_SCHED_UFL_WAIT_ID = (0x00000B54),
 .BMC_SCHED_UFL_NOCMD_ID = (0x00000B56),
 .BMC_CMD2SCHED_PENDSTS_ID = (0x00000B58),
 .BMC_PSMCMD_RST_ID = (0x00000B5A),
 .TXE_SCHED_SENT_L_ID = (0x00000B5C),
 .TXE_SCHED_SENT_H_ID = (0x00000B5E),
 .TXE_SCHED_CTL_ID = (0x00000B60),
 .TXE_MSCHED_USR_EN_ID = (0x00000B62),
 .TXE_MSCHED_SYMB_CYCS_ID = (0x00000B64),
 .TXE_SCHED_UFL_STS_ID = (0x00000B66),
 .TXE_MSCHED_BURSTPH_TOTSZ_ID = (0x00000B68),
 .TXE_MSCHED_BURSTPH_BURSTSZ_ID = (0x00000B6A),
 .TXE_MSCHED_NDPBS_L_ID = (0x00000B6C),
 .TXE_MSCHED_NDPBS_H_ID = (0x00000B6E),
 .TXE_MSCHED_STATE_ID = (0x00000B70),
 .TXE_FRMINPROG_QB0_ID = (0x00000B74),
 .TXE_FRMINPROG_QB1_ID = (0x00000B76),
 .TXE_FRMINPROG_QB2_ID = (0x00000B78),
 .TXE_FRMINPROG_QB3_ID = (0x00000B7A),
 .TXE_FRMINPROG_QB4_ID = (0x00000B7C),
 .TXE_XMT_DMABUSY_QB0_ID = (0x00000B7E),
 .TXE_XMT_DMABUSY_QB1_ID = (0x00000B80),
 .TXE_XMT_DMABUSY_QB2_ID = (0x00000B82),
 .TXE_XMT_DMABUSY_QB3_ID = (0x00000B84),
 .TXE_XMT_DMABUSY_QB4_ID = (0x00000B86),
 .TXE_BMC_FIFOFULL_QB0_ID = (0x00000B88),
 .TXE_BMC_FIFOFULL_QB1_ID = (0x00000B8A),
 .TXE_BMC_FIFOFULL_QB2_ID = (0x00000B8C),
 .TXE_BMC_FIFOFULL_QB3_ID = (0x00000B8E),
 .TXE_BMC_FIFOFULL_QB4_ID = (0x00000B90),
 .TXE_XMT_DPQSL_QB0_ID = (0x00000B92),
 .TXE_XMT_DPQSL_QB1_ID = (0x00000B94),
 .TXE_XMT_DPQSL_QB2_ID = (0x00000B96),
 .TXE_XMT_DPQSL_QB3_ID = (0x00000B98),
 .TXE_XMT_DPQSL_QB4_ID = (0x00000B9A),
 .TXE_AQM_BQSTATUS_ID = (0x00000B9C),
 .TXE_DBG_BMC_STATUS_ID = (0x00000BAC),
 .TXE_HDR_PDLIM_ID = (0x00000BB6),
 .TXE_MAC_PADBYTES_ID = (0x00000BB8),
 .TXE_MPDU_MINLEN_ID = (0x00000BBA),
 .TXE_AMP_EOFPD_L_ID = (0x00000BBC),
 .TXE_AMP_EOFPD_H_ID = (0x00000BBE),
 .TXE_AMP_EOFPADBYTES_ID = (0x00000BC0),
 .TXE_PSDULEN_L_ID = (0x00000BC2),
 .TXE_PSDULEN_H_ID = (0x00000BC4),
 .XMTDMA_CTL_ID = (0x00000BC8),
 .TXE_XMTDMA_PRELD_RANGE_ID = INVALID,
 .TXE_XMTDMA_ACT_RANGE_ID = (0x00000BCC),
 .TXE_XMTDMA_RUWT_QSEL_ID = (0x00000BCE),
 .TXE_XMTDMA_RUWT_DATA_ID = (0x00000BD0),
 .TXE_CTDMA_CTL_ID = (0x00000BD2),
 .TXE_XMTDMA_DBG_CTL_ID = (0x00000BD4),
 .XMTDMA_AQMIF_STS_ID = (0x00000BD6),
 .XMTDMA_PRELD_STS_ID = (0x00000BD8),
 .XMTDMA_ACT_STS_ID = (0x00000BDA),
 .XMTDMA_QUEUE_STS_ID = (0x00000BDC),
 .XMTDMA_ENGINE_STS_ID = (0x00000BDE),
 .XMTDMA_DMATX_STS_ID = (0x00000BE0),
 .XMTDMA_THRUPUTCTL_STS_ID = (0x00000BE2),
 .TXE_SCHED_MPMFC_STS_ID = (0x00000BE4),
 .TXE_SCHED_FORCE_BAD_ID = (0x00000BE6),
 .TXE_BMC_RDMGR_STATE_ID = (0x00000BE8),
 .TXE_SCHED_ERR_ID = (0x00000BEA),
 .TXE_BMC_PFF_CFG_ID = (0x00000BEC),
 .TXE_BMC_PFFSTART_DEF_ID = (0x00000BEE),
 .TXE_BMC_PFFSZ_DEF_ID = (0x00000BF0),
 .XmtDMABusyOtherQ0to15_ID = INVALID,
 .XmtDMABusyOtherQ16to31_ID = INVALID,
 .XmtDMABusyOtherQ32to47_ID = INVALID,
 .XmtDMABusyOtherQ48to63_ID = INVALID,
 .XmtDMABusyOtherQ64to69_ID = INVALID,
 .AQM_CFG_ID = (0x00000C00),
 .AQM_FIFODEF0_ID = (0x00000C02),
 .AQM_FIFODEF1_ID = (0x00000C04),
 .AQM_TXCNTDEF0_ID = (0x00000C06),
 .AQM_TXCNTDEF1_ID = (0x00000C08),
 .AQM_INITREQ_ID = (0x00000C0E),
 .AQM_REGSEL_ID = (0x00000C12),
 .AQM_QMAP_ID = (0x00000C14),
 .AQM_MAXAGGRLEN_H_ID = (0x00000C18),
 .AQM_MAXAGGRLEN_L_ID = (0x00000C16),
 .AQM_MAXAGGRNUM_ID = (0x00000C1A),
 .AQM_MINMLEN_ID = (0x00000C1C),
 .AQM_MAXOSFRAG_ID = (0x00000C1E),
 .AQM_AGGPARM_ID = (0x00000C24),
 .AQM_FRPRAM_ID = (0x00000C26),
 .AQM_MINFRLEN_ID = (0x00000C2C),
 .AQM_MQBURST_ID = (0x00000C2E),
 .AQM_DMLEN_ID = (0x00000C30),
 .AQM_MMLEN_ID = (0x00000C32),
 .AQM_CMLEN_ID = (0x00000C34),
 .AQM_VQENTRY0_ID = (0x00000C36),
 .AQM_VQENTRY1_ID = (0x00000C38),
 .AQM_VQADJ_ID = (0x00000C3A),
 .AQM_VQPADADJ_ID = (0x00000C3C),
 .AQM_AGGREN_ID = (0x00000C3E),
 .AQM_AGGREQ_ID = (0x00000C40),
 .AQM_CAGGLEN_L_ID = (0x00000C42),
 .AQM_CAGGLEN_H_ID = (0x00000C44),
 .AQM_CAGGNUM_ID = (0x00000C46),
 .AQM_QAGGSTATS_ID = (0x00000C48),
 .AQM_QAGGNUM_ID = (0x00000C4A),
 .AQM_QAGGINFO_ID = (0x00000C4C),
 .AQM_AGGRPTR_ID = (0x00000C4E),
 .AQM_QAGGLEN_L_ID = (0x00000C50),
 .AQM_QAGGLEN_H_ID = (0x00000C52),
 .AQM_QMPDULEN_ID = (0x00000C54),
 .AQM_QFRAGOS_ID = (0x00000C56),
 .AQM_QTXCNT_ID = (0x00000C58),
 .AQM_QMPDUINFO0_ID = (0x00000C5C),
 .AQM_QMPDUINFO1_ID = (0x00000C5E),
 .AQM_TXCNTEN_ID = (0x00000C60),
 .AQM_TXCNTUPD_ID = (0x00000C62),
 .AQM_QTXCNTRPTR_ID = (0x00000C64),
 .AQM_QCURTXCNT_ID = (0x00000C66),
 .AQM_BASEL_ID = (0x00000C68),
 .AQM_RCVDBA0_ID = (0x00000C6A),
 .AQM_RCVDBA1_ID = (0x00000C6C),
 .AQM_RCVDBA2_ID = (0x00000C6E),
 .AQM_RCVDBA3_ID = (0x00000C70),
 .AQM_BASSN_ID = (0x00000C72),
 .AQM_REFSN_ID = (0x00000C74),
 .AQM_QMAXBAIDX_ID = (0x00000C76),
 .AQM_BAEN_ID = (0x00000C78),
 .AQM_BAREQ_ID = (0x00000C7A),
 .AQM_UPDBARD_ID = (0x00000C7C),
 .AQM_UPDBA0_ID = (0x00000C7E),
 .AQM_UPDBA1_ID = (0x00000C80),
 .AQM_UPDBA2_ID = (0x00000C82),
 .AQM_UPDBA3_ID = (0x00000C84),
 .AQM_ACKCNT_ID = (0x00000C86),
 .AQM_CONSTYPE_ID = (0x00000C8A),
 .AQM_CONSEN_ID = (0x00000C8C),
 .AQM_CONSREQ_ID = (0x00000C8E),
 .AQM_CONSCNT_ID = (0x00000C90),
 .AQM_XUPDEN_ID = (0x00000C92),
 .AQM_XUPDREQ_ID = (0x00000C94),
 .AQM_TXDMAEN_ID = (0x00000C98),
 .AQM_TXDMAREQ_ID = (0x00000C9A),
 .AQMF_READY0_ID = (0x00000CA0),
 .AQMF_READY1_ID = (0x00000CA2),
 .AQMF_READY2_ID = (0x00000CA4),
 .AQMF_READY3_ID = (0x00000CA6),
 .AQMF_READY4_ID = (0x00000CA8),
 .AQMF_MTE0_ID = (0x00000CAA),
 .AQMF_MTE1_ID = (0x00000CAC),
 .AQMF_MTE2_ID = (0x00000CAE),
 .AQMF_MTE3_ID = (0x00000CB0),
 .AQMF_MTE4_ID = (0x00000CB2),
 .AQMF_PLREADY0_ID = (0x00000CB4),
 .AQMF_PLREADY1_ID = (0x00000CB6),
 .AQMF_PLREADY2_ID = (0x00000CB8),
 .AQMF_PLREADY3_ID = (0x00000CBA),
 .AQMF_PLREADY4_ID = (0x00000CBC),
 .AQMF_FULL0_ID = (0x00000CBE),
 .AQMF_FULL1_ID = (0x00000CC0),
 .AQMF_FULL2_ID = (0x00000CC2),
 .AQMF_FULL3_ID = (0x00000CC4),
 .AQMF_FULL4_ID = (0x00000CC6),
 .AQMF_STATUS_ID = (0x00000CC8),
 .MQF_EMPTY0_ID = (0x00000CCA),
 .MQF_EMPTY1_ID = (0x00000CCC),
 .MQF_EMPTY2_ID = (0x00000CCE),
 .MQF_EMPTY3_ID = (0x00000CD0),
 .MQF_EMPTY4_ID = (0x00000CD2),
 .MQF_STATUS_ID = (0x00000CD4),
 .TXQ_STATUS_ID = (0x00000CD8),
 .AQM_BQEN_ID = (0x00000CE0),
 .AQM_BQSUSP_ID = (0x00000CE2),
 .AQM_BQFLUSH_ID = (0x00000CE4),
 .AQMF_HSUSP0_ID = (0x00000CF2),
 .AQMF_HSUSP1_ID = (0x00000CF4),
 .AQMF_HSUSP2_ID = (0x00000CF6),
 .AQMF_HSUSP3_ID = (0x00000CF8),
 .AQMF_HSUSP4_ID = (0x00000CFA),
 .AQMF_HSUSP_PRGR0_ID = (0x00000CFC),
 .AQMF_HSUSP_PRGR1_ID = (0x00000CFE),
 .AQMF_HSUSP_PRGR2_ID = (0x00000D00),
 .AQMF_HSUSP_PRGR3_ID = (0x00000D02),
 .AQMF_HSUSP_PRGR4_ID = (0x00000D04),
 .AQMF_HFLUSH0_ID = (0x00000D06),
 .AQMF_HFLUSH1_ID = (0x00000D08),
 .AQMF_HFLUSH2_ID = (0x00000D0A),
 .AQMF_HFLUSH3_ID = (0x00000D0C),
 .AQMF_HFLUSH4_ID = (0x00000D0E),
 .AQMTXD_READ_ID = (0x00000D10),
 .AQMTXD_RDOFFSET_ID = (0x00000D12),
 .AQMTXD_RDLEN_ID = (0x00000D14),
 .AQMTXD_DSTADDR_ID = (0x00000D16),
 .AQMTXD_AUTOPF_ID = (0x00000D18),
 .AQMTXD_APFOFFSET_ID = (0x00000D1A),
 .AQMTXD_APFDSTADDR_ID = (0x00000D1C),
 .AQMTXD_PFREADY0_ID = (0x00000D1E),
 .AQMTXD_PFREADY1_ID = (0x00000D20),
 .AQMTXD_PFREADY2_ID = (0x00000D22),
 .AQMTXD_PFREADY3_ID = (0x00000D24),
 .AQMTXD_PFREADY4_ID = (0x00000D26),
 .AQMCT_BUSY0_ID = (0x00000D28),
 .AQMCT_BUSY1_ID = (0x00000D2A),
 .AQMCT_BUSY2_ID = (0x00000D2C),
 .AQMCT_BUSY3_ID = (0x00000D2E),
 .AQMCT_BUSY4_ID = (0x00000D30),
 .AQMCT_QSEL_ID = (0x00000D32),
 .AQMCT_PRI_ID = (0x00000D34),
 .AQMCT_PRIFIFO_ID = (0x00000D36),
 .AQMCT_MAXREQNUM_ID = (0x00000D38),
 .AQMCT_FREECNT_ID = (0x00000D3A),
 .AQMCT_CHDIS_ID = (0x00000D3C),
 .AQMPL_CFG_ID = (0x00000D40),
 .AQMPL_QSEL_ID = (0x00000D42),
 .AQMPL_THRESHOLD_ID = (0x00000D44),
 .AQMPL_EPOCHMASK_ID = (0x00000D46),
 .AQMPL_MAXMPDU_ID = (0x00000D48),
 .AQMPL_DIS_ID = (0x00000D4A),
 .AQMPL_FORCE_ID = (0x00000D4C),
 .AQMTXPL_THRESH_ID = (0x00000D4E),
 .AQMTXPL_MASK_ID = (0x00000D50),
 .AQMTXPL_RDY_ID = (0x00000D52),
 .AQMCSB_REQ_ID = (0x00000D60),
 .AQMF_RPTR_ID = (0x00000D62),
 .AQMF_MPDULEN_ID = (0x00000D64),
 .AQMF_SOFDPTR_ID = (0x00000D66),
 .AQMF_SWDCNT_ID = (0x00000D68),
 .AQMF_FLAG_ID = (0x00000D6A),
 .AQMF_TXDPTR_L_ID = (0x00000D6C),
 .AQMF_TXDPTR_ML_ID = (0x00000D6E),
 .AQMF_TXDPTR_MU_ID = (0x00000D70),
 .AQMF_TXDPTR_H_ID = (0x00000D72),
 .AQMF_SRTIDX_ID = (0x00000D74),
 .AQMF_ENDIDX_ID = (0x00000D76),
 .AQMF_NUMBUF_ID = (0x00000D78),
 .AQMF_BUFLEN_ID = (0x00000D7A),
 .AQMFR_RWD0_ID = (0x00000D7C),
 .AQMFR_RWD1_ID = (0x00000D7E),
 .AQMFR_RWD2_ID = (0x00000D80),
 .AQMFR_RWD3_ID = (0x00000D82),
 .AQMCSB_RPTR_ID = (0x00000D84),
 .AQMCSB_WPTR_ID = (0x00000D86),
 .AQMCSB_BASEL_ID = (0x00000D88),
 .AQMCSB_BA0_ID = (0x00000D8A),
 .AQMCSB_BA1_ID = (0x00000D8C),
 .AQMCSB_BA2_ID = (0x00000D8E),
 .AQMCSB_BA3_ID = (0x00000D90),
 .AQMCSB_QAGGLEN_L_ID = (0x00000D92),
 .AQMCSB_QAGGLEN_H_ID = (0x00000D94),
 .AQMCSB_QAGGNUM_ID = (0x00000D96),
 .AQMCSB_QAGGSTATS_ID = (0x00000D98),
 .AQMCSB_QAGGINFO_ID = (0x00000D9A),
 .AQMCSB_CONSCNT_ID = (0x00000D9C),
 .AQMCSB_TOTCONSCNT_ID = (0x00000D9E),
 .AQMCSB_CFGSTRADDR_ID = (0x00000DA0),
 .AQMCSB_CFGENDADDR_ID = (0x00000DA2),
 .AQM_AGGERR_ID = (0x00000DA8),
 .AQM_QAGGERR_ID = (0x00000DAA),
 .AQM_BRSTATUS_ID = (0x00000DAC),
 .AQM_QBRSTATUS_ID = (0x00000DAE),
 .AQM_DBGCTL_ID = (0x00000DB0),
 .AQM_DBGDATA0_ID = (0x00000DB4),
 .AQM_DBGDATA1_ID = (0x00000DB6),
 .AQM_DBGDATA2_ID = (0x00000DB8),
 .AQM_DBGDATA3_ID = (0x00000DBA),
 .AQM_ERRSEL_ID = (0x00000DC0),
 .AQM_ERRSTS_ID = (0x00000DC4),
 .AQM_SWRESET_ID = (0x00000DE0),
 .TDC_CTL_ID = (0x00000E00),
 .TDC_USRCFG_ID = (0x00000E02),
 .TDC_RUNCMD_ID = (0x00000E04),
 .TDC_RUNSTS_ID = (0x00000E06),
 .TDC_STATUS_ID = (0x00000E08),
 .TDC_NUSR_ID = (0x00000E0A),
 .TDC_ORIGBW_ID = (0x00000E14),
 .TDC_FRMLEN_L_ID = (0x00000E16),
 .TDC_FRMLEN_H_ID = (0x00000E18),
 .TDC_USRINFO_0_ID = (0x00000E1A),
 .TDC_USRINFO_1_ID = (0x00000E1C),
 .TDC_USRAID_ID = (0x00000E20),
 .TDC_PPET0_ID = (0x00000E22),
 .TDC_PPET1_ID = (0x00000E24),
 .TDC_MAX_LENEXP_ID = (0x00000E26),
 .TDC_MAXDUR_ID = (0x00000E28),
 .TDC_MINMDUR_ID = (0x00000E2A),
 .TDC_HDRDUR_ID = (0x00000E2C),
 .TDC_TRIG_PADDUR_ID = (0x00000E2E),
 .TDC_PSDULEN_L_ID = (0x00000E38),
 .TDC_PSDULEN_H_ID = (0x00000E3A),
 .TDC_HDR_PDLIM_ID = (0x00000E3C),
 .TDC_MAC_PADLEN_ID = (0x00000E3E),
 .TDC_EOFPDLIM_L_ID = (0x00000E40),
 .TDC_EOFPDLIM_H_ID = (0x00000E42),
 .TDC_MAXPSDULEN_L_ID = (0x00000E44),
 .TDC_MAXPSDULEN_H_ID = (0x00000E46),
 .TDC_MPDU_MINLEN_ID = (0x00000E48),
 .TDC_CCLEN_ID = (0x00000E4A),
 .TDC_RATE_L_ID = (0x00000E4C),
 .TDC_RATE_H_ID = (0x00000E4E),
 .TDC_NSYMINIT_L_ID = (0x00000E50),
 .TDC_NSYMINIT_H_ID = (0x00000E52),
 .TDC_NDBPS_L_ID = (0x00000E54),
 .TDC_NDBPS_H_ID = (0x00000E56),
 .TDC_NDBPS_S_ID = (0x00000E58),
 .TDC_T_PRE_ID = (0x00000E5A),
 .TDC_TDATA_ID = (0x00000E5C),
 .TDC_TDATA_MIN_ID = (0x00000E5E),
 .TDC_TDATA_MAX_ID = (0x00000E60),
 .TDC_TDATA_AVG_ID = (0x00000E62),
 .TDC_TDATA_TOT_ID = (0x00000E64),
 .TDC_HESIGB_NSYM_ID = (0x00000E66),
 .TDC_AGGLEN_L_ID = (0x00000E68),
 .TDC_AGGLEN_H_ID = (0x00000E6A),
 .TDC_TIME_IN_ID = (0x00000E6C),
 .TDC_INITREQ_ID = (0x00000E6E),
 .TDC_AGG0STS_ID = (0x00000E70),
 .TDC_TRIGENDLOG_ID = (0x00000E72),
 .TDC_RUSTS_ID = (0x00000E74),
 .TDC_DAID_ID = (0x00000E76),
 .TDC_FBWCTL_ID = (0x00000E78),
 .TDC_PPCTL_ID = (0x00000E7A),
 .TDC_FBWSTS_ID = (0x00000E7C),
 .TDC_PPSTS_ID = (0x00000E7E),
 .TDC_CTL1_ID = (0x00000E80),
 .TDC_PDBG_ID = (0x00000E82),
 .TDC_PSDU_ACKSTS_ID = (0x00000E84),
 .TDC_AGG_RDYSTS_ID = (0x00000E86),
 .TDC_AGGLEN_OVR_L_ID = (0x00000E88),
 .TDC_AGGLEN_OVR_H_ID = (0x00000E8A),
 .TDC_DROPUSR_ID = (0x00000E8C),
 .TDC_DURMARGIN_ID = (0x00000E8E),
 .TDC_DURTHRESH_ID = (0x00000E90),
 .TDC_MAXTXOP_ID = (0x00000E92),
 .TDC_RXPPET_ID = (0x00000E94),
 .TDC_AGG0TXOP_ID = (0x00000E96),
};
static const d11regs_regs_common_ge129_t regs_common_ge129_132 = {
 .SCTSTP_MASK_L_ID = (0x00000A2A),
 .SCTSTP_MASK_H_ID = (0x00000A2C),
 .SCTSTP_VAL_L_ID = (0x00000A2E),
 .SCTSTP_VAL_H_ID = (0x00000A30),
 .SCTSTP_HMASK_L_ID = (0x00000A32),
 .SCTSTP_HMASK_H_ID = (0x00000A34),
 .SCTSTP_HVAL_L_ID = (0x00000A36),
 .SCTSTP_HVAL_H_ID = (0x00000A38),
 .SMP_CTRL2_ID = (0x00000A28),
 .PSM_GpioMonMemPtr_ID = (0x0000036c),
 .PSM_GpioMonMemData_ID = (0x00000370),
 .PSM_XMT_TPLDATA_L_ID = (0x00000F20),
 .PSM_XMT_TPLDATA_H_ID = (0x00000F22),
 .PSM_XMT_TPLPTR_ID = (0x00000F24),
 .PSM_SMP_CTRL_ID = (0x00000F26),
 .PSM_SCT_MASK_L_ID = (0x00000F28),
 .PSM_SCT_MASK_H_ID = (0x00000F2A),
 .PSM_SCT_VAL_L_ID = (0x00000F2C),
 .PSM_SCT_VAL_H_ID = (0x00000F2E),
 .PSM_SCTSTP_MASK_L_ID = (0x00000F30),
 .PSM_SCTSTP_MASK_H_ID = (0x00000F32),
 .PSM_SCTSTP_VAL_L_ID = (0x00000F34),
 .PSM_SCTSTP_VAL_H_ID = (0x00000F36),
 .PSM_SCX_MASK_L_ID = (0x00000F38),
 .PSM_SCX_MASK_H_ID = (0x00000F3A),
 .PSM_SCM_MASK_L_ID = (0x00000F3C),
 .PSM_SCM_MASK_H_ID = (0x00000F3E),
 .PSM_SCM_VAL_L_ID = (0x00000F40),
 .PSM_SCM_VAL_H_ID = (0x00000F42),
 .PSM_SCS_MASK_L_ID = (0x00000F44),
 .PSM_SCS_MASK_H_ID = (0x00000F46),
 .PSM_SCP_CURPTR_ID = (0x00000F48),
 .PSM_SCP_STRTPTR_ID = (0x00000F4A),
 .PSM_SCP_STOPPTR_ID = (0x00000F4C),
 .PSM_SMP_CTRL2_ID = (0x00000F4E),
 .PSM_BRWK_4_ID = (0x0000083A),
 .IFS_STAT_CRS_ID = (0x000006AE),
};
static const d11regs_regs_common_ge130_t regs_common_ge130_132 = {
 .TXE_HTC_LOC_ID = (0x0000095A),
 .TXE_HTC_L_ID = (0x0000095C),
 .TXE_HTC_H_ID = (0x0000095E),
 .BMCHWC_CNT_SEL_ID = (0x000009DC),
 .BMCHWC_CNT_ID = (0x000009DE),
 .BMCHWC_CTL_ID = (0x000009E8),
 .BMC_FRM2MPM_PRGRSTS_ID = (0x000009EA),
 .BMC_PSMCMD8_ID = (0x000009F4),
 .TXE_MEND_SIG_ID = (0x00001004),
 .TXE_MENDLAST_STATUS_ID = (0x00000BFA),
 .TXE_BMC_PSMCMDQ_CTL_ID = (0x00000A1E),
 .TXE_BMC_ERRSTSEN_ID = (0x00000A20),
 .TX_PREBM_FATAL_ERRVAL_ID = (0x00000A22),
 .TX_PREBM_FATAL_ERRMASK_ID = (0x00000A24),
 .SCS_TAIL_CNT_ID = (0x00000A26),
 .TXE_BMC_ERRSTS_ID = (0x00000B72),
 .XMTDMA_FATAL_ERR_RPTEN_ID = (0x00000BC6),
 .XMTDMA_FATAL_ERR_ID = (0x00000BFE),
};
static const d11regs_regs_common_ge131_t regs_common_ge131_132 = {
 .XMTDMA_PROGERR_ID = (0x00000BF8),
};
static const d11regs_regs_common_ge132_t regs_common_ge132_132 = {
 .ctdma_ctl_ID = (0x00000058),
 .MAC_BOOT_CTRL_ID = (0x0000014C),
 .MacCapability2_ID = (0x0000019C),
 .RxDmaXferCnt_ID = (0x00000374),
 .RxDmaXferCntMax_ID = (0x00000378),
 .RxFIFOCnt_ID = (0x0000037C),
 .AMT_START_ID = (0x000004E4),
 .AMT_END_ID = (0x000004E6),
 .AMT_MATCH_A1_ID = (0x000004EC),
 .AMT_MATCH_A2_ID = (0x000004EE),
 .AMT_MATCH_A3_ID = (0x000004F0),
 .AMT_MATCH_BSSID_ID = (0x000004F2),
 .FP_PAT1A_ID = (0x00000504),
 .PSM_USR_SEL_ID = (0x000007AE),
 .PSM_FATAL_STS_ID = (0x000007D8),
 .PSM_FATAL_MASK_ID = (0x000007DA),
 .HWA_MACIF_CTL1_ID = (0x000007DE),
 .PSM_BASE_24_ID = (0x00000870),
 .PSM_BASE_25_ID = (0x00000872),
 .PSM_BASE_26_ID = (0x00000874),
 .PSM_BASE_27_ID = (0x00000876),
 .PSM_M2DMA_ADDR_ID = (0x000008F4),
 .PSM_M2DMA_DATA_ID = (0x000008F6),
 .PSM_M2DMA_FREE_ID = (0x000008F8),
 .PSM_M2DMA_CFG_ID = (0x000008FA),
 .PSM_M2DMA_DATA_L_ID = (0x000008FC),
 .PSM_M2DMA_DATA_H_ID = (0x000008FE),
 .TXE_SHMDMA_PHYSTSADDR_ID = (0x00000968),
 .TXE_SHMDMA_MACFIFOADDR_ID = (0x0000096A),
 .PLCP_CC34_LEN_ID = (0x00000A7A),
 .TXE_XMTDMA_SW_RSTCTL_ID = (0x00000BCA),
 .XMTDMA_ACTUSR_DBGCTL_ID = (0x00000BF2),
 .XMTDMA_ACTUSR_VLDSTS_ID = (0x00000BF4),
 .XMTDMA_ACTUSR_QID_ID = (0x00000BF6),
 .XMTDMA_AQM_ACT_CNT_ID = (0x00000BFC),
 .AQM_MSDUDEF0_ID = (0x00000C0A),
 .AQM_MSDUDEF1_ID = (0x00000C0C),
 .AQM_PSMTXDSZ_ID = (0x00000C10),
 .AQM_NDLIM_ID = (0x00000C20),
 .AQM_MAXNDLIMLEN_ID = (0x00000C22),
 .AQM_FRLENLIMIT_ID = (0x00000C28),
 .AQM_FRFIXLEN_ID = (0x00000C2A),
 .AQM_2TDCCTL_ID = (0x00000C96),
 .AQM_QFRTXCNT_ID = (0x00000C5A),
 .AQM_TOTACKCNT_ID = (0x00000C88),
 .AQM_AGGR_PLDRDY_ID = (0x00000C9C),
 .AQM_AUTOBQC_EN_ID = (0x00000CE6),
 .AQM_AUTOBQC_ID = (0x00000CE8),
 .AQM_AUTOBQC_TO_ID = (0x00000CEA),
 .AQM_AUTOBQC_TOSTS_ID = (0x00000CEC),
 .AQM_AUTOBQC_TSCALE_ID = (0x00000CEE),
 .AQM_AUTOBQC_MAXTCNT_ID = (0x00000CF0),
 .AQMHWC_INPROG_ID = (0x00000D58),
 .AQMHWC_STATUS_ID = (0x00000D5A),
 .AQM_DBGCTL1_ID = (0x00000DB2),
 .AQM_ERRSTSEN_ID = (0x00000DC2),
 .AQMFTM_CTL_ID = (0x00000DF2),
 .AQMFTM_MPDULEN_ID = (0x00000DF4),
 .AQMFTM_FLAG_ID = (0x00000DF6),
 .AQM_AUTOBQC_CONSDLY_ID = (0x00000DF8),
 .AQM_MUENGCTL_ID = (0x00000DFA),
 .TDC_NDLIM_ID = (0x00000E98),
 .TDC_MAXNDLIMLEN_ID = (0x00000E9A),
 .TDC_CC34LEN_ID = (0x00000E9C),
 .TDC_NMURU_ID = (0x00000E9E),
 .TDC_MIMOSTS_ID = (0x00000EA0),
 .PSM_SCS_TAIL_CNT_ID = (0x00000F50),
 .TXE_BMC_CMDPUSH_CNT_ID = (0x00001000),
 .TXE_BMC_CMDPOP_CNT_ID = (0x00001002),
 .TXE_RST_TXMEND_ID = (0x00001006),
 .TXE_BMC_BQSEL_ID = (0x00001008),
 .TXE_BMC_WBCNT0_ID = (0x0000100A),
 .TXE_BMC_WBCNT1_ID = (0x0000100C),
 .TXE_XMTDMA_CNTSTS_CTL_ID = (0x0000100E),
 .XMTDMA_QREQBYTE_CNT0_ID = (0x00001010),
 .XMTDMA_QREQBYTE_CNT1_ID = (0x00001012),
 .XMTDMA_QREQ_MPDU_CNT_ID = (0x00001014),
 .XMTDMA_UREQBYTE_CNT0_ID = (0x00001016),
 .XMTDMA_UREQBYTE_CNT1_ID = (0x00001018),
 .XMTDMA_QRESPBYTE_CNT0_ID = (0x0000101A),
 .XMTDMA_QRESPBYTE_CNT1_ID = (0x0000101C),
 .XMTDMA_QRESP_MPDU_CNT_ID = (0x0000101E),
 .XMTDMA_DATAPRI_STS0_ID = (0x00001020),
 .XMTDMA_DATAPRI_STS1_ID = (0x00001022),
 .TXE_MEND_CTL_ID = (0x00001026),
 .TXE_MENDLAST_SIG_ID = (0x00001028),
 .TXE_BMC_WFCNT_ID = (0x0000102A),
 .TXE_SIGBCC3_BASE_ID = (0x0000102C),
 .TXE_SIGBCC4_BASE_ID = (0x0000102E),
 .TXE_BMC_EOFCNT_ID = (0x00001030),
 .TXE_PPR_CFG0_ID = (0x00001180),
 .TXE_PPR_CFG1_ID = (0x00001182),
 .TXE_PPR_CFG2_ID = (0x00001184),
 .TXE_PPR_CFG3_ID = (0x00001186),
 .TXE_PPR_CFG4_ID = (0x00001188),
 .TXE_PPR_CFG5_ID = (0x0000118A),
 .TXE_PPR_CFG6_ID = (0x0000118C),
 .TXE_PPR_CFG7_ID = (0x0000118E),
 .TXE_PPR_CFG8_ID = (0x00001190),
 .TXE_PPR_CFG9_ID = (0x00001192),
 .TXE_PPR_CFG10_ID = (0x00001194),
 .TXE_PPR_CFG11_ID = (0x00001196),
 .TXE_PPR_CFG12_ID = (0x00001198),
 .TXE_PPR_CFG13_ID = (0x0000119A),
 .TXE_PPR_CFG14_ID = (0x0000119C),
 .TXE_PPR_CFG15_ID = (0x0000119E),
 .TXE_PPR_CFG16_ID = (0x000011A0),
 .TXE_PPR_CTL0_ID = (0x000011A2),
 .TXE_PPR_CTL1_ID = (0x000011A4),
 .TXE_PPR_CTL2_ID = (0x000011A6),
 .TXE_PPR_STAT0_ID = (0x000011AA),
 .TXE_PPR_STAT1_ID = (0x000011AC),
 .TXE_PPR_STAT2_ID = (0x000011AE),
 .TXE_PPR_STAT3_ID = (0x000011B0),
 .RXE_IFEVENTDBG_CTL_ID = (0x00001200),
 .RXE_IFEVENTDBG_STAT_ID = (0x00001202),
 .RXE_MEND_FLAT_ID = (0x00001204),
 .RXE_XFERACT_FLAT_ID = (0x00001206),
 .RXE_PHYFIFO_NOT_EMPTY_FLAT_ID = (0x00001208),
 .RXE_PFPLCP_WRDCNT_ID = (0x0000120A),
 .RXE_RXDATA_NOT_EMPTY_FLAT_ID = (0x0000120C),
 .RXE_PHYSTS_SHM_CTL_ID = (0x0000120E),
 .RXE_RXBM_FATAL_ERRVAL_ID = (0x00001210),
 .RXE_RXBM_FATAL_ERRMASK_ID = (0x00001212),
 .RXE_PREBM_FATAL_ERRVAL_ID = (0x00001214),
 .RXE_PREBM_FATAL_ERRMASK_ID = (0x00001216),
 .RXE_CTXSTSFIFO_FATAL_ERRVAL_ID = (0x00001218),
 .RXE_CTXSTSFIFO_FATAL_ERRMASK_ID = (0x0000121A),
 .RXE_POSTBM_FATAL_ERRVAL_ID = (0x0000121C),
 .RXE_POSTBM_FATAL_ERRMASK_ID = (0x0000121E),
 .RXQ_FATAL_ERR_RPTEN_ID = (0x00001220),
 .RXQ_FATAL_ERR_ID = (0x00001222),
 .RXE_RXFRMUP_TSF_L_ID = (0x00001240),
 .RXE_RXFRMUP_TSF_H_ID = (0x00001242),
 .RXE_RXFRMDN_TSF_L_ID = (0x00001244),
 .RXE_RXFRMDN_TSF_H_ID = (0x00001246),
 .RXE_RSF_NP_STATS_ID = (0x00001248),
 .RXE_RSF_HP_STATS_ID = (0x0000124A),
 .RXE_POSTBM_TIMEOUT_ID = (0x0000124C),
 .RDF_CTL8_ID = (0x0000124E),
 .FP_DEBUG_ID = (0x00001250),
 .RXE_RXE2WEP_BYTES_ID = (0x00001252),
 .RXE_WEP2FP_BYTES_ID = (0x00001254),
 .RXE_RXE2BM_BYTES_ID = (0x00001256),
 .RXE_WCS_HDR_THRESHOLD_ID = (0x00001258),
 .RXE_WCS_MIN_THRESHOLD_ID = (0x0000125A),
 .RXE_WCS_MAX_THRESHOLD_ID = (0x0000125C),
 .RXE_DAGG_DEBUG_ID = (0x0000125E),
 .RXE_BFDRPT_XFERPEND_ID = (0x00001260),
 .RXE_BFDRPT_XFERRD_ID = (0x00001262),
 .RXE_BFDRPT_XFERSTS_ID = (0x00001264),
 .RXE_BFDRPT_CAPSTS_ID = (0x00001266),
 .RXE_WCS_TOUT_THRESHOLD_ID = (0x00001268),
 .RXE_WCS_TOUT_STATUS_ID = (0x0000126A),
 .RXE_WCS_CTL_ID = (0x0000126C),
 .RXE_BMCLOOPBACK_DISCARD_ID = (0x0000126E),
 .RXE_STRMRD_THRESHOLD_ID = (0x00001270),
 .RXE_RCM_XFERBMP_ID = (0x00001272),
 .RXE_WCS_COUNTERS_ID = (0x00001274),
 .RXE_WCS_DEBUG_ID = (0x00001276),
 .FP_STATUS6_ID = (0x00001278),
 .FP_ACKTYPETID_BITMAP_ID = (0x0000127A),
 .RXE_PHYSTS_DEBUG1_ID = (0x0000127C),
 .RXE_PHYSTS_DEBUG2_ID = (0x0000127E),
 .RXE_PHYSTS_ADDR_ID = (0x00001280),
 .RXE_PHYSTS_DATA_ID = (0x00001282),
 .RXE_PHYSTS_FREE_ID = (0x00001284),
 .RXE_PHYSTS_CFG_ID = (0x00001286),
 .RXE_PHYSTS_DATA_L_ID = (0x00001288),
 .RXE_PHYSTS_DATA_H_ID = (0x0000128A),
 .CBR_DBG_DATA_ID = (0x00001312),
 .PHYFIFO_SIZE_CFG_ID = (0x00001314),
 .PHYFIFO_SIZE_CFG1_ID = (0x00001316),
 .FP_MUBAR_CONFIG_ID = (0x00001318),
 .FP_MUBAR_TYPE_0_ID = (0x0000131A),
 .FP_MUBAR_TYPE_1_ID = (0x0000131C),
 .FP_MUBAR_TYPE_2_ID = (0x0000131E),
 .FP_MUBAR_TYPE_3_ID = (0x00001320),
 .RXE_EOFPD_CNT_ID = (0x00001322),
 .RXE_EOFPD_THRSH_ID = (0x00001324),
 .RXE_AGGLEN_EST_ID = (0x00001326),
 .RXE_PFWRDCNT_CTL_ID = (0x00001328),
 .RXE_OPMODE_CFG_ID = (0x0000132A),
 .RXE_DBG_CTL_ID = (0x0000132C),
 .POSTBM_DBG_STS_ID = (0x0000132E),
 .PREBM_DBG_STS_ID = (0x00001330),
 .RXE_RCVCTL2_ID = (0x00001332),
 .PSM_CHK0_EADDR_ID = (0x00000892),
 .PSM_CHK1_EADDR_ID = (0x0000089E),
};
