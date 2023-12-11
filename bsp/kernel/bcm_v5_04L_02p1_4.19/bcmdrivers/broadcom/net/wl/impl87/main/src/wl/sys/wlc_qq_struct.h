
#define MAX_MCS_QQ 30
struct rates_counts_txs_qq {
	uint32 tx_cnt[MAX_MCS_QQ];
	uint32 txsucc_cnt[MAX_MCS_QQ];
	uint32 txrts_cnt;
	uint32 rxcts_cnt;
	uint32 ncons;
	uint32 nlost;
};
/* dump_flag_qqdx */
struct pkt_qq {
    uint32 tcp_seq;/* Starting sequence number */
    uint32 ampdu_seq;/* preassigned seqnum for AMPDU */
    uint32 packetid;/* 未知变量packetid */
    uint16 FrameID;//每个数据帧生命周期不变的
    uint16 pktSEQ;//也许每个数据包生命周期不变的
	uint16 n_pkts;       /**< number of queued packets */
    uint8 tid;//tid
    uint8 APnum;//AP数量
    uint32 pkt_qq_chain_len_add_start;//记录是第几个送入硬件的包
    uint32 pkt_qq_chain_len_add_end;//记录数据包被释放时有多少包被送入硬件
    uint32 pktnum_to_send_start;//本包送入硬件时硬件待发送队列包量
    uint32 pktnum_to_send_end;//数据包被释放时硬件待发送队列包量
    uint32 pkt_added_in_wlc_tx_start;//本包送入硬件时wlc_tx文件中实际准备发送的数据包量（不仅仅start数据）
    uint32 pkt_added_in_wlc_tx_end;//数据包被释放时wlc_tx文件中实际准备发送的数据包量
    struct rates_counts_txs_qq rates_counts_txs_qq_start;
    struct rates_counts_txs_qq rates_counts_txs_qq_end;
    uint32 into_CFP_time;/*进入CFP的时间*/
    uint8 into_CFP_time_record_loc;/*记录进入CFP的时间的位置*/
    uint32 into_hw_time;/*进入硬件队列的时间*/
    uint32 free_time;/*传输成功被释放的时间*/
    uint32 into_hw_txop;/*进入硬件队列的txop*/
    uint32 free_txop;/*传输成功被释放的txop*/
    uint32 txop_in_fly;/*传输过程中的busy_time*/
    uint32 busy_time;/*传输过程中的txop*/
    uint32 drop_time;/*传输失败被丢弃的时间*/
    uint32 droped_withoutACK_time;/*传输失败被丢弃的时间*/
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
    /*总的进入PPS 时间
    该统计博通并未开启，通过BCMDBG宏来关闭相关统计，需要一个一个开启（将BCMDBG改为BCMDBG_PPS_qq并define），如下是所开启的相关部分：
    1.wlc_pspretend_scb_time_upd相关（wlc_pspretend.h，wlc_pspretend.c,wlc_app.c）
    2.wlc_pspretend.h中增加#ifndef BCMDBG_PPS_qq   #define BCMDBG_PPS_qq   #endif
    3.wlc_pspretend_supr_upd相关（wlc_pspretend.h，wlc_pspretend.c,wlc_app.c,wlc_ampdu.c,wlc_txs.c）
    4.scb_pps_info_t定义处（wlc_pspretend.c）
    5.本文件   PPS时间统计相关   部分
    */
    uint32 qq_pkttag_pointer;
    struct pkt_qq *next;
    struct pkt_qq *prev;
    
};

struct pkt_ergodic {
    uint8 print_loc;
    uint32 pkt_len;
    uint32 real_pkt_num;
    uint16 pkt_FrameID[390];
};

struct wlc_pps_info {
	wlc_info_t *wlc;
	osl_t *osh;
	int scb_handle;
	struct  wl_timer *ps_pretend_probe_timer;
	bool is_ps_pretend_probe_timer_running;
};
struct pkt_count_qq {
    uint32 pkt_qq_chain_len_now;
    uint32 pkt_qq_chain_len_add;
    uint32 pkt_qq_chain_len_soft_retry;
    uint32 pkt_qq_chain_len_acked;
    uint32 pkt_qq_chain_len_unacked;
    uint32 pkt_qq_chain_len_timeout;
    uint32 pkt_qq_chain_len_outofrange;
    uint32 pkt_qq_chain_len_notfound;
    uint32 pkt_qq_chain_len_found;
    uint32 pkt_phydelay_dict[30];    
};

/*rssi的ring buffer*/
#define RSSI_RING_SIZE 40

typedef struct {
    int8 RSSI;
    int8 noiselevel;
    int32 timestamp;
} DataPoint_qq;

#define RATESEL_MFBR_NUM      4
void save_rssi(int8 RSSI,int8 noiselevel);
struct phy_info_qq {
    uint8 fix_rate;
    uint32 mcs[RATESEL_MFBR_NUM];
    uint32 nss[RATESEL_MFBR_NUM];
    uint32 rate[RATESEL_MFBR_NUM];
    uint8 BW[RATESEL_MFBR_NUM];
    uint32 ISSGI[RATESEL_MFBR_NUM];
    int8 SNR;
    int32 RSSI;
    int16 RSSI_loc;//0:wlc_cfp.c;1:wlc_qq.c;2:wlc_rx;3.wlc_lq_rssi_get;4.cfp...!= FC_TYPE_DATA
    int16 RSSI_type;//数据包类型
    int16 RSSI_subtype;//数据包类型
    int8 noiselevel;
    uint8 rssi_ring_buffer_index;
    DataPoint_qq rssi_ring_buffer[RSSI_RING_SIZE];
};



/*定时器初始化相关*/
#ifdef QQ_TIMER_ABLE
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

/*站点信息同步*/
struct start_sta_info{
	int8_t start_is_on;//判断是否游戏正在运行
	struct ether_addr ea;
	int8_t ac_queue_index;
    uint16_t          flowid;     /* flowid */
};
/*定时器初始化相关*/
#define TIMER_INTERVAL_S_qq (1000) // 1s
void timer_callback_start_info_qq(struct timer_list *timer_qq);


void get_and_print_rssi_from_ant(wlc_info_t *wlc);

/* special rssi sample window in monitor mode */
typedef struct {
    /* raw per antenna rssi - valid when # ants > 1 */
    uint16    rssi_chain_window_sz;
    uint8    rssi_chain_index;
    int8    *rssi_chain_window;    /* int8 [WL_RSSI_ANT_MAX][MA_WINDOW_SZ] */
} monitor_rssi_win_t;
/* bss specific data */
typedef struct {
    /* rssi & snr sample windows sampling sizes */
    uint16 rssi_window_sz;        /* rssi window size. apply to all scbs in the bsscfg. */
    uint16 snr_window_sz;        /* SNR window size. apply to all scbs in the bsscfg. */

/* **** the following fields are all for infra STA only. **** */
/* For IBSS these are allocated but not used. The reason is we change the cfg->BSS
 * type in the middle of a connection creation process hence we can't key their alloc
 * off cfg->BSS in the beginning of a bsscfg creation and their free at the end of
 * the bsscfg deletion.
 */
    /* inline rssi average states */
    uint8 *rssi_qdb_window;        /* window for rssi fraction in qdb units */
    int rssi_tot;            /**< rssi samples total, in qdb units */
    uint8 rssi_count;        /* # of valid samples in the window */
    bool last_rssi_is_ucast;    /**< last RSSI sample is from unicast frame */

    /* inline snr average states */
    uint snr_tot;            /* snr samples total */
    uint8 snr_count;        /**< number of valid values in the window */
    bool last_snr_is_ucast;        /**< last SNR sample is from unicast frame */

    /* RSSI event notification */
    wl_rssi_event_t *rssi_event;    /**< RSSI event notification configuration. */
    struct wl_timer *rssi_event_timer;    /**< timer to limit event notifications */
    bool rssi_event_timer_active;    /**< flag to indicate timer active */
    uint8 rssi_level;        /**< current rssi based on notification configuration */

    uint16 rssi_bcn_window_sz;    /* rssi window size. */

    /* RSSI/SNR auto-deduction upon consecutive beacon loss */
    uint8 rssi_delta;    /* Reduce RSSI by this much upon consecutive beacon loss */
    uint8 snr_delta;    /* Reduce SNR by this much upon consecutive beacon loss */
    int32 max_lq_bcn_loss;    /* beacon loss threshold in ms */
    uint32 lq_last_bcn_time; /* local time for last beacon received */
} bss_lq_info_t;
/* scb specific data */
typedef struct {
    /* RSSI moving average */
    int8    *rssi_window;        /* rssi samples buffer - int8 [MA_WINDOW_SZ] */
    /* raw per antenna rssi - valid when # ants > 1 */
    int8    *rssi_chain_window;    /* int8 [WL_RSSI_ANT_MAX][MA_WINDOW_SZ] */

/* **** the following fields are all for infra STA only. **** */
/* For IBSS these are allocated but not used. The reason is we change the cfg->BSS
 * type in the middle of a connection creation process hence we can't key their alloc
 * off cfg->BSS in the beginning of a bsscfg creation and their free at the end of
 * the bsscfg deletion.
 */
    /* SNR moving average */
    uint8    *snr_window;        /**< SNR moving average window - uint8 [MA_WINDOW_SZ] */

    /* raw per antenna bcn-rssi valid when #ants >1 */
    int8    *rssi_chain_bcn_window;

    uint8    rssi_index;
    uint8    rssi_chain_index;
    uint8    snr_index;        /**< SNR moving average window index */
    uint8    rssi_chain_bcn_index;
} scb_lq_info_t;

/* size of channel_qa_sample array */
#define WLC_CHANNEL_QA_NSAMP    2
/* size of noise_lte_values array */
#define WLC_NOISE_LTE_SIZE 9

/* module specific data */
struct wlc_lq_info {
    /* back pointers */
    wlc_info_t *wlc;    /* pointer to main wlc structure */
    osl_t *osh;

    /* cubby handles */
    int cfgh;
    int scbh;

    /* noise levels */
    int8 noise;
    int8 noise_lte;
    int noise_lte_values[WLC_NOISE_LTE_SIZE];
    uint8 noise_lte_val_idx;

    /* rssi & snr sample windows allocation sizes */
    uint8 sta_ma_window_sz;
    uint8 def_ma_window_sz;

    /* misc */
    uint8 ants;        /* # antennas */

    /* channel quality measure */
    bool channel_qa_active;        /**< true if chan qual measurement in progress */
    int channel_quality;        /* quality metric(0-3) of last measured channel, or
                     * -1 if in progress
                     */
    uint8 channel_qa_channel;    /**< channel number of channel being evaluated */
    int8 channel_qa_sample[WLC_CHANNEL_QA_NSAMP];    /**< rssi samples of background
                             * noise
                             */
    uint channel_qa_sample_num;    /**< count of samples in channel_qa_sample array */

    /* phy noise sample requests */
    uint8 noise_req;

    /* bcn rssi sample window */
    uint8 sta_bcn_window_sz;

    /* monitor mode rssi window */
    monitor_rssi_win_t *monitor;
};



#include <wlc_musched.h>
typedef struct musched_ru_stats {
	uint32	tx_cnt[MUSCHED_RU_TYPE_NUM]; /* total tx cnt per ru size */
	uint32	txsucc_cnt[MUSCHED_RU_TYPE_NUM]; /* succ tx cnt per ru size */
	uint8	ru_idx_use_bmap[MUSCHED_RU_BMP_ROW_SZ][MUSCHED_RU_BMP_COL_SZ];
} musched_ru_stats_t;

/* forward declaration */
#define MUSCHED_RUCFG_ROW		16
#define MUSCHED_RUCFG_COL		16
/* for dl ofdma */
#define MUSCHED_DLOFDMA_MINUSER_SZ		2	/* 0: <=80 MHz 1: 160MHz */
#define MUSCHED_DLOFDMA_BW20_MAX_NUM_USERS	8	/* Default DL OFDMA BW20 max clients */
#define MUSCHED_DLOFDMA_MAX_NUM_USERS		16	/* Default DL OFDMA max clients */
/* module info */
struct wlc_muscheduler_info {
	wlc_info_t *wlc;
	uint16	flags;
	int	scbh;
	int16	dl_policy;
	int8	dl_schidx; /* decided by dl_policy; internal used by ucode */
	int8	rualloc; /* rualloc mode */
	int8	ack_policy;
	bool	mix_ackp; /* 1: allow mixed ackp0 and ackp1. 0: disallow */
	uint8	lowat[D11_REV128_BW_SZ];
	uint8	maxn[D11_REV128_BW_SZ];
	uint8	rucfg[D11_REV128_BW_SZ][MUSCHED_RUCFG_ROW][MUSCHED_RUCFG_COL];
	uint8	rucfg_ack[D11_REV128_BW_SZ][MUSCHED_RUCFG_ROW][MUSCHED_RUCFG_COL];
	bool	rucfg_fixed; /* = TRUE: fixed/set by iovar */
	uint8	use_murts;
	bool	dyn_sigbmcs;
	uint16	tmout;
	int16	num_scb_stats;
	musched_ru_stats_t ru_stats;
	int16	num_dlofdma_users;
	uint16	min_dlofdma_users[MUSCHED_DLOFDMA_MINUSER_SZ]; /* min users to enable dlofdma */
	bool	mixbw;		/* TRUE: enabled; FALSE: disabled */
	bool	wfa20in80;	/* fixed RU alloc WAR for WFA test (HE-4.69.1) */
	bool	omutafwin;
	bool	aggx;		/* aggx feature */
	uint16	txdur_thresh_mu;	/* threshold to enforce dl ofdma */
	uint16	txdur_thresh_su;	/* threshold to fall back to SU */
};
/*musched info*/
struct musched_info_qq {
    wlc_muscheduler_info_t musched;
    bool wlc_fifo_isMU;
    bool wlc_fifo_is_ulofdma;
    //mu_type_t mu_type;
    uint16 mch;
    uint16 mcl;
    uint16 mch2;
};





