
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






/* dump_flag_qqdx */
struct pkt_qq {
    uint32 tcp_seq;/* Starting sequence number */
    uint32 ampdu_seq;/* preassigned seqnum for AMPDU */
    uint32 packetid;/* 未知变量packetid */
    uint16 FrameID;//每个数据帧生命周期不变的
    uint16 pktSEQ;//也许每个数据包生命周期不变的
	uint16 n_pkts;       /**< number of queued packets */
    uint8 tid;//tid
    uint32 into_hw_time;/*进入硬件队列的时间*/
    uint32 free_time;/*传输成功被释放的时间*/
    uint32 into_hw_txop;/*进入硬件队列的txop*/
    uint32 free_txop;/*传输成功被释放的txop*/
    uint32 txop_in_fly;/*传输过程中的busy_time*/
    uint32 busy_time;/*传输过程中的txop*/
    uint32 drop_time;/*传输失败被丢弃的时间*/
    uint8 failed_cnt;/*发射失败次数*/
    uint32 ps_totaltime;/*该scb设备进入ps的总时间，为了这个统计，我在wl_mk中添加了WL_PS_STATS = 1，但是失败了，路由器崩溃
    所以我在多处增加了#ifndef WL_PS_STATS #define WL_PS_STATS*/
    uint32 ps_dur_trans;//传输过程中的PS统计
    uint32 airtime_self;/*该数据包所在帧的airtime*/
    uint32 airtime_all;/*该数据包进入硬件发送队列以后所有已发送帧的airtime之和*/
    uint32 failed_time_list_qq[10];/*发射失败时间列表*/
    uint32 retry_time_list_qq[10];/*发射失败重传时间列表*/
    uint32 retry_time_list_index;/*发射失败重传时间列表当前index*/
    uint32 ccastats_qq[CCASTATS_MAX];/*一些发送时间相关的变量*/
    uint32 ccastats_qq_differ[CCASTATS_MAX];
    /*PPS相关变量*/
	uint32 ps_pretend_probe;
	uint32 ps_pretend_count;
	uint8  ps_pretend_succ_count;
	uint8  ps_pretend_failed_ack_count;
    uint32 time_in_pretend_tot;
    uint32 time_in_pretend_in_fly;
    uint32 pktnum_to_send;
    /*总的进入PPS 时间
    该统计博通并未开启，通过BCMDBG宏来关闭相关统计，需要一个一个开启（将BCMDBG改为BCMDBG_PPS_qq并define），如下是所开启的相关部分：
    1.wlc_pspretend_scb_time_upd相关（wlc_pspretend.h，wlc_pspretend.c,wlc_app.c）
    2.wlc_pspretend.h中增加#ifndef BCMDBG_PPS_qq   #define BCMDBG_PPS_qq   #endif
    3.wlc_pspretend_supr_upd相关（wlc_pspretend.h，wlc_pspretend.c,wlc_app.c,wlc_ampdu.c,wlc_txs.c）
    4.scb_pps_info_t定义处（wlc_pspretend.c）
    5.本文件   PPS时间统计相关   部分
    */

    struct pkt_qq *next;
    struct pkt_qq *prev;
    
}pkt_qq_t;
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
/*统计链表增减节点的各种情况*/
uint32 pkt_qq_chain_len_add = 0;//链表增加
uint32 pkt_qq_chain_len_add_last = 0;//记录上次链表增加过的数据包数量
uint32 pkt_qq_chain_len_acked = 0;//正常收到ack并删除
uint32 pkt_qq_chain_len_unacked = 0;//正常收到ack并删除
uint32 pkt_qq_chain_len_timeout = 0;//超时删除
uint32 pkt_qq_chain_len_outofrange = 0;//超过链表最大长度删除
uint32 pkt_qq_chain_len_notfound = 0;// 遍历链表没有找到
uint32 pkt_qq_chain_len_found = 0;// 遍历链表没有找到
#define PKTCOUNTCYCLE 100000//每隔100000个包打印一次数据包统计信息

//DEFINE_MUTEX(pkt_qq_mutex); // 定义一个互斥锁
DEFINE_MUTEX(pkt_qq_mutex_tail); // 定义一个互斥锁
DEFINE_MUTEX(pkt_qq_mutex_head); // 定义一个互斥锁
DEFINE_RWLOCK(pkt_qq_mutex_len); // 定义一个读写锁

/*记录PS时间
uint32 PS_time_all_cnt = 0;//从开机开始进入PS的时间累加
bool PS_is_on = FALSE;//判断当前是否正处于PS
uint32 PS_start_last = 0;//最近一次进入PS的时间
*/

/*定时器初始化相关*/
#ifdef QQ_TIMER_ABLE
bool timer_qq_loaded = FALSE;
#define TIMER_INTERVAL_MS_qq (1) // 1ms
static struct timer_list timer_qq;
static uint32 timer_index_qq = 0;
void timer_callback_qq(struct timer_list *t) {
    // 每隔1ms对index加一
    timer_index_qq++;
    // 重新设置定时器
    mod_timer(&timer_qq, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_qq));
}
#endif

/*是否打印超时被删除的包，专用于debug*/
#define PRINTTIMEOUTPKT


#ifndef BCMDBG_PPS_qq
#define BCMDBG_PPS_qq
#endif
/* PPS时间统计相关 *//* module private info */
struct wlc_pps_info {
	wlc_info_t *wlc;
	osl_t *osh;
	int scb_handle;
	struct  wl_timer *ps_pretend_probe_timer;
	bool is_ps_pretend_probe_timer_running;
};
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



struct pkt_count_qq {
    uint32 pkt_qq_chain_len_now;
    uint32 pkt_qq_chain_len_add;
    uint32 pkt_qq_chain_len_acked;
    uint32 pkt_qq_chain_len_unacked;
    uint32 pkt_qq_chain_len_timeout;
    uint32 pkt_qq_chain_len_outofrange;
    uint32 pkt_qq_chain_len_notfound;
    uint32 pkt_qq_chain_len_found;
    uint32 pkt_phydelay_dict[30];    
}pkt_count_qq_t;



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

struct phy_info_qq {
    uint8 fix_rate;
    uint32 mcs[RATESEL_MFBR_NUM];
    uint32 nss[RATESEL_MFBR_NUM];
    uint32 rate[RATESEL_MFBR_NUM];
    uint BW[RATESEL_MFBR_NUM];
    uint32 ISSGI[RATESEL_MFBR_NUM];
    int16 RSSI;
    int8 noiselevel;
}phy_info_qq_t;

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


#ifdef QQ_TIMER_ABLE
    if(!timer_qq_loaded){
        timer_setup(&timer_qq, timer_callback_qq, 0);

        // 设置定时器间隔为1ms
        mod_timer(&timer_qq, jiffies + msecs_to_jiffies(TIMER_INTERVAL_MS_qq));
        timer_qq_loaded = TRUE;

    }
#endif
}
void pkt_qq_delete(struct pkt_qq *pkt_qq_cur,osl_t *osh){
    //mutex_lock(&pkt_qq_mutex); // 加锁
    //printk("**************debug11*******************");
    if((pkt_qq_last != (struct pkt_qq *)NULL)&&(pkt_qq_cur->FrameID == pkt_qq_last->FrameID)){
        pkt_qq_last = (struct pkt_qq *)NULL;
        index_last = 0;
    }
    if((pkt_qq_cur->FrameID == pkt_qq_chain_head->FrameID)&&(pkt_qq_cur->prev==(struct pkt_qq *)NULL)){
        //printk("**************debug12******************");
        if(pkt_qq_chain_head->next == (struct pkt_qq *)NULL){//防止删除节点时出错
            //printk("**************debug13*******************");
            MFREE(osh, pkt_qq_cur, sizeof(pkt_qq_t));
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

            MFREE(osh, pkt_qq_cur, sizeof(pkt_qq_t));
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
        
        MFREE(osh, pkt_qq_cur, sizeof(pkt_qq_t));
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
    read_lock(&pkt_qq_mutex_len); // 加锁
#ifdef USE_LAST_PKT/*如果use_last_pkt为false，或者后两个不对劲，就重新设置*/
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
    read_unlock(&pkt_qq_mutex_len); // 解锁
    bool found_pkt_node_qq = FALSE;
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
                found_pkt_node_qq = TRUE;
            if((!was_acked)||((was_acked)&&(pkt_qq_cur_PHYdelay >= 17 || pkt_qq_cur->failed_cnt>=1))){//提前判断一下，降低总体计算量
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
                    
                    pkt_qq_cur->txop_in_fly = (pkt_qq_cur->free_txop - pkt_qq_cur->into_hw_txop)*slottime_qq;
                    scb_pps_info_t *pps_scb_qq = SCB_PPSINFO(wlc->pps_info, scb);            
                    uint32 time_in_pretend_tot_qq = pps_scb_qq->ps_pretend_total_time_in_pps;
                    if (pps_scb_qq == NULL){
                        time_in_pretend_tot_qq += R_REG(wlc->osh, D11_TSFTimerLow(wlc)) - pps_scb_qq->ps_pretend_start;
                    }
                    pkt_qq_cur->time_in_pretend_in_fly = time_in_pretend_tot_qq - pkt_qq_cur->time_in_pretend_tot;
                    pkt_qq_cur->ampdu_seq = cur_mpdu_index;

                    struct phy_info_qq *phy_info_qq_cur = NULL;
                    phy_info_qq_cur = (struct phy_info_qq *) MALLOCZ(osh, sizeof(phy_info_qq_t));
                    phy_info_qq_cur->fix_rate = (ltoh16(txh_info->MacTxControlHigh) & D11AC_TXC_FIX_RATE) ? 1:0;
                    wf_rspec_to_phyinfo_qq(rs_txs, phy_info_qq_cur);
                    phy_info_qq_cur->RSSI = TGTXS_PHYRSSI(TX_STATUS_MACTXS_S8(txs));
                    phy_info_qq_cur->RSSI = ((phy_info_qq_cur->RSSI) & PHYRSSI_SIGN_MASK) ? (phy_info_qq_cur->RSSI - PHYRSSI_2SCOMPLEMENT) : phy_info_qq_cur->RSSI;
                    phy_info_qq_cur->noiselevel = wlc_lq_chanim_phy_noise(wlc);
                    kernel_info_t info_qq[DEBUG_CLASS_MAX_FIELD];
                    memcpy(info_qq, phy_info_qq_cur, sizeof(*phy_info_qq_cur));
                    debugfs_set_info_qq(2, info_qq, 1);
                    MFREE(osh, phy_info_qq_cur, sizeof(phy_info_qq_t));
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
#ifdef PRINTTIMEOUTPKT
                kernel_info_t info_qq[DEBUG_CLASS_MAX_FIELD];
                memcpy(info_qq, pkt_qq_cur, sizeof(*pkt_qq_cur));
                debugfs_set_info_qq(0, info_qq, 1);
#endif
                pkt_qq_delete(pkt_qq_cur,osh);
                pkt_qq_chain_len_timeout ++;
            
                //pkt_qq_cur = pkt_qq_cur_next;
                //index++;
                
                //continue;
            }
        }
        
        pkt_qq_cur = pkt_qq_cur_next;     
        //printk("**************debug4*******************");

        if(cur_pkt_qq_chain_len<=index+2){//代表很有可能是末尾的节点，此时需要加上尾端锁
            mutex_unlock(&pkt_qq_mutex_tail); // 解锁
            //printk("**************debug6*******************");
        }
        //printk("**************debug3*******************");


        
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
    pkt_qq_del_timeout_ergodic(osh);
    //mutex_unlock(&pkt_qq_mutex); // 解锁
    //printk("**************debug8*******************");
    
}



