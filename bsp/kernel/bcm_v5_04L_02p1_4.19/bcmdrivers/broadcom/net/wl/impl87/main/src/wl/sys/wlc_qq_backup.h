
typedef struct ampdu_tx_info ampdu_tx_info_t;
#ifdef PKTQ_LOG
typedef struct scb_ampdu_dpdbg{
    uint32 txmpdu_succ_per_ft[AMPDU_PPDU_FT_MAX];    /**< succ mpdus tx per frame type */
    uint32 txru_succ[MAC_LOG_MU_RU_MAX];
    uint32 txmimo_succ[MAC_LOG_MU_MIMO_USER_MAX];
    uint32 time_lo;
    uint32 time_hi;
} scb_ampdu_dpdbg_t;
#endif /* PKTQ_LOG */

#ifdef AMPDU_NON_AQM
/** structure to store per-tid state for the ampdu initiator */
typedef struct scb_ampdu_tid_ini_non_aqm {
    uint8 txretry[AMPDU_BA_MAX_WSIZE];       /**< tx retry count; indexed by seq modulo */
    uint8 ackpending[AMPDU_BA_MAX_WSIZE/NBBY]; /**< bitmap: set if ack is pending */
    uint8 barpending[AMPDU_BA_MAX_WSIZE/NBBY]; /**< bitmap: set if bar is pending */
    uint16 rem_window;               /**< !AQM only: remaining ba window (in pdus)
                            *    that can be txed.
                            */
    uint16 retry_seq[AMPDU_BA_MAX_WSIZE];       /**< seq of released retried pkts */
    uint16 retry_head;               /**< head of queue ptr for retried pkts */
    uint16 retry_tail;               /**< tail of queue ptr for retried pkts */
    uint16 retry_cnt;               /**< cnt of retried pkts */
} scb_ampdu_tid_ini_non_aqm_t;
#endif /* AMPDU_NON_AQM */

struct scb_ampdu_tid_ini {
    uint8 ba_state;        /**< ampdu ba state */
    uint8 ba_wsize;        /**< negotiated ba window size (in pdu) */
    uint8 tid;        /**< initiator tid for easy lookup */
    bool alive;        /**< true if making forward progress */
    uint16 tx_in_transit;    /**< #packets have left the AMPDU module and haven't been freed */
    uint16 barpending_seq;    /**< seqnum for bar */
    uint16 acked_seq;    /**< last ack received */
    uint16 start_seq;    /**< seqnum of first unack'ed mpdu, increments when window moves */
    uint16 max_seq;        /**< max unacknowledged seqnum released towards hardware */
    uint16 tx_exp_seq;    /**< next exp seq in sendampdu */
    uint16 next_enq_seq;    /**< last pkt seq that has been sent to txfifo */
    uint16 bar_ackpending_seq; /**< seqnum of bar for which ack is pending */
    bool bar_ackpending;    /**< true if there is a bar for which ack is pending */
    uint8 retry_bar;    /**< reason code if bar to be retried at watchdog */
    uint8 bar_cnt;        /**< number of bars sent with no progress */
    uint8 dead_cnt;        /**< number of sec without the window moving */
    struct scb *scb;    /**< backptr for easy lookup */
    uint32    last_addba_ts;    /**< timestamp of last addba req sent */

    uint16 suppr_window;    /**< suppr packet count inside ba window, including reg mpdu's */

    uint8 addba_req_cnt;    /**< number of addba_req sent with no progress */
    uint8 cleanup_ini_cnt;    /**< number of sec waiting in pending off state */
    uint16 off_cnt;        /**< number of sec in off state before next try */
#ifdef AMPDU_NON_AQM
    scb_ampdu_tid_ini_non_aqm_t *non_aqm;
#endif /* AMPDU_NON_AQM */
#ifdef WLATF_BARE
    atf_state_t atf_state; /**< per-tid ATF state information */
#endif /* WLATF_BARE */
#ifdef PKTQ_LOG
    scb_ampdu_dpdbg_t * ampdu_dpdbg;
#endif
};

#ifdef WLOVERTHRUSTER
typedef struct wlc_tcp_ack_info {
	uint8 tcp_ack_ratio;
	uint32 tcp_ack_total;
	uint32 tcp_ack_dequeued;
	uint32 tcp_ack_multi_dequeued;
	uint32 current_dequeued;
	uint8 tcp_ack_ratio_depth;
} wlc_tcp_ack_info_t;
#endif

typedef struct {
	uint32 mpdu_histogram[AMPDU_MAX_MPDU+1];	/**< mpdus per ampdu histogram */
	/* reason for suppressed err code as reported by ucode/aqm, see enum 'TX_STATUS_SUPR...' */
	uint32 supr_reason[NUM_TX_STATUS_SUPR];

	/* txmcs in sw agg is ampdu cnt, and is mpdu cnt for mac agg */
	uint32 txmcs[AMPDU_HT_MCS_ARRAY_SIZE];		/**< mcs of tx pkts */
	uint32 txmcssgi[AMPDU_HT_MCS_ARRAY_SIZE];	/**< mcs of tx pkts */
	uint32 txmcsstbc[AMPDU_HT_MCS_ARRAY_SIZE];	/**< mcs of tx pkts */

	/* used by aqm_agg to get PER */
	uint32 txmcs_succ[AMPDU_HT_MCS_ARRAY_SIZE];	/**< succ mpdus tx per mcs */

#ifdef WL11AC
	uint32 txvht[BW_MAXMHZ][AMPDU_MAX_VHT];			/**< vht of tx pkts */
	uint32 txvhtsgi[AMPDU_MAX_VHT];			/**< vht of tx pkts */
	uint32 txvhtstbc[AMPDU_MAX_VHT];		/**< vht of tx pkts */

	/* used by aqm_agg to get PER */
	uint32 txvht_succ[BW_MAXMHZ][AMPDU_MAX_VHT];		/**< succ mpdus tx per vht */
#endif /* WL11AC */
#ifdef WL11AX
	uint32 txhe[BW_MAXMHZ][AMPDU_MAX_HE_GI][AMPDU_MAX_HE]; /**< HE TX pkt count per GI */
	uint32 txhestbc[AMPDU_MAX_HE];

	/* used by aqm_agg to get PER */
	uint32 txhe_succ[BW_MAXMHZ][AMPDU_MAX_HE_GI][AMPDU_MAX_HE]; /**< succ mpdus tx per he */
#endif /* WL11AX */

	uint32 txmpdu_per_ft[AMPDU_PPDU_FT_MAX];	/**< tot mpdus tx per frame type */
	uint32 txmpdu_succ_per_ft[AMPDU_PPDU_FT_MAX];	/**< succ mpdus tx per frame type */

#ifdef AMPDU_NON_AQM
	uint32 retry_histogram[AMPDU_MAX_MPDU+1];	/**< histogram for retried pkts */
	uint32 end_histogram[AMPDU_MAX_MPDU+1];		/**< errs till end of ampdu */
#endif /* AMPDU_NON_AQM */
} ampdu_dbg_t;

/** ampdu related transmit stats */
typedef struct wlc_ampdu_cnt {
    /* initiator stat counters */
    uint32 txampdu;        /**< ampdus sent */
#ifdef WLCNT
    uint32 txmpdu;        /**< mpdus sent */
    uint32 txregmpdu;    /**< regular(non-ampdu) mpdus sent */
    union {
        uint32 txs;        /**< MAC agg: txstatus received */
        uint32 txretry_mpdu;    /**< retransmitted mpdus */
    } u0;
    uint32 txretry_ampdu;    /**< retransmitted ampdus */
    uint32 txfifofull;    /**< release ampdu due to insufficient tx descriptors */
    uint32 txfbr_mpdu;    /**< retransmitted mpdus at fallback rate */
    uint32 txfbr_ampdu;    /**< retransmitted ampdus at fallback rate */
    union {
        uint32 txampdu_sgi;    /**< ampdus sent with sgi */
        uint32 txmpdu_sgi;    /**< ucode agg: mpdus sent with sgi */
    } u1;
    union {
        uint32 txampdu_stbc;    /**< ampdus sent with stbc */
        uint32 txmpdu_stbc;    /**< ucode agg: mpdus sent with stbc */
    } u2;
    uint32 txampdu_mfbr_stbc; /**< ampdus sent at mfbr with stbc */
    uint32 txrel_wm;    /**< mpdus released due to lookahead wm (water mark) */
    uint32 txrel_size;    /**< mpdus released due to max ampdu size (in mpdu's) */
    uint32 sduretry;    /**< sdus retry returned by sendsdu() */
    uint32 sdurejected;    /**< sdus rejected by sendsdu() */
    uint32 txdrop;        /**< dropped packets */
    uint32 txr0hole;    /**< lost packet between scb and sendampdu */
    uint32 txrnhole;    /**< lost retried pkt */
    uint32 txrlag;        /**< laggard pkt (was considered lost) */
    uint32 txreg_noack;    /**< no ack for regular(non-ampdu) mpdus sent */
    uint32 txaddbareq;    /**< addba req sent */
    uint32 rxaddbaresp;    /**< addba resp recd */
    uint32 txlost;        /**< lost packets reported in txs */
    uint32 txbar;        /**< bar sent */
    uint32 rxba;        /**< ba recd */
    uint32 noba;        /**< ba missing */
    uint32 nocts;        /**< cts missing after rts */
    uint32 txstuck;        /**< watchdog bailout for stuck state */
    uint32 orphan;        /**< orphan pkts where scb/ini has been cleaned */

    uint32 epochdelta;    /**< How many times epoch has changed */
    uint32 echgr1;          /**< epoch change reason -- plcp */
    uint32 echgr2;          /**< epoch change reason -- rate_probe */
    uint32 echgr3;          /**< epoch change reason -- a-mpdu as regmpdu */
    uint32 echgr4;          /**< epoch change reason -- regmpdu */
    uint32 echgr5;          /**< epoch change reason -- dest/tid */
    uint32 echgr6;          /**< epoch change reason -- seq no */
    uint32 echgr7;          /**< epoch change reason -- htc+ */
#ifdef WLTAF
    uint32 echgr8;          /**< epoch change reason -- TAF star marker */
#endif
    uint32 echgr9;        /**< epoch change reason -- D11AC_TXC_PPS */
    uint32 tx_mrt, tx_fbr;  /**< number of MPDU tx at main/fallback rates */
    uint32 txsucc_mrt;      /**< number of successful MPDU tx at main rate */
    uint32 txsucc_fbr;      /**< number of successful MPDU tx at fallback rate */
    uint32 enq;             /**< totally enqueued into aggfifo */
    uint32 cons;            /**< totally reported in txstatus */
    uint32 pending;         /**< number of entries currently in aggfifo or txsq */

    /* general: both initiator and responder */
    uint32 rxunexp;        /**< unexpected packets */
    uint32 txdelba;        /**< delba sent */
    uint32 rxdelba;        /**< delba recd */

    uint32 ampdu_wds;       /**< AMPDU watchdogs */

#ifdef WLPKTDLYSTAT
    /* PER (per mcs) statistics */
    uint32 txmpdu_cnt[AMPDU_HT_MCS_ARRAY_SIZE];        /**< MPDUs per mcs */
    uint32 txmpdu_succ_cnt[AMPDU_HT_MCS_ARRAY_SIZE];    /**< acked MPDUs per MCS */
#ifdef WL11AC
    uint32 txmpdu_vht_cnt[AMPDU_MAX_VHT];            /**< MPDUs per vht */
    uint32 txmpdu_vht_succ_cnt[AMPDU_MAX_VHT];         /**< acked MPDUs per vht */
#endif /* WL11AC */
#ifdef WL11AX
    uint32 txmpdu_he_cnt[AMPDU_MAX_HE];            /**< MPDUs per HE */
    uint32 txmpdu_he_succ_cnt[AMPDU_MAX_HE];        /**< acked MPDUs per HE */
#endif /* WL11AX */
#endif /* WLPKTDLYSTAT */

#ifdef WL_CS_PKTRETRY
    uint32 cs_pktretry_cnt;
#endif
    uint32    txampdubyte_h;        /* tx ampdu data bytes */
    uint32    txampdubyte_l;
#endif /* WLCNT */
} wlc_ampdu_tx_cnt_t;

/** this struct is not used in case of host aggregation */
typedef struct ampdumac_info {
	uint8 epoch;
	bool change_epoch;
	/* any change of following elements will change epoch */
	struct scb *prev_scb;
	uint8 prev_tid;
	uint8 prev_ft;		/**< eg AMPDU_11VHT */
	uint16 prev_txphyctl0, prev_txphyctl1;
	/* To keep ordering consistent with pre-rev40 prev_plcp[] use,
	 * plcp to prev_plcp mapping is not straightforward
	 *
	 * prev_plcp[0] holds plcp[0] (all revs)
	 * prev_plcp[1] holds plcp[3] (all revs)
	 * prev_plcp[2] holds plcp[2] (rev >= 40)
	 * prev_plcp[3] holds plcp[1] (rev >= 40)
	 * prev_plcp[4] holds plcp[4] (rev >= 40)
	 */
	uint8 prev_plcp[5];

	/* stats */
	int in_queue;
	uint8 depth;
	uint8 prev_shdr;
	uint8 prev_cache_gen;	/* Previous cache gen number */
	bool prev_htc;
	bool prev_pps;		/* True if prev packet had D11AC_TXC_PPS bit set */
} ampdumac_info_t;

/** ampdu config info, mostly config information that is common across WLC */
typedef struct ampdu_tx_config {
    bool   manual_mode;            /**< addba tx to be driven by user */
    bool   no_bar;                  /**< do not send bar on failure */
    uint8  ini_enable[AMPDU_MAX_SCB_TID]; /**< per-tid initiator enable/disable of ampdu */
    uint8  ba_policy;               /**< ba policy; immediate vs delayed */
    uint8  ba_max_tx_wsize;         /**< Max Tx ba window size (in pdu) used at attach time */
    uint8  ba_tx_wsize;             /**< Tx ba window size (in pdu) (up to ba_max_tx_wsize) */
    int8   max_pdu;                 /**< max pdus allowed in ampdu (up to ba_tx_wsize) */
    uint8  probe_mpdu;              /**< max mpdus allowed in ampdu for probe pkts */
    uint8  mpdu_density;            /**< min mpdu spacing (0-7) ==> 2^(x-1)/8 usec */
    uint16 dur;                     /**< max duration of an ampdu (in usec) */
    uint16 addba_retry_timeout;     /* Retry timeout for addba requests (ms) */
    uint8  delba_timeout;           /**< timeout after which to send delba (sec) */

    int8   ampdu_aggmode;           /**< aggregation mode, HOST or MAC */
    int8   default_pdu;             /**< default pdus allowed in ampdu */
    bool   fb_override;        /* override for frame bursting */
    bool   fb_override_enable;      /**< configuration to enable/disable ampd_no_frameburst */
    bool   btcx_dur_flag;           /* TRUE if BTCOEX needs TX-AMPDU clamped to 2.5ms */

#ifdef WLATF_BARE
    uint  txq_time_allowance_us;
    uint  txq_time_min_allowance_us;
    /* AMPDU atf low water mark release count threshold */
    uint    ampdu_atf_lowat_rel_cnt;
#endif /* WLATF_BARE */

    uint8 release;          /**< # of mpdus released at a time */
    uint8 tx_rel_lowat;     /**< low watermark for num of pkts in transit */
    uint8 txpkt_weight;     /**< weight of ampdu in txfifo; reduces rate lag */

#ifdef AMPDU_NON_AQM
    uint8 hiagg_mode;    /**< agg mpdus with different retry cnt */
    uint8 retry_limit;    /**< mpdu transmit retry limit */
    uint8 rr_retry_limit;    /**< mpdu transmit retry limit at regular rate */
    uint8 retry_limit_tid[AMPDU_MAX_SCB_TID];    /**< per-tid mpdu transmit retry limit */
    /* per-tid mpdu transmit retry limit at regular rate */
    uint8 rr_retry_limit_tid[AMPDU_MAX_SCB_TID];

    uint32 ffpld_rsvd;    /**< number of bytes to reserve for preload */
#if defined(WLPROPRIETARY_11N_RATES)
    uint32 max_txlen[AMPDU_HT_MCS_ARRAY_SIZE][2][2]; /**< max size of ampdu per [mcs,bw,sgi] */
#else
    uint32 max_txlen[MCS_TABLE_SIZE][2][2];
#endif /* WLPROPRIETARY_11N_RATES */

    bool mfbr;        /**< enable multiple fallback rate */
    uint32 tx_max_funl;     /**< underflows should be kept such that
                             * (tx_max_funfl*underflows) < tx frames
                             */
    wlc_fifo_info_t fifo_tb[NUM_FFPLD_FIFO]; /**< table of fifo infos  */

    uint8  aggfifo_depth;   /**< soft capability of AGGFIFO */
#endif /* non-AQM */
    /* dynamic frameburst variables */
    uint8  dyn_fb_rtsper_on;
    uint8  dyn_fb_rtsper_off;
    uint32 dyn_fb_minrtscnt;

} ampdu_tx_config_t;

/* DBG and counters are replicated per WLC */
/** AMPDU tx module specific state */
struct ampdu_tx_info {
	wlc_info_t	*wlc;		/**< pointer to main wlc structure */
	int		scb_handle;	/**< scb cubby handle to retrieve data from scb */
	int		bsscfg_handle;	/**< BSSCFG cubby offset */
#ifdef WLCNT
	wlc_ampdu_tx_cnt_t *cnt;	/**< counters/stats */
#endif
	ampdu_dbg_t	*amdbg;
	mac_dbg_t	*mdbg;
	struct ampdu_tx_config *config;
	/* dynamic frameburst variables */
	uint32		rtstxcnt;	/**< # rts sent */
	uint32		ctsrxcnt;	/**< # cts received */
	uint8		rtsper_avg;	/**< avg rtsper for stats */
	bool		cfp_head_frame;	/**< Indicate head frame in a chain of packets */
	uint32		tx_intransit;	/**< over *all* remote parties */
#ifdef WLOVERTHRUSTER
	wlc_tcp_ack_info_t tcp_ack_info; /**< stores a mix of config & runtime */
#endif
	bool		txaggr_support;	/**< Support ampdu tx aggregation */
	ampdumac_info_t	hwagg[NFIFO_EXT_MAX]; /**< not used in case of host aggregation */
	uint16		aqm_max_release[AMPDU_MAX_SCB_TID];
#ifdef WL_CS_PKTRETRY
	bool		cs_pktretry;	/**< retry retired during channel switch packets */
#endif
};


#include <wl_linux.h>
#include <wlc_hw.h>
#include "../../shared/hnddma_priv.h"
#include <phy_rssi_api.h>
#include <wlc_qq_struct.h>





/*之前在每次用到链表时都加锁，太浪费时间。
如今将代码分为三种：
1.尾部追加
2.头部删减
3.中间读取和删改
只要保证对链表的这三个操作互斥就可以
为了使逻辑更简单，设置只会在头部删减，而且只有当链表长度大于10时才删减，从而避免出现首尾相互影响的情况。
为了避免中间读取时出现问题，可以单独针对头部和尾部加锁,根据len参数，当读取到头部和尾部时加锁，一旦确定不是头部和尾部，就去掉锁。
具体锁逻辑：
定义两个互斥锁（一个头一个尾）一个读写锁。
所有对于pkt_qq_chain_len的操作全都上锁
每次尾部追加均加pkt_qq_mutex_tail锁
每次删除操作和读写已有节点均追加pkt_qq_mutex_head锁
为了减少删除操作和读写已有节点互斥造成的等待，在删除前判断是否锁已被使用，若是就跳过删除操作。
*/



struct pkt_qq *pkt_qq_chain_head = (struct pkt_qq *)NULL;
struct pkt_qq *pkt_qq_chain_tail = (struct pkt_qq *)NULL;

static struct pkt_qq *pkt_qq_last;/*用于记录上次搜索到的数据包，减少遍历搜索时间*/
static uint16 index_last;/*用于记录上次搜索到的数据包所在编号*/

uint16 pkt_qq_chain_len = 0;
uint16 max_pkt_qq_chain_len = 666;
uint16 pkt_qq_ddl = 666;
uint16 pkt_phydelay_dict_len = 30;
uint16 pkt_phydelay_dict_step = 10;
uint32 pkt_phydelay_dict[30] = {0};//记录不同时延情况下的pkt数量
/*
for(i = 0; i<pkt_phydelay_dict_len; i++){
    pkt_phydelay_dict[i] = 0;
}*/

//DEFINE_MUTEX(pkt_qq_mutex); // 定义一个互斥锁
DEFINE_MUTEX(pkt_qq_mutex_tail); // 定义一个互斥锁
DEFINE_MUTEX(pkt_qq_mutex_head); // 定义一个互斥锁
DEFINE_RWLOCK(pkt_qq_mutex_len); // 定义一个读写锁

/*记录PS时间
uint32 PS_time_all_cnt = 0;//从开机开始进入PS的时间累加
bool PS_is_on = FALSE;//判断当前是否正处于PS
uint32 PS_start_last = 0;//最近一次进入PS的时间
*/







/*是否打印超时被删除的包，专用于debug*/
#define PRINTTIMEOUTPKT


#ifndef BCMDBG_PPS_qq
#define BCMDBG_PPS_qq
#endif
/* PPS时间统计相关 *//* module private info */
typedef struct wlc_pps_info wlc_pps_info_t;
typedef struct {
	uint32 ps_pretend_start;
	uint32 ps_pretend_probe;
	uint32 ps_pretend_count;
	uint8  ps_pretend_succ_count;
	uint8  ps_pretend_failed_ack_count;
	uint8  reserved[2];
#ifdef BCMDBG_PPS_qq
	uint32 ps_pretend_total_time_in_pps;
	uint32 ps_pretend_suppress_count;
	uint32 ps_pretend_suppress_index;
#endif /* BCMDBG_PPS_qq */
} scb_pps_info_t;

#ifdef BCMDBG_PPS_qq
#define SCB_PPSINFO_LOC(pps, scb) (scb_pps_info_t **)SCB_CUBBY(scb, (pps)->scb_handle)
#define SCB_PPSINFO(pps, scb) *SCB_PPSINFO_LOC(pps, scb)
#endif /* BCMDBG_PPS_qq */




/*用于记录出现重传包重传时，函数调用路径*/
struct pkt_qq debug_qqdx_retry_pkt;// = (struct pkt_qq )NULL;
//debug_qqdx_retry_pkt = (struct pkt_qq *) MALLOCZ(osh, sizeof(pkt_qq_t));
//dl_dbg中被我修改了有关ENABLE_CORECAPTURE的信息，失败



/*统计链表增减节点的各种情况*/
uint32 pkt_qq_chain_len_add = 0;//链表增加
uint32 pkt_qq_chain_len_soft_retry = 0;//记录PPS等原因重传导致的
uint32 pkt_qq_chain_len_add_last = 0;//记录上次链表增加过的数据包数量
uint32 pkt_qq_chain_len_acked = 0;//正常收到ack并删除
uint32 pkt_qq_chain_len_unacked = 0;//正常收到ack并删除
uint32 pkt_qq_chain_len_timeout = 0;//超时删除
uint32 pkt_qq_chain_len_outofrange = 0;//超过链表最大长度删除
uint32 pkt_qq_chain_len_notfound = 0;// 遍历链表没有找到
uint32 pkt_qq_chain_len_found = 0;// 遍历链表没有找到
#define PKTCOUNTCYCLE 100000//每隔100000个包打印一次数据包统计信息
uint32 pkt_added_in_wlc_tx = 0;//wlc_tx文件中实际准备发送的数据包量


struct start_sta_info *start_sta_info_cur;
bool start_game_is_on = FALSE;
/*定时器初始化相关*/
bool timer_qq_loaded = FALSE;

osl_t *osh_timer_callback_start_info_qq;
struct timer_list timer_qq_start_info;
void timer_callback_start_info_qq(struct timer_list *t) {
    info_class_t *start_sta_info_buffer;
    start_sta_info_buffer = (info_class_t *) MALLOCZ(osh_timer_callback_start_info_qq, sizeof(*start_sta_info_buffer));
    debugfs_read_info_qq(3, start_sta_info_buffer);
    //struct timespec start_sta_info_time = start_sta_info_buffer->timestamp;
    kernel_info_t info_qq[DEBUG_CLASS_MAX_FIELD];
    memcpy(info_qq, start_sta_info_buffer->info, sizeof(*start_sta_info_cur));
    //info_qq[0] = start_sta_info_buffer->info;
    memcpy(start_sta_info_cur, info_qq, sizeof(*start_sta_info_cur));
    /*printk("----------[fyl] ac_queue_index(%d)",start_sta_info_cur->ac_queue_index);
    printk("sizeof(*start_sta_info_cur)[%d][%d][%d][%d]\n", sizeof(*start_sta_info_cur)\
	, sizeof(start_sta_info_cur->start_is_on), sizeof(start_sta_info_cur->ea), sizeof(start_sta_info_cur->ac_queue_index));
    printf("MAC address (struct ether_addr): %02x:%02x:%02x:%02x:%02x:%02x\n",
        start_sta_info_cur->ea.ether_addr_octet[0],
        start_sta_info_cur->ea.ether_addr_octet[1],
        start_sta_info_cur->ea.ether_addr_octet[2],
        start_sta_info_cur->ea.ether_addr_octet[3],
        start_sta_info_cur->ea.ether_addr_octet[4],
        start_sta_info_cur->ea.ether_addr_octet[5]);*/
    if(start_sta_info_cur->start_is_on>0){
        start_game_is_on = TRUE;
    }else{
        start_game_is_on = FALSE;
    }
    // 重新设置定时器    
    mod_timer(&timer_qq_start_info, jiffies + msecs_to_jiffies(TIMER_INTERVAL_S_qq));
    
}


#include <wlc_lq.h>

#include <wlc_rspec.h>/**
 * Returns the rate in [Kbps] units, 0 for invalid ratespec.
 */
static uint
wf_he_rspec_to_rate(ratespec_t rspec)
{
	uint mcs = RSPEC_HE_MCS(rspec);
	uint nss = RSPEC_HE_NSS(rspec);
	bool dcm = (rspec & WL_RSPEC_DCM) != 0;
	uint bw =  RSPEC_BW(rspec) >> WL_RSPEC_BW_SHIFT;
	uint gi =  RSPEC_HE_LTF_GI(rspec);

	if (mcs <= WLC_MAX_HE_MCS && nss != 0 && nss <= 8) {
		return wf_he_mcs_to_rate(mcs, nss, bw, gi, dcm);
	}
#ifdef BCMDBG
	printf("%s: rspec %x, mcs %u, nss %u\n", __FUNCTION__, rspec, mcs, nss);
#endif
	ASSERT(mcs <= WLC_MAX_HE_MCS);
	ASSERT(nss != 0 && nss <= 8);
	return 0;
} /* wf_he_rspec_to_rate */

struct phy_info_qq phy_info_qq_rx_new;

/** take a well formed ratespec_t arg and return phy rate in [Kbps] units */
void wf_rspec_to_phyinfo_qq(ratesel_txs_t rs_txs, struct phy_info_qq *phy_info_qq_cur)
{
    uint rnum = 0;
    do{
        
    ratespec_t rspec = rs_txs.txrspec[rnum];
	uint rate = (uint)(-1);
	uint mcs, nss;

	switch (rspec & WL_RSPEC_ENCODING_MASK) {
		case WL_RSPEC_ENCODE_HE:
			rate = wf_he_rspec_to_rate(rspec);
			break;
		case WL_RSPEC_ENCODE_VHT:
			mcs = RSPEC_VHT_MCS(rspec);
			nss = RSPEC_VHT_NSS(rspec);
#ifdef BCMDBG
			if (mcs > WLC_MAX_VHT_MCS || nss == 0 || nss > 8) {
				printf("%s: rspec=%x\n", __FUNCTION__, rspec);
			}
#endif /* BCMDBG */
			ASSERT(mcs <= WLC_MAX_VHT_MCS);
			ASSERT(nss != 0 && nss <= 8);
			rate = wf_mcs_to_rate(mcs, nss,
				RSPEC_BW(rspec), RSPEC_ISSGI(rspec));
			break;
		case WL_RSPEC_ENCODE_HT:
			mcs = RSPEC_HT_MCS(rspec);
#ifdef BCMDBG
			if (mcs > 32 && !IS_PROPRIETARY_11N_MCS(mcs)) {
				printf("%s: rspec=%x\n", __FUNCTION__, rspec);
			}
#endif /* BCMDBG */
			ASSERT(mcs <= 32 || IS_PROPRIETARY_11N_MCS(mcs));
			if (mcs == 32) {
				rate = wf_mcs_to_rate(mcs, 1, WL_RSPEC_BW_40MHZ,
					RSPEC_ISSGI(rspec));
			} else {
#if defined(WLPROPRIETARY_11N_RATES)
				nss = GET_11N_MCS_NSS(mcs);
				mcs = wf_get_single_stream_mcs(mcs);
#else /* this ifdef prevents ROM abandons */
				nss = 1 + (mcs / 8);
				mcs = mcs % 8;
#endif /* WLPROPRIETARY_11N_RATES */
				rate = wf_mcs_to_rate(mcs, nss, RSPEC_BW(rspec),
					RSPEC_ISSGI(rspec));
			}
			break;
		case WL_RSPEC_ENCODE_RATE:	/* Legacy */
			rate = 500 * RSPEC2RATE(rspec);
			break;
		default:
			ASSERT(0);
			break;
	}
    phy_info_qq_cur->mcs[rnum] = mcs;
    phy_info_qq_cur->nss[rnum] = nss;
    phy_info_qq_cur->rate[rnum] = rate;
    phy_info_qq_cur->BW[rnum] = RSPEC_BW(rspec) >> WL_RSPEC_BW_SHIFT;;
    phy_info_qq_cur->ISSGI[rnum] = RSPEC_ISSGI(rspec);
    rnum++;
    }while (!(phy_info_qq_cur->fix_rate>0) && rnum < RATESEL_MFBR_NUM); /* loop over rates */

}

uint wf_rspec_to_mcs_qq(ratespec_t rspec)
{
	uint rate = (uint)(-1);
	uint mcs, nss;

	switch (rspec & WL_RSPEC_ENCODING_MASK) {
		case WL_RSPEC_ENCODE_HE:
			rate = wf_he_rspec_to_rate(rspec);
			break;
		case WL_RSPEC_ENCODE_VHT:
			mcs = RSPEC_VHT_MCS(rspec);
			nss = RSPEC_VHT_NSS(rspec);
#ifdef BCMDBG
			if (mcs > WLC_MAX_VHT_MCS || nss == 0 || nss > 8) {
				printf("%s: rspec=%x\n", __FUNCTION__, rspec);
			}
#endif /* BCMDBG */
			ASSERT(mcs <= WLC_MAX_VHT_MCS);
			ASSERT(nss != 0 && nss <= 8);
			rate = wf_mcs_to_rate(mcs, nss,
				RSPEC_BW(rspec), RSPEC_ISSGI(rspec));
			break;
		case WL_RSPEC_ENCODE_HT:
			mcs = RSPEC_HT_MCS(rspec);
#ifdef BCMDBG
			if (mcs > 32 && !IS_PROPRIETARY_11N_MCS(mcs)) {
				printf("%s: rspec=%x\n", __FUNCTION__, rspec);
			}
#endif /* BCMDBG */
			ASSERT(mcs <= 32 || IS_PROPRIETARY_11N_MCS(mcs));
			if (mcs == 32) {
				rate = wf_mcs_to_rate(mcs, 1, WL_RSPEC_BW_40MHZ,
					RSPEC_ISSGI(rspec));
			} else {
#if defined(WLPROPRIETARY_11N_RATES)
				nss = GET_11N_MCS_NSS(mcs);
				mcs = wf_get_single_stream_mcs(mcs);
#else /* this ifdef prevents ROM abandons */
				nss = 1 + (mcs / 8);
				mcs = mcs % 8;
#endif /* WLPROPRIETARY_11N_RATES */
				rate = wf_mcs_to_rate(mcs, nss, RSPEC_BW(rspec),
					RSPEC_ISSGI(rspec));
			}
			break;
		case WL_RSPEC_ENCODE_RATE:	/* Legacy */
			rate = 500 * RSPEC2RATE(rspec);
			break;
		default:
			ASSERT(0);
			break;
	}
    return mcs;

}


/*用于根据beacon包来统计周围的AP数量。*/
#define MAX_NUM_ROUTERS 100
#define ONE_HOUR_SECONDS 103 //beacon帧发送周期
#define MAX_NUM_ROUTERS_RING_BUFFER 100

typedef struct {
    uint8 bssid[6]; // 路由器的 MAC 地址
    uint32 last_seen; // 最后一次检测到的时间
} router_info_t;

router_info_t routers[MAX_NUM_ROUTERS];
int num_routers = 0;
uint8 num_routers_ring_buffer[MAX_NUM_ROUTERS_RING_BUFFER];
uint8 num_routers_ring_buffer_index = 0;

bool is_new_router(const uint8_t *bssid) {
    for (int i = 0; i < num_routers; i++) {
        if (memcmp(routers[i].bssid, bssid, 6) == 0) {
            return false;
        }
    }
    return true;
}

void update_router_list(const uint8_t *bssid) {

    uint32 cur_time = OSL_SYSUPTIME();
    // 遍历路由器列表，检查是否有超过一小时未检测到的路由器
    for (int i = 0; i < num_routers; i++) {
        if (cur_time - routers[i].last_seen >= ONE_HOUR_SECONDS) {
            // 将最后一个路由器移到当前位置，然后递减 num_routers
            num_routers--;
            routers[i] = routers[num_routers];
            i--; // 重新检查当前位置
        }
    }

    if (is_new_router(bssid)) {
        if (num_routers < MAX_NUM_ROUTERS) {
            memcpy(routers[num_routers].bssid, bssid, 6);
            routers[num_routers].last_seen = cur_time;
            num_routers++;
            num_routers_ring_buffer[num_routers_ring_buffer_index] = num_routers;
            num_routers_ring_buffer_index = (num_routers_ring_buffer_index + 1) % MAX_NUM_ROUTERS_RING_BUFFER;
            //printf("New router detected. Total number of routers in the last hour: %d\n", num_routers);
        } else {
            //printf("Router list is full. Cannot add new router.\n");
        }
    } else {
        // 更新已知路由器的 last_seen 时间戳
        for (int i = 0; i < num_routers; i++) {
            if (memcmp(routers[i].bssid, bssid, 6) == 0) {
                routers[i].last_seen = cur_time;
                break;
            }
        }
    }
}

void process_beacon_packet(struct dot11_header *h) {
    // 假设 h 已经指向有效的 802.11 头部结构
    // 提取 BSSID（在头部的地址 1 字段）
    const uint8_t *bssid = (const uint8_t *)&h->a2; // 根据 802.11 头部结构调整偏移量
    if (is_new_router(bssid)) {
        /*
        printk("MAC address (a1): %02x:%02x:%02x:%02x:%02x:%02x\n",
                    h->a1.octet[0],
                    h->a1.octet[1],
                    h->a1.octet[2],
                    h->a1.octet[3],
                    h->a1.octet[4],
                    h->a1.octet[5]);
        printk("MAC address (a2): %02x:%02x:%02x:%02x:%02x:%02x\n",
                    h->a2.octet[0],
                    h->a2.octet[1],
                    h->a2.octet[2],
                    h->a2.octet[3],
                    h->a2.octet[4],
                    h->a2.octet[5]);
        printk("MAC address (a3): %02x:%02x:%02x:%02x:%02x:%02x\n",
                    h->a3.octet[0],
                    h->a3.octet[1],
                    h->a3.octet[2],
                    h->a3.octet[3],
                    h->a3.octet[4],
                    h->a3.octet[5]);
        printk("MAC address (a4): %02x:%02x:%02x:%02x:%02x:%02x\n",
                    h->a4.octet[0],
                    h->a4.octet[1],
                    h->a4.octet[2],
                    h->a4.octet[3],
                    h->a4.octet[4],
                    h->a4.octet[5]);
                    */
    }
    // 更新路由器列表
    update_router_list(bssid);
}




struct rates_counts_txs_qq *cur_rates_counts_txs_qq;
/*
void update_cur_rates_counts_txs_qq(ratesel_txs_t rs_txs){
    cur_rates_counts_txs_qq->ncons += rs_txs.ncons;
    cur_rates_counts_txs_qq->nlost += rs_txs.nlost;
    cur_rates_counts_txs_qq->rxcts_cnt += rs_txs.rxcts_cnt;
    cur_rates_counts_txs_qq->txrts_cnt += rs_txs.txrts_cnt;
    for(uint i = 0; i<RATESEL_MFBR_NUM; i++){
        uint cur_mcs = wf_rspec_to_mcs_qq(rs_txs.txrspec[i]);
        cur_rates_counts_txs_qq->txsucc_cnt[cur_mcs] += rs_txs.txsucc_cnt[i];
        cur_rates_counts_txs_qq->tx_cnt[cur_mcs] += rs_txs.tx_cnt[i];
        //printk("tx_cnt(%u:%u:%u)",cur_mcs,rs_txs.tx_cnt[i],cur_rates_counts_txs_qq->tx_cnt[cur_mcs]);
    }

}*/
void update_cur_rates_counts_txs_qq(wlc_info_t *wlc, uint8 txs_mutype, bool txs_mu, bool fix_rate, tx_status_t *txs,ratesel_txs_t rs_txs,uint16 ncons, uint16 nlost){
    cur_rates_counts_txs_qq->ncons += ncons;
    cur_rates_counts_txs_qq->nlost += nlost;
    cur_rates_counts_txs_qq->rxcts_cnt += txs->status.cts_rx_cnt;
    cur_rates_counts_txs_qq->txrts_cnt += txs->status.rts_tx_cnt;
        /* Parse rs_txs_cur according to transmission type */
    
    uint8    rtidx = 0;
    ratesel_txs_t rs_txs_cur;
    //rs_txs_cur.txrspec[i] = rs_txs.txrspec[i];
    rs_txs_cur.tx_cnt[0]     = 0;
    rs_txs_cur.txsucc_cnt[0] = 0;
    rs_txs_cur.tx_cnt[1]     = 0;
    rs_txs_cur.txsucc_cnt[1] = 0;
    rs_txs_cur.tx_cnt[2]     = 0;
    rs_txs_cur.txsucc_cnt[2] = 0;
    rs_txs_cur.tx_cnt[3]     = 0;
    rs_txs_cur.txsucc_cnt[3] = 0;
    //printk("update_cur_rates_counts_txs_qq1");
    if (fix_rate && !txs_mu) {
        /* if using fix rate, retrying 64 mpdus >=4 times can overflow 8-bit cnt.
         * So ucode treats fix rate specially.
         */
        rs_txs_cur.tx_cnt[0]     = (TX_STATUS_MACTXS_S3(txs)) & 0xffff;
        rs_txs_cur.txsucc_cnt[0] = (TX_STATUS_MACTXS_S3(txs) >> 16) & 0xffff;
        //printk("update_cur_rates_counts_txs_qq2");
    } else {
        //printk("update_cur_rates_counts_txs_qq3");
        txs_mutype = TX_STATUS_MUTYP(wlc->pub->corerev, TX_STATUS_MACTXS_S5(txs));
        //printk("update_cur_rates_counts_txs_qq4");
        if (txs_mu) { /* MU case */
        //printk("update_cur_rates_counts_txs_qq5");
            
            /* MU txstatus uses RT1~RT3 for other purposes */
            if (txs_mutype == TX_STATUS_MUTP_HEOM) {
        //printk("update_cur_rates_counts_txs_qq6");
                rtidx = TX_STATUS128_HEOM_RTIDX(TX_STATUS_MACTXS_S4(txs));
        //printk("update_cur_rates_counts_txs_qq7");
                //ASSERT(rtidx < RATESEL_MFBR_NUM);
            }
        } else { /* SU Case */
        //printk("update_cur_rates_counts_txs_qq8");
            rs_txs_cur.tx_cnt[1]     = (TX_STATUS_MACTXS_S3(txs) >> 16) & 0xff;
            rs_txs_cur.txsucc_cnt[1] = (TX_STATUS_MACTXS_S3(txs) >> 24) & 0xff;
            rs_txs_cur.tx_cnt[2]     = (TX_STATUS_MACTXS_S4(txs) >>  0) & 0xff;
            rs_txs_cur.txsucc_cnt[2] = (TX_STATUS_MACTXS_S4(txs) >>  8) & 0xff;
            rs_txs_cur.tx_cnt[3]     = (TX_STATUS_MACTXS_S4(txs) >> 16) & 0xff;
            rs_txs_cur.txsucc_cnt[3] = (TX_STATUS_MACTXS_S4(txs) >> 24) & 0xff;
        //printk("update_cur_rates_counts_txs_qq9");
        }
        if(rtidx<RATESEL_MFBR_NUM){
        //printk("update_cur_rates_counts_txs_qq10");
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs));
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs) >> 8);
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs));
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs) >> 8);
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs));
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs) >> 8);
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs));
        //printk("TX_STATUS_MACTXS_S3(txs)(%u)",TX_STATUS_MACTXS_S3(txs) >> 8);
            rs_txs_cur.tx_cnt[rtidx]     = (TX_STATUS_MACTXS_S3(txs)) & 0xff;
            rs_txs_cur.txsucc_cnt[rtidx] = (TX_STATUS_MACTXS_S3(txs) >> 8) & 0xff;

        }


    }
    for(uint i = 0; i<RATESEL_MFBR_NUM; i++){

        rs_txs_cur.txrspec[i] = rs_txs.txrspec[i];
        uint cur_mcs = wf_rspec_to_mcs_qq(rs_txs_cur.txrspec[i]);
        //printk("update_cur_rates_counts_txs_qq13(%u)",cur_mcs);

        cur_rates_counts_txs_qq->txsucc_cnt[cur_mcs] += rs_txs_cur.txsucc_cnt[i];
        //printk("update_cur_rates_counts_txs_qq131");
        cur_rates_counts_txs_qq->tx_cnt[cur_mcs] += rs_txs_cur.tx_cnt[i];
        //printk("update_cur_rates_counts_txs_qq132");
        //printk("tx_cnt(%u:%u:%u:%u)",i,cur_mcs,cur_rates_counts_txs_qq->tx_cnt[cur_mcs],rs_txs_cur.tx_cnt[i]);

        //printk("txsucc_cnt(%u:%u:%u:%u)",i,cur_mcs,cur_rates_counts_txs_qq->txsucc_cnt[cur_mcs],rs_txs_cur.txsucc_cnt[i]);
        //printk("tx_cnt(%u:%u:%u)",cur_mcs,rs_txs_cur.tx_cnt[i],cur_rates_counts_txs_qq->tx_cnt[cur_mcs]);
    }
        //printk("update_cur_rates_counts_txs_qq14");
        //printk("update_cur_rates_counts_txs_qq15");

}



/*rssi的ring buffer*/
uint rssi_ring_buffer_index = 0;
DataPoint_qq rssi_ring_buffer_cur[RSSI_RING_SIZE];

void save_rssi(int8 RSSI,int8 noiselevel) {
    rssi_ring_buffer_cur[rssi_ring_buffer_index].RSSI = RSSI;
    rssi_ring_buffer_cur[rssi_ring_buffer_index].noiselevel = noiselevel;
    rssi_ring_buffer_cur[rssi_ring_buffer_index].timestamp = OSL_SYSUPTIME();
    rssi_ring_buffer_index = (rssi_ring_buffer_index + 1) % RSSI_RING_SIZE;
}


//160MHz方案
#define RECENT_BEACON_NUM 10
#define MAX_AP_LIST_SIZE 100
#define MAX_CHANNELS 13
#define EXPIRATION_TIME 5000 // 5秒
int china_5GHz_channels[] = {36, 40, 44, 48, 52, 56, 60, 64, 149, 153, 157, 161, 165};
const uint8 wf_chspec_bw_num[] =
{
	1,
	2,
	20,
	40,
	80,
	160,
	7,
	8
};
typedef struct {
struct ether_addr BSSID;
char SSID_str[33];
int occupied_channels[MAX_CHANNELS];
uint8 num_channels;
int recent_RSSI[RECENT_BEACON_NUM];
chanspec_t recent_chanspec[RECENT_BEACON_NUM];
uint8 recent_qbss_load_chan_free[RECENT_BEACON_NUM];
int16 avg_RSSI;
int16 avg_chanspec;
int16 avg_qbss_load_chan_free;
uint32 last_update_timestamp;
uint32 first_update_timestamp; // 添加第一次更新的时间戳
} APinfo_qq;

APinfo_qq global_AP_list[MAX_AP_LIST_SIZE];
int global_AP_list_size = 0;

// 0. 编写函数，实现输入一个wlc_bss_info_t结构体，以数组形式输出该结构体对应的WiFi路由器所占据的WiFi信道有哪些
//我的做法是让occupied_channels与china_5GHz_channels对应起来，如果为0就无该信道编号，否则有
void get_occupied_channels(int center_channel, int bandwidth, int *occupied_channels, uint8 *num_channels) {
    uint8 start_channel, start_channel_index, end_channel;
    //occupied_channels = {0};
    // 计算信道起始编号和结束编号
    start_channel = center_channel - (bandwidth / 20);

    start_channel_index = 0;
    for(uint8 i = 0;i<MAX_CHANNELS;i++){
        if(china_5GHz_channels[i] == start_channel){
            start_channel_index = i;
            occupied_channels[i] = china_5GHz_channels[i];
        }
        occupied_channels[i] = 0;
    }

    end_channel = center_channel + (bandwidth / 20);

    // 生成包含从起始信道编号到结束信道编号的所有信道的数组
    *num_channels = bandwidth / 20;
    for (int i = start_channel_index; i < start_channel_index+*num_channels; i++) {
        occupied_channels[i] = china_5GHz_channels[i];
    }
}


// 1. 为每个WiFi路由器建立新的结构体APinfo_qq
void create_APinfo_qq_from_wlc_bss_info(wlc_bss_info_t *bss_info, APinfo_qq *ap_info) {
    memcpy(&ap_info->BSSID, &bss_info->BSSID, sizeof(struct ether_addr));
    memcpy(ap_info->SSID_str, bss_info->SSID, bss_info->SSID_len);
    ap_info->SSID_str[bss_info->SSID_len] = '\0';

    get_occupied_channels(bss_info->chanspec & WL_CHANSPEC_CHAN_MASK,
            wf_chspec_bw_num[CHSPEC_BW(bss_info->chanspec)>> WL_CHANSPEC_BW_SHIFT],
            ap_info->occupied_channels,&(ap_info->num_channels));

    // 初始化recent_RSSI、recent_chanspec和recent_qbss_load_chan_free数组
    for (int i = 0; i < RECENT_BEACON_NUM; i++) {
        ap_info->recent_RSSI[i] = bss_info->RSSI;
        ap_info->recent_chanspec[i] = bss_info->chanspec;
        ap_info->recent_qbss_load_chan_free[i] = bss_info->qbss_load_chan_free;
    }

    // 初始化avg_RSSI、avg_chanspec和avg_qbss_load_chan_free
    ap_info->avg_RSSI = bss_info->RSSI;
    ap_info->avg_chanspec = bss_info->chanspec;
    ap_info->avg_qbss_load_chan_free = bss_info->qbss_load_chan_free;

    // 初始化last_update_timestamp和first_update_timestamp
    ap_info->last_update_timestamp = OSL_SYSUPTIME();
    ap_info->first_update_timestamp = OSL_SYSUPTIME();
}

// 2. 将所有APinfo_qq安置在一个全局列表中
void add_APinfo_qq_to_global_list(APinfo_qq *ap_info) {
    if (global_AP_list_size < MAX_AP_LIST_SIZE) {
        memcpy(&global_AP_list[global_AP_list_size], ap_info, sizeof(APinfo_qq));
        global_AP_list_size++;
    } else {
        printf("Error: Global AP list is full.\n");
    }
}

// 3. 编写函数，每次收到一个wlc_bss_info结构体，根据其BSSID取值判断是否在全局列表中，如果在全局列表中便对全局列表进行更新，如果收到了全局列表中没有的新的wlc_bss_info结构体，但全局列表大小不足以将其添加进去，就删掉最久未更新的路由器信息，再将新的添加进去。
void update_global_AP_list(wlc_bss_info_t *bss_info) {
    // 检查全局列表中是否已有相同BSSID的AP
    int index = -1;
    for (int i = 0; i < global_AP_list_size; i++) {
        /*printk("global_AP_list MAC address (%02x:%02x:%02x:%02x:%02x:%02x)----\n",
                            global_AP_list[i].BSSID.octet[0],
                            global_AP_list[i].BSSID.octet[1],
                            global_AP_list[i].BSSID.octet[2],
                            global_AP_list[i].BSSID.octet[3],
                            global_AP_list[i].BSSID.octet[4],
                            global_AP_list[i].BSSID.octet[5]);
        printk("bss_info MAC address (%02x:%02x:%02x:%02x:%02x:%02x)----\n",
                            bss_info->BSSID.octet[0],
                            bss_info->BSSID.octet[1],
                            bss_info->BSSID.octet[2],
                            bss_info->BSSID.octet[3],
                            bss_info->BSSID.octet[4],
                            bss_info->BSSID.octet[5]);*/
        if (memcmp(&global_AP_list[i].BSSID, &bss_info->BSSID, sizeof(struct ether_addr)) == 0) {
            index = i;
            break;
        }
    }

    // 如果找到了相同BSSID的AP，则更新其信息；否则，将新的AP添加到全局列表中
    if (index != -1) {
        // 更新AP信息
        APinfo_qq *ap_info = &global_AP_list[index];

        // 更新recent_RSSI、recent_chanspec和recent_qbss_load_chan_free数组
        for (int i = RECENT_BEACON_NUM - 1; i > 0; i--) {
            ap_info->recent_RSSI[i] = ap_info->recent_RSSI[i - 1];
            ap_info->recent_chanspec[i] = ap_info->recent_chanspec[i - 1];
            ap_info->recent_qbss_load_chan_free[i] = ap_info->recent_qbss_load_chan_free[i - 1];
        }
        ap_info->recent_RSSI[0] = bss_info->RSSI;
        ap_info->recent_chanspec[0] = bss_info->chanspec;
        ap_info->recent_qbss_load_chan_free[0] = bss_info->qbss_load_chan_free;

        // 更新avg_RSSI、avg_chanspec和avg_qbss_load_chan_free
        int16 sum_RSSI = 0;
        int16 sum_chanspec = 0;
        int16 sum_qbss_load_chan_free = 0;
        for (int i = 0; i < RECENT_BEACON_NUM; i++) {
            sum_RSSI += ap_info->recent_RSSI[i];
            sum_chanspec += ap_info->recent_chanspec[i];
            sum_qbss_load_chan_free += ap_info->recent_qbss_load_chan_free[i];
        }
        ap_info->avg_RSSI = sum_RSSI / RECENT_BEACON_NUM;
        ap_info->avg_chanspec = sum_chanspec / RECENT_BEACON_NUM;
        ap_info->avg_qbss_load_chan_free = sum_qbss_load_chan_free / RECENT_BEACON_NUM;
        /*printf("ap_info->avg(%d:%d:%d:%d)\n",
                ap_info->avg_RSSI,bss_info->chanspec & WL_CHANSPEC_CHAN_MASK,
                wf_chspec_bw_num[CHSPEC_BW(bss_info->chanspec)>> WL_CHANSPEC_BW_SHIFT],ap_info->avg_qbss_load_chan_free);
        */
        // 更新last_update_timestamp
        ap_info->last_update_timestamp = OSL_SYSUPTIME();
    } else {
        // 如果全局列表已满，找到最久未更新的AP并将其删除
        if (global_AP_list_size >= MAX_AP_LIST_SIZE) {
            int oldest_index = 0;
            uint32 oldest_timestamp = global_AP_list[0].last_update_timestamp;
            for (int i = 1; i < global_AP_list_size; i++) {
                if (global_AP_list[i].last_update_timestamp < oldest_timestamp) {
                    oldest_index = i;
                    oldest_timestamp = global_AP_list[i].last_update_timestamp;
                }
            }
            // 删除最久未更新的AP
            memmove(&global_AP_list[oldest_index], &global_AP_list[oldest_index + 1],
            (global_AP_list_size - oldest_index - 1) * sizeof(APinfo_qq));
            global_AP_list_size--;
        }

        // 添加新的AP到全局列表中
        APinfo_qq new_ap_info;
        create_APinfo_qq_from_wlc_bss_info(bss_info, &new_ap_info);
        add_APinfo_qq_to_global_list(&new_ap_info);
    }
}

// 4. 删除最近5秒内未收到新的wlc_bss_info_t结构体的WiFi路由器在全局列表中对应的APinfo_qq结构体
void remove_expired_APinfo_qq(void) {
    uint32 current_timestamp = OSL_SYSUPTIME();
    for (int i = 0; i < global_AP_list_size; ) {
        if (current_timestamp - global_AP_list[i].last_update_timestamp > EXPIRATION_TIME) {
            // 删除过期的AP
            memmove(&global_AP_list[i], &global_AP_list[i + 1],
            (global_AP_list_size - i - 1) * sizeof(APinfo_qq));
            global_AP_list_size--;
        } else {
            i++;
        }
    }
}

void find_best_channels(int *best_40MHz_channels, int *best_80MHz_channels) {
    int16 max_avg_qbss_load_chan_free[MAX_CHANNELS] = {-120};

    int8 max_avg_RSSI[MAX_CHANNELS] = {-120};
    int num_channels = MAX_CHANNELS;

    // 步骤一: 记录每个信道的最大平均RSSI的AP的qbss_load_chan_free值
    for (int i = 0; i < global_AP_list_size; i++) {
        APinfo_qq *ap_info = &global_AP_list[i];
        if(ap_info->avg_RSSI>=0){
            continue;
        }
        for (int j = 0; j < num_channels; j++) {
            //if ((ap_info->occupied_channels[j] != 0) && ap_info->avg_qbss_load_chan_free > max_avg_qbss_load_chan_free[j]) {
                
            if ((ap_info->occupied_channels[j] != 0) && ap_info->avg_RSSI > max_avg_RSSI[j]) {
                max_avg_qbss_load_chan_free[j] = ap_info->avg_qbss_load_chan_free;
                max_avg_RSSI[j] = ap_info->avg_RSSI;

            printf("RSSI0(%d:%d:%d)\n",
                china_5GHz_channels[j],ap_info->avg_RSSI, max_avg_qbss_load_chan_free[j]);
            }
            printf("RSSI(%d:%d:%d:%d:%d:%d:%d)\n",global_AP_list_size,
                china_5GHz_channels[j],ap_info->occupied_channels[j] ,ap_info->avg_RSSI, max_avg_RSSI[j], max_avg_qbss_load_chan_free[j],ap_info->avg_qbss_load_chan_free);
    
        }
    }

    // 步骤二: 找到40 MHz大信道的最佳组合
    int8 best_40MHz_score = -120;
    for (int i = 0; i < num_channels - 1; i++) {
        if((china_5GHz_channels[i]%8 == 4)||(china_5GHz_channels[i]%8 == 5)){
            int8 score = (max_avg_qbss_load_chan_free[i] + max_avg_qbss_load_chan_free[i + 1]) / 2;
            if (score > best_40MHz_score) {
                best_40MHz_score = score;
                best_40MHz_channels[0] = china_5GHz_channels[i];
                best_40MHz_channels[1] = china_5GHz_channels[i + 1];
            }
            printf("40 MHz channels: channel num(%d:%d); free(%d, %d)\n",
                china_5GHz_channels[i], china_5GHz_channels[i+1], max_avg_qbss_load_chan_free[i], max_avg_qbss_load_chan_free[i+1]);
    

        }
        }

    // 步骤三: 找到80 MHz大信道的最佳组合
    int8 best_80MHz_score = -120;
    for (int i = 0; i < num_channels - 3; i++) {
        if((china_5GHz_channels[i]%16 == 4)||(china_5GHz_channels[i]%16 == 5)){
            int8 score = (max_avg_qbss_load_chan_free[i] + max_avg_qbss_load_chan_free[i + 1] +
            max_avg_qbss_load_chan_free[i + 2] + max_avg_qbss_load_chan_free[i + 3]) / 4;
            if (score > best_80MHz_score) {
                best_80MHz_score = score;
                best_80MHz_channels[0] = china_5GHz_channels[i];
                best_80MHz_channels[1] = china_5GHz_channels[i + 1];
                best_80MHz_channels[2] = china_5GHz_channels[i + 2];
                best_80MHz_channels[3] = china_5GHz_channels[i + 3];
            }
        }
    }
}


wlc_info_t *wlc_qq;
struct timer_list timer_qq_scan_set;
void timer_callback_scan_set_qq(struct timer_list *t) {
    if(start_game_is_on){
        int best_40MHz_channels[2];
        int best_80MHz_channels[4];

        find_best_channels(best_40MHz_channels, best_80MHz_channels);

        printf("Best 40 MHz channels: %d, %d\n", best_40MHz_channels[0], best_40MHz_channels[1]);
        printf("Best 80 MHz channels: %d, %d, %d, %d\n", best_80MHz_channels[0], best_80MHz_channels[1], best_80MHz_channels[2], best_80MHz_channels[3]);
        chanspec_t chanspec_cur = (best_40MHz_channels[0] << WL_CHANSPEC_CHAN_SHIFT) |
                (WL_CHANSPEC_BAND_5G) |
                (WL_CHANSPEC_BW_40) |
                (WL_CHANSPEC_CTL_SB_NONE) |
                (WL_CHANSPEC_BW_40);

        printk("start switch(wlc->chanspec num(%u))----------[fyl] OSL_SYSUPTIME()----------(%u)",(wlc_qq->chanspec& WL_CHANSPEC_CHAN_MASK),OSL_SYSUPTIME());
        wlc_set_chanspec(wlc_qq, chanspec_cur, 0);
        printk("end switch(wlc->chanspec num(%u))----------[fyl] OSL_SYSUPTIME()----------(%u)",(wlc_qq->chanspec& WL_CHANSPEC_CHAN_MASK),OSL_SYSUPTIME());


    }
        // 重新设置定时器    
        mod_timer(&timer_qq_scan_set, jiffies + msecs_to_jiffies(TIMER_INTERVAL_S_qq*120));
    
}











//scan_test
#define MAX_APNUM_EACH_CHANNEL 20//超过了的话就不再添加新的
#define MAX_CHANNEL_NUM 30//最大channel数量
struct AP_info_each_channel_qq {
    chanspec_t chanspec;
    uint APnum;
    wlc_bss_info_t wlc_bss_info[MAX_APNUM_EACH_CHANNEL];
};
struct AP_info_each_channel_qq AP_info_each_channel_qq_list[MAX_CHANNEL_NUM];//用来测试的或者中转数据的临时结构体
static uint8 AP_info_each_channel_qq_list_len = 0;
static uint8 active_time = 50;
static uint8 scan_time = 0;
//static uint8 home_time = 60;
struct AP_info_each_channel_qq cur_AP_info_each_channel_qq;//用来测试的或者中转数据的临时结构体
uint32 last_scan_time = 0;//用于避免短时间内很多次的scan
uint16 min_scan_interval = 6000;//用于避免短时间内很多次的scan
void update_AP_info_each_channel_qq(void){
    
    bool channel_has_scaned = FALSE;
    if( AP_info_each_channel_qq_list_len>0){

        for(uint8 j = 0; j <  AP_info_each_channel_qq_list_len; j++){
            if(AP_info_each_channel_qq_list[j].chanspec == cur_AP_info_each_channel_qq.chanspec){
                uint8 global_APnum = AP_info_each_channel_qq_list[j].APnum;
                if(global_APnum>0){
                    for(uint8 cur_index = 0; cur_index < cur_AP_info_each_channel_qq.APnum; cur_index++){
                        bool AP_has_scaned = FALSE;
                        for(uint8 global_index = 0; global_index < global_APnum; global_index++){
                            //memcmp(cur_AP_info_each_channel_qq.wlc_bss_info[cur_index].BSSID, AP_info_each_channel_qq_list[j].wlc_bss_info[global_index].BSSID,6)
                            if(memcmp(&(cur_AP_info_each_channel_qq.wlc_bss_info[cur_index].BSSID), &(AP_info_each_channel_qq_list[j].wlc_bss_info[global_index].BSSID),6)==0){
                                memcpy(&AP_info_each_channel_qq_list[j].wlc_bss_info[global_index], &cur_AP_info_each_channel_qq.wlc_bss_info[cur_index], sizeof(wlc_bss_info_t));
                                AP_has_scaned = TRUE;
                                break;
                            }
                        }
                        if(!AP_has_scaned){
                            if(AP_info_each_channel_qq_list[j].APnum<MAX_CHANNEL_NUM - 1){
                                memcpy(&AP_info_each_channel_qq_list[j].wlc_bss_info[AP_info_each_channel_qq_list[j].APnum], &cur_AP_info_each_channel_qq.wlc_bss_info[cur_index], sizeof(wlc_bss_info_t));
                                AP_info_each_channel_qq_list[j].APnum++;

                            }
                        }
                    }
                }
                else{
                    memcpy(&AP_info_each_channel_qq_list[j], &cur_AP_info_each_channel_qq, sizeof(struct AP_info_each_channel_qq));

                }
                
                
                channel_has_scaned = TRUE;
                break;
            }

        }

    }
    if(!channel_has_scaned){
        if(AP_info_each_channel_qq_list_len<MAX_CHANNEL_NUM - 1){

            memcpy(&AP_info_each_channel_qq_list[AP_info_each_channel_qq_list_len], &cur_AP_info_each_channel_qq, sizeof(struct AP_info_each_channel_qq));
            AP_info_each_channel_qq_list_len++;
        }
    }
}

struct AP_info_each_channel_qq AP_info_each_channel_qq_1;
uint8 avg_txop_each_channel(void){
    uint8 valid_txop_num = 0;
    uint16 valid_txop_sum = 0;
    for(uint8 cur_index = 0; cur_index < AP_info_each_channel_qq_1.APnum; cur_index++){
        if(AP_info_each_channel_qq_1.wlc_bss_info[cur_index].qbss_load_chan_free>0){
            valid_txop_num++;
            valid_txop_sum += AP_info_each_channel_qq_1.wlc_bss_info[cur_index].qbss_load_chan_free;
        }
    }
    return(valid_txop_sum/valid_txop_num);
}

chanspec_t get_best_chanspec(void){
    chanspec_t best_chanspec;
    uint8 txop_of_best_chanspec;
    if(AP_info_each_channel_qq_list_len<255){
        best_chanspec = AP_info_each_channel_qq_list[0].chanspec;
        txop_of_best_chanspec = AP_info_each_channel_qq_list[0].wlc_bss_info[0].qbss_load_chan_free;
    }
    else{
        return 0;
    }
    
    if(AP_info_each_channel_qq_list_len>=2){
        for(uint8 j = 0; j <  AP_info_each_channel_qq_list_len; j++){
            memcpy(&AP_info_each_channel_qq_1, &AP_info_each_channel_qq_list[j], sizeof(struct AP_info_each_channel_qq));

            //AP_info_each_channel_qq_1 = AP_info_each_channel_qq_list[j];
            uint8 avg_txop = avg_txop_each_channel();
            if(avg_txop>txop_of_best_chanspec){
                txop_of_best_chanspec = avg_txop;
                best_chanspec = AP_info_each_channel_qq_list[j].chanspec;
            }
        }
    }
    return best_chanspec;

}


// 扫描结果回调函数
//static void scan_result_callback_qq(void *ctx, int status, wlc_bsscfg_t *bsscfg) {
void scan_result_callback_qq(void *ctx, int status, wlc_bsscfg_t *bsscfg) {
// 在这里处理扫描结果，例如更新 g_scan_result 结构体
    wlc_info_t *wlc = (wlc_info_t*)ctx;
    wlc_bss_list_t cur_scan_results;
    memcpy(&cur_scan_results, wlc->scan_results, sizeof(wlc_bss_list_t));

    printk("start scan call back,status(%d)----------[fyl] OSL_SYSUPTIME()----------(%u)",status,OSL_SYSUPTIME());
    //wlc_bss_list_t cur_scan_results = wlc->scan_results;
    if(cur_scan_results.count>0){
        if(cur_scan_results.count<MAX_APNUM_EACH_CHANNEL){
            cur_AP_info_each_channel_qq.APnum = cur_scan_results.count;
        }else{
            cur_AP_info_each_channel_qq.APnum = MAX_APNUM_EACH_CHANNEL;
        }
        cur_AP_info_each_channel_qq.chanspec = cur_scan_results.ptrs[0]->chanspec;
        for(uint8 i = 0; (i < cur_scan_results.count)&&(i<MAX_APNUM_EACH_CHANNEL); i++){
            memcpy(&cur_AP_info_each_channel_qq.wlc_bss_info[i], cur_scan_results.ptrs[i], sizeof(wlc_bss_info_t));
            //cur_AP_info_each_channel_qq.wlc_bss_info[i] = *cur_scan_results.ptrs[i];
            struct ether_addr curAPea;
            memcpy(&curAPea, &(cur_AP_info_each_channel_qq.wlc_bss_info[i].BSSID), sizeof(struct ether_addr));
            //struct ether_addr curAPea = cur_AP_info_each_channel_qq.wlc_bss_info[i]->BSSID;
            printk("status(%u);scan_time(%u);active_time(%u);qbss_load_aac(%u);qbss_load_chan_free(%u);\
                    scan_channel(0x%04x:%u);chanspec(0x%04x:%u);MAC address (curAPea): %02x:%02x:%02x:%02x:%02x:%02x----\n"
                    ,status,scan_time,active_time,cur_AP_info_each_channel_qq.wlc_bss_info[i].qbss_load_aac,cur_AP_info_each_channel_qq.wlc_bss_info[i].qbss_load_chan_free,
                    cur_AP_info_each_channel_qq.chanspec,cur_AP_info_each_channel_qq.chanspec & WL_CHANSPEC_CHAN_MASK,
                    cur_AP_info_each_channel_qq.wlc_bss_info[i].chanspec,cur_AP_info_each_channel_qq.wlc_bss_info[i].chanspec & WL_CHANSPEC_CHAN_MASK,
                    curAPea.octet[0],
                    curAPea.octet[1],
                    curAPea.octet[2],
                    curAPea.octet[3],
                    curAPea.octet[4],
                    curAPea.octet[5]);
        }
        update_AP_info_each_channel_qq();
    }
    

#if 0
    cur_AP_info_each_channel_qq.wlc_bss_info[0] = *bsscfg->target_bss;
    cur_AP_info_each_channel_qq.wlc_bss_info[1] = *bsscfg->current_bss;
    struct ether_addr targetea = bsscfg->target_bss->BSSID;
    struct ether_addr currentea = bsscfg->current_bss->BSSID;
    printk("status(%u);scan_time(%u);MAC address (currentea): %02x:%02x:%02x:%02x:%02x:%02x----\n",status,scan_time,
                currentea.octet[0],
                currentea.octet[1],
                currentea.octet[2],
                currentea.octet[3],
                currentea.octet[4],
                currentea.octet[5]);
    printk("MAC address (targetea): %02x:%02x:%02x:%02x:%02x:%02x----\n",
                targetea.octet[0],
                targetea.octet[1],
                targetea.octet[2],
                targetea.octet[3],
                targetea.octet[4],
                targetea.octet[5]);





    wlc_bss_info_t    *target_bss;    /**< BSS parms during tran. to ASSOCIATED state */
    wlc_bss_info_t    *current_bss;    /**< BSS parms in ASSOCIATED state */

    wlc_oce_info_t *oce = wlc->oce;
    wlc_bsscfg_t *cfg;
    uint8 idx;

    ASSERT(oce);
    ASSERT(bsscfg);
    /* XXX: just one scanresults are sufficient to prepare neighbor info
    * for all other BSSes.
    */
    FOREACH_UP_AP(wlc, idx, cfg) {
        wlc_oce_update_rnr_nbr_ap_info(oce, cfg);
        wlc_oce_update_ap_chrep(oce, cfg);
        /* OCE Standard: 3.4 Reduced neighbor report and ap channel report
        * Note: If an OCE AP is operating multiple BSS on the same channel
        * (multiple VAPs), it is not required to transmit a Reduced
        * Neighbor Report element on all the BSSs
        */
        break;
    }

    wlc_suspend_mac_and_wait(wlc);
    wlc_bss_update_beacon(wlc, bsscfg, FALSE);
    wlc_bss_update_probe_resp(wlc, bsscfg, FALSE);
    wlc_enable_mac(wlc);
#endif
}



// 函数1：扫描5GHz频段的某个信道
void scan_channel(wlc_info_t *wlc, chanspec_t chanspec) {
    //wlc_ssid_t ssid = {0, ""};
    int err;
    chanspec_t chanspec_list[1] = {chanspec};
    wlc_ssid_t req_ssid;
    bzero(&req_ssid, sizeof(req_ssid));
    /*err = wlc_scan_request(wlc, DOT11_BSSTYPE_ANY, NULL, 1, &ssid, 0, NULL,
    DOT11_SCANTYPE_ACTIVE, -1, -1, -1, -1, chanspec_list, 1, 0, FALSE,
    scan_result_callback_qq, wlc, WLC_ACTION_SCAN, 0, *wlc->bsscfg, NULL, NULL);*/
    //scan_time = 0;
    uint8 scan_type = DOT11_SCANTYPE_PASSIVE;
    /*
    if(active_time>scan_time){
        scan_type = DOT11_SCANTYPE_PASSIVE;
    }
	err = wlc_scan_request(wlc, DOT11_BSSTYPE_ANY, &ether_bcast, 1,
		&req_ssid, 0, NULL, scan_type, -1, active_time, scan_time, -1, chanspec_list,
		1, 0, FALSE, scan_result_callback_qq, wlc,
		WLC_ACTION_SCAN, FALSE, NULL, NULL, NULL);*/
    
	err = wlc_scan_request(wlc, DOT11_BSSTYPE_ANY, &ether_bcast, 1,
		&req_ssid, 0, NULL, scan_type, -1, active_time, scan_time, -1, chanspec_list,
		1, 0, FALSE, scan_result_callback_qq, wlc,
		WLC_ACTION_SCAN, FALSE, NULL, NULL, NULL);
    if (err != BCME_OK) {
        WL_ERROR(("wl%d: %s: scan request failed with error %d\n", wlc->pub->unit, __FUNCTION__, err));
    }
}


#if 0
typedef struct {
    int channel;
    int router_count;
    int channel_condition;
    int channel_utilization;
} channel_info_t;

typedef struct {
channel_info_t channels_info[SCAN_MAX_CHANSPECS];
int num_channels;
int current_channel_index;
} scan_result_t;

scan_result_t g_scan_result;

// 函数2：定时调用函数1并在所有信道扫描完毕后选择最优信道
static void scan_timer_callback(void *arg) {
wlc_info_t *wlc = (wlc_info_t *)arg;
chanspec_t chanspec;

// 如果所有信道已经扫描完毕，选择最优信道并切换
if (g_scan_result.current_channel_index >= g_scan_result.num_channels) {
chanspec = select_best_channel();
wlc_set_chanspec(wlc, chanspec, 0);
return;
}

// 扫描下一个信道
chanspec = g_scan_result.channels_info[g_scan_result.current_channel_index].channel;
scan_channel(wlc, chanspec);
g_scan_result.current_channel_index++;

// 重新设置定时器
wl_add_timer(wlc->wl, scan_timer, 60000, FALSE);
}

// 选择最优信道
static chanspec_t select_best_channel(void) {
int i;
int best_channel_index = 0;

// 在这里实现选择最优信道的算法，例如根据信道利用率、路由器数量等指标

return g_scan_result.channels_info[best_channel_index].channel;
}

// 初始化全局结构体
g_scan_result.num_channels = init_channel_list(wlc, g_scan_result.channels_info, SCAN_MAX_CHANSPECS);
g_scan_result.current_channel_index = 0;

// 设置定时器
scan_timer = wl_init_timer(wlc->wl, scan_timer_callback, wlc, "scan_timer");
wl_add_timer(wlc->wl, scan_timer, 60000, FALSE);

#endif











//周期scan
struct timer_list timer_qq_scan_try;
uint8 scan_channel_index = 0;
#define TIMER_INTERVAL_SCAN_qq (500) // 1s
void timer_callback_scan_try_qq(struct timer_list *t) {
    if(start_game_is_on){
        chanspec_t chanspec_cur = (china_5GHz_channels[scan_channel_index] << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BAND_5G) |
            (WL_CHANSPEC_BW_20) |
            (WL_CHANSPEC_CTL_SB_NONE) |
            (WL_CHANSPEC_BW_20);
        printk("start scan----------[fyl] OSL_SYSUPTIME()----------(%u)",OSL_SYSUPTIME());
        scan_channel(wlc_qq, chanspec_cur);
        //scan_channel(wlc, wlc->chanspec);
        scan_channel_index = (scan_channel_index+1)%MAX_CHANNELS;


    }
    // 重新设置定时器    
    mod_timer(&timer_qq_scan_try, jiffies + msecs_to_jiffies(TIMER_INTERVAL_SCAN_qq));
    
}













bool pkt_qq_chain_len_in_range(uint16 upper_bound,uint16 lower_bound){
    
    read_lock(&pkt_qq_mutex_len); // 加锁
    if((pkt_qq_chain_len > upper_bound)||((pkt_qq_chain_len < lower_bound))){
        read_unlock(&pkt_qq_mutex_len); // 解锁
        return FALSE;
    }
    read_unlock(&pkt_qq_mutex_len); // 解锁
    return TRUE;
}






/*
bool pkt_qq_len_error_sniffer(osl_t *osh, uint8 num){
    struct pkt_qq *pkt_qq_cur = pkt_qq_chain_head;

    uint16 index = 0;
    while(pkt_qq_cur != (struct pkt_qq *)NULL){

        index ++;
        pkt_qq_cur = pkt_qq_cur->next;
    }
        printk("__index:::pkt_qq_chain_len:::pkt_qq_len_error_sniffer___(%u,%u,%u)",index,pkt_qq_chain_len,num);
        
    if(index!=pkt_qq_chain_len){
        return TRUE;
    }
        return FALSE;
}
*/

void pkt_qq_add_at_tail(struct pkt_qq *pkt_qq_cur){
    if (pkt_qq_cur == (struct pkt_qq *)NULL){
        printk("_______________error_qq: null pointer_____________");
        return;
    }


    pkt_qq_chain_len_add++;
    pkt_qq_cur->pkt_qq_chain_len_add_start = pkt_qq_chain_len_add;
    pkt_qq_cur->next = (struct pkt_qq *)NULL;
    pkt_qq_cur->prev = (struct pkt_qq *)NULL;

    mutex_lock(&pkt_qq_mutex_tail); // 加锁
    if (pkt_qq_chain_head == NULL){
        pkt_qq_chain_head = (struct pkt_qq *)pkt_qq_cur;
        pkt_qq_chain_tail = (struct pkt_qq *)pkt_qq_cur;

    }else if(pkt_qq_chain_head->next == NULL){
        pkt_qq_chain_head->next = (struct pkt_qq *)pkt_qq_cur;
        pkt_qq_cur->prev = (struct pkt_qq *)pkt_qq_chain_head;
        pkt_qq_chain_tail = (struct pkt_qq *)pkt_qq_cur;
    }else{        
        pkt_qq_chain_tail->next = (struct pkt_qq *)pkt_qq_cur;
        pkt_qq_cur->prev= (struct pkt_qq *)pkt_qq_chain_tail;
        pkt_qq_chain_tail = (struct pkt_qq *)pkt_qq_cur;
    }
    mutex_unlock(&pkt_qq_mutex_tail); // 解锁

    write_lock(&pkt_qq_mutex_len); // 加锁
    pkt_qq_chain_len++;
    write_unlock(&pkt_qq_mutex_len); // 解锁


}
void pkt_qq_delete(struct pkt_qq *pkt_qq_cur,osl_t *osh){
    //mutex_lock(&pkt_qq_mutex); // 加锁
    //printk("**************debug11*******************");
    if((pkt_qq_last != (struct pkt_qq *)NULL)&&(pkt_qq_cur->FrameID == pkt_qq_last->FrameID)){
        pkt_qq_last = (struct pkt_qq *)NULL;
        index_last = 0;
    }
    read_lock(&pkt_qq_mutex_len); // 加锁
    //printk("**************debug14*******************");
    if(pkt_qq_chain_len<1){
        //printk("****************wrong pkt_qq_chain_len----------(%u)",pkt_qq_chain_len);
        read_unlock(&pkt_qq_mutex_len); // 解锁
        return;

    }
    //printk("**************debug15*******************");
    read_unlock(&pkt_qq_mutex_len); // 解锁
    if((pkt_qq_cur->FrameID == pkt_qq_chain_head->FrameID)&&(pkt_qq_cur->prev==(struct pkt_qq *)NULL)){
        //printk("**************debug12******************");
        if(pkt_qq_chain_head->next == (struct pkt_qq *)NULL){//防止删除节点时出错
            //printk("**************debug13*******************");
            MFREE(osh, pkt_qq_cur, sizeof(*pkt_qq_cur));
            pkt_qq_chain_head=(struct pkt_qq *)NULL;
            pkt_qq_chain_tail=pkt_qq_chain_head;
            read_lock(&pkt_qq_mutex_len); // 加锁
            //printk("**************debug14*******************");
            if(pkt_qq_chain_len!=1){
                printk("****************wrong pkt_qq_chain_len----------(%u)",pkt_qq_chain_len);

            }
            //printk("**************debug15*******************");
            read_unlock(&pkt_qq_mutex_len); // 解锁
            write_lock(&pkt_qq_mutex_len); // 加锁
            pkt_qq_chain_len=0;
            write_unlock(&pkt_qq_mutex_len); // 解锁

        }else{
            //printk("**************debug16*******************");
            pkt_qq_chain_head = pkt_qq_chain_head->next;
            (*pkt_qq_chain_head).prev = (struct pkt_qq *)NULL;
            if(pkt_qq_cur->next==NULL){
                pkt_qq_chain_tail=pkt_qq_chain_head;
            }
            //printk("**************debug17*******************");
            write_lock(&pkt_qq_mutex_len); // 加锁
            pkt_qq_chain_len--;
            write_unlock(&pkt_qq_mutex_len); // 解锁

            MFREE(osh, pkt_qq_cur, sizeof(*pkt_qq_cur));
        }
        
    }else{
        //printk("**************debug18*******************");
        if(pkt_qq_cur->prev!=(struct pkt_qq *)NULL){
            (*((*pkt_qq_cur).prev)).next = (*pkt_qq_cur).next;
        }
        if(pkt_qq_cur->next!=(struct pkt_qq *)NULL){
            (*((*pkt_qq_cur).next)).prev = (*pkt_qq_cur).prev;
        }else{
            pkt_qq_chain_tail=pkt_qq_chain_tail->prev;
            
        }
        //printk("**************debug19*******************");
        
        MFREE(osh, pkt_qq_cur, sizeof(*pkt_qq_cur));
        write_lock(&pkt_qq_mutex_len); // 加锁
        pkt_qq_chain_len--;
        write_unlock(&pkt_qq_mutex_len); // 解锁
        //printk("**************debug10*******************");
    }
    return;
//mutex_unlock(&pkt_qq_mutex); // 解锁
}


bool pkt_qq_retry_ergodic(uint16 FrameID, uint16 cur_pktSEQ, osl_t *osh){
    uint32 cur_time = OSL_SYSUPTIME();
    read_lock(&pkt_qq_mutex_len); // 加锁
    uint16 cur_pkt_qq_chain_len = pkt_qq_chain_len;
    read_unlock(&pkt_qq_mutex_len); // 解锁
    if(cur_pkt_qq_chain_len==0){
        return FALSE;
    }
    uint16 index = 0;
    mutex_lock(&pkt_qq_mutex_head); // 加锁
    struct pkt_qq *pkt_qq_cur = pkt_qq_chain_head;
    //printk(KERN_ALERT"###########pkt_qq_chain_len before delete(%d)",pkt_qq_chain_len);
    while((pkt_qq_cur != (struct pkt_qq *)NULL )){                    
        //printk("###****************index----------(%u)",index);
        if((pkt_qq_cur->FrameID == FrameID) && (pkt_qq_cur->pktSEQ == cur_pktSEQ)){
            pkt_qq_cur->retry_time_list_qq[pkt_qq_cur->retry_time_list_index] = cur_time;
            pkt_qq_cur->retry_time_list_index++;
            mutex_unlock(&pkt_qq_mutex_head); // 解锁
            return TRUE;
            break;
        }

        struct pkt_qq *pkt_qq_cur_next = pkt_qq_cur->next;
        pkt_qq_cur = pkt_qq_cur_next;
        //printk("###****************[fyl] pkt_qq_cur_PHYdelay----------(%u)",pkt_qq_cur_PHYdelay);
        //printk("###****************[fyl] FrameID@@@@@@@@@@@@@@@(%u)",pkt_qq_cur->FrameID);
        index++;
        if(cur_pkt_qq_chain_len<index){
            mutex_unlock(&pkt_qq_mutex_head); // 解锁
    return FALSE;
            break;
        }
    }
    mutex_unlock(&pkt_qq_mutex_head); // 解锁
    //printk("###****************index----------(%u)",index);
    //printk(KERN_ALERT"###########pkt_qq_chain_len after delete(%u)",pkt_qq_chain_len);
    return FALSE;

}

void pkt_qq_del_timeout_ergodic(osl_t *osh){
    uint32 cur_time = OSL_SYSUPTIME();
    if(!mutex_trylock(&pkt_qq_mutex_head)){
        //mutex_unlock(&pkt_qq_mutex_head); // 解锁
        return;
    }
    mutex_unlock(&pkt_qq_mutex_head); // 解锁
    read_lock(&pkt_qq_mutex_len); // 加锁
    //uint16 cur_pkt_qq_chain_len = pkt_qq_chain_len;
    read_unlock(&pkt_qq_mutex_len); // 解锁
    uint16 index = 0;

    read_lock(&pkt_qq_mutex_len); // 加锁
    //printk("**************debug14*******************");
    if(pkt_qq_chain_len<1){
        //printk("****************wrong pkt_qq_chain_len----------(%u)",pkt_qq_chain_len);
        read_unlock(&pkt_qq_mutex_len); // 解锁
        return;

    }
    //printk("**************debug15*******************");
    read_unlock(&pkt_qq_mutex_len); // 解锁
    if(!pkt_qq_chain_len_in_range(max_pkt_qq_chain_len,0)){
            
        //bool sniffer_flag = FALSE;
        //sniffer_flag = pkt_qq_len_error_sniffer(osh, 41);
        mutex_lock(&pkt_qq_mutex_head); // 加锁
        pkt_qq_delete(pkt_qq_chain_head,osh);
        pkt_qq_chain_len_outofrange ++;
        mutex_unlock(&pkt_qq_mutex_head); // 解锁
        /*
        if(pkt_qq_len_error_sniffer(osh, 4)&& !sniffer_flag){
            printk("_______error here4");
        }*/
    }
    if(!pkt_qq_chain_len_in_range(max_pkt_qq_chain_len-66,0)){
        //uint16 index = 0;
        mutex_lock(&pkt_qq_mutex_head); // 加锁
        struct pkt_qq *pkt_qq_cur = pkt_qq_chain_head;
        //printk(KERN_ALERT"###########pkt_qq_chain_len before delete(%d)",pkt_qq_chain_len);
        while(pkt_qq_cur != (struct pkt_qq *)NULL){                    
            //printk("###****************index----------(%u)",index);
            //if(cur_pkt_qq_chain_len<index + 10){//如果发现已经接近尾部就停止
            if(pkt_qq_chain_len_in_range((index + 10),0)){//如果发现已经接近尾部就停止
                mutex_unlock(&pkt_qq_mutex_head); // 解锁
                return;
            }
            if(!pkt_qq_chain_len_in_range(max_pkt_qq_chain_len,0)){        
                printk("****************wrong pkt_qq_chain_len2----------(%u)",pkt_qq_chain_len);
            }
            //uint32 cur_time = OSL_SYSUPTIME();
            uint32 pkt_qq_cur_PHYdelay = cur_time - pkt_qq_cur->into_hw_time;
            struct pkt_qq *pkt_qq_cur_next = pkt_qq_cur->next;
            if((pkt_qq_cur_PHYdelay>pkt_qq_ddl)||(pkt_qq_cur->free_time > 0)){/*每隔一段时间删除超时的数据包节点以及已经ACK的数据包*/
#ifdef PRINTTIMEOUTPKT
                kernel_info_t info_qq[DEBUG_CLASS_MAX_FIELD];
                pkt_qq_cur->droped_withoutACK_time = cur_time;
                memcpy(info_qq, pkt_qq_cur, sizeof(*pkt_qq_cur));
                debugfs_set_info_qq(0, info_qq, 1);
#endif
                pkt_qq_delete(pkt_qq_cur,osh);
                pkt_qq_chain_len_timeout ++;
            }
            pkt_qq_cur = pkt_qq_cur_next;
            //printk("###****************[fyl] pkt_qq_cur_PHYdelay----------(%u)",pkt_qq_cur_PHYdelay);
            //printk("###****************[fyl] FrameID@@@@@@@@@@@@@@@(%u)",pkt_qq_cur->FrameID);
            index++;
        }
        mutex_unlock(&pkt_qq_mutex_head); // 解锁
        //printk("###****************index----------(%u)",index);
        //printk(KERN_ALERT"###########pkt_qq_chain_len after delete(%u)",pkt_qq_chain_len);
    }
}


void ack_update_qq(wlc_info_t *wlc, scb_ampdu_tid_ini_t* ini,ampdu_tx_info_t *ampdu_tx, struct scb *scb, 
            tx_status_t *txs, wlc_pkttag_t* pkttag, wlc_txh_info_t *txh_info,bool was_acked,osl_t *osh
            , void *p, bool use_last_pkt, uint cur_mpdu_index, ratesel_txs_t rs_txs,uint32 receive_time,uint32 *ccastats_qq_cur){
    //新bool use_last_pkt变量，用于减少遍历搜索的时间，在此之前每个数据包都需要从头开始搜索，太耗时间
    //加上它以后，就可以从上次搜索的地方开始搜索。对于AMPDU的情况会有较好的结果

    //mutex_lock(&pkt_qq_mutex); // 加锁
    //printk("**************debug1*******************");
    uint slottime_qq = APHY_SLOT_TIME;
    ampdu_tx_config_t *ampdu_tx_cfg = ampdu_tx->config;
    if (wlc->band->gmode && !wlc->shortslot)
        slottime_qq = BPHY_SLOT_TIME;
    //uint16 curTxFrameID = txh_info->TxFrameID;
    uint8 *pkt_data = PKTDATA(osh, p);
    uint corerev = wlc->pub->corerev;
    uint hdrSize;
    uint tsoHdrSize = 0;
#ifdef WLC_MACDBG_FRAMEID_TRACE
        uint8 *tsoHdrPtr;
        uint8 epoch = 0;
#endif
    d11txhdr_t *txh;
    struct dot11_header *d11h = NULL;
    BCM_REFERENCE(d11h);
    if (D11REV_LT(corerev, 40)) {
        hdrSize = sizeof(d11txh_pre40_t);
        txh = (d11txhdr_t *)pkt_data;
        d11h = (struct dot11_header*)((uint8*)txh + hdrSize +
            D11_PHY_HDR_LEN);
    } else {
#ifdef WLTOEHW
        tsoHdrSize = WLC_TSO_HDR_LEN(wlc, (d11ac_tso_t*)pkt_data);
#ifdef WLC_MACDBG_FRAMEID_TRACE
        tsoHdrPtr = (void*)((tsoHdrSize != 0) ? pkt_data : NULL);
        epoch = (tsoHdrPtr[2] & TOE_F2_EPOCH) ? 1 : 0;
#endif /* WLC_MACDBG_FRAMEID_TRACE */
#endif /* WLTOEHW */
        if (D11REV_GE(corerev, 128)) {
            hdrSize = D11_TXH_LEN_EX(wlc);
        } else {
            hdrSize = sizeof(d11actxh_t);
        }

        txh = (d11txhdr_t *)(pkt_data + tsoHdrSize);
        d11h = (struct dot11_header*)((uint8*)txh + hdrSize);
    }
    uint16 curTxFrameID = ltoh16(*D11_TXH_GET_FRAMEID_PTR(wlc, txh));
    uint8 tid = ini->tid;
    //uint16 deleteNUM_delay = 0;
    uint32 cur_airtime = TX_STATUS128_TXDUR(TX_STATUS_MACTXS_S2(txs));
    mutex_lock(&pkt_qq_mutex_head); // 加锁
    read_lock(&pkt_qq_mutex_len); // 加锁
    uint16 cur_pkt_qq_chain_len = pkt_qq_chain_len;
    read_unlock(&pkt_qq_mutex_len); // 解锁
    struct pkt_qq *pkt_qq_cur;
    uint16 index;
    printk(KERN_ALERT"###########pkt_qq_chain_len debug-2(%u)",pkt_qq_chain_len);
    read_lock(&pkt_qq_mutex_len); // 加锁
    if(pkt_qq_chain_len<1){
        pkt_qq_chain_len_notfound++;

        printk("----------[fyl] not found(%u:%u:%u)",OSL_SYSUPTIME(),curTxFrameID,pkttag->seq);
        read_unlock(&pkt_qq_mutex_len); // 解锁
        return;
    }
#ifdef USE_LAST_PKT/*如果use_last_pkt为false，或者后两个不对劲，就重新设置*/
    printk(KERN_ALERT"###########pkt_qq_chain_len debug-1(%u)",pkt_qq_chain_len);

    if((!use_last_pkt)||(index_last <= 0) || (pkt_qq_last == (struct pkt_qq *)NULL)\
        ||(index_last>=cur_pkt_qq_chain_len)){
        /*如果use_last_pkt为false，或者后两个不对劲，就重新设置*/
#endif
        pkt_qq_cur = pkt_qq_chain_head;
        index = 0;
#ifdef USE_LAST_PKT
    }else{
        pkt_qq_cur = pkt_qq_last;
        index = index_last;

    }
#endif
    pkt_qq_cur = (struct pkt_qq *)(uintptr)pkttag->qq_pktinfo_pointer;
    printk(KERN_ALERT"###########pkt_qq_chain_len debug-11(%u)",pkt_qq_chain_len);
    read_unlock(&pkt_qq_mutex_len); // 解锁
    bool found_pkt_node_qq = FALSE;

    #if 0
    if((pkt_qq_cur->pktnum_to_send_end<=0)&&(receive_time>last_scan_time+min_scan_interval)){

        active_time = (active_time + 10) % 100;
        scan_time = (scan_time + 10) % 100;
        if(active_time<10){
            active_time = 10;
        }
        if(scan_time<10){
            scan_time = 10;
        }
        printk("start scan;scan_time(%u);active_time(%u);----------[fyl] OSL_SYSUPTIME()----------(%u)",scan_time,active_time,OSL_SYSUPTIME());
        /*chanspec_t chanspec_cur = (48 << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BAND_5G) |
            (WL_CHANSPEC_BW_20) |
            (WL_CHANSPEC_CTL_SB_NONE) |
            (WL_CHANSPEC_BW_20);
        chanspec_t chanspec_cur = (36 << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BW_160 & WL_CHANSPEC_BW_MASK) |
            (WL_CHANSPEC_BAND_5G & WL_CHANSPEC_BAND_MASK);
        chanspec_t chanspec_cur = (50 & WL_CHANSPEC_CHAN_MASK) |
            (WL_CHANSPEC_BW_160 & WL_CHANSPEC_BW_MASK) |
            (WL_CHANSPEC_BAND_5G & WL_CHANSPEC_BAND_MASK);
        chanspec_t chanspec_cur = (36 << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BAND_5G) |
            (WL_CHANSPEC_BW_80) |
            (WL_CHANSPEC_CTL_SB_NONE) |
            (WL_CHANSPEC_BW_80);*/
        uint8 primary_channel = receive_time%28 + 36;
        chanspec_t chanspec_cur = (chanspec_t ) 0x8e0a;
        if(receive_time%10==0){
            chanspec_cur = (chanspec_t ) 0x8e0a;
        }else if(receive_time%10==1){
            chanspec_cur = (36 << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BAND_5G) |
            (WL_CHANSPEC_BW_80) |
            (WL_CHANSPEC_CTL_SB_NONE) |
            (WL_CHANSPEC_BW_80);
        }else if(receive_time%10==2){
            chanspec_cur = (36 << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BAND_5G) |
            (WL_CHANSPEC_BW_40) |
            (WL_CHANSPEC_CTL_SB_NONE) |
            (WL_CHANSPEC_BW_40);
        }else if(receive_time%10==3){
            chanspec_cur = wf_create_80MHz_chspec(52, 58,
                       WL_CHANSPEC_BW_80);
        }else if(receive_time%10==4){
            chanspec_cur = wf_create_chspec_from_primary(primary_channel, WL_CHANSPEC_BW_40,
                       WL_CHANSPEC_BAND_5G);
        }else if(receive_time%10==5){
            chanspec_cur = wf_create_chspec_from_primary(56, WL_CHANSPEC_BW_40,
                       WL_CHANSPEC_BAND_5G);
        }else if(receive_time%10==6){
            chanspec_cur = wf_create_chspec_from_primary(primary_channel, WL_CHANSPEC_BW_160,
                       WL_CHANSPEC_BAND_5G);
        }else if(receive_time%10==7){
            chanspec_cur = wf_create_chspec_from_primary(42, WL_CHANSPEC_BW_160,
                       WL_CHANSPEC_BAND_5G);
        }else if(receive_time%10==8){
            chanspec_cur = wf_create_chspec_from_primary(primary_channel, WL_CHANSPEC_BW_80,
                       WL_CHANSPEC_BAND_5G);
        }else if(receive_time%10==9){
            chanspec_cur = wf_create_chspec_from_primary(56, WL_CHANSPEC_BW_80,
                       WL_CHANSPEC_BAND_5G);
        }

        //wf_create_chspec_from_primary
        /*printk("receive_time10(%u);primary_channel(%u);chanspec(0x%04x:0x%04x,0x%04x,0x%04x)"
            ,receive_time%10,primary_channel,chanspec_cur,(48 << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BAND_5G) |
            (WL_CHANSPEC_BW_20) |
            (WL_CHANSPEC_CTL_SB_NONE) |
            (WL_CHANSPEC_BW_20),(36 << WL_CHANSPEC_CHAN_SHIFT) |
            (WL_CHANSPEC_BW_160 & WL_CHANSPEC_BW_MASK) |
            (WL_CHANSPEC_BAND_5G & WL_CHANSPEC_BAND_MASK),(50 & WL_CHANSPEC_CHAN_MASK) |
            (WL_CHANSPEC_BW_160 & WL_CHANSPEC_BW_MASK) |
            (WL_CHANSPEC_BAND_5G & WL_CHANSPEC_BAND_MASK));*/
        //scan_channel(wlc, chanspec_cur);
        //scan_channel(wlc, wlc->chanspec);
        last_scan_time = receive_time;
        //printk("end scan----------[fyl] OSL_SYSUPTIME()----------(%u)",OSL_SYSUPTIME());
    }
    #endif

    while((pkt_qq_cur != (struct pkt_qq *)NULL)&&(index<cur_pkt_qq_chain_len)){
        struct pkt_qq *pkt_qq_cur_next = pkt_qq_cur->next;
        //printk("**************debug5*******************");
        index++;
        if(cur_pkt_qq_chain_len<=index+2){//代表很有可能是末尾的节点，此时需要加上尾端锁。
        //但是要注意，一定要避免循环中未解锁，以及多次循环导致的重复加锁。
            mutex_lock(&pkt_qq_mutex_tail); // 加锁
            //printk("**************debug7*******************");
        }
        pkt_qq_cur->airtime_all += cur_airtime;
        uint32 cur_time = receive_time;
        uint32 pkt_qq_cur_PHYdelay = cur_time - pkt_qq_cur->into_hw_time;
        uint16 cur_pktSEQ = pkttag->seq;
        //if(pkt_qq_cur->pktSEQ == cur_pktSEQ ){//如果找到了这个数据包
        //if(pkt_qq_cur->FrameID == htol16(curTxFrameID) ){//如果找到了这个数据包
        if((pkt_qq_cur->FrameID == htol16(curTxFrameID)) && (pkt_qq_cur->pktSEQ == cur_pktSEQ)){//如果找到了这个数据包 
                printk(KERN_ALERT"###########pkt_qq_chain_len debug0(%u)",pkt_qq_chain_len);
                found_pkt_node_qq = TRUE;
            if((!was_acked)||((was_acked)&&(pkt_qq_cur_PHYdelay >= 17 || pkt_qq_cur->failed_cnt>=1))){//提前判断一下，降低总体计算量
                printk(KERN_ALERT"###########index(%u)",index);
                
                pkt_qq_cur->airtime_self = cur_airtime;
                pkt_qq_cur->tid = tid;
                if(was_acked){//如果成功ACK 
                    uint16 index_i = 0;
                    for(int i = 0; i<pkt_phydelay_dict_len; i++){
                        index_i = i;
                        if(i*pkt_phydelay_dict_step+pkt_phydelay_dict_step>pkt_qq_cur_PHYdelay){
                            
                            break;
                        }
                    }
                    pkt_phydelay_dict[index_i]++;
                    pkt_qq_cur->pkt_qq_chain_len_add_end = pkt_qq_chain_len_add;
                    /*计算等待发送的数据包量*/
                    uint fifo = D11_TXFID_GET_FIFO(wlc, htol16(curTxFrameID));
                    hnddma_t *tx_di = WLC_HW_DI(wlc, fifo);
                    dma_info_t *di = DI_INFO(tx_di);
                    pkt_qq_cur->pktnum_to_send_end = NTXDACTIVE(di->txin, di->txout) + 1;
                    pkt_qq_cur->pkt_added_in_wlc_tx_end = pkt_added_in_wlc_tx;
                    pkt_qq_cur->APnum = num_routers;
                    pkt_qq_cur->free_time = cur_time;
                    pkt_qq_cur->free_txop = wlc_bmac_cca_read_counter(wlc->hw, M_CCA_TXOP_L_OFFSET(wlc), M_CCA_TXOP_H_OFFSET(wlc));
                    pkt_qq_cur->ps_dur_trans = 0;//当前帧发送期间PS 时间统计
                    if(scb->PS){
                        pkt_qq_cur->ps_dur_trans = scb->ps_tottime - pkt_qq_cur->ps_totaltime + cur_time - scb->ps_starttime;
                    }else{
                        pkt_qq_cur->ps_dur_trans = scb->ps_tottime - pkt_qq_cur->ps_totaltime;
                    }
                    uint32 ccastats_qq_differ[CCASTATS_MAX];
                    for (int i = 0; i < CCASTATS_MAX; i++) {
                        ccastats_qq_differ[i] = ccastats_qq_cur[i] - pkt_qq_cur->ccastats_qq[i];
                    }
                    pkt_qq_cur->busy_time = ccastats_qq_differ[CCASTATS_TXDUR] +
                        ccastats_qq_differ[CCASTATS_INBSS] +
                        ccastats_qq_differ[CCASTATS_OBSS] +
                        ccastats_qq_differ[CCASTATS_NOCTG] +
                        ccastats_qq_differ[CCASTATS_NOPKT];
                    memcpy(pkt_qq_cur->ccastats_qq_differ, ccastats_qq_differ, sizeof(pkt_qq_cur->ccastats_qq_differ));
                    memcpy(&(pkt_qq_cur->rates_counts_txs_qq_end), cur_rates_counts_txs_qq, sizeof(struct rates_counts_txs_qq));
                    /*for(uint i = 0; i<8; i++){
                        printk("tx_cnt2(%u:%u:%u)",i,pkt_qq_cur->rates_counts_txs_qq_end.tx_cnt[i],cur_rates_counts_txs_qq->tx_cnt[i]);
                    }*/
                    pkt_qq_cur->txop_in_fly = (pkt_qq_cur->free_txop - pkt_qq_cur->into_hw_txop)*slottime_qq;
                    scb_pps_info_t *pps_scb_qq = SCB_PPSINFO(wlc->pps_info, scb);            
                    uint32 time_in_pretend_tot_qq = pps_scb_qq->ps_pretend_total_time_in_pps;
                    if (pps_scb_qq == NULL){
                        time_in_pretend_tot_qq += R_REG(wlc->osh, D11_TSFTimerLow(wlc)) - pps_scb_qq->ps_pretend_start;
                    }
                    pkt_qq_cur->time_in_pretend_in_fly = time_in_pretend_tot_qq - pkt_qq_cur->time_in_pretend_tot;
                    pkt_qq_cur->ampdu_seq = cur_mpdu_index;

                    struct phy_info_qq *phy_info_qq_cur = NULL;
                    phy_info_qq_cur = (struct phy_info_qq *) MALLOCZ(osh, sizeof(*phy_info_qq_cur));
                    phy_info_qq_cur->fix_rate = (ltoh16(txh_info->MacTxControlHigh) & D11AC_TXC_FIX_RATE) ? 1:0;
                    wf_rspec_to_phyinfo_qq(rs_txs, phy_info_qq_cur);
                    //phy_info_qq_cur->RSSI = TGTXS_PHYRSSI(TX_STATUS_MACTXS_S8(txs));
                    //phy_info_qq_cur->RSSI = ((phy_info_qq_cur->RSSI) & PHYRSSI_SIGN_MASK) ? (phy_info_qq_cur->RSSI - PHYRSSI_2SCOMPLEMENT) : phy_info_qq_cur->RSSI;
                    //phy_info_qq_cur->RSSI = pkttag->pktinfo.misc.rssi;
                    //wlc_d11rxhdr_t	*wrxh = (wlc_d11rxhdr_t *)PKTDATA(osh, p);
                    //phy_info_qq_cur->RSSI = phy_rssi_compute_rssi(WLC_PI(wlc), wrxh);
                    //phy_info_qq_cur->RSSI = wrxh->rssi;
                    
                    phy_info_qq_cur->RSSI = phy_info_qq_rx_new.RSSI;
                    memcpy(phy_info_qq_cur->rssi_ring_buffer, phy_info_qq_rx_new.rssi_ring_buffer, sizeof(DataPoint_qq)*RSSI_RING_SIZE);

                    //printk("rssi12345135345(%d,%d,%d)SNR(%d)",phy_info_qq_cur->RSSI,phy_info_qq_rx_new.RSSI,pkttag->pktinfo.misc.rssi,pkttag->pktinfo.misc.snr);
                    phy_info_qq_cur->SNR = pkttag->pktinfo.misc.snr;
                    phy_info_qq_cur->noiselevel = wlc_lq_chanim_phy_noise(wlc);
                    phy_info_qq_cur->rssi_ring_buffer_index = rssi_ring_buffer_index;
                    kernel_info_t info_qq[DEBUG_CLASS_MAX_FIELD];
                    memcpy(info_qq, phy_info_qq_cur, sizeof(*phy_info_qq_cur));
                    debugfs_set_info_qq(2, info_qq, 1);
                    MFREE(osh, phy_info_qq_cur, sizeof(*phy_info_qq_cur));
                    if(pkt_qq_cur_PHYdelay >= 17 || pkt_qq_cur->failed_cnt>=1){//如果时延较高就打印出来
                        //printk("----------[fyl] phy_info_qq_cur:mcs(%u):rate(%u):fix_rate(%u)----------",phy_info_qq_cur->mcs[0],phy_info_qq_cur->rate[0],phy_info_qq_cur->fix_rate);
                        //int dump_rand_flag = OSL_RAND() % 10000;
                        kernel_info_t info_qq[DEBUG_CLASS_MAX_FIELD];
                        memcpy(info_qq, pkt_qq_cur, sizeof(*pkt_qq_cur));
                        debugfs_set_info_qq(0, info_qq, 1);
                        //if (!use_last_pkt) {/*use_last_pkt代表非第一个mpdu，所以这里指的是只打印第一个mpdu的信息*/
                        if (0) {/*use_last_pkt代表非第一个mpdu，所以这里指的是只打印第一个mpdu的信息*/
                            printk("----------[fyl] OSL_SYSUPTIME()1----------(%u)",OSL_SYSUPTIME());
                            printk("----------[fyl] acked_FrameID----------(%u)",pkt_qq_cur->FrameID);
                            printk("----------[fyl] pktSEQ----------(%u)",pkt_qq_cur->pktSEQ);
                            read_lock(&pkt_qq_mutex_len); // 加锁
                            printk("----------[fyl] pkt_qq_chain_len----------(%u)",pkt_qq_chain_len);
                            read_unlock(&pkt_qq_mutex_len); // 解锁
                            printk("----------[fyl] cur_mpdu_index----------(%u)",cur_mpdu_index);/*当前mpdu在ampdu中的编号*/
                            printk("----------[fyl] pkt_qq_cur->failed_cnt----------(%u)",pkt_qq_cur->failed_cnt);
                            printk("----------[fyl] pkt_qq_cur_PHYdelay----------(%u)",pkt_qq_cur_PHYdelay);
                            printk("----------[fyl] pkt_qq_cur->free_time----------(%u)",pkt_qq_cur->free_time);
                            printk("----------[fyl] pkt_qq_cur->into_hw_time----------(%u)",pkt_qq_cur->into_hw_time);
                            printk("----------[fyl] pkt_qq_cur->airtime_self----------(%u)",pkt_qq_cur->airtime_self);
                            //printk("----------[fyl] pkt_qq_cur->airtime_all----------(%u)",pkt_qq_cur->airtime_all);
                            printk("----------[fyl] busy_qq----------(%u)",pkt_qq_cur->busy_time);
                            printk("----------[fyl] free_txop:::into_hw_txop:::txop*9----------(%u:%u:%u)",pkt_qq_cur->free_txop, pkt_qq_cur->into_hw_txop,pkt_qq_cur->txop_in_fly);
                            printk("----------[fyl] pkt_qq_cur:ps_pretend_probe(%u):::ps_pretend_count(%u):::ps_pretend_succ_count(%u):::ps_pretend_failed_ack_count(%u)----------",\
                            pkt_qq_cur->ps_pretend_probe, pkt_qq_cur->ps_pretend_count,pkt_qq_cur->ps_pretend_succ_count,pkt_qq_cur->ps_pretend_failed_ack_count);
                            printk("----------[fyl] pps_scb_qq:ps_pretend_probe(%u):::ps_pretend_count(%u):::ps_pretend_succ_count(%u):::ps_pretend_failed_ack_count(%u)----------",\
                            pps_scb_qq->ps_pretend_probe, pps_scb_qq->ps_pretend_count,pps_scb_qq->ps_pretend_succ_count,pps_scb_qq->ps_pretend_failed_ack_count);

                            printk("----------[fyl] ampdu_tx_cfg->ba_policy----------(%u)",ampdu_tx_cfg->ba_policy);
                            //printk("----------[fyl] ampdu_tx_cfg->ba_policy::ba_rx_wsize::delba_timeout----------(%u)",
                                        //ampdu_tx_cfg->ba_policy,ba_rx_wsize,delba_timeout);
                            /*printk("ccastats_qq_differ:TXDUR(%u)INBSS(%u)OBSS(%u)NOCTG(%u)NOPKT(%u)",ccastats_qq_differ[0]\
                                ,ccastats_qq_differ[1],ccastats_qq_differ[2],ccastats_qq_differ[3]\
                                ,ccastats_qq_differ[4]);*/
                            printk("----------[fyl] time_in_pretend_tot_qq:::pkt_qq_cur->time_in_pretend_tot:::R_REG(wlc->osh, D11_TSFTimerLow(wlc)):::pps_scb_qq->ps_pretend_start:::time_in_pretend----------(%u:%u:%u:%u:%u)",time_in_pretend_tot_qq,pkt_qq_cur->time_in_pretend_tot,R_REG(wlc->osh, D11_TSFTimerLow(wlc)),pps_scb_qq->ps_pretend_start,time_in_pretend_tot_qq - pkt_qq_cur->time_in_pretend_tot);
                            printk("----------[fyl] ini->tid----------(%u)",tid);
                            printk("----------[fyl] scb->ps_tottime:scb->ps_starttime:ps_dur_trans----------(%u:%u:%u)",scb->ps_tottime,scb->ps_starttime,pkt_qq_cur->ps_dur_trans);
                            
                            printk("----------[fyl] PS:::ps_pretend:::PS_TWT:::ps_txfifo_blk----------(%u:%u:%u:%u)",
                                        scb->PS, scb->ps_pretend,scb->PS_TWT,scb->ps_txfifo_blk);
                            printk("--[fyl] txs->status.rts_tx_cnt:txs->status.cts_tx_cnt---(%u:%u)",txs->status.rts_tx_cnt,txs->status.cts_rx_cnt);
                            printk("ccastats_qq_differ:TXDUR(%u)INBSS(%u)OBSS(%u)NOCTG(%u)NOPKT(%u)DOZE(%u)TXOP(%u)GDTXDUR(%u)BDTXDUR(%u)",ccastats_qq_differ[0]\
                                ,ccastats_qq_differ[1],ccastats_qq_differ[2],ccastats_qq_differ[3]\
                                ,ccastats_qq_differ[4],ccastats_qq_differ[5],ccastats_qq_differ[6]\
                                ,ccastats_qq_differ[7],ccastats_qq_differ[8]);
                            if(pkt_qq_cur->failed_cnt>0){
                                printk("failed_time_list_qq:0(%u)1(%u)2(%u)3(%u)4(%u)5(%u)6(%u)7(%u)8(%u)9(%u)",pkt_qq_cur->failed_time_list_qq[0]\
                                ,pkt_qq_cur->failed_time_list_qq[1],pkt_qq_cur->failed_time_list_qq[2],pkt_qq_cur->failed_time_list_qq[3]\
                                ,pkt_qq_cur->failed_time_list_qq[4],pkt_qq_cur->failed_time_list_qq[5],pkt_qq_cur->failed_time_list_qq[6]\
                                ,pkt_qq_cur->failed_time_list_qq[7],pkt_qq_cur->failed_time_list_qq[8],pkt_qq_cur->failed_time_list_qq[9]);
                            }
                            printk("----------[fyl] OSL_SYSUPTIME()2----------(%u)",OSL_SYSUPTIME());
                        }

                    }
                    /*删除已经ACK的数据包节点*/
                    //struct pkt_qq *pkt_qq_cur_next = pkt_qq_cur->next;
                    pkt_qq_last = pkt_qq_cur_next;
                    index_last = index;
                    pkt_qq_delete(pkt_qq_cur,osh);
                    pkt_qq_chain_len_acked++;
                    //pkt_qq_cur = pkt_qq_cur_next;

                    //break;                   
                    //pkt_qq_cur = pkt_qq_cur->next;
                    //continue; 
                }else{//未收到ACK则增加计数
                    /*用于记录出现重传包重传时，函数调用路径*/
                    debug_qqdx_retry_pkt.FrameID = pkt_qq_cur->FrameID;
                    debug_qqdx_retry_pkt.pktSEQ = pkt_qq_cur->pktSEQ;
                    debug_qqdx_retry_pkt.into_hw_time = pkt_qq_cur->into_hw_time;
                    debug_qqdx_retry_pkt.time_in_pretend_tot = pkt_qq_cur->time_in_pretend_tot;
                    debug_qqdx_retry_pkt.ps_totaltime = pkt_qq_cur->ps_totaltime;
                    pkt_qq_chain_len_unacked ++;
                    if((pkt_qq_cur->failed_cnt>0)&&(pkt_qq_cur->failed_time_list_qq[pkt_qq_cur->failed_cnt-1]==cur_time)){/*如果同时到达的，就不认为是重传*/
                        
                    }else{
                        if(pkt_qq_cur->failed_cnt<10){
                            pkt_qq_cur->failed_time_list_qq[pkt_qq_cur->failed_cnt] = cur_time;
                        }
                        
                        pkt_qq_cur->failed_cnt++;
                        //break;
                    }
                    memcpy(debug_qqdx_retry_pkt.failed_time_list_qq,pkt_qq_cur->failed_time_list_qq,sizeof(pkt_qq_cur->failed_time_list_qq));
                    
                    /*
                    printk("----------[fyl] unacked_FrameID----------(%u)",pkt_qq_cur->FrameID);
                    printk("----------[fyl] pktSEQ----------(%u)",pkt_qq_cur->pktSEQ);
                    printk("----------[fyl] cur_time----------(%u)",cur_time);
                    printk("----------[fyl] into_hw_time----------(%u)",pkt_qq_cur->into_hw_time);
                    printk("----------[fyl] now-into_hw_time----------(%u)",cur_time-pkt_qq_cur->into_hw_time);
                    */
                    
                    
                    //debug_qqdx_retry_pkt.failed_time_list_qq = pkt_qq_cur->failed_time_list_qq;
                    //pkt_qq_cur = pkt_qq_cur->next;continue;
                }
            }
            else{
                if(was_acked){
                    pkt_qq_last = pkt_qq_cur_next;
                    index_last = index;
                    pkt_qq_delete(pkt_qq_cur,osh);
                    pkt_qq_chain_len_acked++;
                    uint16 index_i = 0;
                    for(int i = 0; i<pkt_phydelay_dict_len; i++){
                        index_i = i;
                        if(i*pkt_phydelay_dict_step+pkt_phydelay_dict_step>pkt_qq_cur_PHYdelay){
                            
                            break;
                        }
                    }
                    pkt_phydelay_dict[index_i]++;
                }
            }
        }else{

            if(pkt_qq_cur_PHYdelay > pkt_qq_ddl){//如果该节点并非所要找的节点，并且该数据包时延大于ddl，就删除该节点
                //deleteNUM_delay++;
                //struct pkt_qq *pkt_qq_cur_next = pkt_qq_cur->next;
                printk(KERN_ALERT"###########pkt_qq_chain_len debug01(%u)",pkt_qq_chain_len);
#ifdef PRINTTIMEOUTPKT
                kernel_info_t info_qq[DEBUG_CLASS_MAX_FIELD];
                pkt_qq_cur->droped_withoutACK_time = cur_time;
                memcpy(info_qq, pkt_qq_cur, sizeof(*pkt_qq_cur));
                debugfs_set_info_qq(0, info_qq, 1);
#endif
                printk(KERN_ALERT"###########pkt_qq_chain_len debug1(%u)",pkt_qq_chain_len);
                pkt_qq_delete(pkt_qq_cur,osh);
                printk(KERN_ALERT"###########pkt_qq_chain_len debug2(%u)",pkt_qq_chain_len);
                pkt_qq_chain_len_timeout ++;
            
                //pkt_qq_cur = pkt_qq_cur_next;
                //index++;
                
                //continue;
            }
        }
        printk(KERN_ALERT"###########pkt_qq_chain_len debug3(%u)",pkt_qq_chain_len);
        pkt_qq_cur = pkt_qq_cur_next;     
        //printk("**************debug4*******************");

        if(cur_pkt_qq_chain_len<=index+2){//代表很有可能是末尾的节点，此时需要加上尾端锁
            mutex_unlock(&pkt_qq_mutex_tail); // 解锁
            //printk("**************debug6*******************");
        }
        //printk("**************debug3*******************");
printk(KERN_ALERT"###########pkt_qq_chain_len debug4(%u)",pkt_qq_chain_len);

        
    }
    if(found_pkt_node_qq){
        pkt_qq_chain_len_found++;
    }else{
        pkt_qq_chain_len_notfound++;

        printk("----------[fyl] not found(%u:%u:%u)",OSL_SYSUPTIME(),curTxFrameID,pkttag->seq);
    }
    mutex_unlock(&pkt_qq_mutex_head); // 解锁
    //printk("****************[fyl] index:deleteNUM_delay----------(%u:%u)",index,deleteNUM_delay);

    //printk("**************debug2*******************");

    read_lock(&pkt_qq_mutex_len); // 加锁
    //printk("**************debug14*******************");
    if(pkt_qq_chain_len>0){
        read_unlock(&pkt_qq_mutex_len); // 解锁
        pkt_qq_del_timeout_ergodic(osh);
    }else{
        read_unlock(&pkt_qq_mutex_len); // 解锁

    }
printk(KERN_ALERT"###########pkt_qq_chain_len debug5(%u)",pkt_qq_chain_len);
    //printk("**************debug15*******************");
    
    //mutex_unlock(&pkt_qq_mutex); // 解锁
    //printk("**************debug8*******************");
    
}
