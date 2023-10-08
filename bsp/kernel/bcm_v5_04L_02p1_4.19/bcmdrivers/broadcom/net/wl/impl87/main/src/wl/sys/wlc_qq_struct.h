
/* dump_flag_qqdx */
struct pkt_qq {
    uint32 tcp_seq;/* Starting sequence number */
    uint32 ampdu_seq;/* preassigned seqnum for AMPDU */
    uint32 packetid;/* 未知变量packetid */
    uint16 FrameID;//每个数据帧生命周期不变的
    uint16 pktSEQ;//也许每个数据包生命周期不变的
	uint16 n_pkts;       /**< number of queued packets */
    uint8 tid;//tid
    uint32 pkt_qq_chain_len_add;//记录是第几个送入硬件的包
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
struct phy_info_qq {
    uint8 fix_rate;
    uint32 mcs[RATESEL_MFBR_NUM];
    uint32 nss[RATESEL_MFBR_NUM];
    uint32 rate[RATESEL_MFBR_NUM];
    uint8 BW[RATESEL_MFBR_NUM];
    uint32 ISSGI[RATESEL_MFBR_NUM];
    int8 SNR;
    int8 RSSI;
    int8 noiselevel;
};