/* 
     Intel wireless/Pro 2200bg/2945abg device driver for Darwin.
	 
     Copyright(C) 2007  Marco Rodrigues <jalavoui @ gmail. com> 
			    Kazuhisa TAKEI <kazuhisa @ gmail. com>. All right reserved.
									
     This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
   
   this file contain parts from original source at http://prdownloads.sourceforge.net/ipw2200/ipw2200-1.2.1.tgz?download

*/

#include "iwi2200.h"
//ipw-2.3 firmware
#include "firmware/iwi_boot.fw.h"
#include "firmware/iwi_ucode_bss.fw.h"
#include "firmware/iwi_bss.fw.h"
#include "firmware/iwi_ucode_ibss.fw.h"
#include "firmware/iwi_ibss.fw.h"
#include "firmware/iwi_ucode_sniffer.fw.h"
#include "firmware/iwi_sniffer.fw.h"
// Define my superclass
#define super IO80211Controller
// REQUIRED! This macro defines the class's constructors, destructors,
// and several other methods I/O Kit requires. Do NOT use super as the
// second parameter. You must use the literal name of the superclass.
OSDefineMetaClassAndStructors(darwin_iwi2200, IO80211Controller);

u8 P802_1H_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0xf8 };
u8 RFC1042_OUI[P80211_OUI_LEN] = { 0x00, 0x00, 0x00 };

int darwin_iwi2200::ieee80211_copy_snap(u8 * data, u16 h_proto)
{
	struct ieee80211_snap_hdr *snap;
	u8 *oui;

	snap = (struct ieee80211_snap_hdr *)data;
	snap->dsap = 0xaa;
	snap->ssap = 0xaa;
	snap->ctrl = 0x03;

	if (h_proto == 0x8137 || h_proto == 0x80f3)
		oui = P802_1H_OUI;
	else
		oui = RFC1042_OUI;
	snap->oui[0] = oui[0];
	snap->oui[1] = oui[1];
	snap->oui[2] = oui[2];

	*(u16 *) (data + SNAP_SIZE) = htons(h_proto);

	return SNAP_SIZE + sizeof(u16);
}

static const struct ipw_status_code ipw_status_codes[] = {
	{0x00, "Successful"},
	{0x01, "Unspecified failure"},
	{0x0A, "Cannot support all requested capabilities in the "
	 "Capability information field"},
	{0x0B, "Reassociation denied due to inability to confirm that "
	 "association exists"},
	{0x0C, "Association denied due to reason outside the scope of this "
	 "standard"},
	{0x0D,
	 "Responding station does not support the specified authentication "
	 "algorithm"},
	{0x0E,
	 "Received an Authentication frame with authentication sequence "
	 "transaction sequence number out of expected sequence"},
	{0x0F, "Authentication rejected because of challenge failure"},
	{0x10, "Authentication rejected due to timeout waiting for next "
	 "frame in sequence"},
	{0x11, "Association denied because AP is unable to handle additional "
	 "associated stations"},
	{0x12,
	 "Association denied due to requesting station not supporting all "
	 "of the datarates in the BSSBasicServiceSet Parameter"},
	{0x13,
	 "Association denied due to requesting station not supporting "
	 "short preamble operation"},
	{0x14,
	 "Association denied due to requesting station not supporting "
	 "PBCC encoding"},
	{0x15,
	 "Association denied due to requesting station not supporting "
	 "channel agility"},
	{0x19,
	 "Association denied due to requesting station not supporting "
	 "short slot operation"},
	{0x1A,
	 "Association denied due to requesting station not supporting "
	 "DSSS-OFDM operation"},
	{0x28, "Invalid Information Element"},
	{0x29, "Group Cipher is not valid"},
	{0x2A, "Pairwise Cipher is not valid"},
	{0x2B, "AKMP is not valid"},
	{0x2C, "Unsupported RSN IE version"},
	{0x2D, "Invalid RSN IE Capabilities"},
	{0x2E, "Cipher suite is rejected per security policy"},
};

static const struct ieee80211_geo ipw_geos[] = {
	{			/* Restricted */
	 "---",
	 11,
	 NULL,
	{{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}},
	NULL
	 },

	{			/* Custom US/Canada */
	 "ZZF",
	 11,
	 8,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}},
	 {{5180, 36},
	       {5200, 40},
	       {5220, 44},
	       {5240, 48},
	       {5260, 52, IEEE80211_CH_PASSIVE_ONLY},
	       {5280, 56, IEEE80211_CH_PASSIVE_ONLY},
	       {5300, 60, IEEE80211_CH_PASSIVE_ONLY},
	       {5320, 64, IEEE80211_CH_PASSIVE_ONLY}}
	 },

	{			/* Rest of World */
	 "ZZD",
	 13,
	 NULL,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}, {2467, 12},
		{2472, 13}},
	NULL
	 },

	{			/* Custom USA & Europe & High */
	 "ZZA",
	 11,
	 NULL,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}},
	 {{5180, 36},
	       {5200, 40},
	       {5220, 44},
	       {5240, 48},
	       {5260, 52, IEEE80211_CH_PASSIVE_ONLY},
	       {5280, 56, IEEE80211_CH_PASSIVE_ONLY},
	       {5300, 60, IEEE80211_CH_PASSIVE_ONLY},
	       {5320, 64, IEEE80211_CH_PASSIVE_ONLY},
	       {5745, 149},
	       {5765, 153},
	       {5785, 157},
	       {5805, 161},
	       {5825, 165}}
	 },

	{			/* Custom NA & Europe */
	 "ZZB",
	 11,
	 13,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}},
	 {{5180, 36},
	       {5200, 40},
	       {5220, 44},
	       {5240, 48},
	       {5260, 52, IEEE80211_CH_PASSIVE_ONLY},
	       {5280, 56, IEEE80211_CH_PASSIVE_ONLY},
	       {5300, 60, IEEE80211_CH_PASSIVE_ONLY},
	       {5320, 64, IEEE80211_CH_PASSIVE_ONLY},
	       {5745, 149, IEEE80211_CH_PASSIVE_ONLY},
	       {5765, 153, IEEE80211_CH_PASSIVE_ONLY},
	       {5785, 157, IEEE80211_CH_PASSIVE_ONLY},
	       {5805, 161, IEEE80211_CH_PASSIVE_ONLY},
	       {5825, 165, IEEE80211_CH_PASSIVE_ONLY}}
	 },

	{			/* Custom Japan */
	 "ZZC",
	 11,
	 4,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}},
	 {{5170, 34}, {5190, 38},
	       {5210, 42}, {5230, 46}}
	 },

	{			/* Custom */
	 "ZZM",
	 11,
	 NULL,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}},
	NULL
	 },

	{			/* Europe */
	 "ZZE",
	 13,
	 19,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}, {2467, 12},
		{2472, 13}},
	 {{5180, 36},
	       {5200, 40},
	       {5220, 44},
	       {5240, 48},
	       {5260, 52, IEEE80211_CH_PASSIVE_ONLY},
	       {5280, 56, IEEE80211_CH_PASSIVE_ONLY},
	       {5300, 60, IEEE80211_CH_PASSIVE_ONLY},
	       {5320, 64, IEEE80211_CH_PASSIVE_ONLY},
	       {5500, 100, IEEE80211_CH_PASSIVE_ONLY},
	       {5520, 104, IEEE80211_CH_PASSIVE_ONLY},
	       {5540, 108, IEEE80211_CH_PASSIVE_ONLY},
	       {5560, 112, IEEE80211_CH_PASSIVE_ONLY},
	       {5580, 116, IEEE80211_CH_PASSIVE_ONLY},
	       {5600, 120, IEEE80211_CH_PASSIVE_ONLY},
	       {5620, 124, IEEE80211_CH_PASSIVE_ONLY},
	       {5640, 128, IEEE80211_CH_PASSIVE_ONLY},
	       {5660, 132, IEEE80211_CH_PASSIVE_ONLY},
	       {5680, 136, IEEE80211_CH_PASSIVE_ONLY},
	       {5700, 140, IEEE80211_CH_PASSIVE_ONLY}}
	 },

	{			/* Custom Japan */
	 "ZZJ",
	 14,
	 4,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}, {2467, 12},
		{2472, 13}, {2484, 14, IEEE80211_CH_B_ONLY}},
	 {{5170, 34}, {5190, 38},
	       {5210, 42}, {5230, 46}}
	 },

	{			/* Rest of World */
	 "ZZR",
	 14,
	 NULL,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}, {2467, 12},
		{2472, 13}, {2484, 14, IEEE80211_CH_B_ONLY |
			     IEEE80211_CH_PASSIVE_ONLY}},
	NULL
	 },

	{			/* High Band */
	 "ZZH",
	 13,
	 4,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11},
		{2467, 12, IEEE80211_CH_PASSIVE_ONLY},
		{2472, 13, IEEE80211_CH_PASSIVE_ONLY}},
	 {{5745, 149}, {5765, 153},
	       {5785, 157}, {5805, 161}}
	 },

	{			/* Custom Europe */
	 "ZZG",
	 13,
	 4,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11},
		{2467, 12}, {2472, 13}},
	 {{5180, 36}, {5200, 40},
	       {5220, 44}, {5240, 48}}
	 },

	{			/* Europe */
	 "ZZK",
	 13,
	 24,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11},
		{2467, 12, IEEE80211_CH_PASSIVE_ONLY},
		{2472, 13, IEEE80211_CH_PASSIVE_ONLY}},
	 {{5180, 36, IEEE80211_CH_PASSIVE_ONLY},
	       {5200, 40, IEEE80211_CH_PASSIVE_ONLY},
	       {5220, 44, IEEE80211_CH_PASSIVE_ONLY},
	       {5240, 48, IEEE80211_CH_PASSIVE_ONLY},
	       {5260, 52, IEEE80211_CH_PASSIVE_ONLY},
	       {5280, 56, IEEE80211_CH_PASSIVE_ONLY},
	       {5300, 60, IEEE80211_CH_PASSIVE_ONLY},
	       {5320, 64, IEEE80211_CH_PASSIVE_ONLY},
	       {5500, 100, IEEE80211_CH_PASSIVE_ONLY},
	       {5520, 104, IEEE80211_CH_PASSIVE_ONLY},
	       {5540, 108, IEEE80211_CH_PASSIVE_ONLY},
	       {5560, 112, IEEE80211_CH_PASSIVE_ONLY},
	       {5580, 116, IEEE80211_CH_PASSIVE_ONLY},
	       {5600, 120, IEEE80211_CH_PASSIVE_ONLY},
	       {5620, 124, IEEE80211_CH_PASSIVE_ONLY},
	       {5640, 128, IEEE80211_CH_PASSIVE_ONLY},
	       {5660, 132, IEEE80211_CH_PASSIVE_ONLY},
	       {5680, 136, IEEE80211_CH_PASSIVE_ONLY},
	       {5700, 140, IEEE80211_CH_PASSIVE_ONLY},
	       {5745, 149, IEEE80211_CH_PASSIVE_ONLY},
	       {5765, 153, IEEE80211_CH_PASSIVE_ONLY},
	       {5785, 157, IEEE80211_CH_PASSIVE_ONLY},
	       {5805, 161, IEEE80211_CH_PASSIVE_ONLY},
	       {5825, 165, IEEE80211_CH_PASSIVE_ONLY}}
	 },

	{			/* Europe */
	 "ZZL",
	 11,
	 13,
	 {{2412, 1}, {2417, 2}, {2422, 3},
		{2427, 4}, {2432, 5}, {2437, 6},
		{2442, 7}, {2447, 8}, {2452, 9},
		{2457, 10}, {2462, 11}},
	 {{5180, 36, IEEE80211_CH_PASSIVE_ONLY},
	       {5200, 40, IEEE80211_CH_PASSIVE_ONLY},
	       {5220, 44, IEEE80211_CH_PASSIVE_ONLY},
	       {5240, 48, IEEE80211_CH_PASSIVE_ONLY},
	       {5260, 52, IEEE80211_CH_PASSIVE_ONLY},
	       {5280, 56, IEEE80211_CH_PASSIVE_ONLY},
	       {5300, 60, IEEE80211_CH_PASSIVE_ONLY},
	       {5320, 64, IEEE80211_CH_PASSIVE_ONLY},
	       {5745, 149, IEEE80211_CH_PASSIVE_ONLY},
	       {5765, 153, IEEE80211_CH_PASSIVE_ONLY},
	       {5785, 157, IEEE80211_CH_PASSIVE_ONLY},
	       {5805, 161, IEEE80211_CH_PASSIVE_ONLY},
	       {5825, 165, IEEE80211_CH_PASSIVE_ONLY}}
	 }
};

	 
bool darwin_iwi2200::init(OSDictionary *dict)
{

	
	/* Initialize module parameter values here */
#ifdef CONFIG_IPW2200_QOS	
	qos_enable = 1;
	qos_burst_enable = 1;
#else
	qos_enable = 0;
 	qos_burst_enable = 0;
#endif
	qos_no_ack_mask = 0;
	burst_duration_CCK = 0;
	burst_duration_OFDM = 0;
	config = 0;
	cmdlog = 0;
	debug = 0;
	channel = 0;
	mode = 0;
	disable2=1;
	early_up=0;
	associate = 1;
	auto_create = 1;
	led = 1;
	bt_coexist = 1;
	hwcrypto = 0;
	roaming = 0; // roaming is dont work correctly.
	antenna = CFG_SYS_ANTENNA_BOTH;
	test_lock = 0;
	
	disable2=OSDynamicCast(OSNumber,dict->getObject("p_disable"))->unsigned32BitValue();
	led=OSDynamicCast(OSNumber,dict->getObject("p_led"))->unsigned32BitValue();
	mode=OSDynamicCast(OSNumber,dict->getObject("p_mode"))->unsigned32BitValue();
	//early_up=OSDynamicCast(OSNumber,dict->getObject("use_10_4_8"))->unsigned32BitValue();
	
	IWI_LOG("disable %d led %d mode %d\n",disable2, led, mode);
	return super::init(dict);
}

void darwin_iwi2200::ipw_qos_init(struct ipw_priv *priv, int enable,
			 int burst_enable, u32 burst_duration_CCK,
			 u32 burst_duration_OFDM)
{
	priv->qos_data.qos_enable = enable;

	if (priv->qos_data.qos_enable) {
		priv->qos_data.def_qos_parm_CCK = &def_qos_parameters_CCK;
		priv->qos_data.def_qos_parm_OFDM = &def_qos_parameters_OFDM;
		IWI_DEBUG("QoS is enabled\n");
	} else {
		priv->qos_data.def_qos_parm_CCK = &def_parameters_CCK;
		priv->qos_data.def_qos_parm_OFDM = &def_parameters_OFDM;
		IWI_DEBUG("QoS is not enabled\n");
	}

	priv->qos_data.burst_enable = burst_enable;

	if (burst_enable) {
		priv->qos_data.burst_duration_CCK = burst_duration_CCK;
		priv->qos_data.burst_duration_OFDM = burst_duration_OFDM;
	} else {
		priv->qos_data.burst_duration_CCK = 0;
		priv->qos_data.burst_duration_OFDM = 0;
	}
}

int darwin_iwi2200::ipw_sw_reset(int option)
{


	int err = 0;
	struct net_device *net_dev;
	void __iomem *base;
	u32 length, val;
	int i;
	struct ieee80211_device *ieee;
	
	
	//net_dev=(struct net_device*)fifnet;
	net_dev=&net_dev2;
	//memset(&net_dev,0,sizeof(struct ieee80211_device) + sizeof(struct ipw_priv));
	if (!net_dev) {
		IWI_ERR("Unable to network device.\n");
		return -1;
	}
	
	//ieee = (struct ieee80211_device*)netdev_priv(net_dev);
	ieee=&ieee2;
	ieee->dev = net_dev;

	(void*)ieee->networks = kmalloc(MAX_NETWORK_COUNT * sizeof(struct ieee80211_network), NULL);
	memset(ieee->networks, 0, MAX_NETWORK_COUNT * sizeof(struct ieee80211_network));
	INIT_LIST_HEAD(&ieee->network_free_list);
	INIT_LIST_HEAD(&ieee->network_list);
	for (i = 0; i < MAX_NETWORK_COUNT; i++)
		list_add_tail(&ieee->networks[i].list,
			      &ieee->network_free_list);
	/* Default fragmentation threshold is maximum payload size */
	ieee->fts = DEFAULT_FTS;
	ieee->rts = DEFAULT_FTS;
	ieee->scan_age = DEFAULT_MAX_SCAN_AGE;
	ieee->open_wep = 1;

	/* Default to enabling full open WEP with host based encrypt/decrypt */
	ieee->host_encrypt = 1;
	ieee->host_decrypt = 1;
	ieee->host_mc_decrypt = 1;

	/* Host fragementation in Open mode. Default is enabled.
	 * Note: host fragmentation is always enabled if host encryption
	 * is enabled. For cards can do hardware encryption, they must do
	 * hardware fragmentation as well. So we don't need a variable
	 * like host_enc_frag. */
	ieee->host_open_frag = 1;
	ieee->ieee802_1x = 1;	/* Default to supporting 802.1x */

	INIT_LIST_HEAD(&ieee->crypt_deinit_list);
	//init_timer(&ieee->crypt_deinit_timer);
	ieee->crypt_deinit_timer.data = (unsigned long)ieee;
	//ieee->crypt_deinit_timer.function = ieee80211_crypt_deinit_handler;
	ieee->crypt_quiesced = 0;

	ieee->wpa_enabled = 0;
	ieee->drop_unencrypted = 0;
	ieee->privacy_invoked = 0;

	priv = &priv2;
	
	//priv=(struct ipw_priv*)ieee80211_priv(net_dev);
	priv->ieee = ieee;

	priv->net_dev = net_dev;
	
	// if getHarwareAddress is called, put macaddress priv->mac_addr
	for (i=0;i<6;i++){
		if(fEnetAddr.bytes[i] == 0	) continue;
		memcpy(priv->mac_addr, &fEnetAddr.bytes, ETH_ALEN);
		memcpy(priv->net_dev->dev_addr, &fEnetAddr.bytes, ETH_ALEN);
		memcpy(priv->ieee->dev->dev_addr, &fEnetAddr.bytes, ETH_ALEN);
		break;
	}
	
	
	//priv->pci_dev = pdev;
	for (i = 0; i < IPW_IBSS_MAC_HASH_SIZE; i++)
		INIT_LIST_HEAD(&priv->ibss_mac_hash[i]);

	priv->ieee->perfect_rssi = -20;
	priv->ieee->worst_rssi = -85;

	int band, modulation;
	int old_mode = priv->ieee->iw_mode;
	IWI_DEBUG("old_mode %d\n",old_mode);
	/* Initialize module parameter values here */
	priv->config = 0;

	/* We default to disabling the LED code as right now it causes
	 * too many systems to lock up... */
	if (!led)
		priv->config |= CFG_NO_LED;

	if (associate)
		priv->config |= CFG_ASSOCIATE;
	else
		IWI_LOG("Auto associate disabled.\n");

	if (auto_create)
		priv->config |= CFG_ADHOC_CREATE;
	else
		IWI_LOG("Auto adhoc creation disabled.\n");

	priv->config &= ~CFG_STATIC_ESSID;
	priv->essid_len = 0;
	memset(priv->essid, 0, IW_ESSID_MAX_SIZE);

	if (disable2 && option) {
		priv->status |= STATUS_RF_KILL_SW;
		IWI_LOG("Radio disabled.\n");
	}

	if (channel != 0) {
		priv->config |= CFG_STATIC_CHANNEL;
		priv->channel = channel;
		IWI_LOG("Bind to static channel %d\n", channel);
		/* TODO: Validate that provided channel is in range */
	}

#ifdef CONFIG_IPW2200_QOS
	if (qos_enable) 
	ipw_qos_init(priv, qos_enable, qos_burst_enable, burst_duration_CCK, burst_duration_OFDM);
#endif				/* CONFIG_IPW2200_QOS */

	switch (mode) {
	case 1:
		priv->ieee->iw_mode = IW_MODE_ADHOC;
		priv->net_dev->type = ARPHRD_ETHER;

		break;
	case 2:
		priv->ieee->iw_mode = IW_MODE_MONITOR;
		priv->net_dev->type = ARPHRD_IEEE80211_RADIOTAP;
		break;
	default:
	case 0:
		priv->net_dev->type = ARPHRD_ETHER;
		priv->ieee->iw_mode = IW_MODE_INFRA;
		break;
	}

	if (hwcrypto) {
		priv->ieee->host_encrypt = 0;
		priv->ieee->host_encrypt_msdu = 0;
		priv->ieee->host_decrypt = 0;
		priv->ieee->host_mc_decrypt = 0;
	}
	IWI_DEBUG("Hardware crypto [%s]\n", hwcrypto ? "on" : "off");

	/* IPW2200/2915 is abled to do hardware fragmentation. */
	priv->ieee->host_open_frag = 0;
				   
	if ((fPCIDevice->configRead16(kIOPCIConfigDeviceID) == 0x4223) ||
	    (fPCIDevice->configRead16(kIOPCIConfigDeviceID) == 0x4224)) {
		if (option == 1)
			IWI_LOG("Detected Intel PRO/Wireless 2915ABG Network "
			       "Connection\n");
		priv->ieee->abg_true = 1;
		band = IEEE80211_52GHZ_BAND | IEEE80211_24GHZ_BAND;
		modulation = IEEE80211_OFDM_MODULATION |
		    IEEE80211_CCK_MODULATION;
		priv->adapter = IPW_2915ABG;
		priv->ieee->mode = IEEE_A | IEEE_G | IEEE_B;
	} else {
		if (option == 1)
			IWI_LOG(
			       ": Detected Intel PRO/Wireless 2200BG Network "
			       "Connection\n");

		priv->ieee->abg_true = 0;
		band = IEEE80211_24GHZ_BAND;
		modulation = IEEE80211_OFDM_MODULATION |
		    IEEE80211_CCK_MODULATION;
		priv->adapter = IPW_2200BG;
		priv->ieee->mode = IEEE_G | IEEE_B;
	}

	priv->ieee->freq_band = band;
	priv->ieee->modulation = modulation;

	priv->rates_mask = IEEE80211_DEFAULT_RATES_MASK;

	priv->disassociate_threshold = IPW_MB_DISASSOCIATE_THRESHOLD_DEFAULT;
	priv->roaming_threshold = IPW_MB_ROAMING_THRESHOLD_DEFAULT;

	priv->rts_threshold = DEFAULT_RTS_THRESHOLD;
	priv->short_retry_limit = DEFAULT_SHORT_RETRY_LIMIT;
	priv->long_retry_limit = DEFAULT_LONG_RETRY_LIMIT;

	/* If power management is turned on, default to AC mode */
	priv->power_mode = IPW_POWER_AC;
	priv->tx_power = IPW_TX_POWER_DEFAULT;

	return old_mode == priv->ieee->iw_mode;
	
}

IOOptionBits darwin_iwi2200::getState( void ) const
{
	IOOptionBits r=super::getState();
	IWI_DEBUG_FN("getState = %x\n",r);
	return r;
}

bool darwin_iwi2200::start(IOService *provider)
{
	UInt16	reg;

	do {
				
		if ( super::start(provider) == 0) {
			IWI_ERR("%s super::start failed\n", getName());
			break;
		}
			
		if ( (fPCIDevice = OSDynamicCast(IOPCIDevice, provider)) == 0) {
			IWI_ERR("%s  fPCIDevice == 0 :(\n", getName());
			break;
		}

		fPCIDevice->retain();
		
		if (fPCIDevice->open(this) == 0) {
			IWI_ERR("%s fPCIDevice->open(this) failed\n", getName());
			break;
		}
		
		// Request domain power.
        	// Without this, the PCIDevice may be in state 0, and the
        	// PCI config space may be invalid if the machine has been
       		// sleeping.
		if (fPCIDevice->requestPowerDomainState(kIOPMPowerOn, 
			(IOPowerConnection *) getParentEntry(gIOPowerPlane),
			IOPMLowestState ) != IOPMNoErr) {
				IWI_ERR("%s Power thingi failed\n", getName());
				break;
       		}

		fPCIDevice->setBusMasterEnable(true);
		fPCIDevice->setMemoryEnable(true);
		//fPCIDevice->setIOEnable(true);
		
		irqNumber = fPCIDevice->configRead8(kIOPCIConfigInterruptLine);
		vendorID = fPCIDevice->configRead16(kIOPCIConfigVendorID);
		deviceID = fPCIDevice->configRead16(kIOPCIConfigDeviceID);		
		pciReg = fPCIDevice->configRead16(kIOPCIConfigRevisionID);

  		map = fPCIDevice->mapDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0, kIOMapInhibitCache);
  		if (map == 0) {
			IWI_ERR("%s map is zero\n", getName());
			break;
		}
		ioBase = map->getPhysicalAddress();
		memBase = (UInt16 *)map->getVirtualAddress();
		memDes = map->getMemoryDescriptor();
		mem = fPCIDevice->getDeviceMemoryWithRegister(kIOPCIConfigBaseAddress0);
		
		memDes->initWithPhysicalAddress(ioBase, map->getLength(), kIODirectionOutIn);
		
		/* We disable the RETRY_TIMEOUT register (0x41) to keep
		 * PCI Tx retries from interfering with C3 CPU state */
		reg = fPCIDevice->configRead16(0x40);
		if((reg & 0x0000ff00) != 0)
			fPCIDevice->configWrite16(0x40, reg & 0xffff00ff);
			

		IWI_DEBUG("%s iomemory length: 0x%x @ 0x%x\n", getName(), map->getLength(), ioBase);
		IWI_DEBUG("%s virt: 0x%x physical: 0x%x\n", getName(), memBase, ioBase);
		IWI_DEBUG("%s IRQ: %d, Vendor ID: %04x, Product ID: %04x\n", getName(), irqNumber, vendorID, deviceID);
		
		fWorkLoop = (IOWorkLoop *) getWorkLoop();
		if (!fWorkLoop) {
			IWI_ERR("%s ERR: start - getWorkLoop failed\n", getName());
			break;
		}
	
		fInterruptSrc = IOInterruptEventSource::interruptEventSource(
			this, (IOInterruptEventAction) &darwin_iwi2200::interruptOccurred,
			provider);
			
		if(!fInterruptSrc || (fWorkLoop->addEventSource(fInterruptSrc) != kIOReturnSuccess)) {
			IWI_ERR("%s fInterruptSrc error\n", getName());
			break;;
		}

		// This is important. If the interrupt line is shared with other devices,
		// then the interrupt vector will be enabled only if all corresponding
		// interrupt event sources are enabled. To avoid masking interrupts for
		// other devices that are sharing the interrupt line, the event source
		// is enabled immediately.
		fInterruptSrc->enable();

		/*if(!initCmdQueue())
		{
			IWI_DEBUG("CmdQueue alloc error\n");
			break;
		}
		if(!initRxQueue())
		{
			IWI_DEBUG("RxQueue alloc error\n");
			break;
		}
		if(!initTxQueue())
		{
			IWI_DEBUG("TxQueue alloc error\n");
			break;
		}*/
		
		fTransmitQueue = createOutputQueue();
		if (fTransmitQueue == NULL)
		{
			IWI_ERR("ERR: getOutputQueue()\n");
			break;
		}
		fTransmitQueue->setCapacity(kTransmitQueueCapacity);
		
		resetDevice((UInt16 *)memBase);
		
		if (attachInterface((IONetworkInterface **) &fNetif, false) == false) {
			IWI_ERR("%s attach failed\n", getName());
			break;
		}
		fNetif->registerService();

		
		mediumDict = OSDictionary::withCapacity(MEDIUM_TYPE_INVALID + 1);
		addMediumType(kIOMediumIEEE80211None,  0,  MEDIUM_TYPE_NONE);
		addMediumType(kIOMediumIEEE80211Auto,  0,  MEDIUM_TYPE_AUTO);
		addMediumType(kIOMediumIEEE80211DS1,   1000000, MEDIUM_TYPE_1MBIT);
		addMediumType(kIOMediumIEEE80211DS2,   2000000, MEDIUM_TYPE_2MBIT);
		addMediumType(kIOMediumIEEE80211DS5,   5500000, MEDIUM_TYPE_5MBIT);
		addMediumType(kIOMediumIEEE80211DS11, 11000000, MEDIUM_TYPE_11MBIT);
		addMediumType(kIOMediumIEEE80211,     54000000, MEDIUM_TYPE_54MBIT, "OFDM54");
		addMediumType(kIOMediumIEEE80211OptionAdhoc, 0, MEDIUM_TYPE_ADHOC,"ADHOC");

		publishMediumDictionary(mediumDict);
		setCurrentMedium(mediumTable[MEDIUM_TYPE_AUTO]);
		setSelectedMedium(mediumTable[MEDIUM_TYPE_AUTO]);
		setLinkStatus(kIONetworkLinkValid, mediumTable[MEDIUM_TYPE_AUTO]);
		
		registerService();
		
		mutex=IOLockAlloc();
	
		/*lck_grp_attr_t	*ga=lck_grp_attr_alloc_init();
		lck_grp_t		*gr=lck_grp_alloc_init("mut",ga);
		lck_attr_t		*lca=lck_attr_alloc_init();
		mutex=lck_mtx_alloc_init(gr,lca);
	
		/*lck_grp_attr_t	*ga2=lck_grp_attr_alloc_init();
		lck_grp_t		*gr2=lck_grp_alloc_init("spn",ga2);
		lck_attr_t		*lca2=lck_attr_alloc_init();
		//spin= lck_spin_alloc_init(gr2,lca2);
		spin=IOSimpleLockAlloc();
		
		if (!spin) return false;
		if (!mutex) return false;*/
		setInterferenceRobustness(true);
		spin=IOSimpleLockAlloc();
		//IW_SCAN_TYPE_ACTIVE
		queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),NULL,NULL,false);
		queue_te(1,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adapter_restart),NULL,NULL,false);
		queue_te(2,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_link_on),NULL,NULL,false);
		queue_te(3,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rf_kill),NULL,NULL,false);
		queue_te(4,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan_check),NULL,NULL,false);
		queue_te(5,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_associate),NULL,NULL,false);
		queue_te(6,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_gather_stats),NULL,NULL,false);
		queue_te(7,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rx_queue_replenish),NULL,NULL,false);
		queue_te(8,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adhoc_check),NULL,NULL,false);
		queue_te(9,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_bg_qos_activate),NULL,NULL,false);
		queue_te(10,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_activity_off),NULL,NULL,false);
		
		countnonet=0;
		countb=0;
		for (int i=0;i<20;i++) memset(&nonets[i],0,sizeof(struct ieee80211_network));
		pl=1;
		return true;			// end start successfully
	} while (false);
		
	//stop(provider);
	free();
	return false;			// end start insuccessfully
}

IOReturn darwin_iwi2200::selectMedium(const IONetworkMedium * medium)
{
	bool  r;

	if ( OSDynamicCast(IONetworkMedium, medium) == 0 )
    {
        // Defaults to Auto.
		medium = mediumTable[MEDIUM_TYPE_AUTO];
        if ( medium == 0 ) {
		IWI_DEBUG("selectMedium failed\n");
		return kIOReturnError;
	}
    }

	// Program PHY to select the desired medium.
	//r = _phySetMedium( (mediumType_t) medium->getIndex() );

	if ( r && !setCurrentMedium(medium) )
		IWI_DEBUG("%s: setCurrentMedium error\n", getName());

	IWI_DEBUG("Medium is set to: %s\n", medium->getName()->getCStringNoCopy());
	return ( r ? kIOReturnSuccess : kIOReturnIOError );
}

bool darwin_iwi2200::addMediumType(UInt32 type, UInt32 speed, UInt32 code, char* name) {	
    IONetworkMedium	* medium;
    bool              ret = false;
    
    medium = IONetworkMedium::medium(type, speed, 0, code, name);
    if (medium) {
        ret = IONetworkMedium::addMedium(mediumDict, medium);
        if (ret)
            mediumTable[code] = medium;
        medium->release();
    }
    return ret;
}

IOOutputQueue * darwin_iwi2200::createOutputQueue( void )
{
	// An IOGatedOutputQueue will serialize all calls to the driver's
    // outputPacket() function with its work loop. This essentially
    // serializes all access to the driver and the hardware through
    // the driver's work loop, which simplifies the driver but also
    // carries a small performance cost (relatively for 10/100 Mb).

    return IOGatedOutputQueue::withTarget( this, getWorkLoop() );
}

bool darwin_iwi2200::createWorkLoop( void )
{
    fWorkLoop = IOWorkLoop::workLoop();
	
    return ( fWorkLoop != 0 );
}

IOWorkLoop * darwin_iwi2200::getWorkLoop( void ) const
{
    // Override IOService::getWorkLoop() method to return the work loop
    // we allocated in createWorkLoop().

	return fWorkLoop;
}

const OSString * darwin_iwi2200::newVendorString( void ) const
{
    return OSString::withCString("Intel");
}

const OSString * darwin_iwi2200::newModelString( void ) const
{
    const char * model = "2200 BG";
	if ((fPCIDevice->configRead16(kIOPCIConfigDeviceID) == 0x4223) ||
	    (fPCIDevice->configRead16(kIOPCIConfigDeviceID) == 0x4224)) 
	{
		model = "2915 ABG";
	};
    return OSString::withCString(model);
}

int darwin_iwi2200::ipw_stop_nic()
{
	int rc = 0;

	/* stop */
	ipw_write32(IPW_RESET_REG, IPW_RESET_REG_STOP_MASTER);

	rc = ipw_poll_bit(IPW_RESET_REG,
			  IPW_RESET_REG_MASTER_DISABLED, 500);
	if (rc < 0) {
		IWI_DEBUG("wait for reg master disabled failed after 500ms\n");
		return rc;
	}

	ipw_set_bit(IPW_RESET_REG, CBD_RESET_REG_PRINCETON_RESET);

	return rc;
}

int darwin_iwi2200::ipw_init_nic()
{
	int rc;

	/* reset */
	/*prvHwInitNic */
	/* set "initialization complete" bit to move adapter to D0 state */
	ipw_set_bit(IPW_GP_CNTRL_RW, IPW_GP_CNTRL_BIT_INIT_DONE);

	/* low-level PLL activation */
	ipw_write32(IPW_READ_INT_REGISTER,
		    IPW_BIT_INT_HOST_SRAM_READ_INT_REGISTER);

	/* wait for clock stabilization */
	rc = ipw_poll_bit(IPW_GP_CNTRL_RW,
			  IPW_GP_CNTRL_BIT_CLOCK_READY, 250);
	if (rc < 0)
		IWI_DEBUG("FAILED wait for clock stablization\n");

	/* assert SW reset */
	ipw_set_bit(IPW_RESET_REG, IPW_RESET_REG_SW_RESET);

	udelay(10);

	/* set "initialization complete" bit to move adapter to D0 state */
	ipw_set_bit(IPW_GP_CNTRL_RW, IPW_GP_CNTRL_BIT_INIT_DONE);

	return 0;
}

int darwin_iwi2200::ipw_reset_nic(struct ipw_priv *priv)
{
	int rc = 0;
	unsigned long flags;


	rc = ipw_init_nic();

	/* Clear the 'host command active' bit... */
	priv->status &= ~STATUS_HCMD_ACTIVE;
	//wake_up_interruptible(&priv->wait_command_queue);
	priv->status &= ~(STATUS_SCANNING | STATUS_SCAN_ABORTING);
	//wake_up_interruptible(&priv->wait_state);

	return rc;
}


void darwin_iwi2200::ipw_start_nic()
{

	/* prvHwStartNic  release ARC */
	ipw_clear_bit(IPW_RESET_REG,
		      IPW_RESET_REG_MASTER_DISABLED |
		      IPW_RESET_REG_STOP_MASTER |
		      CBD_RESET_REG_PRINCETON_RESET);

	/* enable power management */
	ipw_set_bit(IPW_GP_CNTRL_RW,
		    IPW_GP_CNTRL_BIT_HOST_ALLOWS_STANDBY);

}

struct ipw_rx_queue *darwin_iwi2200::ipw_rx_queue_alloc(struct ipw_priv *priv)
{
	struct ipw_rx_queue *rxq;
	int i;

	(void*)rxq = kzalloc(sizeof(*rxq), NULL);//GFP_KERNEL);
	if (unlikely(!rxq)) {
		IWI_DEBUG("memory allocation failed\n");
		return NULL;
	}
	//spin_lock_init(&rxq->lock);
	INIT_LIST_HEAD(&rxq->rx_free);
	INIT_LIST_HEAD(&rxq->rx_used);

	/* Fill the rx_used queue with _all_ of the Rx buffers */
	for (i = 0; i < RX_FREE_BUFFERS + RX_QUEUE_SIZE; i++)
		list_add_tail(&rxq->pool[i].list, &rxq->rx_used);

	/* Set us so that we have processed and used all buffers, but have
	 * not restocked the Rx queue with fresh buffers */
	rxq->read = rxq->write = 0;
	rxq->processed = RX_QUEUE_SIZE - 1;
	rxq->free_count = 0;

	return rxq;
}

inline void darwin_iwi2200::ipw_enable_interrupts(struct ipw_priv *priv)
{
	if (priv->status & STATUS_INT_ENABLED)
		return;
	priv->status |= STATUS_INT_ENABLED;
	ipw_write32(IPW_INTA_MASK_R, IPW_INTA_MASK_ALL);
}



void darwin_iwi2200::ipw_rx_queue_reset(struct ipw_priv *priv, struct ipw_rx_queue *rxq)
{
	unsigned long flags;
	int i;

	//spin_lock_irqsave(&rxq->lock, flags);

	INIT_LIST_HEAD(&rxq->rx_free);
	INIT_LIST_HEAD(&rxq->rx_used);

	/* Fill the rx_used queue with _all_ of the Rx buffers */
	for (i = 0; i < RX_FREE_BUFFERS + RX_QUEUE_SIZE; i++) {
		/* In the reset function, these buffers may have been allocated
		 * to an SKB, so we need to unmap and free potential storage */
		if (rxq->pool[i].skb != NULL) {
			//pci_unmap_single(priv->pci_dev, rxq->pool[i].dma_addr, IPW_RX_BUF_SIZE, PCI_DMA_FROMDEVICE);
			//dev_kfree_skb(rxq->pool[i].skb);
			//rxq->pool[i].dma_addr=NULL;
			//mbuf_freem_list(rxq->pool[i].skb);
			if (!(rxq->pool[i].skb) == MBUF_TYPE_FREE) freePacket(rxq->pool[i].skb);
			rxq->pool[i].skb = NULL;
			rxq->pool[i].dma_addr=NULL;
		}
		list_add_tail(&rxq->pool[i].list, &rxq->rx_used);
	}

	/* Set us so that we have processed and used all buffers, but have
	 * not restocked the Rx queue with fresh buffers */
	rxq->read = rxq->write = 0;
	rxq->processed = RX_QUEUE_SIZE - 1;
	rxq->free_count = 0;
	//spin_unlock_irqrestore(&rxq->lock, flags);
}

int darwin_iwi2200::ipw_queue_inc_wrap(int index, int n_bd)
{
	return (++index == n_bd) ? 0 : index;
}

void darwin_iwi2200::ipw_queue_tx_free_tfd(struct ipw_priv *priv,
				  struct clx2_tx_queue *txq)
{
	struct tfd_frame *bd = &txq->bd[txq->q.last_used];
	//struct pci_dev *dev = priv->pci_dev;
	int i;

	/* classify bd */
	if (bd->control_flags.message_type == TX_HOST_COMMAND_TYPE)
		/* nothing to cleanup after for host commands */
		return;

	/* sanity check */
	if (le32_to_cpu(bd->u.data.num_chunks) > NUM_TFD_CHUNKS) {
		IWI_DEBUG("Too many chunks: %d\n",
			  le32_to_cpu(bd->u.data.num_chunks));
		/** @todo issue fatal error, it is quite serious situation */
		return;
	}

	/* unmap chunks if any */
	for (i = 0; i < le32_to_cpu(bd->u.data.num_chunks); i++) {
		bd->u.data.chunk_ptr[i]=NULL;
		//pci_unmap_single(dev, le32_to_cpu(bd->u.data.chunk_ptr[i]),
		//		 le16_to_cpu(bd->u.data.chunk_len[i]),
		//		 PCI_DMA_TODEVICE);
		if (txq->txb[txq->q.last_used]) {
			ieee80211_txb_free(txq->txb[txq->q.last_used]);
			//txq->txb[txq->q.last_used] = NULL;
		}
	}
}

void darwin_iwi2200::ieee80211_txb_free(struct ieee80211_txb *txb)
{
	int i;
	if (unlikely(!txb))
		return;
	for (i = 0; i < txb->nr_frags; i++)
		if (txb->fragments[i]) 
		{
			//mbuf_freem_list(txb->fragments[i]);
			if (!(txb->fragments[i]) == MBUF_TYPE_FREE)freePacket(txb->fragments[i]);
			txb->fragments[i]=NULL;
			
		}
	kfree(txb);
	txb=NULL;
}

void darwin_iwi2200::ipw_queue_tx_free(struct ipw_priv *priv, struct clx2_tx_queue *txq)
{
	struct clx2_queue *q = &txq->q;
	//struct pci_dev *dev = priv->pci_dev;

	if (q->n_bd == 0)
		return;

	/* first, empty all BD's */
	for (; q->first_empty != q->last_used;
	     q->last_used = ipw_queue_inc_wrap(q->last_used, q->n_bd)) {
		ipw_queue_tx_free_tfd(priv, txq);
	}

	/* free buffers belonging to queue itself */
	//pci_free_consistent(dev, sizeof(txq->bd[0]) * q->n_bd, txq->bd, q->dma_addr);
	
	q->memD->release();
	q->dma_addr=NULL;
	
	kfree(txq->txb);
	txq->txb=NULL;
	/* 0 fill whole structure */
	memset(txq, 0, sizeof(*txq));
}

void darwin_iwi2200::ipw_tx_queue_free(struct ipw_priv *priv)
{
	/* Tx CMD queue */
	ipw_queue_tx_free(priv, &priv->txq_cmd);

	/* Tx queues */
	ipw_queue_tx_free(priv, &priv->txq[0]);
	ipw_queue_tx_free(priv, &priv->txq[1]);
	ipw_queue_tx_free(priv, &priv->txq[2]);
	ipw_queue_tx_free(priv, &priv->txq[3]);
}

void darwin_iwi2200::ipw_queue_init(struct ipw_priv *priv, struct clx2_queue *q,
			   int count, u32 read, u32 write, u32 base, u32 size)
{
	q->n_bd = count;

	q->low_mark = q->n_bd / 4;
	if (q->low_mark < 4)
		q->low_mark = 4;

	q->high_mark = q->n_bd / 8;
	if (q->high_mark < 2)
		q->high_mark = 2;

	q->first_empty = q->last_used = 0;
	q->reg_r = read;
	q->reg_w = write;

	ipw_write32( base, q->dma_addr);
	ipw_write32( size, count);
	ipw_write32( read, 0);
	ipw_write32( write, 0);

	_ipw_read32(memBase, 0x90);
}

int darwin_iwi2200::ipw_queue_tx_init(struct ipw_priv *priv,
			     struct clx2_tx_queue *q,
			     int count, u32 read, u32 write, u32 base, u32 size)
{
	//struct pci_dev *dev = priv->pci_dev;

	(void*)q->txb = kmalloc(sizeof(q->txb[0]) * count, NULL);
	if (!q->txb) {
		IWI_DEBUG("vmalloc for auxilary BD structures failed\n");
		return -ENOMEM;
	}

	//q->bd = pci_alloc_consistent(dev, sizeof(q->bd[0]) * count, &q->q.dma_addr);
	q->q.memD = MemoryDmaAlloc(sizeof(q->bd[0]) * count, &q->q.dma_addr, &q->bd);
	q->q.memD->prepare();
	
	if (!q->bd) {
		IWI_DEBUG("pci_alloc_consistent(%zd) failed\n",
			  sizeof(q->bd[0]) * count);
		
		q->q.memD->release();
		kfree(q->txb);
		q->txb = NULL;
		return -ENOMEM;
	}

	ipw_queue_init(priv, &q->q, count, read, write, base, size);
	return 0;
}

int darwin_iwi2200::ipw_queue_reset(struct ipw_priv *priv)
{
	int rc = 0;
	/** @todo customize queue sizes */
	int nTx = 64, nTxCmd = 8;
	ipw_tx_queue_free(priv);
	/* Tx CMD queue */
	rc = ipw_queue_tx_init(priv, &priv->txq_cmd, nTxCmd,
			       IPW_TX_CMD_QUEUE_READ_INDEX,
			       IPW_TX_CMD_QUEUE_WRITE_INDEX,
			       IPW_TX_CMD_QUEUE_BD_BASE,
			       IPW_TX_CMD_QUEUE_BD_SIZE);
	if (rc) {
		IWI_DEBUG("Tx Cmd queue init failed\n");
		goto error;
	}
	/* Tx queue(s) */
	rc = ipw_queue_tx_init(priv, &priv->txq[0], nTx,
			       IPW_TX_QUEUE_0_READ_INDEX,
			       IPW_TX_QUEUE_0_WRITE_INDEX,
			       IPW_TX_QUEUE_0_BD_BASE, IPW_TX_QUEUE_0_BD_SIZE);
	if (rc) {
		IWI_DEBUG("Tx 0 queue init failed\n");
		goto error;
	}
	rc = ipw_queue_tx_init(priv, &priv->txq[1], nTx,
			       IPW_TX_QUEUE_1_READ_INDEX,
			       IPW_TX_QUEUE_1_WRITE_INDEX,
			       IPW_TX_QUEUE_1_BD_BASE, IPW_TX_QUEUE_1_BD_SIZE);
	if (rc) {
		IWI_DEBUG("Tx 1 queue init failed\n");
		goto error;
	}
	rc = ipw_queue_tx_init(priv, &priv->txq[2], nTx,
			       IPW_TX_QUEUE_2_READ_INDEX,
			       IPW_TX_QUEUE_2_WRITE_INDEX,
			       IPW_TX_QUEUE_2_BD_BASE, IPW_TX_QUEUE_2_BD_SIZE);
	if (rc) {
		IWI_DEBUG("Tx 2 queue init failed\n");
		goto error;
	}
	rc = ipw_queue_tx_init(priv, &priv->txq[3], nTx,
			       IPW_TX_QUEUE_3_READ_INDEX,
			       IPW_TX_QUEUE_3_WRITE_INDEX,
			       IPW_TX_QUEUE_3_BD_BASE, IPW_TX_QUEUE_3_BD_SIZE);
	if (rc) {
		IWI_DEBUG("Tx 3 queue init failed\n");
		goto error;
	}
	/* statistics */
	priv->rx_bufs_min = 0;
	priv->rx_pend_max = 0;
	return rc;

      error:
	ipw_tx_queue_free(priv);
	return rc;
}

void darwin_iwi2200::ipw_rx_queue_replenish(void *data)
{
	struct ipw_priv *priv = (struct ipw_priv*)data;
	struct ipw_rx_queue *rxq = priv->rxq;
	struct list_head *element;
	struct ipw_rx_mem_buffer *rxb;
	unsigned long flags;
	//spin_lock_irqsave(&rxq->lock, flags);
	while (!list_empty(&rxq->rx_used)) {
		element = rxq->rx_used.next;
		rxb = list_entry(element, struct ipw_rx_mem_buffer, list);
		rxb->skb = alloc_skb(IPW_RX_BUF_SIZE, GFP_ATOMIC);
		if (rxb->skb==0) {
			IWI_DEBUG( "%s: Can not allocate SKB buffers.\n",
			       priv->net_dev->name);
			/* We don't reschedule replenish work here -- we will
			 * call the restock method and if it still needs
			 * more buffers it will schedule replenish */
			break;
		}
		list_del(element);

		//rxb->dma_addr = pci_map_single(priv->pci_dev, rxb->skb->data, IPW_RX_BUF_SIZE, PCI_DMA_FROMDEVICE);
		rxb->dma_addr = mbuf_data_to_physical(mbuf_data(rxb->skb));	
		list_add_tail(&rxb->list, &rxq->rx_free);
		rxq->free_count++;
	}
	//spin_unlock_irqrestore(&rxq->lock, flags);

	ipw_rx_queue_restock(priv);
}

void darwin_iwi2200::ipw_rx_queue_restock(struct ipw_priv *priv)
{
	struct ipw_rx_queue *rxq = priv->rxq;
	struct list_head *element;
	struct ipw_rx_mem_buffer *rxb;
	unsigned long flags;
	int write;

	//spin_lock_irqsave(&rxq->lock, flags);
	write = rxq->write;
	while ((rxq->write != rxq->processed) && (rxq->free_count)) {
		element = rxq->rx_free.next;
		rxb = list_entry(element, struct ipw_rx_mem_buffer, list);
		list_del(element);

		ipw_write32(IPW_RFDS_TABLE_LOWER + rxq->write * RFD_SIZE,
			    rxb->dma_addr);
		rxq->queue[rxq->write] = rxb;
		rxq->write = (rxq->write + 1) % RX_QUEUE_SIZE;
		rxq->free_count--;
	}
	//spin_unlock_irqrestore(&rxq->lock, flags);

	/* If the pre-allocated buffer pool is dropping low, schedule to
	 * refill it */
	//if (rxq->free_count <= RX_LOW_WATERMARK) queue_work(priv->workqueue, &priv->rx_replenish);
	if (rxq->free_count <= RX_LOW_WATERMARK)
		queue_te(7,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rx_queue_replenish),priv,NULL,true);
	/* If we've added more space for the firmware to place data, tell it */
	if (write != rxq->write)
		ipw_write32( IPW_RX_WRITE_INDEX, rxq->write);
}

int darwin_iwi2200::ipw_load(struct ipw_priv *priv)
{
	int rc = 0, retries = 3;
		
	/*struct ipw_fw *fw;
	char *name;
	u8 *boot_img, *ucode_img, *fw_img;
	
	switch (priv->ieee->iw_mode) {
		case IW_MODE_ADHOC:
		name = "ipw2200-ibss.fw";
		break;
	case IW_MODE_MONITOR:
		name = "ipw2200-sniffer.fw";
		break;
	case IW_MODE_INFRA:
		name = "ipw2200-bss.fw";
		break;
	default:
			return -1;
			break;
		}
				
	fw=fw_get(name);
	if (fw==NULL) return -1;
	boot_img = &fw->data[0];
	ucode_img = &fw->data[le32_to_cpu(fw->boot_size)];
	fw_img = &fw->data[le32_to_cpu(fw->boot_size) +
			   le32_to_cpu(fw->ucode_size)];*/
			   
	//resetRxQueue();
	//resetTxQueue();
	//resetCmdQueue();
	if (!priv->rxq)
		priv->rxq = ipw_rx_queue_alloc(priv);
	else
		ipw_rx_queue_reset(priv, priv->rxq);
	if (!priv->rxq) {
		IWI_DEBUG("Unable to initialize Rx queue\n");
		goto error;
	}
	
 retry:
	/* Ensure interrupts are disabled */
	ipw_write32(IPW_INTA_MASK_R, ~IPW_INTA_MASK_ALL);
	priv->status &= ~STATUS_INT_ENABLED;

	/* ack pending interrupts */
	ipw_write32(IPW_INTA_RW, IPW_INTA_MASK_ALL);

	ipw_stop_nic();

	rc = ipw_reset_nic(priv);
	if (rc) {
		IWI_DEBUG("Unable to reset NIC\n");
		goto error;
	}

	ipw_zero_memory( IPW_NIC_SRAM_LOWER_BOUND,
			IPW_NIC_SRAM_UPPER_BOUND - IPW_NIC_SRAM_LOWER_BOUND);

	/* DMA the initial boot firmware into the device */
	rc = uploadFirmware((u8*)iwi_boot, sizeof(iwi_boot));
	//rc=uploadFirmware(boot_img, le32_to_cpu(fw->boot_size));
	if (rc < 0) {
		IWI_ERR("Unable to load boot firmware\n");
		goto error;
	}

	/* kick start the device */
	ipw_start_nic();

	/* wait for the device to finish it's initial startup sequence */
	rc = ipw_poll_bit( IPW_INTA_RW,
			  IPW_INTA_BIT_FW_INITIALIZATION_DONE, 500);
	if (rc < 0) {
		IWI_ERR("device failed to boot initial fw image\n");
		goto error;
	}
	IWI_DEBUG("boot firmware ok\n");
	/* ack fw init done interrupt */
	ipw_write32(IPW_INTA_RW, IPW_INTA_BIT_FW_INITIALIZATION_DONE);

	/* DMA the ucode into the device */
	switch (priv->ieee->iw_mode) {
		case 1://IW_MODE_ADHOC:
			rc=uploadUCode(iwi_ucode_ibss, sizeof(iwi_ucode_ibss));
			break;

		case 2://IW_MODE_MONITOR:
			rc=uploadUCode(iwi_ucode_sniffer, sizeof(iwi_ucode_sniffer));
			break;
			
		case 0://IW_MODE_INFRA:
			rc=uploadUCode(iwi_ucode_bss, sizeof(iwi_ucode_bss));
			break;
			
		default:
			return -1;
			break;
		}
		
	//rc = uploadUCode(ucode_img, le32_to_cpu(fw->ucode_size));
	if(rc < 0) {
		IWI_ERR("%s ucode failed to upload\n", getName());
		return kIOReturnError;
	}

	/* stop nic */
	ipw_stop_nic();

	/* DMA bss firmware into the device */
	switch (priv->ieee->iw_mode) {
		case 1://IW_MODE_ADHOC:
			rc=uploadFirmware((u8*)iwi_ibss, sizeof(iwi_ibss));
			break;

		case 2://IW_MODE_MONITOR:
			rc=uploadFirmware((u8*)iwi_sniffer, sizeof(iwi_sniffer));
			break;
			
		case 0://IW_MODE_INFRA:
			rc=uploadFirmware((u8*)iwi_bss, sizeof(iwi_bss));
			break;
			
		default:
			return -1;
			break;
		}
	//rc = uploadFirmware(fw_img, le32_to_cpu(fw->fw_size));
	if (rc < 0) {
		IWI_ERR("Unable to load firmware\n");
		goto error;
	}


	ipw_write32(IPW_EEPROM_LOAD_DISABLE, 0);

	rc = ipw_queue_reset(priv);
	if (rc < 0) {
		IWI_ERR("Unable to initialize queues\n");
		goto error;
	}
	
/*	resetRxQueue();
	resetTxQueue();
	resetCmdQueue();
	
	CSR_WRITE_4(memBase, IWI_CSR_CMD_BASE, cmdq.physaddr);
	CSR_WRITE_4(memBase, IWI_CSR_CMD_SIZE, cmdq.count);
	CSR_WRITE_4(memBase, IWI_CSR_CMD_WIDX, cmdq.cur);

	CSR_WRITE_4(memBase, IWI_CSR_TX1_BASE, txq.physaddr);
	CSR_WRITE_4(memBase, IWI_CSR_TX1_SIZE, txq.count);
	CSR_WRITE_4(memBase, IWI_CSR_TX1_WIDX, txq.cur);

	CSR_WRITE_4(memBase, IWI_CSR_TX2_BASE, txq.physaddr);
	CSR_WRITE_4(memBase, IWI_CSR_TX2_SIZE, txq.count);
	CSR_WRITE_4(memBase, IWI_CSR_TX2_WIDX, txq.cur);

	CSR_WRITE_4(memBase, IWI_CSR_TX3_BASE, txq.physaddr);
	CSR_WRITE_4(memBase, IWI_CSR_TX3_SIZE, txq.count);
	CSR_WRITE_4(memBase, IWI_CSR_TX3_WIDX, txq.cur);

	CSR_WRITE_4(memBase, IWI_CSR_TX4_BASE, txq.physaddr);
	CSR_WRITE_4(memBase, IWI_CSR_TX4_SIZE, txq.count);
	CSR_WRITE_4(memBase, IWI_CSR_TX4_WIDX, txq.cur);

	struct iwi_rx_data *data2;
	for (int i = 0; i < rxq.count; i++) {
		data2 = &rxq.data[i];
		CSR_WRITE_4(memBase, data2->reg, data2->physaddr);
	}

	CSR_WRITE_4(memBase, IWI_CSR_RX_WIDX, rxq.count - 1);
*/

	/* Ensure interrupts are disabled */
	ipw_write32( IPW_INTA_MASK_R, ~IPW_INTA_MASK_ALL);
	/* ack pending interrupts */
	ipw_write32( IPW_INTA_RW, IPW_INTA_MASK_ALL);


	/* kick start the device */
	ipw_start_nic();

	if (ipw_read32( IPW_INTA_RW) & IPW_INTA_BIT_PARITY_ERROR) {
		if (retries > 0) {
			IWI_DEBUG("Parity error.  Retrying init.\n");
			retries--;
			goto retry;
		}

		IWI_DEBUG("TODO: Handle parity error -- schedule restart?\n");
		rc = -EIO;
		goto error;
	}

	/* wait for the device */
	rc = ipw_poll_bit(IPW_INTA_RW,
			  IPW_INTA_BIT_FW_INITIALIZATION_DONE, 500);
	if (rc < 0) {
		IWI_DEBUG("device failed to start after 500ms\n");
		goto error;
	}
	IWI_DEBUG("bss firmware ok\n");
	/* ack fw init done interrupt */
	ipw_write32(IPW_INTA_RW, IPW_INTA_BIT_FW_INITIALIZATION_DONE);
	priv->eeprom_delay = 1;
	/* read eeprom data and initialize the eeprom region of sram */
	cacheEEPROM(priv);

	/* enable interrupts */
	ipw_enable_interrupts(priv);

	/* Ensure our queue has valid packets */
	ipw_rx_queue_replenish(priv);

	ipw_write32(IPW_RX_READ_INDEX, priv->rxq->read);

	/* ack pending interrupts */
	ipw_write32(IPW_INTA_RW, IPW_INTA_MASK_ALL);

	return 0;

 error:
	
	return rc;

}

int darwin_iwi2200::rf_kill_active(struct ipw_priv *priv)
{
	if (0 == (ipw_read32( 0x30) & 0x10000))
		priv->status |= STATUS_RF_KILL_HW;
	else
		priv->status &= ~STATUS_RF_KILL_HW;
	
	return (priv->status & STATUS_RF_KILL_HW) ? 1 : 0;
}

void darwin_iwi2200::ipw_adapter_restart(ipw_priv *adapter)
{
	struct ipw_priv *priv = adapter;	
	if (priv->status & STATUS_RF_KILL_MASK)
		return;

	IWI_DEBUG("ipw_adapter_restart\n");
	priv->ieee->scans=0;
	
	countb=0;
	if (priv->ieee->iw_mode == IW_MODE_INFRA)
	for (int i=0;i<20;i++) if (nonets[i].bssid) memset(&nonets[i],0,sizeof(struct ieee80211_network));

	//queue_td(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan));
	//queue_td(4,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan_check));
	//priv->status |= STATUS_RF_KILL_HW;
	//queue_td(2,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_link_on));
	//priv->status  &= ~(STATUS_RF_KILL_HW);
	ipw_down(priv);
	
	if (priv->assoc_network &&
	    (priv->assoc_network->capability & WLAN_CAPABILITY_IBSS))
		ipw_remove_current_network(priv);

	
	pl=1;
	if (ipw_up(priv)) {
		IWI_DEBUG("Failed to up device\n");
		return;
	}
}

void darwin_iwi2200::ipw_remove_current_network(struct ipw_priv *priv)
{
	struct list_head *element, *safe;
	struct ieee80211_network *network = NULL;
	unsigned long flags;

	list_for_each_safe(element, safe, &priv->ieee->network_list) {
		network = list_entry(element, struct ieee80211_network, list);
		if (!memcmp(network->bssid, priv->bssid, ETH_ALEN)) {
			list_del(element);
			list_add_tail(&network->list,
				      &priv->ieee->network_free_list);
		}
	}
}

void darwin_iwi2200::ipw_rf_kill(ipw_priv *priv)
{
	//struct ipw_priv *priv = adapter;
	unsigned long flags;

	//IOSimpleLockLock(spin);
	//flags=IOSimpleLockLockDisableInterrupt(spin);
	if (rf_kill_active(priv)) {
		//IWI_DEBUG("RF Kill active, rescheduling GPIO check\n");
		//IODelay(5000*1000);
		//ipw_rf_kill();
		//queue_td(2,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_link_on));
		//ipw_led_link_down();
		queue_te(3,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rf_kill),priv,2,true);
		goto exit_unlock;
	}

	/* RF Kill is now disabled, so bring the device back up */

	if (!(priv->status & STATUS_RF_KILL_MASK)) {
		IWI_DEBUG("HW RF Kill no longer active, restarting "
				  "device\n");

		/* we can not do an adapter restart while inside an irq lock */
		queue_te(1,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adapter_restart),priv,NULL,true);
	} else
		IWI_DEBUG("HW RF Kill deactivated.  SW RF Kill still "
				  "enabled\n");

      exit_unlock:
	//IOSimpleLockUnlock(spin);
	//IOSimpleLockUnlockEnableInterrupt(spin,flags);

	return;
}

int darwin_iwi2200::ipw_set_geo(struct ieee80211_device *ieee,
		       const struct ieee80211_geo *geo)
{
	memcpy(ieee->geo.name, geo->name, 3);
	ieee->geo.name[3] = '\0';
	ieee->geo.bg_channels = geo->bg_channels;
	ieee->geo.a_channels = geo->a_channels;
	memcpy(ieee->geo.bg, geo->bg, geo->bg_channels *
	       sizeof(struct ieee80211_channel));
	memcpy(ieee->geo.a, geo->a, ieee->geo.a_channels *
	       sizeof(struct ieee80211_channel));
	return 0;
}

IOReturn darwin_iwi2200::setPowerState ( unsigned long powerStateOrdinal, IOService* whatDevice )
{
	IWI_DEBUG("setPowerState to %d\n",powerStateOrdinal);
	power=powerStateOrdinal;
	return super::setPowerState(powerStateOrdinal,whatDevice);
}

void darwin_iwi2200::ipw_init_ordinals(struct ipw_priv *priv)
{
	priv->table0_addr = IPW_ORDINALS_TABLE_LOWER;
	priv->table0_len = ipw_read32(priv->table0_addr);

	IWI_DEBUG("table 0 offset at 0x%08x, len = %d\n",
		      priv->table0_addr, priv->table0_len);

	priv->table1_addr = ipw_read32( IPW_ORDINALS_TABLE_1);
	priv->table1_len = ipw_read_reg32( priv->table1_addr);

	IWI_DEBUG("table 1 offset at 0x%08x, len = %d\n",
		      priv->table1_addr, priv->table1_len);

	priv->table2_addr = ipw_read32( IPW_ORDINALS_TABLE_2);
	priv->table2_len = ipw_read_reg32( priv->table2_addr);
	priv->table2_len &= 0x0000ffff;	/* use first two bytes */

	IWI_DEBUG("table 2 offset at 0x%08x, len = %d\n",
		      priv->table2_addr, priv->table2_len);

}

#define MAX_HW_RESTARTS 5

int darwin_iwi2200::ipw_up(struct ipw_priv *priv)
{
	int rc, i, j=0;
	
	if (priv->status & STATUS_EXIT_PENDING)	return -EIO;

	//super::enable(fNetif);
	//fNetif->setPoweredOnByUser(true);
	for (i = 0; i < MAX_HW_RESTARTS; i++) {
#if 0	
	if (pl<MAX_HW_RESTARTS)
	{ 
		pl++;

		/* Load the microcode, firmware, and eeprom.
		 * Also start the clocks. */
		 if (pl>0)//==2)
		 {

#endif		 
			rc = ipw_load(priv);
			if (rc) {
				IWI_DEBUG("Unable to load firmware: %d\n", rc);
				ipw_adapter_restart(priv);
				return rc;
			}
#if 0						
		 }
#endif		 
		ipw_init_ordinals(priv);
		if (!(config & CFG_CUSTOM_MAC))
				memcpy(priv->net_dev->dev_addr, priv->mac_addr, ETH_ALEN);
			
		for (j = 0; j < ARRAY_SIZE(ipw_geos); j++) {
			if (!memcmp(&priv->eeprom[EEPROM_COUNTRY_CODE],
				    ipw_geos[j].name, 3))
				break;
		}
		if (j == ARRAY_SIZE(ipw_geos)) {
			IWI_DEBUG("SKU [%c%c%c] not recognized.\n",
				    priv->eeprom[EEPROM_COUNTRY_CODE + 0],
				    priv->eeprom[EEPROM_COUNTRY_CODE + 1],
				    priv->eeprom[EEPROM_COUNTRY_CODE + 2]);
			j = 0;
		}
		if (ipw_set_geo(priv->ieee, &ipw_geos[j])) {
			IWI_ERR("Could not set geography.");
			return 0;
		}
		IWI_LOG("geography %d = %s\n",j,ipw_geos[j].name);
		
		if (priv->status & STATUS_RF_KILL_SW) {
			IWI_DEBUG("Radio disabled by module parameter.\n");
			return 0;
		} else if (rf_kill_active(priv)) {
			IWI_DEBUG("Radio Frequency Kill Switch is On:\n"
				    "Kill switch must be turned off for "
				    "wireless networking to work.\n");
			queue_te(3,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rf_kill),priv,2,true);
			return 0;
		}

		
		rc = configu(priv);
		//memcpy(priv->net_dev->dev_addr, priv->mac_addr, ETH_ALEN);
		if (!rc) {
			IWI_DEBUG("Configured device on count %d\n", pl);
			/* If configure to try and auto-associate, kick
			 * off a scan. */
			queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,NULL,true);
			fTransmitQueue->setCapacity(kTransmitQueueCapacity);
			fTransmitQueue->start();
			return 0;
		}

		IWI_DEBUG("Device configuration failed: 0x%08X\n", rc);
		IWI_DEBUG("Failed to config device on retry %d of %d\n", i, MAX_HW_RESTARTS);
		/* We had an error bringing up the hardware, so take it
		 * all the way back down so we can try again */
		ipw_down(priv);
	}

	/* tried to restart and config the device for as long as our
	 * patience could withstand */
	IWI_ERR("Unable to initialize device after %d attempts.\n", pl);
	return -1;
}

IOReturn darwin_iwi2200::disable( IONetworkInterface * netif )
{
	//if ((priv->status & STATUS_ASSOCIATED)) return -1;
	
	
	IWI_DEBUG("ifconfig down\n");
	switch ((fNetif->getFlags() & IFF_UP) && (fNetif->getFlags() & IFF_RUNNING))
	{
	case true:
		IWI_DEBUG("ifconfig going down\n");
		//super::disable(fNetif);
		//fNetif->setPoweredOnByUser(false);
		setLinkStatus(kIONetworkLinkValid, mediumTable[MEDIUM_TYPE_AUTO]);
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		
		//fNetif->syncSIOCSIFFLAGS( /*IONetworkController * */this);
		//(if_flags & ~mask) | (new_flags & mask) if mask has IFF_UP if_updown fires up (kpi_interface.c in xnu)
		ifnet_set_flags(fifnet, 0 , IFF_UP | IFF_RUNNING );
				
		if ((priv->status & STATUS_ASSOCIATED)) enable(fNetif);
		
		return kIOReturnSuccess;
		
		break;
	default:
		IWI_DEBUG("ifconfig already down\n");
		return -1;
		break;
	}
}

IOReturn darwin_iwi2200::enable( IONetworkInterface * netif ) 
{
	if ((priv->status & STATUS_RF_KILL_HW)) return -1;
	IWI_DEBUG("ifconfig up\n");
	switch ((ifnet_flags(fifnet) & IFF_UP) && (ifnet_flags(fifnet) & IFF_RUNNING))
	{
	case false:
		IWI_DEBUG("ifconfig going up\n ");
		//super::enable(fNetif);
		//fNetif->setPoweredOnByUser(true);
		setLinkStatus(kIONetworkLinkActive, mediumTable[MEDIUM_TYPE_AUTO]);
		fNetif->setLinkState(kIO80211NetworkLinkUp);
		//(if_flags & ~mask) | (new_flags & mask) if mask has IFF_UP if_updown fires up (kpi_interface.c in xnu)
		ifnet_set_flags(fifnet, IFF_UP|IFF_RUNNING|IFF_BROADCAST|IFF_SIMPLEX|IFF_MULTICAST|IFF_NOTRAILERS 
		, IFF_UP | IFF_RUNNING );
		return kIOReturnSuccess;
		break;
	default:
		IWI_DEBUG("ifconfig already up\n");
		return kIOReturnExclusiveAccess;
		break;
	}
	
}

inline int darwin_iwi2200::ipw_is_init(struct ipw_priv *priv)
{
	return (priv->status & STATUS_INIT) ? 1 : 0;
}

u32 darwin_iwi2200::ipw_register_toggle(u32 reg)
{
	reg &= ~IPW_START_STANDBY;
	if (reg & IPW_GATE_ODMA)
		reg &= ~IPW_GATE_ODMA;
	if (reg & IPW_GATE_IDMA)
		reg &= ~IPW_GATE_IDMA;
	if (reg & IPW_GATE_ADMA)
		reg &= ~IPW_GATE_ADMA;
	return reg;
}

void darwin_iwi2200::ipw_led_activity_off(struct ipw_priv *priv)
{
	unsigned long flags;
	u32 led;

	if (priv->config & CFG_NO_LED)
		return;

	if (priv->status & STATUS_LED_ACT_ON) {
		led = ipw_read_reg32(IPW_EVENT_REG);
		led &= priv->led_activity_off;

		led = ipw_register_toggle(led);

		ipw_write_reg32(IPW_EVENT_REG, led);


		priv->status &= ~STATUS_LED_ACT_ON;
	}

}

void darwin_iwi2200::ipw_led_link_down(struct ipw_priv *priv)
{
	ipw_led_activity_off(priv);
	ipw_led_link_off(priv);

	if (priv->status & STATUS_RF_KILL_MASK)
		ipw_led_radio_off(priv);	
}

void darwin_iwi2200::ipw_led_link_off(struct ipw_priv *priv)
{
	unsigned long flags;
	u32 led;

	/* If configured not to use LEDs, or nic type is 1,
	 * then we don't goggle the LINK led. */
	if (priv->config & CFG_NO_LED || priv->nic_type == EEPROM_NIC_TYPE_1)
		return;


	if (priv->status & STATUS_LED_LINK_ON) {
		led = ipw_read_reg32(IPW_EVENT_REG);
		led &= priv->led_association_off;
		led = ipw_register_toggle(led);

		ipw_write_reg32( IPW_EVENT_REG, led);


		priv->status &= ~STATUS_LED_LINK_ON;

		/* If we aren't associated and the radio is on, schedule
		 * turning the LED on (blink while unassociated) */
		if (!(priv->status & STATUS_RF_KILL_MASK) &&
		    !(priv->status & STATUS_ASSOCIATED))
		{
			IODelay(LD_TIME_LINK_OFF*1000);
			ipw_led_link_on(priv);
		}

	}

}

void darwin_iwi2200::ipw_led_band_off(struct ipw_priv *priv)
{
	unsigned long flags;
	u32 led;

	/* Only nic type 1 supports mode LEDs */
	if (priv->config & CFG_NO_LED || priv->nic_type != EEPROM_NIC_TYPE_1)
		return;


	led = ipw_read_reg32(IPW_EVENT_REG);
	led &= priv->led_ofdm_off;
	led &= priv->led_association_off;

	led = ipw_register_toggle(led);

	ipw_write_reg32( IPW_EVENT_REG, led);

}

void darwin_iwi2200::ipw_led_shutdown(struct ipw_priv *priv)
{
	ipw_led_activity_off(priv);
	ipw_led_link_off(priv);
	ipw_led_band_off(priv);
	queue_td(2,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_link_on));
	//cancel_delayed_work(&priv->led_link_off);
	//cancel_delayed_work(&priv->led_act_off);
}

void darwin_iwi2200::ipw_abort_scan(struct ipw_priv *priv)
{
	int err;

	if (priv->status & STATUS_SCAN_ABORTING) {
		IWI_DEBUG("Ignoring concurrent scan abort request.\n");
		return;
	}
	priv->status |= STATUS_SCAN_ABORTING;	
	queue_td(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan));
	queue_td(4,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan_check));
	//err = sendCommand(IPW_CMD_SCAN_ABORT, NULL,0, 0);
	err= ipw_send_cmd_simple(priv, IPW_CMD_SCAN_ABORT);
	if (err)
		IWI_DEBUG("Request to abort scan failed.\n");
}

void darwin_iwi2200::ipw_send_disassociate(struct ipw_priv *priv, int quiet)
{
	int err;

	if (priv->status & STATUS_ASSOCIATING) {
		IWI_DEBUG("Disassociating while associating.\n");
		//queue_work(priv->workqueue, &priv->disassociate);
		return;
	}

	if (!(priv->status & STATUS_ASSOCIATED)) {
		IWI_DEBUG("Disassociating while not associated.\n");
		return;
	}

	IWI_DEBUG("Disassocation attempt from %02x:%02x:%02x:%02x:%02x:%02x "
			"on channel %d.\n",
			MAC_ARG(priv->assoc_request.bssid),
			priv->assoc_request.channel);

	if (quiet)
		priv->assoc_request.assoc_type = HC_DISASSOC_QUIET;
	else
		priv->assoc_request.assoc_type = HC_DISASSOCIATE;

	err = ipw_send_associate(priv, &priv->assoc_request);
	if (err) {
		IWI_DEBUG("Attempt to send [dis]associate command "
			     "failed.\n");
		return;
	}
	
	priv->status &= ~(STATUS_ASSOCIATING | STATUS_ASSOCIATED);
	priv->status |= STATUS_DISASSOCIATING;
	

}

int darwin_iwi2200::ipw_send_associate(struct ipw_priv *priv,
			      struct ipw_associate *associate)
{
	struct ipw_associate tmp_associate;

	if (!priv || !associate) {
		IWI_DEBUG("Invalid args\n");
		return -1;
	}

	memcpy(&tmp_associate, associate, sizeof(*associate));
	tmp_associate.policy_support =
	    cpu_to_le16(tmp_associate.policy_support);
	tmp_associate.assoc_tsf_msw = cpu_to_le32(tmp_associate.assoc_tsf_msw);
	tmp_associate.assoc_tsf_lsw = cpu_to_le32(tmp_associate.assoc_tsf_lsw);
	tmp_associate.capability = cpu_to_le16(tmp_associate.capability);
	tmp_associate.listen_interval =
	    cpu_to_le16(tmp_associate.listen_interval);
	tmp_associate.beacon_interval =
	    cpu_to_le16(tmp_associate.beacon_interval);
	tmp_associate.atim_window = cpu_to_le16(tmp_associate.atim_window);

	//return sendCommand(IPW_CMD_ASSOCIATE, &tmp_associate,sizeof(tmp_associate), 1);
	return ipw_send_cmd_pdu(priv, IPW_CMD_ASSOCIATE, sizeof(tmp_associate), &tmp_associate);
}

int darwin_iwi2200::ipw_disassociate(struct ipw_priv *data)
{
	struct ipw_priv *priv = data;

	if (!(priv->status & (STATUS_ASSOCIATED | STATUS_ASSOCIATING)))
		return 0;
	ipw_send_disassociate(data, 0);
	return 1;
}

void darwin_iwi2200::ipw_deinit(struct ipw_priv *priv)
{
	int i;

	if (priv->status & STATUS_SCANNING) {
		IWI_DEBUG("Aborting scan during shutdown.\n");
		ipw_abort_scan(priv);
	}

	if (priv->status & STATUS_ASSOCIATED) {
		IWI_DEBUG("Disassociating during shutdown.\n");
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		ipw_disassociate(priv);
	}

	ipw_led_shutdown(priv);

	/* Wait up to 1s for status to change to not scanning and not
	 * associated (disassociation can take a while for a ful 802.11
	 * exchange */
	for (i = 1000; i && (priv->status &
			     (STATUS_DISASSOCIATING |
			      STATUS_ASSOCIATED | STATUS_SCANNING)); i--)
		udelay(10);

	if (priv->status & (STATUS_DISASSOCIATING |
			    STATUS_ASSOCIATED | STATUS_SCANNING))
		IWI_DEBUG("Still associated or scanning...\n");
	else
		IWI_DEBUG("Took %dms to de-init\n", 1000 - i);

	/* Attempt to disable the card */
	u32 phy_off = cpu_to_le32(0);
	//sendCommand(IPW_CMD_CARD_DISABLE, &phy_off,sizeof(phy_off), 1);
	ipw_send_cmd_pdu(priv, IPW_CMD_CARD_DISABLE, sizeof(phy_off),&phy_off);
	priv->status &= ~STATUS_INIT;
}


inline void darwin_iwi2200::ipw_disable_interrupts(struct ipw_priv *priv)
{
	if (!(priv->status & STATUS_INT_ENABLED))
		return;
	priv->status &= ~STATUS_INT_ENABLED;
	ipw_write32( IPW_INTA_MASK_R, ~IPW_INTA_MASK_ALL);
}

void darwin_iwi2200::ipw_down(struct ipw_priv *priv)
{	
	if (priv->status & (STATUS_ASSOCIATED | STATUS_ASSOCIATING))
	{
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		ipw_disassociate(priv);
	}
	int exit_pending = priv->status & STATUS_EXIT_PENDING;
	priv->status |= STATUS_EXIT_PENDING;

	if (ipw_is_init(priv))
		ipw_deinit(priv);

	/* Wipe out the EXIT_PENDING status bit if we are not actually
	 * exiting the module */
	if (!exit_pending)
		priv->status &= ~STATUS_EXIT_PENDING;

	/* tell the device to stop sending interrupts */
	ipw_disable_interrupts(priv);

	/* Clear all bits but the RF Kill */
	priv->status &= STATUS_RF_KILL_MASK | STATUS_EXIT_PENDING;
	fTransmitQueue->stop();
	fTransmitQueue->setCapacity(0);
	fTransmitQueue->flush();
	fNetif->setLinkState(kIO80211NetworkLinkDown);
	//netif_stop_queue(priv->net_dev);

	ipw_stop_nic();

	ipw_led_radio_off(priv);	
}


void darwin_iwi2200::ipw_led_radio_off(struct ipw_priv *priv)
{
	ipw_led_activity_off(priv);
	ipw_led_link_off(priv);
}

void darwin_iwi2200::interruptOccurred(OSObject * owner, 
	//IOInterruptEventSource * src, int /*count*/) 
	void		*src,  IOService *nub, int source)
{
	darwin_iwi2200 *self = OSDynamicCast(darwin_iwi2200, owner); //(darwin_iwi2200 *)owner;
	self->handleInterrupt();
}

int darwin_iwi2200::ipw_queue_tx_reclaim(struct ipw_priv *priv,
				struct clx2_tx_queue *txq, int qindex)
{
	u32 hw_tail;
	int used;
	struct clx2_queue *q = &txq->q;
	hw_tail = ipw_read32(q->reg_r);
	if (hw_tail >= q->n_bd) {
		IWI_DEBUG
		    ("Read index for DMA queue (%d) is out of range [0-%d)\n",
		     hw_tail, q->n_bd);
		goto done;
	}
	for (; q->last_used != hw_tail;
	     q->last_used = ipw_queue_inc_wrap(q->last_used, q->n_bd)) {
		ipw_queue_tx_free_tfd(priv, txq);
		priv->tx_packets++;
	}
      done:
#if TX_QUEUE_CHECK	  
	if ((ipw_queue_space(q) > q->low_mark) &&
	    (qindex >= 0) &&
	    (priv->status & STATUS_ASSOCIATED) && (ifnet_flags(fifnet) & IFF_RUNNING)){ //&& netif_running(priv->net_dev)){
		IWI_DEBUG("queue is available\n");
		fTransmitQueue->start();
	}
#endif		
		//netif_wake_queue(priv->net_dev);
	used = q->first_empty - q->last_used;
	if (used < 0)
		used += q->n_bd;
	return used;
}

UInt32 darwin_iwi2200::handleInterrupt(void)
{
	UInt32 r,inta_mask;
	UInt32 ret=true;
	int flags;

	r = ipw_read32(IPW_INTA_RW);
	inta_mask = ipw_read32(IPW_INTA_MASK_R);
	r &= (IPW_INTA_MASK_ALL & inta_mask);
	
	/* Add any cached INTA values that need to be handled */
	//r |= priv->isr_inta;
	
	if ((r = CSR_READ_4(memBase, IWI_CSR_INTR)) == 0 || r == 0xffffffff) {
		//IWI_UNLOCK(memBase);
		return false;
	}
	//IWI_DEBUG("%s: GotInterrupt: 0x%8x\t (", getName(), r);

	/* disable interrupts */
	CSR_WRITE_4(memBase, IWI_CSR_INTR_MASK, 0);


	if (r & IPW_INTA_BIT_FW_CARD_DISABLE_PHY_OFF_DONE)
	{
		IWI_DEBUG_FULL("PHY_OFF_DONE Restarting\n");
		priv->status |= STATUS_RF_KILL_HW;
		priv->status &= ~(STATUS_ASSOCIATED | STATUS_ASSOCIATING);
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		if ((ifnet_flags(fifnet) & IFF_RUNNING)) ipw_link_down(priv); else ipw_led_link_off(priv);
		queue_te(1,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adapter_restart),priv,NULL,true);
		ret |= IPW_INTA_BIT_FW_CARD_DISABLE_PHY_OFF_DONE;		
	}
	
	if (r & (IPW_INTA_BIT_FATAL_ERROR | IWI_INTR_PARITY_ERROR)) {
		IWI_DEBUG_FULL("Firmware error detected.  Restarting.\n");
		priv->status |= STATUS_RF_KILL_HW;
		priv->status &= ~STATUS_INIT;
		priv->status &= ~STATUS_HCMD_ACTIVE;
		priv->status &= ~(STATUS_ASSOCIATED | STATUS_ASSOCIATING);
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		if ((ifnet_flags(fifnet) & IFF_RUNNING)) ipw_link_down(priv); else ipw_led_link_off(priv);
		queue_te(1,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adapter_restart),priv,NULL,true);
		ret |= IPW_INTA_BIT_FATAL_ERROR;
	}

	if (r & IPW_INTA_BIT_RF_KILL_DONE) {
	
		IWI_LOG("IPW_INTA_BIT_RF_KILL_DONE\nPress wireless button to turn interface on\n");
		priv->status |= STATUS_RF_KILL_HW;
		priv->status &= ~STATUS_RF_KILL_SW;
		priv->status &= ~(STATUS_ASSOCIATED | STATUS_ASSOCIATING);
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		if ((ifnet_flags(fifnet) & IFF_RUNNING)) ipw_link_down(priv); else ipw_led_link_off(priv);
		queue_te(3,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rf_kill),priv,2,true);
		ret |= IPW_INTA_BIT_RF_KILL_DONE;
	}

	if (r & IPW_INTA_BIT_RX_TRANSFER) {
		IWI_DEBUG_FULL("IPW_INTA_BIT_RX_TRANSFER)\n");
		ipw_rx(priv);
		ret = IPW_INTA_BIT_RX_TRANSFER;
	}

	if (r & IPW_INTA_BIT_TX_CMD_QUEUE) {
		IWI_DEBUG_FULL("Command completed.\n");
		ipw_queue_tx_reclaim(priv, &priv->txq_cmd, -1);
		priv->status &= ~STATUS_HCMD_ACTIVE;
		//wake_up_interruptible(&priv->wait_command_queue);
		ret |= IPW_INTA_BIT_TX_CMD_QUEUE;
		//ret = IWI_INTR_CMD_DONE;
	}
	if (r & IPW_INTA_BIT_TX_QUEUE_1) {
		IWI_DEBUG_FULL("TX_QUEUE_1\n");
		ipw_queue_tx_reclaim(priv, &priv->txq[0], 0);
		ret |= IPW_INTA_BIT_TX_QUEUE_1;
	}

	if (r & IPW_INTA_BIT_TX_QUEUE_2) {
		IWI_DEBUG_FULL("TX_QUEUE_2\n");
		ipw_queue_tx_reclaim(priv, &priv->txq[1], 1);
		ret |= IPW_INTA_BIT_TX_QUEUE_2;
	}

	if (r & IPW_INTA_BIT_TX_QUEUE_3) {
		IWI_DEBUG_FULL("TX_QUEUE_3\n");
		ipw_queue_tx_reclaim(priv, &priv->txq[2], 2);
		ret |= IPW_INTA_BIT_TX_QUEUE_3;
	}

	if (r & IPW_INTA_BIT_TX_QUEUE_4) {
		IWI_DEBUG_FULL("TX_QUEUE_4\n");
		ipw_queue_tx_reclaim(priv, &priv->txq[3], 3);
		ret |= IPW_INTA_BIT_TX_QUEUE_4;
	}

	if (r & IPW_INTA_BIT_STATUS_CHANGE) {
		IWI_DEBUG("STATUS_CHANGE\n");
		ret |= IPW_INTA_BIT_STATUS_CHANGE;
	}

	if (r & IPW_INTA_BIT_BEACON_PERIOD_EXPIRED) {
		IWI_DEBUG_FULL("TX_PERIOD_EXPIRED\n");
		ret |= IPW_INTA_BIT_BEACON_PERIOD_EXPIRED;
	}

	if (r & IPW_INTA_BIT_SLAVE_MODE_HOST_CMD_DONE) {
		IWI_DEBUG_FULL("IPW_INTA_BIT_SLAVE_MODE_HOST_CMD_DONE\n");
		ret |= IPW_INTA_BIT_SLAVE_MODE_HOST_CMD_DONE;
	}

	if (r & IPW_INTA_BIT_FW_INITIALIZATION_DONE) {
		IWI_DEBUG_FULL("FW_INITIALIZATION_DONE\n");
		ret |= IPW_INTA_BIT_FW_INITIALIZATION_DONE;
	}
	// 
#ifdef TX_QUEUE_CHECK	
	if( (ret & 
		( IPW_INTA_BIT_TX_QUEUE_1 | IPW_INTA_BIT_TX_QUEUE_2 | IPW_INTA_BIT_TX_QUEUE_3  | IPW_INTA_BIT_TX_QUEUE_4) ) 
	    && (fNetif->getFlags() & IFF_UP) && (fNetif->getFlags() & IFF_RUNNING) )
	{
		 //IWI_DEBUG("fTrasmitQueue->service()\n");
		 fTransmitQueue->service();
		 //ipw_send_cmd_simple(priv, IPW_CMD_TX_FLUSH);
	}
#endif
	//ipw_enable_interrupts(priv);
	
	/* acknowledge interrupts */
	CSR_WRITE_4(memBase, IWI_CSR_INTR, r);

	/* re-enable interrupts */
	CSR_WRITE_4(memBase, IWI_CSR_INTR_MASK, IPW_INTA_MASK_ALL);

	return ret;
}


UInt16 darwin_iwi2200::readPromWord(UInt16 *base, UInt8 addr)
{
	UInt32 tmp;
	UInt16 val;
	int n;
	
	/* clock C once before the first command */
	IWI_EEPROM_CTL(base, 0);
	IWI_EEPROM_CTL(base, IWI_EEPROM_S);
	IWI_EEPROM_CTL(base, IWI_EEPROM_S | IWI_EEPROM_C);
	IWI_EEPROM_CTL(base, IWI_EEPROM_S);

	/* write start bit (1) */
	IWI_EEPROM_CTL(base, IWI_EEPROM_S | IWI_EEPROM_D);
	IWI_EEPROM_CTL(base, IWI_EEPROM_S | IWI_EEPROM_D | IWI_EEPROM_C);

	/* write READ opcode (10) */
	IWI_EEPROM_CTL(base, IWI_EEPROM_S | IWI_EEPROM_D);
	IWI_EEPROM_CTL(base, IWI_EEPROM_S | IWI_EEPROM_D | IWI_EEPROM_C);
	IWI_EEPROM_CTL(base, IWI_EEPROM_S);
	IWI_EEPROM_CTL(base, IWI_EEPROM_S | IWI_EEPROM_C);


	for (n = 7; n >= 0; n--) {
		IWI_EEPROM_CTL(base, IWI_EEPROM_S |
		    (((addr >> n) & 1) << IWI_EEPROM_SHIFT_D));
		IWI_EEPROM_CTL(base, IWI_EEPROM_S |
		    (((addr >> n) & 1) << IWI_EEPROM_SHIFT_D) | IWI_EEPROM_C);
	}

	IWI_EEPROM_CTL(base, IWI_EEPROM_S);
	
	/* read data Q15-Q0 */
	val = 0;
	for (n = 15; n >= 0; n--) {
		IWI_EEPROM_CTL(base, IWI_EEPROM_S | IWI_EEPROM_C);
		IWI_EEPROM_CTL(base, IWI_EEPROM_S);
		tmp = MEM_READ_4(base, IWI_MEM_EEPROM_CTL);
		val |= ((tmp & IWI_EEPROM_Q) >> IWI_EEPROM_SHIFT_Q) << n;
	}

	IWI_EEPROM_CTL(base, 0);

	/* clear Chip Select and clock C */
	IWI_EEPROM_CTL(base, IWI_EEPROM_S);
	IWI_EEPROM_CTL(base, 0);
	IWI_EEPROM_CTL(base, IWI_EEPROM_C);

#if defined(__BIG_ENDIAN__)
	return (val);
#else
	return OSSwapInt16(val);
#endif
}


IOReturn darwin_iwi2200::getHardwareAddress( IOEthernetAddress * addr )
{
	UInt16 val;
	if (fEnetAddr.bytes[0]==0 && fEnetAddr.bytes[1]==0 && fEnetAddr.bytes[2]==0
	&& fEnetAddr.bytes[3]==0 && fEnetAddr.bytes[4]==0 && fEnetAddr.bytes[5]==0)
	{
		IWI_DEBUG_FN(" is called \n");
		val = readPromWord(memBase, IWI_EEPROM_MAC + 0);
		fEnetAddr.bytes[0]=val >> 8;
		fEnetAddr.bytes[1]=val & 0xff;
		val = readPromWord(memBase, IWI_EEPROM_MAC + 1);
		fEnetAddr.bytes[2]=val >> 8;
		fEnetAddr.bytes[3]=val & 0xff;
		val = readPromWord(memBase, IWI_EEPROM_MAC + 2);
		fEnetAddr.bytes[4]=val >> 8;
		fEnetAddr.bytes[5]=val & 0xff;
	}
	memcpy(addr, &fEnetAddr, sizeof(*addr));
	if (priv)
	{
		memcpy(priv->mac_addr, &fEnetAddr.bytes, ETH_ALEN);
		memcpy(priv->net_dev->dev_addr, &fEnetAddr.bytes, ETH_ALEN);
		memcpy(priv->ieee->dev->dev_addr, &fEnetAddr.bytes, ETH_ALEN);
		IWI_DEBUG("getHardwareAddress " MAC_FMT "\n",MAC_ARG(priv->mac_addr));
	}else {
		IWI_DEBUG(" priv->mac_addr is not set \n");
	}

	return kIOReturnSuccess;
}


void darwin_iwi2200::stopMaster(UInt16 *base) {
	UInt32 tmp;
	int ntries;

	CSR_WRITE_4(base, IWI_CSR_INTR_MASK, 0);

	CSR_WRITE_4(base, IWI_CSR_RST, IWI_RST_STOP_MASTER);
	for (ntries = 0; ntries < 5; ntries++) {
		if (CSR_READ_4(base, IWI_CSR_RST) & IWI_RST_MASTER_DISABLED)
			break;
		IODelay(100);
	}
	if(ntries == 5)
		IWI_DEBUG("%s timeout waiting for master\n", getName());

	tmp = CSR_READ_4(base, IWI_CSR_RST);
	CSR_WRITE_4(base, IWI_CSR_RST, tmp | IWI_RST_PRINCETON_RESET);
}

void darwin_iwi2200::stopDevice(UInt16 *base)
{
	stopMaster(base);
	
	CSR_WRITE_4(base, IWI_CSR_RST, IWI_RST_SOFT_RESET);
}

bool darwin_iwi2200::resetDevice(UInt16 *base) 
{
	int i;
	UInt32 tmp;

	stopMaster(base);
	
	tmp = CSR_READ_4(base, IWI_CSR_CTL);

	CSR_WRITE_4(base, IWI_CSR_CTL, tmp | IWI_CTL_INIT);

	CSR_WRITE_4(base, IWI_CSR_READ_INT, IWI_READ_INT_INIT_HOST);
	
	for(i=0; i<100; i++) {
		if (CSR_READ_4(base, IWI_CSR_CTL) & IWI_CTL_CLOCK_READY)
			break;
		IODelay(10);
	}

	if(i==100) {
		IWI_DEBUG("%s timeout waiting for clock stabilization\n", getName());
		return false;
	}


	tmp = CSR_READ_4(base, IWI_CSR_RST);
	CSR_WRITE_4(base, IWI_CSR_RST, tmp | IWI_RST_SOFT_RESET);
	
	IODelay(10);

	tmp = CSR_READ_4(base, IWI_CSR_CTL);
	CSR_WRITE_4(base, IWI_CSR_CTL, tmp | IWI_CTL_INIT);

	for(i = 0; i < 0xc000; i++)
		CSR_WRITE_4(base, IWI_CSR_AUTOINC_DATA, 0);

	return true;
}


void darwin_iwi2200::ipw_write_reg8(UInt32 reg, UInt8 value)
{
	UInt32 aligned_addr = reg & IPW_INDIRECT_ADDR_MASK;	/* dword align */
	UInt32 dif_len = reg - aligned_addr;

	_ipw_write32(memBase, IPW_INDIRECT_ADDR, aligned_addr);
	_ipw_write8(memBase, IPW_INDIRECT_DATA + dif_len, value);
}

UInt8 darwin_iwi2200::ipw_read_reg8(UInt32 reg)
{
	UInt32 word;
	_ipw_write32(memBase, IPW_INDIRECT_ADDR, reg & IPW_INDIRECT_ADDR_MASK);
	word = _ipw_read32(memBase, IPW_INDIRECT_DATA);
	return (word >> ((reg & 0x3) * 8)) & 0xff;
}

void darwin_iwi2200::ipw_write_reg16(UInt32 reg, UInt16 value)
{
	UInt32 aligned_addr = reg & IPW_INDIRECT_ADDR_MASK;	/* dword align */
	UInt32 dif_len = (reg - aligned_addr) & (~0x1ul);

	_ipw_write32(memBase, IPW_INDIRECT_ADDR, aligned_addr);
	_ipw_write16(memBase, IPW_INDIRECT_DATA + dif_len, value);
	
}

int darwin_iwi2200::ipw_stop_master()
{
	int rc;

	/* stop master. typical delay - 0 */
	ipw_set_bit( IPW_RESET_REG, IPW_RESET_REG_STOP_MASTER);

	/* timeout is in msec, polled in 10-msec quanta */
	rc = ipw_poll_bit( IPW_RESET_REG,
			  IPW_RESET_REG_MASTER_DISABLED, 100);
	if (rc < 0) {
		IWI_DEBUG("wait for stop master failed after 100ms\n");
		return -1;
	}

	//IWI_DEBUG("stop master %dms\n", rc);

	return rc;
}

void darwin_iwi2200::ipw_arc_release()
{
	mdelay(5);

	ipw_clear_bit( IPW_RESET_REG, CBD_RESET_REG_PRINCETON_RESET);

	/* no one knows timing, for safety add some delay */
	mdelay(5);
}

bool darwin_iwi2200::uploadUCode(const unsigned char * data, UInt16 len)
{
int rc = 0, i, addr;
	u8 cr = 0;
	u16 *image;

	//data += 8;
	//len -= 8;
	
	//image = (u16 *) data;

	rc = ipw_stop_master();

	if (rc < 0)
		return rc;

	for (addr = IPW_SHARED_LOWER_BOUND;
	     addr < IPW_REGISTER_DOMAIN1_END; addr += 4) {
		ipw_write32(addr, 0);
	}

	/* no ucode (yet) */
	memset(&priv->dino_alive, 0, sizeof(priv->dino_alive));
	/* destroy DMA queues */
	/* reset sequence */

	ipw_write_reg32( IPW_MEM_HALT_AND_RESET, IPW_BIT_HALT_RESET_ON);
	ipw_arc_release();
	ipw_write_reg32( IPW_MEM_HALT_AND_RESET, IPW_BIT_HALT_RESET_OFF);
	mdelay(1);

	/* reset PHY */
	ipw_write_reg32( IPW_INTERNAL_CMD_EVENT, IPW_BASEBAND_POWER_DOWN);
	mdelay(1);

	ipw_write_reg32( IPW_INTERNAL_CMD_EVENT, 0);
	mdelay(1);

	/* enable ucode store */
	ipw_write_reg8( IPW_BASEBAND_CONTROL_STATUS, 0x0);
	ipw_write_reg8( IPW_BASEBAND_CONTROL_STATUS, DINO_ENABLE_CS);
	mdelay(1);

	/* write ucode */
	/**
	 * @bug
	 * Do NOT set indirect address register once and then
	 * store data to indirect data register in the loop.
	 * It seems very reasonable, but in this case DINO do not
	 * accept ucode. It is essential to set address each time.
	 */
	/* load new ipw uCode */
	//for (i = 0; i < len / 2; i++) ipw_write_reg16( IPW_BASEBAND_CONTROL_STORE, cpu_to_le16(image[i]));
	UInt16 *w;
	for(w = (UInt16 *)data; len > 0; len-=2, w++) {
		ipw_write_reg16( IPW_BASEBAND_CONTROL_STORE, *w);
	}
	
	/* enable DINO */
	ipw_write_reg8( IPW_BASEBAND_CONTROL_STATUS, 0);
	ipw_write_reg8( IPW_BASEBAND_CONTROL_STATUS, DINO_ENABLE_SYSTEM);

	/* this is where the igx / win driver deveates from the VAP driver. */

	/* wait for alive response */
	for (i = 0; i < 100; i++) {
		/* poll for incoming data */
		cr = ipw_read_reg8( IPW_BASEBAND_CONTROL_STATUS);

		if (cr & DINO_RXFIFO_DATA)
			break;
		mdelay(1);
	}


	if (cr & DINO_RXFIFO_DATA) {
		/* alive_command_responce size is NOT multiple of 4 */
		u32 response_buffer[(sizeof(priv->dino_alive) + 3) / 4];

		for (i = 0; i < ARRAY_SIZE(response_buffer); i++)
			response_buffer[i] =
			    le32_to_cpu(ipw_read_reg32(IPW_BASEBAND_RX_FIFO_READ));
							   
			   
		memcpy(&priv->dino_alive, response_buffer,sizeof(priv->dino_alive));
		if (priv->dino_alive.alive_command == 1
		    && priv->dino_alive.ucode_valid == 1) {
			rc = 0;
			IWI_DEBUG
			    ("Microcode OK, rev. %d (0x%x) dev. %d (0x%x) "
			     "of %02d/%02d/%02d %02d:%02d\n",
			     priv->dino_alive.software_revision,
			     priv->dino_alive.software_revision,
			     priv->dino_alive.device_identifier,
			     priv->dino_alive.device_identifier,
			     priv->dino_alive.time_stamp[0],
			     priv->dino_alive.time_stamp[1],
			     priv->dino_alive.time_stamp[2],
			     priv->dino_alive.time_stamp[3],
			     priv->dino_alive.time_stamp[4]);
		} else {
			IWI_DEBUG("Microcode is not alive\n");
			rc = -EINVAL;
		}
	} else {
		IWI_DEBUG("No alive response from DINO\n");
		rc = -ETIME;
	}

	/* disable DINO, otherwise for some reason
	   firmware have problem getting alive resp. */
	ipw_write_reg8( IPW_BASEBAND_CONTROL_STATUS, 0);

	return rc;
	
}



void inline darwin_iwi2200::ipw_write32(UInt32 offset, UInt32 data)
{
	//OSWriteLittleInt32((void*)memBase, offset, data);
	_ipw_write32(memBase, offset, data);
}

UInt32 inline darwin_iwi2200::ipw_read32(UInt32 offset)
{
	//return OSReadLittleInt32((void*)memBase, offset);
	return _ipw_read32(memBase,offset);
}

void inline darwin_iwi2200::ipw_clear_bit(UInt32 reg, UInt32 mask)
{
	ipw_write32(reg, ipw_read32(reg) & ~mask);
}

void inline darwin_iwi2200::ipw_set_bit(UInt32 reg, UInt32 mask)
{
	ipw_write32(reg, ipw_read32(reg) | mask);
}

int darwin_iwi2200::ipw_fw_dma_add_command_block(
					UInt32 src_address,
					UInt32 dest_address,
					UInt32 length,
					int interrupt_enabled, int is_last)
{

	UInt32 control = CB_VALID | CB_SRC_LE | CB_DEST_LE | CB_SRC_AUTOINC |
	    CB_SRC_IO_GATED | CB_DEST_AUTOINC | CB_SRC_SIZE_LONG |
	    CB_DEST_SIZE_LONG;
	struct command_block *cb;
	UInt32 last_cb_element = 0;

if (sram_desc.last_cb_index >= CB_NUMBER_OF_ELEMENTS_SMALL)
		return -1;
		
	last_cb_element = sram_desc.last_cb_index;
	cb = &sram_desc.cb_list[last_cb_element];
	sram_desc.last_cb_index++;

	/* Calculate the new CB control word */
	if (interrupt_enabled)
		control |= CB_INT_ENABLED;

	if (is_last)
		control |= CB_LAST_VALID;

	control |= length;

	/* Calculate the CB Element's checksum value */
	cb->status = control ^ src_address ^ dest_address;

	/* Copy the Source and Destination addresses */
	cb->dest_addr = dest_address;
	cb->source_addr = src_address;

	/* Copy the Control Word last */
	cb->control = control;

	return 0;
}

void darwin_iwi2200::ipw_zero_memory(UInt32 start, UInt32 count)
{
	count >>= 2;
	if (!count)
		return;
	_ipw_write32(memBase,IPW_AUTOINC_ADDR, start);
	while (count--)
		_ipw_write32(memBase,IPW_AUTOINC_DATA, 0);
}

void darwin_iwi2200::ipw_fw_dma_reset_command_blocks()
{
	ipw_zero_memory(IPW_SHARED_SRAM_DMA_CONTROL,
			CB_NUMBER_OF_ELEMENTS_SMALL *
			sizeof(struct command_block));
}

void darwin_iwi2200::ipw_write_reg32( UInt32 reg, UInt32 value)
{
	_ipw_write32(memBase,IPW_INDIRECT_ADDR, reg);
	_ipw_write32(memBase,IPW_INDIRECT_DATA, value);
}

int darwin_iwi2200::ipw_fw_dma_enable()
{				/* start dma engine but no transfers yet */

	ipw_fw_dma_reset_command_blocks();
	ipw_write_reg32(IPW_DMA_I_CB_BASE, IPW_SHARED_SRAM_DMA_CONTROL);
	return 0;
}

void darwin_iwi2200::ipw_write_indirect(UInt32 addr, UInt8 * buf,
				int num)
{
	UInt32 aligned_addr = addr & IPW_INDIRECT_ADDR_MASK;	/* dword align */
	UInt32 dif_len = addr - aligned_addr;
	UInt32 i;


	if (num <= 0) {
		return;
	}

	/* Write the first dword (or portion) byte by byte */
	if (unlikely(dif_len)) {
		_ipw_write32(memBase,IPW_INDIRECT_ADDR, aligned_addr);
		/* Start writing at aligned_addr + dif_len */
		for (i = dif_len; ((i < 4) && (num > 0)); i++, num--, buf++)
			_ipw_write8(memBase, IPW_INDIRECT_DATA + i, *buf);
		aligned_addr += 4;
	}

	/* Write all of the middle dwords as dwords, with auto-increment */
	_ipw_write32(memBase, IPW_AUTOINC_ADDR, aligned_addr);
	for (; num >= 4; buf += 4, aligned_addr += 4, num -= 4)
		_ipw_write32(memBase, IPW_AUTOINC_DATA, *(UInt32 *) buf);

	/* Write the last dword (or portion) byte by byte */
	if (unlikely(num)) {
		_ipw_write32(memBase,IPW_INDIRECT_ADDR, aligned_addr);
		for (i = 0; num > 0; i++, num--, buf++)
			_ipw_write8(memBase, IPW_INDIRECT_DATA + i, *buf);
	}
}


int darwin_iwi2200::ipw_fw_dma_add_buffer(UInt32 src_phys, UInt32 dest_address, UInt32 length)
{
	UInt32 bytes_left = length;
	UInt32 src_offset = 0;
	UInt32 dest_offset = 0;
	int status = 0;

	while (bytes_left > CB_MAX_LENGTH) {
		status = ipw_fw_dma_add_command_block(
						      src_phys + src_offset,
						      dest_address +
						      dest_offset,
						      CB_MAX_LENGTH, 0, 0);
		if (status) {
			return -1;
		};

		src_offset += CB_MAX_LENGTH;
		dest_offset += CB_MAX_LENGTH;
		bytes_left -= CB_MAX_LENGTH;
	}

	/* add the buffer tail */
	if (bytes_left > 0) {
		status =
		    ipw_fw_dma_add_command_block( src_phys + src_offset,
						 dest_address + dest_offset,
						 bytes_left, 0, 0);
		if (status) {
			return -1;
		};
	}

	return 0;
}

int darwin_iwi2200::ipw_fw_dma_write_command_block(int index,
					  struct command_block *cb)
{
	UInt32 address =
	    IPW_SHARED_SRAM_DMA_CONTROL +
	    (sizeof(struct command_block) * index);

	ipw_write_indirect(address, (UInt8 *) cb,
			   (int)sizeof(command_block));

	return 0;

}

int darwin_iwi2200::ipw_fw_dma_kick()
{
	UInt32 control = 0;
	UInt32 index = 0;


	for (index = 0; index < sram_desc.last_cb_index; index++)
		ipw_fw_dma_write_command_block(index,
					       &sram_desc.cb_list[index]);

	/* Enable the DMA in the CSR register */
	ipw_clear_bit( IPW_RESET_REG,
		      IPW_RESET_REG_MASTER_DISABLED |
		      IPW_RESET_REG_STOP_MASTER);

	/* Set the Start bit. */
	control = DMA_CONTROL_SMALL_CB_CONST_VALUE | DMA_CB_START;
	ipw_write_reg32( IPW_DMA_I_DMA_CONTROL, control);

	return 0;
}

UInt32 darwin_iwi2200::ipw_read_reg32( UInt32 reg)
{
	UInt32 value;


	_ipw_write32(memBase, IPW_INDIRECT_ADDR, reg);
	value = _ipw_read32(memBase, IPW_INDIRECT_DATA);
	return value;
}

int darwin_iwi2200::ipw_fw_dma_command_block_index()
{
	UInt32 current_cb_address = 0;
	UInt32 current_cb_index = 0;

	current_cb_address = ipw_read_reg32(IPW_DMA_I_CURRENT_CB);

	current_cb_index = (current_cb_address - IPW_SHARED_SRAM_DMA_CONTROL) /
	    sizeof(command_block);

	return current_cb_index;
}

void darwin_iwi2200::ipw_fw_dma_dump_command_block()
{
	UInt32 address;
	UInt32 register_value = 0;
	UInt32 cb_fields_address = 0;

	address = ipw_read_reg32(IPW_DMA_I_CURRENT_CB);

	/* Read the DMA Controlor register */
	register_value = ipw_read_reg32(IPW_DMA_I_DMA_CONTROL);

	/* Print the CB values */
	cb_fields_address = address;
	register_value = ipw_read_reg32( cb_fields_address);

	cb_fields_address += sizeof(UInt32);
	register_value = ipw_read_reg32( cb_fields_address);

	cb_fields_address += sizeof(UInt32);
	register_value = ipw_read_reg32( cb_fields_address);

	cb_fields_address += sizeof(UInt32);
	register_value = ipw_read_reg32( cb_fields_address);

}

void darwin_iwi2200::ipw_fw_dma_abort()
{
	UInt32 control = 0;


	/* set the Stop and Abort bit */
	control = DMA_CONTROL_SMALL_CB_CONST_VALUE | DMA_CB_STOP_AND_ABORT;
	ipw_write_reg32(IPW_DMA_I_DMA_CONTROL, control);
	sram_desc.last_cb_index = 0;

}

int darwin_iwi2200::ipw_fw_dma_wait()
{
	UInt32 current_index = 0, previous_index;
	UInt32 watchdog = 0;


	current_index = ipw_fw_dma_command_block_index();

	while (current_index < sram_desc.last_cb_index) {
		udelay(50);
		previous_index = current_index;
		current_index = ipw_fw_dma_command_block_index();

		if (previous_index < current_index) {
			watchdog = 0;
			continue;
		}
		if (++watchdog > 400) {
			ipw_fw_dma_dump_command_block();
			ipw_fw_dma_abort();
			return -1;
		}
	}

	ipw_fw_dma_abort();

	/*Disable the DMA in the CSR register */
	ipw_set_bit(IPW_RESET_REG,
		    IPW_RESET_REG_MASTER_DISABLED | IPW_RESET_REG_STOP_MASTER);

	return 0;
}


bool darwin_iwi2200::uploadFirmware(u8 * data, size_t len)
{	
	int rc = -1;
	int offset = 0;
	struct fw_chunk *chunk;
	dma_addr_t shared_phys;
	u8 *shared_virt;
	IOBufferMemoryDescriptor *memD;

	memD = MemoryDmaAlloc(len, &shared_phys, &shared_virt);
	if(!memD) 
		return -ENOMEM;

	memmove(shared_virt, data, len);

	/* Start the Dma */
	rc = ipw_fw_dma_enable();

	if (sram_desc.last_cb_index > 0) {
		/* the DMA is already ready this would be a bug. */
		goto out;
	}

	do {
		chunk = (struct fw_chunk *)(data + offset);
		offset += sizeof(struct fw_chunk);
		/* build DMA packet and queue up for sending */
		/* dma to chunk->address, the chunk->length bytes from data +
		 * offeset*/
		/* Dma loading */
		rc = ipw_fw_dma_add_buffer(shared_phys + offset,
					   le32_to_cpu(chunk->address), le32_to_cpu(chunk->length));
		if (rc) {
			IWI_DEBUG("dmaAddBuffer Failed\n");
			goto out;
		}

		offset += le32_to_cpu(chunk->length);
	} while (offset < len);

	/* Run the DMA and wait for the answer*/
	rc = ipw_fw_dma_kick();
	if (rc) {
		IWI_DEBUG("dmaKick Failed\n");
		goto out;
	}

	rc = ipw_fw_dma_wait();
	if (rc) {
		IWI_DEBUG("dmaWaitSync Failed\n");
		goto out;
	}
	
 out:
	memD->release();
	return rc;
		   
}

bool darwin_iwi2200::uploadUCode2(UInt16 *base, const unsigned char *uc, UInt16 size, int offset)
{
	UInt32 tmp;
	int ntries;
	UInt16 *w;
	UInt32 x[7];
	struct alive_command_responce alive;
	
	uc += offset;;
	size -= offset;
	
	CSR_WRITE_4(base, IWI_CSR_RST, CSR_READ_4(base, IWI_CSR_RST) | 
		IWI_RST_STOP_MASTER);
	for (ntries = 0; ntries < 5; ntries++) {
		if (CSR_READ_4(base, IWI_CSR_RST) & IWI_RST_MASTER_DISABLED)
			break;
		IODelay(100);
	}
	if (ntries == 5) {
		IWI_DEBUG("timeout waiting for master\n");
		goto fail;
	}

	MEM_WRITE_4(base, 0x3000e0, 0x80000000);
	IODelay(5000);

	tmp = CSR_READ_4(base, IWI_CSR_RST);
	tmp &= ~IWI_RST_PRINCETON_RESET;
	CSR_WRITE_4(base, IWI_CSR_RST, tmp);

	IODelay(5000);
	MEM_WRITE_4(base, 0x3000e0, 0);
	IODelay(1000);
	MEM_WRITE_4(base, 0x300004, 1);
	IODelay(1000);
	MEM_WRITE_4(base, 0x300004, 0);
	IODelay(1000);
	MEM_WRITE_1(base, 0x200000, 0x00);
	MEM_WRITE_1(base, 0x200000, 0x40);
	IODelay(1000);
	
	/* write microcode onto the device */
	for(w = (UInt16 *)uc; size > 0; size-=2, w++) {
		MEM_WRITE_2(base, 0x200010, *w);
	//	if((size % 3000) == 0)
	//		IWI_DEBUG("*w: 0x%x (%d)\n", *w, size);
	}
	
	MEM_WRITE_1(base, 0x200000, 0x00);
	MEM_WRITE_1(base, 0x200000, 0x80);

	/* wait until we get an answer */
	for (ntries = 0; ntries < 10; ntries++) {
		if (MEM_READ_1(base, 0x200000) & 1)
			break;
		IODelay(100);
	}
	if(ntries == 10)
		IWI_DEBUG("timeout waiting for answer\n");
	
	/* read the answer or the firmware will not initialize properly */
	for (int i = 0; i < 7; i++)
		x[i] = MEM_READ_4(base, 0x200004);
	
	memcpy(&alive, x, sizeof(struct alive_command_responce));

	if(alive.alive_command == 1 && alive.ucode_valid == 1)
	IWI_DEBUG("%s: Microcode OK, rev. %d (0x%x) dev. %d (0x%x) of %02d/%02d/%02d %02d:%02d\n", getName(),
		alive.software_revision,
		alive.software_revision, 
		alive.device_identifier,
		alive.device_identifier,
		alive.time_stamp[0],
		alive.time_stamp[1],
		alive.time_stamp[2],
		alive.time_stamp[3],
		alive.time_stamp[4]);
	else
		IWI_DEBUG("Microcode NOT ok!\n");
	
	MEM_WRITE_1(base, 0x200000, 0x00);

	return true;

fail:
	return false;
}


bool darwin_iwi2200::uploadFirmware2(UInt16 *base, const unsigned char *fw, UInt32 size, int offset)
{	
	dma_addr_t physAddr, src;
	UInt8 *virtAddr, *p, *end;
	UInt32 dst, len, mlen, ctl, sum, sentinel, tmp, ntries;
	IOBufferMemoryDescriptor *memD;
	size -= offset;
	fw += offset;
	
	memD = MemoryDmaAlloc(size, &physAddr, &virtAddr);
	if(!memD) 
		IWI_DEBUG("%s: dma_mem_alloc failer\n", getName());
//	XXX	bus_dmamap_sync(dmat, map, BUS_DMASYNC_PREWRITE); 
	memcpy(virtAddr, fw, size);
	
	// tell the adapter where the command blocks are stored 
	MEM_WRITE_4(base, 0x3000a0, 0x27000);

	/*
	 * Store command blocks into adapter's internal memory using register
	 * indirections. The adapter will read the firmware image through DMA
	 * using information stored in command blocks.
	 */
	src = physAddr;
	p = virtAddr;
	end = p + size;

	CSR_WRITE_4(base, IWI_CSR_AUTOINC_ADDR, 0x27000);

	while (p < end)
	{
		dst = GETLE32(p); p += 4; src += 4;
		len = GETLE32(p); p += 4; src += 4;
		p += len;

	//	IWI_DEBUG("dst: 0x%8x    len: 0x%8x\n",dst,len);
		while (len > 0)
		{
			mlen = min(len, IWI_CB_MAXDATALEN);

			ctl = IWI_CB_DEFAULT_CTL | mlen;
			sum = ctl ^ src ^ dst;

			// write a command block
			CSR_WRITE_4(base, IWI_CSR_AUTOINC_DATA, ctl);
			CSR_WRITE_4(base, IWI_CSR_AUTOINC_DATA, src);
			CSR_WRITE_4(base, IWI_CSR_AUTOINC_DATA, dst);
			CSR_WRITE_4(base, IWI_CSR_AUTOINC_DATA, sum);

			src += mlen;
			dst += mlen;
			len -= mlen;
		}
	}

	// write a fictive final command block (sentinel)
	sentinel = CSR_READ_4(base, IWI_CSR_AUTOINC_ADDR);
	CSR_WRITE_4(base, IWI_CSR_AUTOINC_DATA, 0);


	tmp = CSR_READ_4(base, IWI_CSR_RST);
	tmp &= ~(IWI_RST_MASTER_DISABLED | IWI_RST_STOP_MASTER);
	CSR_WRITE_4(base, IWI_CSR_RST, tmp);

	// tell the adapter to start processing command blocks
	MEM_WRITE_4(base, 0x3000a4, 0x540100);

	// wait until the adapter reach the sentinel 
	for (ntries = 0; ntries < 400; ntries++) {
		if (MEM_READ_4(base, 0x3000d0) >= sentinel)
			break;
		IODelay(100*100);
	}
	
	if (ntries == 400) {
		IWI_DEBUG("timeout processing command blocks\n");
		return false;
	}
	
	// we're done with command blocks processing 
	MEM_WRITE_4(base, 0x3000a4, 0x540c00);
  
  
/*
	// allow interrupts so we know when the firmware is inited 
	CSR_WRITE_4(base, IWI_CSR_INTR_MASK, IWI_INTR_MASK);

	// tell the adapter to initialize the firmware
	CSR_WRITE_4(base, IWI_CSR_RST, 0);

	tmp = CSR_READ_4(base, IWI_CSR_CTL);
	CSR_WRITE_4(base, IWI_CSR_CTL, tmp | IWI_CTL_ALLOW_STANDBY);
	for(ntries = 0; ntries < 50; ntries++) {
		if(handleInterrupt() == IWI_INTR_FW_INITED)
			break;  
		IODelay(1000);
	}       
	if(ntries == 50) {
		memD->release();
		return false;
	}*/
	memD->release();
	return true;
}


int darwin_iwi2200::ipw_get_fw(const struct firmware **fw, const char *name)
{
	struct fw_header *header;
	int rc;

	/* ask firmware_class module to get the boot firmware off disk */
	if (name=="bss") fw=(const struct firmware**)iwi_bss;
	if (name=="bss_ucode") fw=(const struct firmware**)iwi_ucode_bss;
	if (name=="boot") fw=(const struct firmware**)iwi_boot;

	header = (struct fw_header *)(*fw)->data;
	if (IPW_FW_MAJOR(le32_to_cpu(header->version)) != IPW_FW_MAJOR_VERSION) {
		IWI_DEBUG("'%s' firmware version not compatible (%d != %d)\n",
			  name,
			  IPW_FW_MAJOR(le32_to_cpu(header->version)), IPW_FW_MAJOR_VERSION);
		return -EINVAL;
	}

	IWI_DEBUG("Loading firmware '%s' file v%d.%d (%d bytes)\n",
		       name,
		       IPW_FW_MAJOR(le32_to_cpu(header->version)),
		       IPW_FW_MINOR(le32_to_cpu(header->version)),
		       (*fw)->size - sizeof(struct fw_header));
	return 0;

	
}

IOBufferMemoryDescriptor*
darwin_iwi2200::MemoryDmaAlloc(UInt32 buf_size, dma_addr_t *phys_add, void *virt_add)
{
	IOBufferMemoryDescriptor *memBuffer;
	void *virt_address;
	dma_addr_t phys_address;
	IOMemoryMap *memMap;
	
	memBuffer = IOBufferMemoryDescriptor::inTaskWithOptions(kernel_task,
						kIODirectionOutIn | kIOMemoryPhysicallyContiguous | \
						kIOMemoryAutoPrepare | kIOMapInhibitCache, buf_size, \
						PAGE_SIZE);

	if (memBuffer == NULL) {
		IWI_DEBUG("Memory Allocation failed - RLC");

		return NULL;
	}

	memMap = memBuffer->map();

	if (memMap == NULL) {
		IWI_DEBUG("mapping failed\n");
		memBuffer->release();
		memBuffer = NULL;
		
		return NULL;	
	}

	phys_address = memMap->getPhysicalAddress();

	virt_address = (void *)memMap->getVirtualAddress();

	if (virt_address == NULL || phys_address == NULL) {
		memMap->release();
		memBuffer->release();
		memBuffer = NULL;

		return NULL;
	}

	*phys_add = phys_address;
	*(IOVirtualAddress*)virt_add = (IOVirtualAddress)virt_address;
	memMap->release();

	return memBuffer;
}

int darwin_iwi2200::ipw_queue_space(const struct clx2_queue *q)
{
	int s = q->last_used - q->first_empty;
	if (s <= 0)
		s += q->n_bd;
	s -= 2;			/* keep some reserve to not confuse empty and full situations */
	if (s < 0)
		s = 0;
	return s;
}

int darwin_iwi2200::ipw_queue_tx_hcmd(struct ipw_priv *priv, int hcmd, void *buf,
			     int len, int sync)
{
	struct clx2_tx_queue *txq = &priv->txq_cmd;
	struct clx2_queue *q = &txq->q;
	struct tfd_frame *tfd;

	if (ipw_queue_space(q) < (sync ? 1 : 2)) {
		IWI_DEBUG("No space for Tx\n");
		return -EBUSY;
	}

	tfd = &txq->bd[q->first_empty];
	txq->txb[q->first_empty] = NULL;

	memset(tfd, 0, sizeof(*tfd));
	tfd->control_flags.message_type = TX_HOST_COMMAND_TYPE;
	tfd->control_flags.control_bits = TFD_NEED_IRQ_MASK;
	priv->hcmd_seq++;
	tfd->u.cmd.index = hcmd;
	tfd->u.cmd.length = len;
	memcpy(tfd->u.cmd.payload, buf, len);
	q->first_empty = ipw_queue_inc_wrap(q->first_empty, q->n_bd);
	ipw_write32( q->reg_w, q->first_empty);
	_ipw_read32(memBase, 0x90);

	return 0;
}

int darwin_iwi2200::__ipw_send_cmd(struct ipw_priv *priv, struct host_cmd *cmd)
{
	int rc = 0;
	unsigned long flags;

	//spin_lock_irqsave(&priv->lock, flags);
	if (priv->status & STATUS_HCMD_ACTIVE) {
		IWI_DEBUG("Failed to send %s: Already sending a command.\n",
			  get_cmd_string(cmd->cmd));
		//spin_unlock_irqrestore(&priv->lock, flags);
		return -EAGAIN;
	}

	if (priv->status & STATUS_ASSOCIATING) {
		IWI_DEBUG("abandon a command while associating\n");
		//spin_unlock_irqrestore(&priv->lock, flags);
		return -1;
	}

	if (priv->status & STATUS_DISASSOCIATING) {
		IWI_DEBUG("abandon a command while disassociating\n");
		//spin_unlock_irqrestore(&priv->lock, flags);
		return -1;
	}
	priv->status |= STATUS_HCMD_ACTIVE;
	
	if (priv->cmdlog) {
		//priv->cmdlog[priv->cmdlog_pos].jiffies = jiffies;
		priv->cmdlog[priv->cmdlog_pos].cmd.cmd = cmd->cmd;
		priv->cmdlog[priv->cmdlog_pos].cmd.len = cmd->len;
		memcpy(priv->cmdlog[priv->cmdlog_pos].cmd.param, cmd->param,
		       cmd->len);
		priv->cmdlog[priv->cmdlog_pos].retcode = -1;
	}

	IWI_DEBUG("%s command (#%d) %d bytes: 0x%08X\n",
		     get_cmd_string(cmd->cmd), cmd->cmd, cmd->len,
		     priv->status);


	IWI_DEBUG_DUMP((u8 *) cmd->param, cmd->len);

	rc = ipw_queue_tx_hcmd(priv, cmd->cmd, cmd->param, cmd->len, 0);
	if (rc) {
		priv->status &= ~STATUS_HCMD_ACTIVE;		
		IWI_DEBUG("Failed to send %s: Reason %d\n",
			  get_cmd_string(cmd->cmd), rc);
		//spin_unlock_irqrestore(&priv->lock, flags);
		goto exit;
	}
	//spin_unlock_irqrestore(&priv->lock, flags);

	//rc = wait_event_interruptible_timeout(priv->wait_command_queue,	!(priv->status & STATUS_HCMD_ACTIVE), HOST_COMPLETE_TIMEOUT);
	rc=0;
	while (priv->status & STATUS_HCMD_ACTIVE) 
	{
		rc++;
		IODelay(HZ);
		if (rc==HZ) break;
	}
	if (rc == 0) {
		//spin_lock_irqsave(&priv->lock, flags);
		if (priv->status & STATUS_HCMD_ACTIVE) {
			IWI_DEBUG("Failed to send %s: Command timed out.\n",
				  get_cmd_string(cmd->cmd));
			priv->status &= ~STATUS_HCMD_ACTIVE;
			//spin_unlock_irqrestore(&priv->lock, flags);
			rc = -EIO;
			goto exit;
		}
		//spin_unlock_irqrestore(&priv->lock, flags);
	} else
		rc = 0;

	if (priv->status & STATUS_RF_KILL_HW) {
		IWI_DEBUG("Failed to send %s: Aborted due to RF kill switch.\n",
			  get_cmd_string(cmd->cmd));
		rc = -EIO;
		goto exit;
	}

      exit:
	if (priv->cmdlog) {
		priv->cmdlog[priv->cmdlog_pos++].retcode = rc;
		priv->cmdlog_pos %= priv->cmdlog_len;
	}
	return rc;
}

int darwin_iwi2200::ipw_send_cmd_simple(struct ipw_priv *priv, u8 command)
{
	struct host_cmd cmd = {
		command,
		NULL,
		NULL,
		NULL
	};

	return __ipw_send_cmd(priv, &cmd);
}

int darwin_iwi2200::ipw_send_cmd_pdu(struct ipw_priv *priv, u8 command, u8 len,
			    void *data)
{
	struct host_cmd cmd = {
		command,
		len,
		NULL,
		(u32*)data
	};

	return __ipw_send_cmd(priv, &cmd);
}

int darwin_iwi2200::sendCommand(UInt8 type,void *data,UInt8 len,bool async)
{

	
	struct iwi_cmd_desc *desc;
	priv->status |= STATUS_HCMD_ACTIVE;
	
	desc = &this->cmdq.desc[cmdq.cur];
	desc->hdr.type = IWI_HDR_TYPE_COMMAND;
	desc->hdr.flags = IWI_HDR_FLAG_IRQ;
	desc->type = type;
	desc->len = len;
	memcpy(desc->data, data, len);
	
//	bus_dmamap_sync(sc->cmdq.desc_dmat, sc->cmdq.desc_map,
//	    BUS_DMASYNC_PREWRITE);

//	IWI_DEBUG("sending command idx=%u type=%u len=%u\n", cmdq.cur, type, len);

	cmdq.cur = (cmdq.cur + 1) % IWI_CMD_RING_COUNT;
	CSR_WRITE_4(memBase, IWI_CSR_CMD_WIDX, cmdq.cur);
	
	int r=0;
	if (async) 
	while (priv->status & STATUS_HCMD_ACTIVE) 
	{
		r++;
		IODelay(HZ);
		if (r==HZ) break;
	}	
//	return async ? 0 : msleep(sc, &sc->sc_mtx, 0, "iwicmd", hz);
	return 0;
}

const struct ieee80211_geo* darwin_iwi2200::ipw_get_geo(struct ieee80211_device *ieee)
{
	return &ieee->geo;
}

int darwin_iwi2200::ipw_set_tx_power(struct ipw_priv *priv)
{
	const struct ieee80211_geo *geo = ipw_get_geo(priv->ieee);
	struct ipw_tx_power tx_power;
	s8 max_power;
	int i;

	memset(&tx_power, 0, sizeof(tx_power));

	/* configure device for 'G' band */
	tx_power.ieee_mode = IPW_G_MODE;
	tx_power.num_channels = geo->bg_channels;
	for (i = 0; i < geo->bg_channels; i++) {
		max_power = geo->bg[i].max_power;
		tx_power.channels_tx_power[i].channel_number =
		    geo->bg[i].channel;
		tx_power.channels_tx_power[i].tx_power = max_power ?
		    min(max_power, priv->tx_power) : priv->tx_power;
	}
	ipw_send_cmd_pdu(priv, IPW_CMD_TX_POWER, sizeof(tx_power), &tx_power);
	//if (sendCommand(IPW_CMD_TX_POWER, &tx_power,sizeof(tx_power), 1))		return -EIO;

	/* configure device to also handle 'B' band */
	tx_power.ieee_mode = IPW_B_MODE;
	ipw_send_cmd_pdu(priv, IPW_CMD_TX_POWER, sizeof(tx_power), &tx_power);
	//if (sendCommand(IPW_CMD_TX_POWER, &tx_power,sizeof(tx_power), 1))		return -EIO;

	/* configure device to also handle 'A' band */
	if (priv->ieee->abg_true) {
		tx_power.ieee_mode = IPW_A_MODE;
		tx_power.num_channels = geo->a_channels;
		for (i = 0; i < tx_power.num_channels; i++) {
			max_power = geo->a[i].max_power;
			tx_power.channels_tx_power[i].channel_number =
			    geo->a[i].channel;
			tx_power.channels_tx_power[i].tx_power = max_power ?
			    min(max_power, priv->tx_power) : priv->tx_power;
		}
		ipw_send_cmd_pdu(priv, IPW_CMD_TX_POWER, sizeof(tx_power), &tx_power);
		//if (sendCommand(IPW_CMD_TX_POWER, &tx_power,sizeof(tx_power), 1))			return -EIO;
	}
	return 0;
}

void darwin_iwi2200::init_sys_config(struct ipw_sys_config *sys_config)
{
	memset(sys_config, 0, sizeof(struct ipw_sys_config));
	sys_config->bt_coexistence = 0;
	sys_config->answer_broadcast_ssid_probe = 0;
	sys_config->accept_all_data_frames = 0;
	sys_config->accept_non_directed_frames = 1;
	sys_config->exclude_unicast_unencrypted = 0;
	sys_config->disable_unicast_decryption = 1;
	sys_config->exclude_multicast_unencrypted = 0;
	sys_config->disable_multicast_decryption = 1;
	if (antenna < CFG_SYS_ANTENNA_BOTH || antenna > CFG_SYS_ANTENNA_B)
		antenna = CFG_SYS_ANTENNA_BOTH;
	sys_config->antenna_diversity = antenna;
	sys_config->pass_crc_to_host = 0;	/* TODO: See if 1 gives us FCS */
	sys_config->dot11g_auto_detection = 0;
	sys_config->enable_cts_to_self = 0;
	sys_config->bt_coexist_collision_thr = 0;
	sys_config->pass_noise_stats_to_host = 1;	/* 1 -- fix for 256 */
	sys_config->silence_threshold = 0x1e;
}

void darwin_iwi2200::ipw_add_cck_scan_rates(struct ipw_supported_rates *rates,
				   u8 modulation, u32 rate_mask)
{
	u8 basic_mask = (IEEE80211_OFDM_MODULATION == modulation) ?
	    IEEE80211_BASIC_RATE_MASK : 0;

	if (rate_mask & IEEE80211_CCK_RATE_1MB_MASK)
		rates->supported_rates[rates->num_rates++] =
		    IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_1MB;

	if (rate_mask & IEEE80211_CCK_RATE_2MB_MASK)
		rates->supported_rates[rates->num_rates++] =
		    IEEE80211_BASIC_RATE_MASK | IEEE80211_CCK_RATE_2MB;

	if (rate_mask & IEEE80211_CCK_RATE_5MB_MASK)
		rates->supported_rates[rates->num_rates++] = basic_mask |
		    IEEE80211_CCK_RATE_5MB;

	if (rate_mask & IEEE80211_CCK_RATE_11MB_MASK)
		rates->supported_rates[rates->num_rates++] = basic_mask |
		    IEEE80211_CCK_RATE_11MB;
}

void darwin_iwi2200::ipw_add_ofdm_scan_rates(struct ipw_supported_rates *rates,
				    u8 modulation, u32 rate_mask)
{
	u8 basic_mask = (IEEE80211_OFDM_MODULATION == modulation) ?
	    IEEE80211_BASIC_RATE_MASK : 0;

	if (rate_mask & IEEE80211_OFDM_RATE_6MB_MASK)
		rates->supported_rates[rates->num_rates++] = basic_mask |
		    IEEE80211_OFDM_RATE_6MB;

	if (rate_mask & IEEE80211_OFDM_RATE_9MB_MASK)
		rates->supported_rates[rates->num_rates++] =
		    IEEE80211_OFDM_RATE_9MB;

	if (rate_mask & IEEE80211_OFDM_RATE_12MB_MASK)
		rates->supported_rates[rates->num_rates++] = basic_mask |
		    IEEE80211_OFDM_RATE_12MB;

	if (rate_mask & IEEE80211_OFDM_RATE_18MB_MASK)
		rates->supported_rates[rates->num_rates++] =
		    IEEE80211_OFDM_RATE_18MB;

	if (rate_mask & IEEE80211_OFDM_RATE_24MB_MASK)
		rates->supported_rates[rates->num_rates++] = basic_mask |
		    IEEE80211_OFDM_RATE_24MB;

	if (rate_mask & IEEE80211_OFDM_RATE_36MB_MASK)
		rates->supported_rates[rates->num_rates++] =
		    IEEE80211_OFDM_RATE_36MB;

	if (rate_mask & IEEE80211_OFDM_RATE_48MB_MASK)
		rates->supported_rates[rates->num_rates++] =
		    IEEE80211_OFDM_RATE_48MB;

	if (rate_mask & IEEE80211_OFDM_RATE_54MB_MASK)
		rates->supported_rates[rates->num_rates++] =
		    IEEE80211_OFDM_RATE_54MB;
}

int darwin_iwi2200::init_supported_rates(struct ipw_priv *priv,
				struct ipw_supported_rates *rates)
{
	/* TODO: Mask out rates based on priv->rates_mask */

	memset(rates, 0, sizeof(*rates));
	/* configure supported rates */
	switch (priv->ieee->freq_band) {
	case IEEE80211_52GHZ_BAND:
		rates->ieee_mode = IPW_A_MODE;
		rates->purpose = IPW_RATE_CAPABILITIES;
		ipw_add_ofdm_scan_rates(rates, IEEE80211_CCK_MODULATION,
					IEEE80211_OFDM_DEFAULT_RATES_MASK);
		break;

	default:		/* Mixed or 2.4Ghz */
		rates->ieee_mode = IPW_G_MODE;
		rates->purpose = IPW_RATE_CAPABILITIES;
		ipw_add_cck_scan_rates(rates, IEEE80211_CCK_MODULATION,
				       IEEE80211_CCK_DEFAULT_RATES_MASK);
		if (priv->ieee->modulation & IEEE80211_OFDM_MODULATION) {
			ipw_add_ofdm_scan_rates(rates, IEEE80211_CCK_MODULATION,
						IEEE80211_OFDM_DEFAULT_RATES_MASK);
		}
		break;
	}

	return 0;
}

void darwin_iwi2200::ipw_send_tgi_tx_key(struct ipw_priv *priv, int type, int index)
{
	struct ipw_tgi_tx_key key;

	if (!(priv->ieee->sec.flags & (1 << index)))
		return;

	key.key_id = index;
	memcpy(key.key, priv->ieee->sec.keys[index], SCM_TEMPORAL_KEY_LENGTH);
	key.security_type = type;
	key.station_index = 0;	/* always 0 for BSS */
	key.flags = 0;
	/* 0 for new key; previous value of counter (after fatal error) */
	key.tx_counter[0] = cpu_to_le32(0);
	key.tx_counter[1] = cpu_to_le32(0);
	ipw_send_cmd_pdu(priv, IPW_CMD_TGI_TX_KEY, sizeof(key), &key);
	//sendCommand(IPW_CMD_TGI_TX_KEY, &key,sizeof(key), 1);
}

void darwin_iwi2200::ipw_send_wep_keys(struct ipw_priv *priv, int type)
{
	struct ipw_wep_key key;
	int i;

	key.cmd_id = DINO_CMD_WEP_KEY;
	key.seq_num = 0;

	/* Note: AES keys cannot be set for multiple times.
	 * Only set it at the first time. */
	for (i = 0; i < 4; i++) {
		key.key_index = i | type;
		if (!(priv->ieee->sec.flags & (1 << i))) {
			key.key_size = 0;
			continue;
		}

		key.key_size = priv->ieee->sec.key_sizes[i];
		memcpy(key.key, priv->ieee->sec.keys[i], key.key_size);
		ipw_send_cmd_pdu(priv, IPW_CMD_WEP_KEY, sizeof(key), &key);
		//sendCommand(IPW_CMD_WEP_KEY, &key,sizeof(key), 1);
	}
}

void darwin_iwi2200::ipw_set_hw_decrypt_unicast(struct ipw_priv *priv, int level)
{
	if (priv->ieee->host_encrypt)
		return;

	switch (level) {
	case SEC_LEVEL_3:
		priv->sys_config.disable_unicast_decryption = 0;
		priv->ieee->host_decrypt = 0;
		break;
	case SEC_LEVEL_2:
		priv->sys_config.disable_unicast_decryption = 1;
		priv->ieee->host_decrypt = 1;
		break;
	case SEC_LEVEL_1:
		priv->sys_config.disable_unicast_decryption = 0;
		priv->ieee->host_decrypt = 0;
		break;
	case SEC_LEVEL_0:
		priv->sys_config.disable_unicast_decryption = 1;
		break;
	default:
		break;
	}
}

void darwin_iwi2200::ipw_set_hw_decrypt_multicast(struct ipw_priv *priv, int level)
{
	if (priv->ieee->host_encrypt)
		return;

	switch (level) {
	case SEC_LEVEL_3:
		priv->sys_config.disable_multicast_decryption = 0;
		break;
	case SEC_LEVEL_2:
		priv->sys_config.disable_multicast_decryption = 1;
		break;
	case SEC_LEVEL_1:
		priv->sys_config.disable_multicast_decryption = 0;
		break;
	case SEC_LEVEL_0:
		priv->sys_config.disable_multicast_decryption = 1;
		break;
	default:
		break;
	}
}

void darwin_iwi2200::ipw_set_hwcrypto_keys(struct ipw_priv *priv)
{
	switch (priv->ieee->sec.level) {
	case SEC_LEVEL_3:
		if (priv->ieee->sec.flags & SEC_ACTIVE_KEY)
			ipw_send_tgi_tx_key(priv,
					    DCT_FLAG_EXT_SECURITY_CCM,
					    priv->ieee->sec.active_key);

		if (!priv->ieee->host_mc_decrypt)
			ipw_send_wep_keys(priv, DCW_WEP_KEY_SEC_TYPE_CCM);
		break;
	case SEC_LEVEL_2:
		if (priv->ieee->sec.flags & SEC_ACTIVE_KEY)
			ipw_send_tgi_tx_key(priv,
					    DCT_FLAG_EXT_SECURITY_TKIP,
					    priv->ieee->sec.active_key);
		break;
	case SEC_LEVEL_1:
		ipw_send_wep_keys(priv, DCW_WEP_KEY_SEC_TYPE_WEP);
		ipw_set_hw_decrypt_unicast(priv, priv->ieee->sec.level);
		ipw_set_hw_decrypt_multicast(priv, priv->ieee->sec.level);
		break;
	case SEC_LEVEL_0:
	default:
		break;
	}
}

bool darwin_iwi2200::configureInterface(IONetworkInterface * netif)
 {
    IONetworkData * data;
    IWI_DEBUG("configureInterface\n");
    if (super::configureInterface(netif) == false)
            return false;
    
    // Get the generic network statistics structure.

    data = netif->getParameter(kIONetworkStatsKey);
    if (!data || !(netStats = (IONetworkStats *)data->getBuffer())) {
            return false;
    }

    // Get the Ethernet statistics structure.

    data = netif->getParameter(kIOEthernetStatsKey);
    if (!data || !(etherStats = (IOEthernetStats *)data->getBuffer())) {
            return false;
    }
    return true;
}

int darwin_iwi2200::configu(struct ipw_priv *priv)
{
	int rc;
	
	if (ipw_set_tx_power(priv))	goto error;

	/* initialize adapter address */
	//memcpy(priv->mac_addr, &priv->eeprom[EEPROM_MAC_ADDRESS], 6);

	IWI_DEBUG("Setting MAC address to %6D\n",priv->mac_addr,":");
	ipw_send_cmd_pdu(priv, IPW_CMD_ADAPTER_ADDRESS, ETH_ALEN, priv->mac_addr);
	//rc = sendCommand(IWI_CMD_SET_MAC_ADDRESS, Addr,6, 1);
	//if (rc != 0)		return rc;

	/* set basic system config settings */
	init_sys_config(&priv->sys_config);

	/* Support Bluetooth if we have BT h/w on board, and user wants to.
	 * Does not support BT priority yet (don't abort or defer our Tx) */
	if (bt_coexist) {
		unsigned char bt_caps = priv->eeprom[EEPROM_SKU_CAPABILITY];

		if (bt_caps & EEPROM_SKU_CAP_BT_CHANNEL_SIG)
			priv->sys_config.bt_coexistence
			    |= CFG_BT_COEXISTENCE_SIGNAL_CHNL;
		if (bt_caps & EEPROM_SKU_CAP_BT_OOB)
			priv->sys_config.bt_coexistence
			    |= CFG_BT_COEXISTENCE_OOB;
	}

	if (priv->ieee->iw_mode == IW_MODE_ADHOC)
		priv->sys_config.answer_broadcast_ssid_probe = 1;
	else
		priv->sys_config.answer_broadcast_ssid_probe = 0;

	IWI_DEBUG("Configuring adapter\n");
	ipw_send_cmd_pdu(priv, IPW_CMD_SYSTEM_CONFIG,sizeof(priv->sys_config), &priv->sys_config);
	//rc = sendCommand(IWI_CMD_SET_CONFIG, &priv->sys_config, sizeof(priv->sys_config), 1);
	//if (rc != 0)		return rc;

	init_supported_rates(priv, &priv->rates);
	ipw_send_cmd_pdu(priv, IPW_CMD_SUPPORTED_RATES, sizeof(priv->rates),&priv->rates);
	//rc = sendCommand(IWI_CMD_SET_RATES, &priv->rates, sizeof priv->rates, 1);
	//if (rc != 0)		return rc;

	/* Set request-to-send threshold */
	if (priv->rts_threshold) {
		struct ipw_rts_threshold rts_threshold2 = {
		cpu_to_le16(priv->rts_threshold),
		NULL
		};
		IWI_DEBUG("Setting RTS threshold to %u\n", rts_threshold2);
		ipw_send_cmd_pdu(priv, IPW_CMD_RTS_THRESHOLD, sizeof(rts_threshold2), &rts_threshold2);
		//rc = sendCommand(IWI_CMD_SET_RTS_THRESHOLD, &rts_threshold2, sizeof(rts_threshold2), 1);
		//if (rc != 0)			return rc;
	}
	
#ifdef CONFIG_IPW2200_QOS
	IWI_DEBUG("QoS: call ipw_qos_activate\n");	
	if (qos_enable) ipw_qos_activate(priv, NULL);
#endif				/* CONFIG_IPW2200_QOS */	

	UInt32 data;
	data = 0x12345678;
	IWI_DEBUG("Setting IPW_CMD_SEED_NUMBER to %u\n", data);
	ipw_send_cmd_pdu(priv, IPW_CMD_SEED_NUMBER, sizeof(data), &data);
	//rc = sendCommand(IPW_CMD_SEED_NUMBER, &data, sizeof data, 1);
	//if (rc != 0)		return rc;
	
	 IWI_DEBUG("Setting IPW_CMD_HOST_COMPLETE\n");
	 ipw_send_cmd_simple(priv, IPW_CMD_HOST_COMPLETE);
	//rc = sendCommand(IPW_CMD_HOST_COMPLETE, NULL,0, 0);
	//if (rc != 0)		return rc;
	
	//super::configureInterface(fNetif);

	priv->status |= STATUS_INIT;

	ipw_led_init(priv);
	queue_te(2,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_link_on),priv,NULL,true);
	priv->notif_missed_beacons = 0;

	/* Set hardware WEP key if it is configured. */
	if ((priv->capability & CAP_PRIVACY_ON) &&
	    (priv->ieee->sec.level == SEC_LEVEL_1) &&
	    !(priv->ieee->host_encrypt || priv->ieee->host_decrypt))
		ipw_set_hwcrypto_keys(priv);

	return 0;

      error:
	return -EIO;
}

u8 darwin_iwi2200::ipw_qos_current_mode(struct ipw_priv *priv)
{
	u8 mode = 0;

	if (priv->status & STATUS_ASSOCIATED) {
		unsigned long flags;

		//spin_lock_irqsave(&priv->ieee->lock, flags);
		mode = priv->assoc_network->mode;
		//spin_unlock_irqrestore(&priv->ieee->lock, flags);
	} else {
		mode = priv->ieee->mode;
	}
	IWI_LOG("QoS network/card mode %d \n", mode);
	return mode;
}

u32 darwin_iwi2200::ipw_qos_get_burst_duration(struct ipw_priv *priv)
{
	u32 ret = 0;

	if ((priv == NULL))
		return 0;

	if (!(priv->ieee->modulation & IEEE80211_OFDM_MODULATION))
		ret = priv->qos_data.burst_duration_CCK;
	else
		ret = priv->qos_data.burst_duration_OFDM;

	return ret;
}

int darwin_iwi2200::ipw_qos_activate(struct ipw_priv *priv,
			    struct ieee80211_qos_data *qos_network_data)
{
	int err;
	struct ieee80211_qos_parameters qos_parameters[QOS_QOS_SETS];
	struct ieee80211_qos_parameters *active_one = NULL;
	u32 size = sizeof(struct ieee80211_qos_parameters);
	u32 burst_duration;
	int i;
	u8 type;

	type = ipw_qos_current_mode(priv);

	active_one = &(qos_parameters[QOS_PARAM_SET_DEF_CCK]);
	memcpy(active_one, priv->qos_data.def_qos_parm_CCK, size);
	active_one = &(qos_parameters[QOS_PARAM_SET_DEF_OFDM]);
	memcpy(active_one, priv->qos_data.def_qos_parm_OFDM, size);

	if (qos_network_data == NULL) {
		if (type == IEEE_B) {
			IWI_DEBUG("QoS activate network mode %d\n", type);
			active_one = &def_parameters_CCK;
		} else
			active_one = &def_parameters_OFDM;

		memcpy(&qos_parameters[QOS_PARAM_SET_ACTIVE], active_one, size);
		burst_duration = ipw_qos_get_burst_duration(priv);
		for (i = 0; i < QOS_QUEUE_NUM; i++)
			qos_parameters[QOS_PARAM_SET_ACTIVE].tx_op_limit[i] =
			    (u16) burst_duration;
	} else if (priv->ieee->iw_mode == IW_MODE_ADHOC) {
		if (type == IEEE_B) {
			IWI_DEBUG("QoS activate IBSS nework mode %d\n",
				      type);
			if (priv->qos_data.qos_enable == 0)
				active_one = &def_parameters_CCK;
			else
				active_one = priv->qos_data.def_qos_parm_CCK;
		} else {
			if (priv->qos_data.qos_enable == 0)
				active_one = &def_parameters_OFDM;
			else
				active_one = priv->qos_data.def_qos_parm_OFDM;
		}
		memcpy(&qos_parameters[QOS_PARAM_SET_ACTIVE], active_one, size);
	} else {
		unsigned long flags;
		int active;

		//spin_lock_irqsave(&priv->ieee->lock, flags);
		active_one = &(qos_network_data->parameters);
		qos_network_data->old_param_count =
		    qos_network_data->param_count;
		memcpy(&qos_parameters[QOS_PARAM_SET_ACTIVE], active_one, size);
		active = qos_network_data->supported;
		//spin_unlock_irqrestore(&priv->ieee->lock, flags);

		if (active == 0) {
			burst_duration = ipw_qos_get_burst_duration(priv);
			for (i = 0; i < QOS_QUEUE_NUM; i++)
				qos_parameters[QOS_PARAM_SET_ACTIVE].
				    tx_op_limit[i] = (u16) burst_duration;
		}
	}

	IWI_DEBUG("QoS sending IPW_CMD_QOS_PARAMETERS\n");
	for (i = 0; i < 3; i++) {
		int j;
		for (j = 0; j < QOS_QUEUE_NUM; j++) {
			qos_parameters[i].cw_min[j] =
			    cpu_to_le16(qos_parameters[i].cw_min[j]);
			qos_parameters[i].cw_max[j] =
			    cpu_to_le16(qos_parameters[i].cw_max[j]);
			qos_parameters[i].tx_op_limit[j] =
			    cpu_to_le16(qos_parameters[i].tx_op_limit[j]);
		}
	}


	struct ieee80211_qos_parameters *qos_param=(struct ieee80211_qos_parameters *)
					  &(qos_parameters[0]);
	
	//err=sendCommand(IPW_CMD_QOS_PARAMETERS, &qos_param, sizeof(*qos_param) * 3, 1);			
	err=ipw_send_cmd_pdu(priv, IPW_CMD_QOS_PARAMETERS,sizeof(*qos_param) * 3, &qos_param);
	if (err)
		IWI_DEBUG("QoS IPW_CMD_QOS_PARAMETERS failed\n");

	return err;
}

void darwin_iwi2200::ipw_led_link_on(struct ipw_priv *priv)
{
	unsigned long flags;
	u32 led;

	/* If configured to not use LEDs, or nic_type is 1,
	 * then we don't toggle a LINK led */
	if (priv->config & CFG_NO_LED || priv->nic_type == EEPROM_NIC_TYPE_1)
		return;


	if (!(priv->status & STATUS_RF_KILL_MASK) &&
	    !(priv->status & STATUS_LED_LINK_ON)) {
		led = ipw_read_reg32( IPW_EVENT_REG);
		led |= priv->led_association_on;

		led = ipw_register_toggle(led);

		ipw_write_reg32( IPW_EVENT_REG, led);

		priv->status |= STATUS_LED_LINK_ON;

		/* If we aren't associated, schedule turning the LED off */
		if (!(priv->status & STATUS_ASSOCIATED))
		{
			IODelay(LD_TIME_LINK_ON*1000);
			ipw_led_link_off(priv);
		}
	}
}

void darwin_iwi2200::ipw_led_init(struct ipw_priv *priv)
{
	priv->nic_type = priv->eeprom[EEPROM_NIC_TYPE];

	/* Set the default PINs for the link and activity leds */
	priv->led_activity_on = IPW_ACTIVITY_LED;
	priv->led_activity_off = ~(IPW_ACTIVITY_LED);

	priv->led_association_on = IPW_ASSOCIATED_LED;
	priv->led_association_off = ~(IPW_ASSOCIATED_LED);

	/* Set the default PINs for the OFDM leds */
	priv->led_ofdm_on = IPW_OFDM_LED;
	priv->led_ofdm_off = ~(IPW_OFDM_LED);

	switch (priv->nic_type) {
	case EEPROM_NIC_TYPE_1:
		/* In this NIC type, the LEDs are reversed.... */
		priv->led_activity_on = IPW_ASSOCIATED_LED;
		priv->led_activity_off = ~(IPW_ASSOCIATED_LED);
		priv->led_association_on = IPW_ACTIVITY_LED;
		priv->led_association_off = ~(IPW_ACTIVITY_LED);

		if (!(priv->config & CFG_NO_LED))
			ipw_led_band_on(priv);

		/* And we don't blink link LEDs for this nic, so
		 * just return here */
		return;

	case EEPROM_NIC_TYPE_3:
	case EEPROM_NIC_TYPE_2:
	case EEPROM_NIC_TYPE_4:
	case EEPROM_NIC_TYPE_0:
		break;

	default:
		IWI_DEBUG("Unknown NIC type from EEPROM: %d\n",
			       priv->nic_type);
		priv->nic_type = EEPROM_NIC_TYPE_0;
		break;
	}

	if (!(priv->config & CFG_NO_LED)) {
		if (priv->status & STATUS_ASSOCIATED)
			ipw_led_link_on(priv);
		else
			ipw_led_link_off(priv);
	}
}


void darwin_iwi2200::ipw_led_band_on(struct ipw_priv *priv)
{
	unsigned long flags;
	u32 led;

	/* Only nic type 1 supports mode LEDs */
	if (priv->config & CFG_NO_LED ||
	    priv->nic_type != EEPROM_NIC_TYPE_1 || !priv->assoc_network)
		return;


	led = ipw_read_reg32(IPW_EVENT_REG);
	if (priv->assoc_network->mode == IEEE_A) {
		led |= priv->led_ofdm_on;
		led &= priv->led_association_off;
		IWI_DEBUG("Mode LED On: 802.11a\n");
	} else if (priv->assoc_network->mode == IEEE_G) {
		led |= priv->led_ofdm_on;
		led |= priv->led_association_on;
		IWI_DEBUG("Mode LED On: 802.11g\n");
	} else {
		led &= priv->led_ofdm_off;
		led |= priv->led_association_on;
		IWI_DEBUG("Mode LED On: 802.11b\n");
	}

	led = ipw_register_toggle(led);

	ipw_write_reg32( IPW_EVENT_REG, led);

}

int darwin_iwi2200::ipw_channel_to_index(struct ieee80211_device *ieee, u8 channel)
{
	int i;

	/* Driver needs to initialize the geography map before using
	 * these helper functions */
	if(ieee->geo.bg_channels == 0 && ieee->geo.a_channels == 0) return -1;

	if (ieee->freq_band & IEEE80211_24GHZ_BAND)
		for (i = 0; i < ieee->geo.bg_channels; i++)
			if (ieee->geo.bg[i].channel == channel)
				return i;

	if (ieee->freq_band & IEEE80211_52GHZ_BAND)
		for (i = 0; i < ieee->geo.a_channels; i++)
			if (ieee->geo.a[i].channel == channel)
				return i;

	return -1;
}

void darwin_iwi2200::ipw_add_scan_channels(struct ipw_priv *priv,
				  struct ipw_scan_request_ext *scan,
				  int scan_type)
{
	int channel_index = 0;
	const struct ieee80211_geo *geo;
	int i;

	geo = ipw_get_geo(priv->ieee);

	if (priv->ieee->freq_band & IEEE80211_52GHZ_BAND) {
		int start = channel_index;
		for (i = 0; i < geo->a_channels; i++) {
			if ((priv->status & STATUS_ASSOCIATED) &&
			    geo->a[i].channel == priv->channel)
				continue;
			channel_index++;
			scan->channels_list[channel_index] = geo->a[i].channel;
			ipw_set_scan_type(scan, channel_index,
					  geo->a[i].
					  flags & IEEE80211_CH_PASSIVE_ONLY ?
					  IPW_SCAN_PASSIVE_FULL_DWELL_SCAN :
					  scan_type);
		}

		if (start != channel_index) {
			scan->channels_list[start] = (u8) (IPW_A_MODE << 6) |
			    (channel_index - start);
			channel_index++;
		}
	}

	if (priv->ieee->freq_band & IEEE80211_24GHZ_BAND) {
		int start = channel_index;
		if (priv->config & CFG_SPEED_SCAN) {
			int index;
			u8 channels[IEEE80211_24GHZ_CHANNELS] = {};

			u8 channel;
			while (channel_index < IPW_SCAN_CHANNELS) {
				channel =
				    priv->speed_scan[priv->speed_scan_pos];
				if (channel == 0) {
					priv->speed_scan_pos = 0;
					channel = priv->speed_scan[0];
				}
				if ((priv->status & STATUS_ASSOCIATED) &&
				    channel == priv->channel) {
					priv->speed_scan_pos++;
					continue;
				}

				/* If this channel has already been
				 * added in scan, break from loop
				 * and this will be the first channel
				 * in the next scan.
				 */
				if (channels[channel - 1] != 0)
					break;

				channels[channel - 1] = 1;
				priv->speed_scan_pos++;
				channel_index++;
				scan->channels_list[channel_index] = channel;
				index =
				    ipw_channel_to_index(priv->ieee, channel);
				ipw_set_scan_type(scan, channel_index,
						  geo->bg[index].
						  flags &
						  IEEE80211_CH_PASSIVE_ONLY ?
						  IPW_SCAN_PASSIVE_FULL_DWELL_SCAN
						  : scan_type);
			}
		} else {
			for (i = 0; i < geo->bg_channels; i++) {
				if ((priv->status & STATUS_ASSOCIATED) &&
				    geo->bg[i].channel == priv->channel)
					continue;
				channel_index++;
				scan->channels_list[channel_index] =
				    geo->bg[i].channel;
				ipw_set_scan_type(scan, channel_index,
						  geo->bg[i].
						  flags &
						  IEEE80211_CH_PASSIVE_ONLY ?
						  IPW_SCAN_PASSIVE_FULL_DWELL_SCAN
						  : scan_type);
			}
		}

		if (start != channel_index) {
			scan->channels_list[start] = (u8) (IPW_B_MODE << 6) |
			    (channel_index - start);
		}
	}
}

int darwin_iwi2200::ipw_scan(struct ipw_priv *priv, int type)
{
	struct ipw_scan_request_ext scan;
	int err = 0, scan_type;
	
	if (!(priv->status & STATUS_INIT) ||
	    (priv->status & STATUS_EXIT_PENDING))
		return 0;


	if (priv->status & STATUS_SCANNING) {
		IWI_DEBUG("Concurrent scan requested.  Ignoring.\n");
		priv->status |= STATUS_SCAN_PENDING;
		goto done;
	}

	if (!(priv->status & STATUS_SCAN_FORCED) &&
	    priv->status & STATUS_SCAN_ABORTING) {
		IWI_DEBUG("Scan request while abort pending.  Queuing.\n");
		priv->status |= STATUS_SCAN_PENDING;
		goto done;
	}

	if (priv->status & STATUS_RF_KILL_MASK) {
		IWI_DEBUG("Aborting scan due to RF Kill activation\n");
		priv->status |= STATUS_SCAN_PENDING;
		goto done;
	}

	memset(&scan, 0, sizeof(scan));
	scan.full_scan_index = cpu_to_le32(ieee80211_get_scans(priv->ieee));

	if (type == IW_SCAN_TYPE_PASSIVE) {
		IWI_DEBUG("use passive scanning\n");
		scan_type = IPW_SCAN_PASSIVE_FULL_DWELL_SCAN;
		scan.dwell_time[IPW_SCAN_PASSIVE_FULL_DWELL_SCAN] =
		    cpu_to_le16(120);
		ipw_add_scan_channels(priv, &scan, scan_type);
		goto send_request;
	}

	if (priv->config & CFG_SPEED_SCAN)
		scan.dwell_time[IPW_SCAN_ACTIVE_BROADCAST_SCAN] =
		    cpu_to_le16(30);
	else
		scan.dwell_time[IPW_SCAN_ACTIVE_BROADCAST_SCAN] =
		    cpu_to_le16(20);

	scan.dwell_time[IPW_SCAN_ACTIVE_BROADCAST_AND_DIRECT_SCAN] =
	    cpu_to_le16(20);

	scan.dwell_time[IPW_SCAN_PASSIVE_FULL_DWELL_SCAN] = cpu_to_le16(120);

	if (priv->ieee->iw_mode == IW_MODE_MONITOR) {
		u8 channel;
		u8 band = 0;

		switch (ipw_is_valid_channel(priv->ieee, priv->channel)) {
		case IEEE80211_52GHZ_BAND:
			band = (u8) (IPW_A_MODE << 6) | 1;
			channel = priv->channel;
			break;

		case IEEE80211_24GHZ_BAND:
			band = (u8) (IPW_B_MODE << 6) | 1;
			channel = priv->channel;
			break;

		default:
			band = (u8) (IPW_B_MODE << 6) | 1;
			channel = 9;
			break;
		}

		scan.channels_list[0] = band;
		scan.channels_list[1] = channel;
		ipw_set_scan_type(&scan, 1, IPW_SCAN_PASSIVE_FULL_DWELL_SCAN);


		scan.dwell_time[IPW_SCAN_PASSIVE_FULL_DWELL_SCAN] =
		    cpu_to_le16(2000);
	} else {

		if ((priv->status & STATUS_ROAMING)
		    || (!(priv->status & STATUS_ASSOCIATED)
			&& (priv->config & CFG_STATIC_ESSID)
			&& (le32_to_cpu(scan.full_scan_index) % 2))) {
			err=ipw_send_cmd_pdu(priv, IPW_CMD_SSID, min(priv->essid_len, IW_ESSID_MAX_SIZE),&priv->essid);
			//err=sendCommand(IPW_CMD_SSID, &priv->essid,min( priv->essid_len, IW_ESSID_MAX_SIZE), 1);
			if (err) {
				IWI_DEBUG("Attempt to send SSID command "
					     "failed.\n");
				goto done;
			}

			scan_type = IPW_SCAN_ACTIVE_BROADCAST_AND_DIRECT_SCAN;
		} else
			scan_type = IPW_SCAN_ACTIVE_BROADCAST_SCAN;

		ipw_add_scan_channels(priv, &scan, scan_type);
	}

      send_request:
	  err=ipw_send_cmd_pdu(priv, IPW_CMD_SCAN_REQUEST_EXT, sizeof(scan), &scan);
	//err = sendCommand(IPW_CMD_SCAN_REQUEST_EXT, &scan,sizeof(scan), 1);

	if (err) {
		IWI_DEBUG("Sending scan command failed: %08X\n", err);
		goto done;
	}

	priv->status |= STATUS_SCANNING;
	priv->status &= ~STATUS_SCAN_PENDING;
	queue_te(4,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan_check),priv,5,true);

 
	  done:
	  

	return err;

}

void darwin_iwi2200::ipw_scan_check(ipw_priv *priv)
{
	if (priv->status & (STATUS_SCANNING | STATUS_SCAN_ABORTING)) {
		IWI_DEBUG("Scan completion resetting\n");
		queue_te(1,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adapter_restart),priv,NULL,true);
	}
}

int darwin_iwi2200::initCmdQueue()
{
	cmdq.count=IWI_CMD_RING_COUNT;
	cmdq.queued=0;
	cmdq.cur=cmdq.next=0;
	
	cmdq.memD = MemoryDmaAlloc(cmdq.count*IWI_CMD_DESC_SIZE, &cmdq.physaddr, &cmdq.desc);
	if(!cmdq.memD || !cmdq.physaddr || !cmdq.desc)
	{ 
		IWI_DEBUG("dma_mem_alloc failer (initCmdQueue)\n");
		return false;
	}

	return true;
}

int darwin_iwi2200::resetCmdQueue()
{
	cmdq.queued=0;
	cmdq.cur=0;
	cmdq.next=0;
	
	return 0;
}


int darwin_iwi2200::initRxQueue()
{
	struct iwi_rx_data *data;
	rxq.count=IWI_RX_RING_COUNT;
	rxq.cur=0;
	
	rxq.data = IONew(struct iwi_rx_data, IWI_RX_RING_COUNT);
	if(!rxq.data)
	{
		IWI_DEBUG("failed to allocate RX Queue data\n");
		return false;
	}
	memset(rxq.data, 0, rxq.count*sizeof(struct iwi_rx_data));
	
	for(int i=0; i<rxq.count; i++)
	{
		data=&rxq.data[i];

		dma_addr_t physAddr;
		UInt16 *virtAddr;

		data->memD = MemoryDmaAlloc(10000, &physAddr, &virtAddr);
		/*data->memD = IOBufferMemoryDescriptor::withOptions(
				kIOMemoryPhysicallyContiguous,
				IWI_RX_RING_COUNT, PAGE_SIZE);*/
		if(!data->memD)
		{
			IWI_DEBUG("failed to alloc rx mem\n");
			return false;
		}

		if(data->memD->prepare() != kIOReturnSuccess) {
			data->memD->release();
			return false;
		}
		
		//data->physaddr = data->memD->getPhysicalSegment(0, 0);
		data->physaddr = (dma_addr_t)physAddr;
		data->m_data = (void *)virtAddr;
		//data->m = allocatePacket(10000); //virtAddr;
		/*if(!data->m) {
			IWI_DEBUG("alloc failure\n");
			return false;
		}*/
		data->reg = IWI_CSR_RX_BASE + i * 4;
	}
    return true;
}


int darwin_iwi2200::resetRxQueue()
{
	rxq.cur=0;
	return 0;
}

int darwin_iwi2200::ipw_is_multicast_ether_addr(const u8 * addr)
{
	return (0x01 & addr[0]);
}

int darwin_iwi2200::is_network_packet(struct ipw_priv *priv,
			     struct ieee80211_hdr_4addr *header)
{
	/* Filter incoming packets to determine if they are targetted toward
	 * this network, discarding packets coming from ourselves */
	switch (priv->ieee->iw_mode) {
	case IW_MODE_ADHOC:	/* Header: Dest. | Source    | BSSID */
		/* packets from our adapter are dropped (echo) */
		if (!memcmp(header->addr2, priv->net_dev->dev_addr, ETH_ALEN))
			return 0;

		/* {broad,multi}cast packets to our BSSID go through */
		if (ipw_is_multicast_ether_addr(header->addr1))
			return !memcmp(header->addr3, priv->bssid, ETH_ALEN);

		/* packets to our adapter go through */
		return !memcmp(header->addr1, priv->net_dev->dev_addr,
			       ETH_ALEN);

	case IW_MODE_INFRA:	/* Header: Dest. | BSSID | Source */
		/* packets from our adapter are dropped (echo) */
		if (!memcmp(header->addr3, priv->mac_addr, ETH_ALEN)){
			IWI_DEBUG("packet from me\n"); 
			return 0;
		}

		/* {broad,multi}cast packets to our BSS go through */
		if (ipw_is_multicast_ether_addr(header->addr1)){
			return !memcmp(header->addr2, priv->bssid, ETH_ALEN);
		}
		/* packets to our adapter go through */
		if (memcmp(header->addr1, priv->net_dev->dev_addr, ETH_ALEN)) {
			IWI_DEBUG("who? dst "  MAC_FMT " self " MAC_FMT "\n",
				MAC_ARG(header->addr1),
				MAC_ARG(priv->net_dev->dev_addr));
		}
		return !memcmp(header->addr1, priv->mac_addr, ETH_ALEN);
	}

	return 1;
}

s16 darwin_iwi2200::exponential_average(s16 prev_avg, s16 val, u8 depth)
{
	return ((depth - 1) * prev_avg + val) / depth;
}

u8 darwin_iwi2200::ipw_add_station(struct ipw_priv *priv, u8 * bssid)
{
	struct ipw_station_entry entry;
	int i;

	for (i = 0; i < priv->num_stations; i++) {
		if (!memcmp(priv->stations[i], bssid, ETH_ALEN)) {
			/* Another node is active in network */
			priv->missed_adhoc_beacons = 0;
			if (!(priv->config & CFG_STATIC_CHANNEL))
				/* when other nodes drop out, we drop out */
				priv->config &= ~CFG_ADHOC_PERSIST;

			return i;
		}
	}

	if (i == MAX_STATIONS)
		return IPW_INVALID_STATION;

	IWI_DEBUG("Adding AdHoc station: " MAC_FMT "\n", MAC_ARG(bssid));

	entry.reserved = 0;
	entry.support_mode = 0;
	memcpy(entry.mac_addr, bssid, ETH_ALEN);
	memcpy(priv->stations[i], bssid, ETH_ALEN);
	ipw_write_direct(priv, IPW_STATION_TABLE_LOWER + i * sizeof(entry),
			 &entry, sizeof(entry));
	priv->num_stations++;

	return i;
}

void darwin_iwi2200::ipw_write_direct(struct ipw_priv *priv, u32 addr, void *buf,
			     int num)
{
		memcpy_toio((memBase + addr), buf, num);
}
	
void darwin_iwi2200::ipw_handle_mgmt_packet(struct ipw_priv *priv,
				   struct ipw_rx_mem_buffer *rxb,
				   struct ieee80211_rx_stats *stats)
{
	if (priv->status & STATUS_ASSOCIATED) return;
	
	mbuf_t skb = rxb->skb;
	struct ipw_rx_packet *pkt = (struct ipw_rx_packet *)(mbuf_data(skb));
	//mbuf_prepend(&rxb->skb,IPW_RX_FRAME_SIZE,MBUF_WAITOK);
	//mbuf_align_32(skb,IPW_RX_FRAME_SIZE);
	//struct ieee80211_hdr_4addr *header = (struct ieee80211_hdr_4addr *)(mbuf_datastart(skb));// + IPW_RX_FRAME_SIZE);
	//struct ieee80211_hdr_4addr *header =(struct ieee80211_hdr_4addr *)mbuf_pkthdr_header(skb);
	struct ieee80211_hdr_4addr *header =
		(struct ieee80211_hdr_4addr *)((UInt8*)mbuf_data(skb)+IPW_RX_FRAME_SIZE);
	
	ieee80211_rx_mgt(priv->ieee, header, stats);

	if (priv->ieee->iw_mode == IW_MODE_ADHOC &&
	    ((WLAN_FC_GET_STYPE(le16_to_cpu(header->frame_ctl)) ==
	      IEEE80211_STYPE_PROBE_RESP) ||
	     (WLAN_FC_GET_STYPE(le16_to_cpu(header->frame_ctl)) ==
	      IEEE80211_STYPE_BEACON))) {
		if (!memcmp(header->addr3, priv->bssid, ETH_ALEN)) ipw_add_station(priv, header->addr2);
	}

	if (priv->config & CFG_NET_STATS) {
		IWI_DEBUG("sending stat packet.\n");

		/* Set the size of the skb to the size of the full
		 * ipw header and 802.11 frame */
		skb_put(skb, le16_to_cpu(pkt->u.frame.length) +	IPW_RX_FRAME_SIZE);
		/* Advance past the ipw packet header to the 802.11 frame */
		skb_pull(skb, IPW_RX_FRAME_SIZE);

		/* Push the ieee80211_rx_stats before the 802.11 frame */
		memcpy(skb_push(skb, sizeof(*stats)), stats, sizeof(*stats));

		//skb->dev = priv->ieee->dev;

		/* Point raw at the ieee80211_stats */
		/*skb->mac.raw = skb->data;
		
		skb->pkt_type = PACKET_OTHERHOST;
		skb->protocol = __constant_htons(ETH_P_80211_STATS);
		memset(skb->cb, 0, sizeof(rxb->skb->cb));
		netif_rx(skb);*/
		//ifnet_input(fifnet, skb, (const struct ifnet_stat_increment_param*)stats);
		fNetif->inputPacket(skb,mbuf_len(skb),IONetworkInterface::kInputOptionQueuePacket);
		// fix me: fushInputQueue is called in interruptOccured with check?
		//fNetif->flushInputQueue();
		rxb->skb = NULL;
		
	}
}

void darwin_iwi2200::__ipw_led_activity_on(struct ipw_priv *priv)
{
	u32 led;

	if (priv->config & CFG_NO_LED)
		return;

	if (priv->status & STATUS_RF_KILL_MASK)
		return;

	if (!(priv->status & STATUS_LED_ACT_ON)) {
		led = ipw_read_reg32( IPW_EVENT_REG);
		led |= priv->led_activity_on;

		led = ipw_register_toggle(led);

		IPW_DEBUG_LED("Reg: 0x%08X\n", led);
		ipw_write_reg32( IPW_EVENT_REG, led);

		IWI_DEBUG("Activity LED On\n");

		priv->status |= STATUS_LED_ACT_ON;

		//cancel_delayed_work(&priv->led_act_off);
		//queue_delayed_work(priv->workqueue, &priv->led_act_off,
		//				   LD_TIME_ACT_ON);
		queue_te(10, 
			OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_activity_off),
			priv,.25,false);
	} else {
		/* Reschedule LED off for full time period */
		//cancel_delayed_work(&priv->led_act_off);
		//queue_delayed_work(priv->workqueue, &priv->led_act_off,		   LD_TIME_ACT_ON);
		queue_te(10,
			OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_activity_off),
			priv,.25,false);
	}
}

void darwin_iwi2200::ipw_rebuild_decrypted_skb(struct ipw_priv *priv,
				      mbuf_t skb)
{
	struct ieee80211_hdr *hdr;
	u16 fc;

	hdr = (struct ieee80211_hdr *)(mbuf_data(skb));
	fc = le16_to_cpu(hdr->frame_ctl);
	if (!(fc & IEEE80211_FCTL_PROTECTED))
		return;

	fc &= ~IEEE80211_FCTL_PROTECTED;
	hdr->frame_ctl = cpu_to_le16(fc);
	switch (priv->ieee->sec.level) {
	case SEC_LEVEL_3:
		IWI_DEBUG_FULL("ipw_rebuild_decrypted_skb level3");
		/* Remove CCMP HDR */
		memmove(((UInt8*)mbuf_data(skb) + IEEE80211_3ADDR_LEN),
			((UInt8*)mbuf_data(skb) + IEEE80211_3ADDR_LEN + 8),
			(size_t)((UInt8*)mbuf_data(skb) - IEEE80211_3ADDR_LEN - 8));
		//skb_trim(skb, skb->len - 16);	/* CCMP_HDR_LEN + CCMP_MIC_LEN */
		mbuf_adj(skb,mbuf_len(skb)-16);
		break;
	case SEC_LEVEL_2:
		IWI_DEBUG_FULL("ipw_rebuild_decrypted_skb level2");
		break;
	case SEC_LEVEL_1:
		/* Remove IV */
		IWI_DEBUG_FULL("ipw_rebuild_decrypted_skb level1");
		memmove(((UInt8*)mbuf_data(skb) + IEEE80211_3ADDR_LEN),
			((UInt8*)mbuf_data(skb) + IEEE80211_3ADDR_LEN + 4),
			(size_t)((UInt8*)mbuf_data(skb) - IEEE80211_3ADDR_LEN - 4));
		//skb_trim(skb, skb->len - 8);	/* IV + ICV */
		mbuf_adj(skb,mbuf_len(skb)-8);
		break;
	case SEC_LEVEL_0:
		IWI_DEBUG_FULL("ipw_rebuild_decrypted_skb level0");
		break;
	default:
		IWI_DEBUG_FULL( "Unknow security level %d\n",
		       priv->ieee->sec.level);
		break;
	}
}

bool darwin_iwi2200::ipw_handle_data_packet(struct ipw_priv *priv,
				   struct ipw_rx_mem_buffer *rxb,
				   struct ieee80211_rx_stats *stats)
{
	bool                doFlushQueue = false;
	
	struct ieee80211_hdr_4addr *hdr;
	struct ipw_rx_packet *pkt = (struct ipw_rx_packet *)(mbuf_data(rxb->skb));
#ifdef IW_RX_REPLACING	
	mbuf_t inputPkt;
	bool replaced;
#endif	
	
	/* We received data from the HW, so stop the watchdog */
	priv->net_dev->trans_start = jiffies;

	/* We only process data packets if the
	 * interface is open */
	 // if( unlikely((le16_to_cpu(pkt->u.frame.length) + IPW_RX_FRAME_SIZE) > skb_tailroom(rxb->skb))) {
	 // fix me: may change mbuf_len to mbuf_trailingspace.
	// IWI_DEBUG_FN(" mbuf_len %d mbuf_trailingspace  %d\n",mbuf_len(rxb->skb),mbuf_trailingspace(rxb->skb));
#if 0	
	if (unlikely((le16_to_cpu(pkt->u.frame.length) + IPW_RX_FRAME_SIZE) >
		     mbuf_len(rxb->skb))) {
          //		     mbuf_trailingspace(rxb->skb))) {
		priv->ieee->stats.rx_errors++;
		priv->wstats.discard.misc++;
		IWI_DEBUG("Corruption detected! Oh no!\n");
		return false;
	} //else if (unlikely(!netif_running(priv->net_dev))) {
		else if (!(ifnet_flags(fifnet) & IFF_RUNNING)) {
		priv->ieee->stats.rx_dropped++;
		priv->wstats.discard.misc++;
		IWI_DEBUG("Dropping packet while interface is not up.\n");
		return false;
	}
#else
	if (unlikely((le16_to_cpu(pkt->u.frame.length) + IPW_RX_FRAME_SIZE) >
		     skb_tailroom(rxb->skb))) {
		priv->ieee->stats.rx_errors++;
		priv->wstats.discard.misc++;
		IWI_DEBUG("Corruption detected! Oh no!\n");
		return false;
	} else if (!(ifnet_flags(fifnet) & IFF_RUNNING)) {
		priv->ieee->stats.rx_dropped++;
		priv->wstats.discard.misc++;
		IWI_DEBUG("Dropping packet while interface is not up.\n");
		return false;
	}
#endif	
	IWI_DEBUG("before dump rx packet size mbuf_len = %d,mbuf_pkthdr_len %d , rx frame size %d. cluster size %d"
		       " mbuf_next is %s\n", mbuf_len(rxb->skb),  
			                              mbuf_pkthdr_len(rxb->skb),
						le16_to_cpu(pkt->u.frame.length),
						mbuf_maxlen(rxb->skb),
		  		                    mbuf_next(rxb->skb) ? "exist"  : "nothing");
#if 1	
	
	/* Advance skb->data to the start of the actual payload */
	skb_reserve(rxb->skb, offsetof(struct ipw_rx_packet, u.frame.data));
	//mbuf_setlen(rxb->skb, offsetof(struct ipw_rx_packet, u.frame.data));
	/* Set the size of the skb to the size of the frame */
	skb_put(rxb->skb, le16_to_cpu(pkt->u.frame.length));
#else	
	

	if( mbuf_flags(rxb->skb) & MBUF_PKTHDR)
		mbuf_pkthdr_setlen(rxb->skb,le16_to_cpu(pkt->u.frame.length));
	
	mbuf_setdata(rxb->skb, 
	                      (UInt8*)mbuf_data(rxb->skb) + offsetof(struct ipw_rx_packet, u.frame.data),
			  pkt->u.frame.length);

	if( mbuf_flags(rxb->skb) & MBUF_PKTHDR)
			mbuf_pkthdr_setlen(rxb->skb,
				 pkt->u.frame.length);
#endif	
	IWI_DEBUG("dump rx packet size mbuf_len = %d,mbuf_pkthdr_len %d , rx frame size %d. cluster size %d "
		       " mbuf_next is %s\n", mbuf_len(rxb->skb),  
			                              mbuf_pkthdr_len(rxb->skb),
						le16_to_cpu(pkt->u.frame.length),
						mbuf_maxlen(rxb->skb),
		  		                    mbuf_next(rxb->skb) ? "exist"  : "nothing");
#ifdef IW_RX_REPLACING									
	inputPkt = replaceOrCopyPacket(&rxb->skb,	mbuf_len(rxb->skb),&replaced);
	if(inputPkt == 0){
		IWI_DEBUG("fail to replace or copy packet \n");
		priv->ieee->stats.rx_errors++;
		return doFlushQueue;
	}
#endif	
							
#if 0
	 mbuf_setlen(rxb->skb,le16_to_cpu(pkt->u.frame.length));
#endif
				
	//mbuf_setlen(rxb->skb,   mbuf_len(rxb->skb)+ le16_to_cpu(pkt->u.frame.length));
	
	//mbuf_adj(rxb->skb, offsetof(struct ipw_rx_packet, u.frame.data));
	//mbuf_setlen(rxb->skb, le16_to_cpu(pkt->u.frame.length));
	//if( mbuf_flags(rxb->skb) & MBUF_PKTHDR)
	//	mbuf_pkthdr_setlen(rxb->skb, le16_to_cpu(pkt->u.frame.length));

	/* HW decrypt will not clear the WEP bit, MIC, PN, etc. */
	/*hdr = (struct ieee80211_hdr_4addr *)mbuf_data(rxb->skb);
	if (priv->ieee->iw_mode != IW_MODE_MONITOR &&
	    (ipw_is_multicast_ether_addr(hdr->addr1) ?
	     !priv->ieee->host_mc_decrypt : !priv->ieee->host_decrypt))
		ipw_rebuild_decrypted_skb(priv, rxb->skb);*/
#ifdef IW_RX_REPLACING
	if (!ieee80211_rx(priv->ieee, inputPkt, stats))
#else
	if (!ieee80211_rx(priv->ieee, rxb->skb, stats))
#endif	
		priv->ieee->stats.rx_errors++;
	else {			/* ieee80211_rx succeeded, so it now owns the SKB */
		doFlushQueue = true;
		rxb->skb = NULL;
		__ipw_led_activity_on(priv);
	}
	return doFlushQueue;
}

int darwin_iwi2200::is_duplicate_packet(struct ipw_priv *priv,
			       struct ieee80211_hdr_4addr *header)
{
	u16 sc = le16_to_cpu(header->seq_ctl);
	u16 seq = WLAN_GET_SEQ_SEQ(sc);
	u16 frag = WLAN_GET_SEQ_FRAG(sc);
	u16 *last_seq, *last_frag;
	unsigned long *last_time;

	switch (priv->ieee->iw_mode) {
	case IW_MODE_ADHOC:
		{
			struct list_head *p;
			struct ipw_ibss_seq *entry = NULL;
			u8 *mac = header->addr2;
			int index = mac[5] % IPW_IBSS_MAC_HASH_SIZE;

			__list_for_each(p, &priv->ibss_mac_hash[index]) {
				entry =
				    list_entry(p, struct ipw_ibss_seq, list);
				if (!memcmp(entry->mac, mac, ETH_ALEN))
					break;
			}
			if (p == &priv->ibss_mac_hash[index]) {
				(void*)entry = kmalloc(sizeof(*entry), NULL);
				if (!entry) {
					IWI_DEBUG_FULL
					    ("Cannot malloc new mac entry\n");
					return 0;
				}
				memcpy(entry->mac, mac, ETH_ALEN);
				entry->seq_num = seq;
				entry->frag_num = frag;
				entry->packet_time = jiffies;
				list_add(&entry->list,
					 &priv->ibss_mac_hash[index]);
				return 0;
			}
			last_seq = &entry->seq_num;
			last_frag = &entry->frag_num;
			last_time = &entry->packet_time;
			break;
		}
	case IW_MODE_INFRA:
		last_seq = &priv->last_seq_num;
		last_frag = &priv->last_frag_num;
		last_time = &priv->last_packet_time;
		break;
	default:
		return 0;
	}
//#define	SEQ_LEQ(a,b)	((int)((a)-(b)) <= 0)
         if ( (*last_seq == seq)  && 
	     time_after(jiffies, *last_time + 5*IPW_PACKET_RETRY_TIME ) ) {
		if (*last_frag == frag)
			goto drop;
		if (*last_frag + 1 != frag)
			/* out-of-order fragment */
			goto drop;
	} else
		*last_seq = seq;

	*last_frag = frag;
	*last_time = jiffies;
	return 0;

      drop:
	/* Comment this line now since we observed the card receives
	 * duplicate packets but the FCTL_RETRY bit is not set in the
	 * IBSS mode with fragmentation enabled.
	 BUG_ON(!(le16_to_cpu(header->frame_ctl) & IEEE80211_FCTL_RETRY)); */
	if (  !(le16_to_cpu(header->frame_ctl) & IEEE80211_FCTL_RETRY) ) {
	    IWI_ERR("packet dropping and IEEE80211_FCTL_RETRY\n");
		*last_seq = seq;
		return 0;
	}else{
	    IWI_WARNING("Dropped packet\n");
	}
	return 1;
}

void darwin_iwi2200::ipw_rx(struct ipw_priv *priv)
{

	struct ipw_rx_mem_buffer *rxb;
	struct ipw_rx_packet *pkt;
	struct ieee80211_hdr_4addr *header;
	u32 r, w, i;
	u8 network_packet;
	bool doFlushQueue = false;

	r = ipw_read32( IPW_RX_READ_INDEX);
	w = ipw_read32( IPW_RX_WRITE_INDEX);
	i = (priv->rxq->processed + 1) % RX_QUEUE_SIZE;

	while (i != r) {
		rxb = priv->rxq->queue[i];
		if (unlikely(rxb == NULL)) {
			IWI_DEBUG( "Queue not allocated!\n");
			break;
		}
		priv->rxq->queue[i] = NULL;

		//pci_dma_sync_single_for_cpu(priv->pci_dev, rxb->dma_addr,IPW_RX_BUF_SIZE,PCI_DMA_FROMDEVICE);
		pkt = (struct ipw_rx_packet *)mbuf_data(rxb->skb);
		IWI_DEBUG_FULL("Packet: type=%02X seq=%02X bits=%02X\n",
			     pkt->header.message_type,
			     pkt->header.rx_seq_num, pkt->header.control_bits);

		switch (pkt->header.message_type) {
		case RX_FRAME_TYPE:	/* 802.11 frame */  {
				struct ieee80211_rx_stats stats;
				stats.rssi = pkt->u.frame.rssi_dbm - IPW_RSSI_TO_DBM;
				stats.signal =le16_to_cpu(pkt->u.frame.rssi_dbm) - IPW_RSSI_TO_DBM + 0x100;
				stats.noise = le16_to_cpu(pkt->u.frame.noise);
				stats.rate = pkt->u.frame.rate;
				stats.mac_time = jiffies;
				stats.received_channel =
					    pkt->u.frame.received_channel;
				stats.freq =
					    (pkt->u.frame.
					     control & (1 << 0)) ?
					    IEEE80211_24GHZ_BAND :
					    IEEE80211_52GHZ_BAND;
				stats.len = le16_to_cpu(pkt->u.frame.length);

				if (stats.rssi != 0)
					stats.mask |= IEEE80211_STATMASK_RSSI;
				if (stats.signal != 0)
					stats.mask |= IEEE80211_STATMASK_SIGNAL;
				if (stats.noise != 0)
					stats.mask |= IEEE80211_STATMASK_NOISE;
				if (stats.rate != 0)
					stats.mask |= IEEE80211_STATMASK_RATE;

				priv->rx_packets++;

				if (priv->ieee->iw_mode == IW_MODE_MONITOR) {

					//ipw_handle_data_packet_monitor(priv,rxb,&stats);
					doFlushQueue = ipw_handle_data_packet(priv, rxb,&stats);
					break;
				}
				//mbuf_align_32(rxb->skb,IPW_RX_FRAME_SIZE);
				//mbuf_prepend(&rxb->skb,IPW_RX_FRAME_SIZE,MBUF_WAITOK);
				//header =(struct ieee80211_hdr_4addr *)mbuf_pkthdr_header(rxb->skb);
				//header =(struct ieee80211_hdr_4addr *)(mbuf_datastart(rxb->skb) );//+  IPW_RX_FRAME_SIZE);
				header = (struct ieee80211_hdr_4addr *)((UInt8*)mbuf_data(rxb->skb)+IPW_RX_FRAME_SIZE);
				/* TODO: Check Ad-Hoc dest/source and make sure
				 * that we are actually parsing these packets
				 * correctly -- we should probably use the
				 * frame control of the packet and disregard
				 * the current iw_mode */

				network_packet =
				    is_network_packet(priv, header);
				if (network_packet && priv->assoc_network) {
					priv->assoc_network->stats.rssi =
					    stats.rssi;
					priv->exp_avg_rssi =
					    exponential_average(priv->
								exp_avg_rssi,
								stats.rssi,
								DEPTH_RSSI);
				}

				IWI_DEBUG_FULL("Frame: len=%u\n",
					     le16_to_cpu(pkt->u.frame.length));

				if (le16_to_cpu(pkt->u.frame.length) <
				    ieee80211_get_hdrlen(le16_to_cpu
							 (header->frame_ctl))) {
					IWI_DEBUG_FULL
					    ("Received packet is too small. "
					     "Dropping.\n");
					priv->ieee->stats.rx_errors++;
					priv->wstats.discard.misc++;
					break;
				}

				switch (WLAN_FC_GET_TYPE
					(le16_to_cpu(header->frame_ctl))) {

				case IEEE80211_FTYPE_MGMT:
					IWI_DEBUG_FULL("IEEE80211_FTYPE_MGMT\n");
					ipw_handle_mgmt_packet(priv, rxb,&stats);
					break;

				case IEEE80211_FTYPE_CTL:
					break;

				case IEEE80211_FTYPE_DATA:
					IWI_DEBUG_FULL("IEEE80211_FTYPE_DATA\n");
#if 1		
					// fix me checking routine is not correct?			
					if (unlikely( !network_packet ||is_duplicate_packet(priv, header)))
					{
						
						IWI_DEBUG("Dropping: dup?(%d) "
							       MAC_FMT ", "
							       MAC_FMT ", "
							       MAC_FMT "\n",
							       is_duplicate_packet(priv, header),
							       MAC_ARG(header->
								       addr1),
							       MAC_ARG(header->
								       addr2),
							       MAC_ARG(header->
								       addr3));
						break;
					}else {
						IWI_DEBUG("Recieve: bssid"
							       MAC_FMT ", src "
							       MAC_FMT ",  dest"
							       MAC_FMT "\n",
							       MAC_ARG(header->
								       addr1),
							       MAC_ARG(header->
								       addr2),
							       MAC_ARG(header->
								       addr3));
					}
#endif
					doFlushQueue = ipw_handle_data_packet(priv, rxb,&stats);

					break;
				}
				break;
			}

		case RX_HOST_NOTIFICATION_TYPE:{
				IWI_DEBUG_FULL
				    ("Notification: subtype=%02X flags=%02X size=%d\n",
				     pkt->u.notification.subtype,
				     pkt->u.notification.flags,
				     pkt->u.notification.size);
				notifIntr(priv, &pkt->u.notification);
				break;
			}

		default:
			IWI_DEBUG("Bad Rx packet of type %d\n",
				     pkt->header.message_type);
			break;
		}

		/* For now we just don't re-use anything.  We can tweak this
		 * later to try and re-use notification packets and SKBs that
		 * fail to Rx correctly */
		if (rxb->skb != NULL) {
			//dev_kfree_skb_any(rxb->skb);
			if (!(rxb->skb) == MBUF_TYPE_FREE) freePacket(rxb->skb);
			rxb->skb = NULL;
			//pci_unmap_single(priv->pci_dev, rxb->dma_addr, IPW_RX_BUF_SIZE, PCI_DMA_FROMDEVICE);
			rxb->dma_addr=NULL;
		}
		
		list_add_tail(&rxb->list, &priv->rxq->rx_used);

		i = (i + 1) % RX_QUEUE_SIZE;
	}
	// if called ipw_handle_data_packet and queued, flushInputQueue()
	if(doFlushQueue){
		IWI_DEBUG("flushing Input Queue\n");
		fNetif->flushInputQueue();
	}
	//releaseFreePackets();
	/* Backtrack one entry */
	priv->rxq->processed = (i ? i : RX_QUEUE_SIZE) - 1;

	ipw_rx_queue_restock(priv);
	
}


int darwin_iwi2200::initTxQueue()
{
	txq.count = IWI_TX_RING_COUNT;
	txq.queued = 0;
	txq.cur = 0;

	txq.memD = MemoryDmaAlloc(txq.count * IWI_TX_DESC_SIZE, &txq.physaddr, &txq.desc);
	txq.data = IONew(iwi_tx_data, txq.count);

	return true;
}

int darwin_iwi2200::resetTxQueue()
{
	rxq.cur=0;
	return 0;
}
void ipw_rx_queue_free(struct ipw_priv *priv, struct ipw_rx_queue *rxq)
{
	int i;

	if (!rxq)
		return;

	for (i = 0; i < RX_QUEUE_SIZE + RX_FREE_BUFFERS; i++) {
		if (rxq->pool[i].skb != NULL) {
			//pci_unmap_single(priv->pci_dev, rxq->pool[i].dma_addr,
			//		 IPW_RX_BUF_SIZE, PCI_DMA_FROMDEVICE);
			//dev_kfree_skb(rxq->pool[i].skb);
			mbuf_freem_list(rxq->pool[i].skb);
		}
	}

	kfree(rxq);
}

void darwin_iwi2200::free(void)
{

	IWI_DEBUG("TODO: Free\n");
	return;
	
	IWI_DEBUG("%s Freeing\n", getName());
	if (fInterruptSrc && fWorkLoop)
	        fWorkLoop->removeEventSource(fInterruptSrc);

	ipw_down(priv);

	int i;
	if (priv->rxq) {
		ipw_rx_queue_free(priv, priv->rxq);
		priv->rxq = NULL;
	}
	ipw_tx_queue_free(priv);
	
	// free ieee
	if (priv->ieee->networks){
		for (i = 0; i < MAX_NETWORK_COUNT; i++)
			if (priv->ieee->networks[i].ibss_dfs)
				kfree(priv->ieee->networks[i].ibss_dfs);
		kfree(priv->ieee->networks);
		priv->ieee->networks = NULL;
	}
	
	RELEASE(map);
	fPCIDevice->close(this);
	RELEASE(fInterruptSrc);
	RELEASE(fPCIDevice);
	RELEASE(map);
	IOFree(this,sizeof(this));
	super::free();
}

void darwin_iwi2200::stop(IOService *provider)
{
	IWI_DEBUG("%s stop\n", getName());
	/*CSR_WRITE_4(memBase, IWI_CSR_RST, IWI_RST_SOFT_RESET);

	if (fInterruptSrc && fWorkLoop)
	        fWorkLoop->removeEventSource(fInterruptSrc);

	fPCIDevice->close(this);
	RELEASE(fInterruptSrc);
	RELEASE(fPCIDevice);
	RELEASE(map);*/
		
	super::stop(provider);
}


/*const char * darwin_iwi2200::getNamePrefix() const
{
	return "wlan";
}*/

void inline
darwin_iwi2200::eeprom_write_reg(UInt32 data)
{
	OSWriteLittleInt32((void*)memBase, IPW_INDIRECT_ADDR, FW_MEM_REG_EEPROM_ACCESS);
	OSWriteLittleInt32((void*)memBase, IPW_INDIRECT_DATA, data);
	
	// Sleep for 1 uS to hold the data there
	IODelay(1);
}

/* EEPROM Chip Select */
void inline
darwin_iwi2200::eeprom_cs(bool sel)
{
	if (sel)	// Set the CS pin on the EEPROM
	{
		// clear everything out
		eeprom_write_reg(0);
		// set the chip select pin and keep it selected
		eeprom_write_reg(EEPROM_BIT_CS);
		// give the eeprom a cycle on the clock (SK) pin to register the enable
		eeprom_write_reg(EEPROM_BIT_CS | EEPROM_BIT_SK);
		// end the clock cycle, keeping CS selected
		eeprom_write_reg(EEPROM_BIT_CS);
	}
	else		// Clear the CS pin on the EEPROM
	{
		// Make sure CS is set
		eeprom_write_reg(EEPROM_BIT_CS);
		// Clear everything out
		eeprom_write_reg(0);
		// Give the EEPROM a clock
		eeprom_write_reg(EEPROM_BIT_SK);
	}
}

void inline
darwin_iwi2200::eeprom_write_bit(UInt8 bit)
{
	// short way of saying: if bit, then set DI line high, data = 0 otherwise.
	// Note that because of this implementation we can pass in any value > 0 and
	// it will be interpreted as a '1' bit. Simplifies some operations in other 
	// functions.
	UInt32 data = (bit ? EEPROM_BIT_DI:0);
	// write data with the chip enabled
	eeprom_write_reg(EEPROM_BIT_CS | data);
	// keep data, chip enabled, and give it a rising clock edge
	eeprom_write_reg(EEPROM_BIT_CS | data | EEPROM_BIT_SK);
}

void
darwin_iwi2200::eeprom_op(UInt8 op, UInt8 addr)
{
	int i;
	
	// enable the chip
	eeprom_cs(true);
	
	// write the command (all commands start with a '1' bit followed by 2 other bits)
	eeprom_write_bit(1);
	eeprom_write_bit(op & 2);
	eeprom_write_bit(op & 1);
	
	// write the 8-bit address
	for (i=7; i>=0; i--) {
		eeprom_write_bit(addr & (1 << i));
	}
}

UInt16
darwin_iwi2200::eeprom_read_UInt16(UInt8 addr)
{
	int i;
	u16 r = 0;

	/* Send READ Opcode */
	eeprom_op(EEPROM_CMD_READ, addr);

	/* Send dummy bit */
	eeprom_write_reg(EEPROM_BIT_CS);

	/* Read the byte off the eeprom one bit at a time */
	for (i = 0; i < 16; i++) {
		u32 data = 0;
		eeprom_write_reg(EEPROM_BIT_CS | EEPROM_BIT_SK);
		eeprom_write_reg(EEPROM_BIT_CS);
		data = ipw_read_reg32(FW_MEM_REG_EEPROM_ACCESS);
		r = (r << 1) | ((data & EEPROM_BIT_DO) ? 1 : 0);
	}

	/* Send another dummy bit */
	eeprom_write_reg(0);
	//eeprom_disable_cs();
	eeprom_cs(false);

	return r;
}

/*
 * Here we cache the EEPROM into memory for ease of use
 * FIXME: Can the EEPROM change behind our backs?
 */
void
darwin_iwi2200::cacheEEPROM(struct ipw_priv *priv)
{
	int i;
	u16 *eeprom = (u16 *) priv->eeprom;


	/* read entire contents of eeprom into private buffer */
	for (i = 0; i < 128; i++)
		eeprom[i] = le16_to_cpu(eeprom_read_UInt16( (u8) i));

	/*
	   If the data looks correct, then copy it to our private
	   copy.  Otherwise let the firmware know to perform the operation
	   on its own.
	 */
	if (priv->eeprom[EEPROM_VERSION] != 0) {
		IWI_DEBUG("Writing EEPROM data into SRAM\n");

		/* write the eeprom data to sram */
		for (i = 0; i < IPW_EEPROM_IMAGE_SIZE; i++)
			_ipw_write8(memBase, IPW_EEPROM_DATA + i, priv->eeprom[i]);

		/* Do not load eeprom data on fatal error or suspend */
		ipw_write32( IPW_EEPROM_LOAD_DISABLE, 0);
	} else {
		IWI_DEBUG("Enabling FW initializationg of SRAM\n");

		/* Load eeprom data on fatal error or suspend */
		ipw_write32(IPW_EEPROM_LOAD_DISABLE, 1);
	}	
}


UInt32
darwin_iwi2200::read_reg_UInt32(UInt32 reg)
{
	UInt32 value;
	
	OSWriteLittleInt32((void*)memBase, IPW_INDIRECT_ADDR, reg);
	value = OSReadLittleInt32((void*)memBase, IPW_INDIRECT_DATA);
	return value;
}

int
darwin_iwi2200::ipw_poll_bit(UInt32 reg, UInt32 mask, int timeout)
{
	int i = 0;

	do {
		if ((ipw_read32(reg) & mask) == mask)
			return i;
		mdelay(10);
		i += 10;
	} while (i < timeout);

	return -ETIME;
}


/******************************************************************************* 
 * Functions which MUST be implemented by any class which inherits
 * from IO80211Controller.
 ******************************************************************************/
SInt32
darwin_iwi2200::getSSID(IO80211Interface *interface,
						struct apple80211_ssid_data *sd)
{
	//size_t t=strtouni((u_int16_t *)sd->ssid_bytes, (const char *)priv->essid, (size_t)priv->essid_len,UTF_NO_NULL_TERM); 
	sd->ssid_len=priv->essid_len;
	memcpy(sd->ssid_bytes, priv->essid,priv->essid_len);
	IWI_LOG("getSSID %s\n",escape_essid((const char*)priv->essid, priv->essid_len));
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getCHANNEL(IO80211Interface *interface,
						  struct apple80211_channel_data *cd)
{
	cd->channel.flags=0;
	cd->channel.channel=priv->assoc_request.channel;
	IWI_LOG("getCHANNEL c:%d f:%d\n",cd->channel.channel,cd->channel.flags);
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getBSSID(IO80211Interface *interface,
						struct apple80211_bssid_data *bd)
{
	memcpy(bd->bssid.octet, priv->bssid, ETH_ALEN);
	IWI_LOG("getBSSID %02x:%02x:%02x:%02x:%02x:%02x\n",MAC_ARG(bd->bssid.octet));
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getCARD_CAPABILITIES(IO80211Interface *interface,
									  struct apple80211_capability_data *cd)
{
	IWI_LOG("getCARD_CAPABILITIES\n");
	UInt32 cp=0;
	cp|=APPLE80211_CAP_WEP;//			= 1,	// CAPABILITY: WEP available
	cp|=APPLE80211_CAP_TKIP;//			= 2,	// CAPABILITY: TKIP available
	cp|=APPLE80211_CAP_AES;	//		= 3,	// CAPABILITY: AES OCB avail
	cp|=APPLE80211_CAP_AES_CCM;//		= 4,	// CAPABILITY: AES CCM avail
	cp|=APPLE80211_CAP_CKIP;	//		= 5,	// CAPABILITY: CKIP available
	cp|=APPLE80211_CAP_IBSS;	//		= 6,	// CAPABILITY: IBSS available
	cp|=APPLE80211_CAP_PMGT;	//		= 7,	// CAPABILITY: Power mgmt
	cp|=APPLE80211_CAP_HOSTAP;	//	= 8,	// CAPABILITY: HOSTAP avail
	cp|=APPLE80211_CAP_TXPMGT;	//	= 9,	// CAPABILITY: tx power mgmt
	cp|=APPLE80211_CAP_SHSLOT;	//	= 10,	// CAPABILITY: short slottime
	cp|=APPLE80211_CAP_SHPREAMBLE;//	= 11,	// CAPABILITY: short preamble
	cp|=APPLE80211_CAP_MONITOR;		//= 12,	// CAPABILITY: monitor mode
	cp|=APPLE80211_CAP_TKIPMIC;//		= 13,	// CAPABILITY: TKIP MIC avail
	cp|=APPLE80211_CAP_WPA1;	//		= 14,	// CAPABILITY: WPA1 avail
	cp|=APPLE80211_CAP_WPA2;	//		= 15,	// CAPABILITY: WPA2 avail
	cp|=APPLE80211_CAP_WPA;		//	= 16,	// CAPABILITY: WPA1+WPA2 avail
	cp|=APPLE80211_CAP_BURST;	//	= 17,	// CAPABILITY: frame bursting
	cp|=APPLE80211_CAP_WME;		//	= 18,	// CAPABILITY: WME avail

	memcpy(&cd->capabilities, &cp, sizeof(cp));
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getSTATE(IO80211Interface *interface,
						  struct apple80211_state_data *sd)
{
	IWI_LOG("getSTATE %d\n",sd->state);
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getRSSI(IO80211Interface *interface,
					   struct apple80211_rssi_data *rd)
{
	IWI_LOG("getRSSI \n");
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getPOWER(IO80211Interface *interface,
						struct apple80211_power_data *pd)
{
	IWI_LOG("getPOWER %d, %d %d %d %d\n",pd->num_radios, pd->power_state[0],pd->power_state[1],pd->power_state[2],pd->power_state[3]);

	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getSCAN_RESULT(IO80211Interface *interface,
							  struct apple80211_scan_result **scan_result)
{
	IWI_LOG("getSCAN_RESULT \n");
	return kIOReturnSuccess;
}

/*SInt32
darwin_iwi2200::getASSOCIATE_RESULT(IO80211Interface *interface,
								   struct apple80211_assoc_result_data *ard)
{
	IWI_DEBUG("getASSOCIATE_RESULT \n");
	return 0;
}*/

SInt32
darwin_iwi2200::getRATE(IO80211Interface *interface,
					   struct apple80211_rate_data *rd)
{
	rd->num_radios=(u_int32_t)priv->rates.num_rates;
	memcpy(rd->rate,priv->rates.supported_rates,sizeof(priv->rates.supported_rates));
	IWI_LOG("getRATE %d\n",rd->rate);
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::getSTATUS_DEV(IO80211Interface *interface,
							 struct apple80211_status_dev_data *dd)
{
	char i[4];
	if (fNetif == interface) {
	    IWI_DEBUG("same interface\n");
	}else {
	    IWI_DEBUG("not same interface\n");
	    return -1;
	}
	
	sprintf(i,"%s%d" ,interface->getNamePrefix(), interface->getUnitNumber());
	
	IWI_LOG("getSTATUS_DEV %s\n",dd->dev_name);
	
	ifnet_find_by_name(i,&fifnet);
		
	IWI_DEBUG("ifnet_t %s%d = %x\n",ifnet_name(fifnet),ifnet_unit(fifnet),fifnet);
	ipw_sw_reset(1);
	memcpy(&priv->ieee->dev->name,i,sizeof(i));

	super::enable(fNetif);
	interface->setPoweredOnByUser(true);
	//interface->setEnabledBySystem(true);
	//ipw_up(priv);

	return -1;
}

SInt32
darwin_iwi2200::getRATE_SET(IO80211Interface	*interface,
						   struct apple80211_rate_set_data *rd)
{
	rd->num_rates=(u_int32_t)priv->rates.num_rates;
	for (u_int32_t i=0;i<rd->num_rates;i++)
	{
		rd->rates[i].flags=0;
		rd->rates[i].rate=priv->rates.supported_rates[i];
		//memcpy(&rd->rates[i].rate,&priv->rates.supported_rates[i],sizeof(&priv->rates.supported_rates[i]));
	}
	//memcpy(rd->rates,priv->rates.supported_rates,sizeof(priv->rates.supported_rates));
	IWI_LOG("getRATE_SET %d r:%d\n",rd->num_rates, rd->rates);
	return kIOReturnSuccess;
}

SInt32	darwin_iwi2200::getASSOCIATION_STATUS( IO80211Interface * interface, struct apple80211_assoc_status_data * asd )
{
	IWI_LOG("getASSOCIATION_STATUS %d\n",asd->status);
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::setSCAN_REQ(IO80211Interface *interface,
						   struct apple80211_scan_data *sd)
{
	IWI_LOG("setSCAN_REQ \n");
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::setASSOCIATE(IO80211Interface *interface,
							struct apple80211_assoc_data *ad)
{
	IWI_LOG("setASSOCIATE \n");
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::setPOWER(IO80211Interface *interface,
						struct apple80211_power_data *pd)
{
	IWI_LOG("setPOWER %d, %d %d %d %d\n",pd->num_radios, pd->power_state[0],pd->power_state[1],pd->power_state[2],pd->power_state[3]);
	//if (pd->power_state[pd->num_radios]==1)
	if (power_state)
	{
		IWI_LOG("power on\n");
		power_state=0;
		queue_te(3,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rf_kill),priv,NULL,true);
		if (rf_kill_active(priv)) 
			{	
				if (BITC(ipw_read32(0x30),0) & 0x1) ipw_write32(0x30, 0x0);
				else
				{
					UInt32 r1=0;
					priv->status &= ~STATUS_SCANNING;
					while (!((priv->status & STATUS_SCANNING)))
					{
						if (!(BITC(ipw_read32(0x30),0) & 0x1)) ipw_write32(0x30, 0x1);
						IODelay(2000*1000);
						r1++;
						if (r1==5) return -1;
					}
				}
			}
		priv->status &= ~STATUS_RF_KILL_HW;
		priv->status &= ~STATUS_RF_KILL_SW;
		priv->status &= ~(STATUS_ASSOCIATED | STATUS_ASSOCIATING);
	}
	else
	{
		IWI_LOG("power off\n");
		power_state=1;
		if (!(rf_kill_active(priv))) 
			{
				if (BITC(ipw_read32(0x30),0) & 0x1) ipw_write32(0x30, 0x0);
				else
				ipw_write32(0x30, 0x1);
			}
		priv->status |= STATUS_RF_KILL_HW;
		priv->status &= ~STATUS_RF_KILL_SW;
		priv->status &= ~(STATUS_ASSOCIATED | STATUS_ASSOCIATING);
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		if ((ifnet_flags(fifnet) & IFF_RUNNING)) ipw_link_down(priv); else ipw_led_link_off(priv);
		queue_te(3,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_rf_kill),priv,NULL,true);
	}
	return -1;
}

SInt32
darwin_iwi2200::setCIPHER_KEY(IO80211Interface *interface,
							 struct apple80211_key *key)
{
	IWI_LOG("setCIPHER_KEY \n");
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::setAUTH_TYPE(IO80211Interface *interface,
							struct apple80211_authtype_data *ad)
{
	IWI_LOG("setAUTH_TYPE \n");
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::setDISASSOCIATE(IO80211Interface	*interface)
{
	IWI_LOG("setDISASSOCIATE \n");
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::setSSID(IO80211Interface *interface,
					   struct apple80211_ssid_data *sd)
{
	IWI_LOG("setSSID \n");
	return kIOReturnSuccess;
}

SInt32
darwin_iwi2200::setAP_MODE(IO80211Interface *interface,
						  struct apple80211_apmode_data *ad)
{
	IWI_LOG("setAP_MODE \n");
	return kIOReturnSuccess;
}

bool darwin_iwi2200::attachInterfaceWithMacAddress( void * macAddr, 
												UInt32 macLen, 
												IONetworkInterface ** interface, 
												bool doRegister ,
												UInt32 timeout  )
{
	IWI_DEBUG("attachInterfaceWithMacAddress \n");
	return super::attachInterfaceWithMacAddress(macAddr,macLen,interface,doRegister,timeout);
}												
												
void darwin_iwi2200::dataLinkLayerAttachComplete( IO80211Interface * interface )											
{
	IWI_DEBUG("dataLinkLayerAttachComplete \n");
	super::dataLinkLayerAttachComplete(interface);
}


void darwin_iwi2200::queue_te(int num, thread_call_func_t func, thread_call_param_t par, UInt32 timei, bool start)
{
	if (tlink[num]) queue_td(num,NULL);
	//IWI_DEBUG("queue_te0 %d\n",tlink[num]);
	tlink[num]=thread_call_allocate(func,this);
	//IWI_DEBUG("queue_te1 %d\n",tlink[num]);
	uint64_t timei2;
	if (timei) clock_interval_to_deadline(timei,kSecondScale,&timei2);
	//IWI_DEBUG("queue_te time %d %d\n",timei,timei2);
	int r;
	if (start==true && tlink[num])
	{
		if (!par && !timei)	r=thread_call_enter(tlink[num]);
		if (!par && timei)	r=thread_call_enter_delayed(tlink[num],timei2);
		if (par && !timei)	r=thread_call_enter1(tlink[num],par);
		if (par && timei)	r=thread_call_enter1_delayed(tlink[num],par,timei2);
	}
	//IWI_DEBUG("queue_te result %d\n",r);
}

void darwin_iwi2200::queue_td(int num , thread_call_func_t func)
{
	//IWI_DEBUG("queue_td0 %d\n",tlink[num]);
	int r=1,r1;
	//IWI_DEBUG("queue_td0 %d\n",tlink[num]);
	if (tlink[num])
	{ 
		//rep1:
		r=thread_call_cancel(tlink[num]);
		//if (r!=0) goto rep1;
		//rep2:
		r1=thread_call_free(tlink[num]);
		//if (r!=1) goto rep2;
		tlink[num]=NULL;
	}
	//IWI_DEBUG("queue_td1-%d , %d %d\n",num,r,r1);
}

IOReturn darwin_iwi2200::message( UInt32 type, IOService * provider,
                              void * argument)
{
	IWI_DEBUG("message type %8x argument %8x\n",type,argument);
	return super::message(type,provider,argument);

}

int darwin_iwi2200::ipw_is_valid_channel(struct ieee80211_device *ieee, u8 channel)
{
	int i;

	/* Driver needs to initialize the geography map before using
	 * these helper functions */
	if (ieee->geo.bg_channels == 0 && ieee->geo.a_channels == 0) return -1;

	if (ieee->freq_band & IEEE80211_24GHZ_BAND)
		for (i = 0; i < ieee->geo.bg_channels; i++)
			/* NOTE: If G mode is currently supported but
			 * this is a B only channel, we don't see it
			 * as valid. */
			if ((ieee->geo.bg[i].channel == channel) &&
			    (!(ieee->mode & IEEE_G) ||
			     !(ieee->geo.bg[i].flags & IEEE80211_CH_B_ONLY)))
				return IEEE80211_24GHZ_BAND;

	if (ieee->freq_band & IEEE80211_52GHZ_BAND)
		for (i = 0; i < ieee->geo.a_channels; i++)
			if (ieee->geo.a[i].channel == channel)
				return IEEE80211_52GHZ_BAND;

	return 0;
}

void darwin_iwi2200::ipw_create_bssid(struct ipw_priv *priv, u8 * bssid)
{
	/* First 3 bytes are manufacturer */
	bssid[0] = priv->mac_addr[0];
	bssid[1] = priv->mac_addr[1];
	bssid[2] = priv->mac_addr[2];

	/* Last bytes are random */
	//get_random_bytes(&bssid[3], ETH_ALEN - 3);
	bssid[3]=(u8)random();
	bssid[4]=(u8)random();
	bssid[5]=(u8)random();

	bssid[0] &= 0xfe;	/* clear multicast bit */
	bssid[0] |= 0x02;	/* set local assignment bit (IEEE802) */
}

void darwin_iwi2200::ipw_adhoc_create(struct ipw_priv *priv,
			     struct ieee80211_network *network)
{
	const struct ieee80211_geo *geo = &priv->ieee->geo;
	int i;

	/*
	 * For the purposes of scanning, we can set our wireless mode
	 * to trigger scans across combinations of bands, but when it
	 * comes to creating a new ad-hoc network, we have tell the FW
	 * exactly which band to use.
	 *
	 * We also have the possibility of an invalid channel for the
	 * chossen band.  Attempting to create a new ad-hoc network
	 * with an invalid channel for wireless mode will trigger a
	 * FW fatal error.
	 *
	 */
	switch (ipw_is_valid_channel(priv->ieee, priv->channel)) {
	case IEEE80211_52GHZ_BAND:
		network->mode = IEEE_A;
		i = ipw_channel_to_index(priv->ieee, priv->channel);
		if (i == -1) return;
		if (geo->a[i].flags & IEEE80211_CH_PASSIVE_ONLY) {
			IWI_DEBUG("Overriding invalid channel\n");
			priv->channel = geo->a[0].channel;
		}
		break;

	case IEEE80211_24GHZ_BAND:
		if (priv->ieee->mode & IEEE_G)
			network->mode = IEEE_G;
		else
			network->mode = IEEE_B;
		i = ipw_channel_to_index(priv->ieee, priv->channel);
		if (i == -1) return;
		if (geo->bg[i].flags & IEEE80211_CH_PASSIVE_ONLY) {
			IWI_DEBUG("Overriding invalid channel\n");
			priv->channel = geo->bg[0].channel;
		}
		break;

	default:
		IWI_DEBUG("Overriding invalid channel\n");
		if (priv->ieee->mode & IEEE_A) {
			network->mode = IEEE_A;
			priv->channel = geo->a[0].channel;
		} else if (priv->ieee->mode & IEEE_G) {
			network->mode = IEEE_G;
			priv->channel = geo->bg[0].channel;
		} else {
			network->mode = IEEE_B;
			priv->channel = geo->bg[0].channel;
		}
		break;
	}

	network->channel = priv->channel;
	priv->config |= CFG_ADHOC_PERSIST;
	ipw_create_bssid(priv, network->bssid);
	network->ssid_len = priv->essid_len;
	memcpy(network->ssid, priv->essid, priv->essid_len);
	memset(&network->stats, 0, sizeof(network->stats));
	network->capability = WLAN_CAPABILITY_IBSS;
	if (!(priv->config & CFG_PREAMBLE_LONG))
		network->capability |= WLAN_CAPABILITY_SHORT_PREAMBLE;
	if (priv->capability & CAP_PRIVACY_ON)
		network->capability |= WLAN_CAPABILITY_PRIVACY;
	network->rates_len = min(priv->rates.num_rates, MAX_RATES_LENGTH);
	memcpy(network->rates, priv->rates.supported_rates, network->rates_len);
	network->rates_ex_len = priv->rates.num_rates - network->rates_len;
	memcpy(network->rates_ex,
	       &priv->rates.supported_rates[network->rates_len],
	       network->rates_ex_len);
	network->last_scanned = 0;
	network->flags = 0;
	network->last_associate = 0;
	network->time_stamp[0] = 0;
	network->time_stamp[1] = 0;
	network->beacon_interval = 100;	/* Default */
	network->listen_interval = 10;	/* Default */
	network->atim_window = 0;	/* Default */
	network->wpa_ie_len = 0;
	network->rsn_ie_len = 0;
}

int darwin_iwi2200::ipw_is_rate_in_mask(struct ipw_priv *priv, int ieee_mode, u8 rate)
{
	rate &= ~IEEE80211_BASIC_RATE_MASK;
	if (ieee_mode == IEEE_A) {
		switch (rate) {
		case IEEE80211_OFDM_RATE_6MB:
			return priv->rates_mask & IEEE80211_OFDM_RATE_6MB_MASK ?
			    1 : 0;
		case IEEE80211_OFDM_RATE_9MB:
			return priv->rates_mask & IEEE80211_OFDM_RATE_9MB_MASK ?
			    1 : 0;
		case IEEE80211_OFDM_RATE_12MB:
			return priv->
			    rates_mask & IEEE80211_OFDM_RATE_12MB_MASK ? 1 : 0;
		case IEEE80211_OFDM_RATE_18MB:
			return priv->
			    rates_mask & IEEE80211_OFDM_RATE_18MB_MASK ? 1 : 0;
		case IEEE80211_OFDM_RATE_24MB:
			return priv->
			    rates_mask & IEEE80211_OFDM_RATE_24MB_MASK ? 1 : 0;
		case IEEE80211_OFDM_RATE_36MB:
			return priv->
			    rates_mask & IEEE80211_OFDM_RATE_36MB_MASK ? 1 : 0;
		case IEEE80211_OFDM_RATE_48MB:
			return priv->
			    rates_mask & IEEE80211_OFDM_RATE_48MB_MASK ? 1 : 0;
		case IEEE80211_OFDM_RATE_54MB:
			return priv->
			    rates_mask & IEEE80211_OFDM_RATE_54MB_MASK ? 1 : 0;
		default:
			return 0;
		}
	}

	/* B and G mixed */
	switch (rate) {
	case IEEE80211_CCK_RATE_1MB:
		return priv->rates_mask & IEEE80211_CCK_RATE_1MB_MASK ? 1 : 0;
	case IEEE80211_CCK_RATE_2MB:
		return priv->rates_mask & IEEE80211_CCK_RATE_2MB_MASK ? 1 : 0;
	case IEEE80211_CCK_RATE_5MB:
		return priv->rates_mask & IEEE80211_CCK_RATE_5MB_MASK ? 1 : 0;
	case IEEE80211_CCK_RATE_11MB:
		return priv->rates_mask & IEEE80211_CCK_RATE_11MB_MASK ? 1 : 0;
	}

	/* If we are limited to B modulations, bail at this point */
	if (ieee_mode == IEEE_B)
		return 0;

	/* G */
	switch (rate) {
	case IEEE80211_OFDM_RATE_6MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_6MB_MASK ? 1 : 0;
	case IEEE80211_OFDM_RATE_9MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_9MB_MASK ? 1 : 0;
	case IEEE80211_OFDM_RATE_12MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_12MB_MASK ? 1 : 0;
	case IEEE80211_OFDM_RATE_18MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_18MB_MASK ? 1 : 0;
	case IEEE80211_OFDM_RATE_24MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_24MB_MASK ? 1 : 0;
	case IEEE80211_OFDM_RATE_36MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_36MB_MASK ? 1 : 0;
	case IEEE80211_OFDM_RATE_48MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_48MB_MASK ? 1 : 0;
	case IEEE80211_OFDM_RATE_54MB:
		return priv->rates_mask & IEEE80211_OFDM_RATE_54MB_MASK ? 1 : 0;
	}

	return 0;
}

int darwin_iwi2200::ipw_compatible_rates(struct ipw_priv *priv,
				const struct ieee80211_network *network,
				struct ipw_supported_rates *rates)
{
	int num_rates, i;

	memset(rates, 0, sizeof(*rates));
	num_rates = min(network->rates_len, (u8) IPW_MAX_RATES);
	rates->num_rates = 0;
	for (i = 0; i < num_rates; i++) {
		if (!ipw_is_rate_in_mask(priv, network->mode,
					 network->rates[i])) {

			if (network->rates[i] & IEEE80211_BASIC_RATE_MASK) {
				IWI_DEBUG("Adding masked mandatory "
					       "rate %02X\n",
					       network->rates[i]);
				rates->supported_rates[rates->num_rates++] =
				    network->rates[i];
				continue;
			}

			IWI_DEBUG("Rate %02X masked : 0x%08X\n",
				       network->rates[i], priv->rates_mask);
			continue;
		}

		rates->supported_rates[rates->num_rates++] = network->rates[i];
	}

	num_rates = min(network->rates_ex_len,
			(u8) (IPW_MAX_RATES - num_rates));
	for (i = 0; i < num_rates; i++) {
		if (!ipw_is_rate_in_mask(priv, network->mode,
					 network->rates_ex[i])) {
			if (network->rates_ex[i] & IEEE80211_BASIC_RATE_MASK) {
				IWI_DEBUG("Adding masked mandatory "
					       "rate %02X\n",
					       network->rates_ex[i]);
				rates->supported_rates[rates->num_rates++] =
				    network->rates[i];
				continue;
			}

			IWI_DEBUG("Rate %02X masked : 0x%08X\n",
				       network->rates_ex[i], priv->rates_mask);
			continue;
		}

		rates->supported_rates[rates->num_rates++] =
		    network->rates_ex[i];
	}

	return 1;
}

void darwin_iwi2200::ipw_copy_rates(struct ipw_supported_rates *dest,
			   const struct ipw_supported_rates *src)
{
	u8 i;
	for (i = 0; i < src->num_rates; i++)
		dest->supported_rates[i] = src->supported_rates[i];
	dest->num_rates = src->num_rates;
}

int darwin_iwi2200::ipw_best_network(struct ipw_priv *priv,
			    struct ipw_network_match *match,
			    struct ieee80211_network *network, int roaming)
{
	struct ipw_supported_rates rates;
	IWI_DEBUG("ipw_best_network\n");
	
	/* dump information */
	IWI_DEBUG("iw_mode[%d] capability[%d] flag[%d] scan_age[%d]\n",priv->ieee->iw_mode,
	  network->capability,network->flags,priv->ieee->scan_age);
	IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' \n",
	  escape_essid((const char*)network->ssid, network->ssid_len),
	  MAC_ARG(network->bssid));


	//check if the network should be excluded
	if (priv->ieee->iw_mode == IW_MODE_INFRA)
	if (network->bssid)
	{
		for (int i=0;i<20;i++) 
		{
			if (nonets[i].bssid)
			if (!memcmp(nonets[i].bssid, network->bssid, ETH_ALEN)) 
			{
				IWI_LOG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' in exclude list. "
				"restart card to include.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
				//return 0;
			}
		}
	}

	/* Verify that this network's capability is compatible with the
	 * current mode (AdHoc or Infrastructure) */

	 //BUG: kernel panic - the driver attach to a bss network when p_mode=0 !!
	if ((priv->ieee->iw_mode == IW_MODE_INFRA &&
	     !(network->capability & WLAN_CAPABILITY_ESS)) ||
	    (priv->ieee->iw_mode == IW_MODE_ADHOC &&
	     !(network->capability & WLAN_CAPABILITY_IBSS))) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded due to "
				"capability mismatch.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	/* If we do not have an ESSID for this AP, we can not associate with
	 * it */
	if (network->flags & NETWORK_EMPTY_ESSID) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of hidden ESSID.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	if (unlikely(roaming)) {
		/* If we are roaming, then ensure check if this is a valid
		 * network to try and roam to */
		if ((network->ssid_len != match->network->ssid_len) ||
		    memcmp(network->ssid, match->network->ssid,
			   network->ssid_len)) {
			IWI_DEBUG("Netowrk '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
					"because of non-network ESSID.\n",
					escape_essid((const char*)network->ssid,
						     network->ssid_len),
					MAC_ARG(network->bssid));
			return 0;
		}
	} else {
		/* If an ESSID has been configured then compare the broadcast
		 * ESSID to ours */
		if ((priv->config & CFG_STATIC_ESSID) &&
		    ((network->ssid_len != priv->essid_len) ||
		     memcmp(network->ssid, priv->essid,
			    min(network->ssid_len, priv->essid_len)))) {
			char escaped[IW_ESSID_MAX_SIZE * 2 + 1];
			strncpy(escaped,
				escape_essid((const char*)network->ssid, network->ssid_len),
				sizeof(escaped));
			IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
					"because of ESSID mismatch: '%s'.\n",
					escaped, MAC_ARG(network->bssid),
					escape_essid((const char*)priv->essid,
						     priv->essid_len));
			return 0;
		}
	}

	/* If the old network rate is better than this one, don't bother
	 * testing everything else. */
	if (match->network && match->network->stats.rssi > network->stats.rssi) {
		char escaped[IW_ESSID_MAX_SIZE * 2 + 1];
		strncpy(escaped,
			escape_essid((const char*)network->ssid, network->ssid_len),
			sizeof(escaped));
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded because "
				"'%s (%02x:%02x:%02x:%02x:%02x:%02x)' has a stronger signal.\n",
				escaped, MAC_ARG(network->bssid),
				escape_essid((const char*)match->network->ssid,
					     match->network->ssid_len),
				MAC_ARG(match->network->bssid));
		return 0;
	}

	/* If this network has already had an association attempt within the
	 * last 3 seconds, do not try and associate again... */
	if (network->last_associate &&
	    time_after(network->last_associate + (HZ * 3UL), jiffies)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of storming (%ums since last "
				"assoc attempt).\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid),
				jiffies_to_msecs(jiffies -
						 network->last_associate));
		return 0;
	}

	/* Now go through and see if the requested network is valid... */
	if (priv->ieee->scan_age != 0 &&
	    time_after(jiffies, network->last_scanned + priv->ieee->scan_age)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of age: %ums.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid),
				jiffies_to_msecs(jiffies -
						 network->last_scanned));
		return 0;
	}

	if ((priv->config & CFG_STATIC_CHANNEL) &&
	    (network->channel != priv->channel)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of channel mismatch: %d != %d.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid),
				network->channel, priv->channel);
		return 0;
	}

	/* Verify privacy compatability */
	if (((priv->capability & CAP_PRIVACY_ON) ? 1 : 0) !=
	    ((network->capability & WLAN_CAPABILITY_PRIVACY) ? 1 : 0)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of privacy mismatch: %s != %s.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid),
				priv->capability & CAP_PRIVACY_ON ? "on" :
				"off",
				network->capability &
				WLAN_CAPABILITY_PRIVACY ? "on" : "off");
		return 0;
	}

	if ((priv->config & CFG_STATIC_BSSID) &&
	    memcmp(network->bssid, priv->bssid, ETH_ALEN)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of BSSID mismatch: %02x:%02x:%02x:%02x:%02x:%02x.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid), MAC_ARG(priv->bssid));
		return 0;
	}

	/* Filter out any incompatible freq / mode combinations */
	if (!ieee80211_is_valid_mode(priv->ieee, network->mode)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of invalid frequency/mode "
				"combination.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	/* Filter out invalid channel in current GEO */
	// if ignored the association can be done
	// we should be a list of excluded networks and allow the user to choose the desired network
	if (!ipw_is_valid_channel(priv->ieee, network->channel)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of invalid channel in current GEO\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		//return 0;
	}

	/* Ensure that the rates supported by the driver are compatible with
	 * this AP, including verification of basic rates (mandatory) */
	if (!ipw_compatible_rates(priv, network, &rates)) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because configured rate mask excludes "
				"AP mandatory rate.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	if (rates.num_rates == 0) {
		IWI_DEBUG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' excluded "
				"because of no compatible rates.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	/* TODO: Perform any further minimal comparititive tests.  We do not
	 * want to put too much policy logic here; intelligent scan selection
	 * should occur within a generic IEEE 802.11 user space tool.  */

	/* Set up 'new' AP to this network */
	ipw_copy_rates(&match->rates, &rates);
	match->network = network;

	IWI_LOG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' is a viable match.\n",
			escape_essid((const char*)network->ssid, network->ssid_len),
			MAC_ARG(network->bssid));

	return 1;
}

int darwin_iwi2200::ipw_associate(ipw_priv *data)
{
	struct ipw_priv *priv = data;
	struct ieee80211_network *network = NULL;
	struct ipw_network_match match = {NULL};
	struct ipw_supported_rates *rates;
	struct list_head *element;
	unsigned long flags;
	
	IWI_DEBUG("associate...\n");
	if (priv->ieee->iw_mode == IW_MODE_MONITOR) {
		IWI_DEBUG("Not attempting association (monitor mode)\n");
		return 0;
	}

	if (priv->status & (STATUS_ASSOCIATED | STATUS_ASSOCIATING)) {
		IWI_DEBUG("Not attempting association (already in "
				"progress)\n");
		return 0;
	}

	if (priv->status & STATUS_DISASSOCIATING) {
		IWI_DEBUG("Not attempting association (in "
				"disassociating)\n ");
		//queue_work(priv->workqueue, &priv->associate);
		queue_te(5,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_associate),priv,NULL,true);
		return 0;
	}

	if (!ipw_is_init(priv) || (priv->status & STATUS_SCANNING)) {
		IWI_DEBUG("Not attempting association (scanning or not "
				"initialized)\n");
		return 0;
	}

	if (!(priv->config & CFG_ASSOCIATE) &&
	    !(priv->config & (CFG_STATIC_ESSID |
			      CFG_STATIC_CHANNEL | CFG_STATIC_BSSID))) {
		IWI_DEBUG("Not attempting association (associate=0)\n");
		return 0;
	}

	int i;
	list_for_each_entry(network, &priv->ieee->network_list, list) 
		ipw_best_network(priv, &match, network, 0);
	
	network = match.network;
	rates = &match.rates;
	
	if (network == NULL &&
	    priv->ieee->iw_mode == IW_MODE_ADHOC &&
	    priv->config & CFG_ADHOC_CREATE &&
	    //priv->config & CFG_STATIC_ESSID &&
	    //priv->config & CFG_STATIC_CHANNEL &&
	    !list_empty(&priv->ieee->network_free_list)) {
		IWI_DEBUG("network == NULL doing ipw_adhoc_create\n");
		element = priv->ieee->network_free_list.next;
		network = list_entry(element, struct ieee80211_network, list);
		ipw_adhoc_create(priv, network);
		char cc[7];
		sprintf(cc,"iwi2200");
		memcpy(network->ssid,cc,sizeof(cc));
		network->ssid_len=7;
		rates = &priv->rates;
		list_del(element);
		list_add_tail(&network->list, &priv->ieee->network_list);
	}

	if (priv->ieee->iw_mode == IW_MODE_INFRA)
	if (network)
	{
		for (int i=0;i<20;i++) 
		{
			if (nonets[i].bssid)
			if (!memcmp(nonets[i].bssid, network->bssid, ETH_ALEN)) 
			{
				IWI_LOG("Network '%s (%02x:%02x:%02x:%02x:%02x:%02x)' in exclude list. "
				"restart card to include.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
				//ipw_scan(priv,0);
				//return 0;
			}
		}
	}
	/* If we reached the end of the list, then we don't have any valid
	 * matching APs */
	 
	if (!network) {
		IWI_LOG("no network found\n");
		if (!(priv->status & STATUS_SCANNING)) {
			if (!(priv->config & CFG_SPEED_SCAN))
				queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,3,true);
			else
				queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,3,true);
		}
		return 0;
	}

	IWI_DEBUG("trying to associate '%s (%02x:%02x:%02x:%02x:%02x:%02x)'\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
	queue_td(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan));
	ipw_associate_network(priv, network, rates, 0);

	return 1;
}

void darwin_iwi2200::ipw_set_fixed_rate(struct ipw_priv *priv, int mode)
{
	/* TODO: Verify that this works... */
	struct ipw_fixed_rate fr = {
		priv->rates_mask,
		NULL
	};
	u32 reg;
	u16 mask = 0;

	/* Identify 'current FW band' and match it with the fixed
	 * Tx rates */

	switch (priv->ieee->freq_band) {
	case IEEE80211_52GHZ_BAND:	/* A only */
		/* IEEE_A */
		if (priv->rates_mask & ~IEEE80211_OFDM_RATES_MASK) {
			/* Invalid fixed rate mask */
			IWI_DEBUG
			    ("invalid fixed rate mask in ipw_set_fixed_rate\n");
			fr.tx_rates = 0;
			break;
		}

		fr.tx_rates >>= IEEE80211_OFDM_SHIFT_MASK_A;
		break;

	default:		/* 2.4Ghz or Mixed */
		/* IEEE_B */
		if (mode == IEEE_B) {
			if (fr.tx_rates & ~IEEE80211_CCK_RATES_MASK) {
				/* Invalid fixed rate mask */
				IWI_DEBUG
				    ("invalid fixed rate mask in ipw_set_fixed_rate\n");
				fr.tx_rates = 0;
			}
			break;
		}

		/* IEEE_G */
		if (fr.tx_rates & ~(IEEE80211_CCK_RATES_MASK |
				    IEEE80211_OFDM_RATES_MASK)) {
			/* Invalid fixed rate mask */
			IWI_DEBUG
			    ("invalid fixed rate mask in ipw_set_fixed_rate\n");
			fr.tx_rates = 0;
			break;
		}

		if (IEEE80211_OFDM_RATE_6MB_MASK & fr.tx_rates) {
			mask |= (IEEE80211_OFDM_RATE_6MB_MASK >> 1);
			fr.tx_rates &= ~IEEE80211_OFDM_RATE_6MB_MASK;
		}

		if (IEEE80211_OFDM_RATE_9MB_MASK & fr.tx_rates) {
			mask |= (IEEE80211_OFDM_RATE_9MB_MASK >> 1);
			fr.tx_rates &= ~IEEE80211_OFDM_RATE_9MB_MASK;
		}

		if (IEEE80211_OFDM_RATE_12MB_MASK & fr.tx_rates) {
			mask |= (IEEE80211_OFDM_RATE_12MB_MASK >> 1);
			fr.tx_rates &= ~IEEE80211_OFDM_RATE_12MB_MASK;
		}

		fr.tx_rates |= mask;
		break;
	}

	reg = ipw_read32(IPW_MEM_FIXED_OVERRIDE);
	ipw_write_reg32( reg, *(u32 *) & fr);
}

int darwin_iwi2200::ipw_qos_association(struct ipw_priv *priv,
			       struct ieee80211_network *network)
{
	int err = 0;
	struct ieee80211_qos_data *qos_data = NULL;
	struct ieee80211_qos_data ibss_data;
	ibss_data.supported = 1;
	ibss_data.active = 1;

	switch (priv->ieee->iw_mode) {
	case IW_MODE_ADHOC:
		if(!(network->capability & WLAN_CAPABILITY_IBSS)) return -1;

		qos_data = &ibss_data;
		break;

	case IW_MODE_INFRA:
		qos_data = &network->qos_data;
		break;

	default:
		return -1;//BUG();
		break;
	}

	err = ipw_qos_activate(priv, qos_data);
	if (err) {
		priv->assoc_request.policy_support &= ~HC_QOS_SUPPORT_ASSOC;
		return err;
	}

	if (priv->qos_data.qos_enable && qos_data->supported) {
		IWI_DEBUG("QoS will be enabled for this association\n");
		priv->assoc_request.policy_support |= HC_QOS_SUPPORT_ASSOC;
		return ipw_qos_set_info_element(priv);
	}

	return 0;
}

int darwin_iwi2200::ipw_send_qos_info_command(struct ipw_priv *priv, struct ieee80211_qos_information_element
				     *qos_param)
{
	return ipw_send_cmd_pdu(priv, IPW_CMD_WME_INFO, sizeof(*qos_param),
				qos_param);
}

int darwin_iwi2200::ipw_qos_set_info_element(struct ipw_priv *priv)
{
	int ret = 0;
	struct ieee80211_qos_information_element qos_info;

	if (priv == NULL)
		return -1;

	qos_info.elementID = QOS_ELEMENT_ID;
	qos_info.length = sizeof(struct ieee80211_qos_information_element) - 2;

	qos_info.version = QOS_VERSION_1;
	qos_info.ac_info = 0;

	memcpy(qos_info.qui, qos_oui, QOS_OUI_LEN);
	qos_info.qui_type = QOS_OUI_TYPE;
	qos_info.qui_subtype = QOS_OUI_INFO_SUB_TYPE;

	ret = ipw_send_qos_info_command(priv, &qos_info);
	if (ret != 0) {
		IWI_DEBUG("QoS error calling ipw_send_qos_info_command\n");
	}
	return ret;
}

int darwin_iwi2200::ipw_associate_network(struct ipw_priv *priv,
				 struct ieee80211_network *network,
				 struct ipw_supported_rates *rates, int roaming)
{
	int err;
	
	// checking duplicate called?
	
	IWI_DEBUG("ASSOCIATED | ASSOCIATING 0x%08x\n", (STATUS_ASSOCIATED | STATUS_ASSOCIATING));
	if (priv->status & (STATUS_ASSOCIATED | STATUS_ASSOCIATING)) {
		IWI_DEBUG("Not attempting association (already in "
				"progress)\n");
		return 0;
	}

	if (priv->config & CFG_FIXED_RATE)
		ipw_set_fixed_rate(priv, network->mode);

	if (!(priv->config & CFG_STATIC_ESSID)) {
		priv->essid_len = min(network->ssid_len,
				      (u8) IW_ESSID_MAX_SIZE);
		memcpy(priv->essid, network->ssid, priv->essid_len);
	}

	network->last_associate = jiffies;

	memset(&priv->assoc_request, 0, sizeof(priv->assoc_request));
	priv->assoc_request.channel = network->channel;
	priv->assoc_request.auth_key = 0;

	if ((priv->capability & CAP_PRIVACY_ON) &&
	    (priv->ieee->sec.auth_mode == WLAN_AUTH_SHARED_KEY)) {
		priv->assoc_request.auth_type = AUTH_SHARED_KEY;
		priv->assoc_request.auth_key = priv->ieee->sec.active_key;

		if (priv->ieee->sec.level == SEC_LEVEL_1)
			ipw_send_wep_keys(priv, DCW_WEP_KEY_SEC_TYPE_WEP);

	} else if ((priv->capability & CAP_PRIVACY_ON) &&
		   (priv->ieee->sec.auth_mode == WLAN_AUTH_LEAP))
		priv->assoc_request.auth_type = AUTH_LEAP;
	else
		priv->assoc_request.auth_type = AUTH_OPEN;

	if (priv->ieee->wpa_ie_len) {
		priv->assoc_request.policy_support = 0x02;	/* RSN active */
		IWI_DEBUG("IPW_CMD_RSN_CAPABILITIES.\n");
		ipw_send_cmd_pdu(priv, IPW_CMD_RSN_CAPABILITIES, priv->ieee->wpa_ie_len, &priv->ieee->wpa_ie);
		//sendCommand(IPW_CMD_RSN_CAPABILITIES, &priv->ieee->wpa_ie,priv->ieee->wpa_ie_len, 1);
	}

	/*
	 * It is valid for our ieee device to support multiple modes, but
	 * when it comes to associating to a given network we have to choose
	 * just one mode.
	 */
	if (network->mode & priv->ieee->mode & IEEE_A)
		priv->assoc_request.ieee_mode = IPW_A_MODE;
	else if (network->mode & priv->ieee->mode & IEEE_G)
		priv->assoc_request.ieee_mode = IPW_G_MODE;
	else if (network->mode & priv->ieee->mode & IEEE_B)
		priv->assoc_request.ieee_mode = IPW_B_MODE;

	priv->assoc_request.capability = network->capability;
	if ((network->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
	    && !(priv->config & CFG_PREAMBLE_LONG)) {
		priv->assoc_request.preamble_length = DCT_FLAG_SHORT_PREAMBLE;
	} else {
		priv->assoc_request.preamble_length = DCT_FLAG_LONG_PREAMBLE;

		/* Clear the short preamble if we won't be supporting it */
		priv->assoc_request.capability &=
		    ~WLAN_CAPABILITY_SHORT_PREAMBLE;
	}

	/* Clear capability bits that aren't used in Ad Hoc */
	if (priv->ieee->iw_mode == IW_MODE_ADHOC)
		priv->assoc_request.capability &=
		    ~WLAN_CAPABILITY_SHORT_SLOT_TIME;

	IWI_DEBUG("%sssocation attempt: '%s', channel %d, "
			"802.11%c [%d], %s[:%s], enc=%s%s%s%c%c\n",
			roaming ? "Rea" : "A",
			escape_essid((const char*)priv->essid, priv->essid_len),
			network->channel,
			ipw_modes[priv->assoc_request.ieee_mode],
			rates->num_rates,
			(priv->assoc_request.preamble_length ==
			 DCT_FLAG_LONG_PREAMBLE) ? "long" : "short",
			network->capability &
			WLAN_CAPABILITY_SHORT_PREAMBLE ? "short" : "long",
			priv->capability & CAP_PRIVACY_ON ? "on " : "off",
			priv->capability & CAP_PRIVACY_ON ?
			(priv->capability & CAP_SHARED_KEY ? "(shared)" :
			 "(open)") : "",
			priv->capability & CAP_PRIVACY_ON ? " key=" : "",
			priv->capability & CAP_PRIVACY_ON ?
			'1' + priv->ieee->sec.active_key : '.',
			priv->capability & CAP_PRIVACY_ON ? '.' : ' ');

	priv->assoc_request.beacon_interval = network->beacon_interval;
	if ((priv->ieee->iw_mode == IW_MODE_ADHOC) &&
	    (network->time_stamp[0] == 0) && (network->time_stamp[1] == 0)) {
		priv->assoc_request.assoc_type = HC_IBSS_START;
		priv->assoc_request.assoc_tsf_msw = 0;
		priv->assoc_request.assoc_tsf_lsw = 0;
	} else {
		if (unlikely(roaming))
			priv->assoc_request.assoc_type = HC_REASSOCIATE;
		else
			priv->assoc_request.assoc_type = HC_ASSOCIATE;
		priv->assoc_request.assoc_tsf_msw = network->time_stamp[1];
		priv->assoc_request.assoc_tsf_lsw = network->time_stamp[0];
	}

	memcpy(priv->assoc_request.bssid, network->bssid, ETH_ALEN);

	if (priv->ieee->iw_mode == IW_MODE_ADHOC) {
		memset(&priv->assoc_request.dest, 0xFF, ETH_ALEN);
		priv->assoc_request.atim_window = network->atim_window;
	} else {
		memcpy(priv->assoc_request.dest, network->bssid, ETH_ALEN);
		priv->assoc_request.atim_window = 0;
	}

	priv->assoc_request.listen_interval = network->listen_interval;
	IWI_DEBUG("SSID command.\n");
	err=ipw_send_cmd_pdu(priv, IPW_CMD_SSID, min(priv->essid_len, IW_ESSID_MAX_SIZE),&priv->essid);
	//err = sendCommand(IPW_CMD_SSID, &priv->essid,min(priv->essid_len, IW_ESSID_MAX_SIZE), 1);
	if (err) {
		IWI_DEBUG("Attempt to send SSID command failed.\n");
		return err;
	}

	rates->ieee_mode = priv->assoc_request.ieee_mode;
	rates->purpose = IPW_RATE_CONNECT;
	IWI_DEBUG("IPW_CMD_SUPPORTED_RATES .\n");
	ipw_send_cmd_pdu(priv, IPW_CMD_SUPPORTED_RATES, sizeof(*rates), rates);
	//sendCommand(IPW_CMD_SUPPORTED_RATES, &rates,sizeof(rates), 1);

	if (priv->assoc_request.ieee_mode == IPW_G_MODE)
		priv->sys_config.dot11g_auto_detection = 1;
	else
		priv->sys_config.dot11g_auto_detection = 0;

	if (priv->ieee->iw_mode == IW_MODE_ADHOC)
		priv->sys_config.answer_broadcast_ssid_probe = 1;
	else
		priv->sys_config.answer_broadcast_ssid_probe = 0;

	IWI_DEBUG("sys config command.\n");
	err=ipw_send_cmd_pdu(priv, IPW_CMD_SYSTEM_CONFIG,sizeof(priv->sys_config), &priv->sys_config);
	//err = sendCommand(IPW_CMD_HOST_COMPLETE, &priv->sys_config,sizeof(priv->sys_config), 1);
	if (err) {
		IWI_DEBUG("Attempt to send sys config command failed.\n");
		return err;
	}

	IWI_DEBUG("Association sensitivity: %d\n", network->stats.rssi);
	struct ipw_sensitivity_calib calib = {
		cpu_to_le16(network->stats.rssi + IPW_RSSI_TO_DBM),
		NULL
	};
	err=ipw_send_cmd_pdu(priv, IPW_CMD_SENSITIVITY_CALIB, sizeof(calib), &calib);
	//err = sendCommand(IPW_CMD_SENSITIVITY_CALIB, &calib,sizeof(calib), 1);
	if (err) {
		IWI_DEBUG("Attempt to send associate command failed.\n");
		return err;
	}

	/*
	 * If preemption is enabled, it is possible for the association
	 * to complete before we return from ipw_send_associate.  Therefore
	 * we have to be sure and update our priviate data first.
	 */
	priv->channel = network->channel;
	memcpy(priv->bssid, network->bssid, ETH_ALEN);

	priv->assoc_network = network;
#ifdef CONFIG_IPW2200_QOS
	if (qos_enable) ipw_qos_association(priv, network);
#endif
	err = ipw_send_associate(priv, &priv->assoc_request);
	if (err) {
		IWI_DEBUG("Attempt to send associate command failed.\n");
		return err;
	}

	priv->status |= STATUS_ASSOCIATING;
	priv->status &= ~STATUS_SECURITY_UPDATED;
	IWI_DEBUG("associating: '%s' %02x:%02x:%02x:%02x:%02x:%02x \n",
		  escape_essid((const char*)priv->essid, priv->essid_len),
		  MAC_ARG(priv->bssid));

	return 0;
}

int darwin_iwi2200::ipw_get_ordinal(struct ipw_priv *priv, u32 ord, void *val, u32 * len)
{
	u32 addr, field_info, field_len, field_count, total_len;

	IWI_DEBUG("ordinal = %d\n", ord);

	if (!priv || !val || !len) {
		IWI_DEBUG("Invalid argument\n");
		return -EINVAL;
	}

	/* verify device ordinal tables have been initialized */
	if (!priv->table0_addr || !priv->table1_addr || !priv->table2_addr) {
		IWI_DEBUG("Access ordinals before initialization\n");
		return -EINVAL;
	}

	switch (IPW_ORD_TABLE_ID_MASK & ord) {
	case IPW_ORD_TABLE_0_MASK:
		/*
		 * TABLE 0: Direct access to a table of 32 bit values
		 *
		 * This is a very simple table with the data directly
		 * read from the table
		 */

		/* remove the table id from the ordinal */
		ord &= IPW_ORD_TABLE_VALUE_MASK;

		/* boundary check */
		if (ord > priv->table0_len) {
			IWI_DEBUG("ordinal value (%d) longer then "
				      "max (%d)\n", ord, priv->table0_len);
			return -EINVAL;
		}

		/* verify we have enough room to store the value */
		if (*len < sizeof(u32)) {
			IWI_DEBUG("ordinal buffer length too small, "
				      "need %zd\n", sizeof(u32));
			return -EINVAL;
		}

		IWI_DEBUG("Reading TABLE0[%d] from offset 0x%08x\n",
			      ord, priv->table0_addr + (ord << 2));

		*len = sizeof(u32);
		ord <<= 2;
		*((u32 *) val) = ipw_read32( priv->table0_addr + ord);
		break;

	case IPW_ORD_TABLE_1_MASK:
		/*
		 * TABLE 1: Indirect access to a table of 32 bit values
		 *
		 * This is a fairly large table of u32 values each
		 * representing starting addr for the data (which is
		 * also a u32)
		 */

		/* remove the table id from the ordinal */
		ord &= IPW_ORD_TABLE_VALUE_MASK;

		/* boundary check */
		if (ord > priv->table1_len) {
			IWI_DEBUG("ordinal value too long\n");
			return -EINVAL;
		}

		/* verify we have enough room to store the value */
		if (*len < sizeof(u32)) {
			IWI_DEBUG("ordinal buffer length too small, "
				      "need %zd\n", sizeof(u32));
			return -EINVAL;
		}

		*((u32 *) val) =
		    ipw_read_reg32( (priv->table1_addr + (ord << 2)));
		*len = sizeof(u32);
		break;

	case IPW_ORD_TABLE_2_MASK:
		/*
		 * TABLE 2: Indirect access to a table of variable sized values
		 *
		 * This table consist of six values, each containing
		 *     - dword containing the starting offset of the data
		 *     - dword containing the lengh in the first 16bits
		 *       and the count in the second 16bits
		 */

		/* remove the table id from the ordinal */
		ord &= IPW_ORD_TABLE_VALUE_MASK;

		/* boundary check */
		if (ord > priv->table2_len) {
			IWI_DEBUG("ordinal value too long\n");
			return -EINVAL;
		}

		/* get the address of statistic */
		addr = ipw_read_reg32( priv->table2_addr + (ord << 3));

		/* get the second DW of statistics ;
		 * two 16-bit words - first is length, second is count */
		field_info =
		    ipw_read_reg32(
				   priv->table2_addr + (ord << 3) +
				   sizeof(u32));

		/* get each entry length */
		field_len = *((u16 *) & field_info);

		/* get number of entries */
		field_count = *(((u16 *) & field_info) + 1);

		/* abort if not enought memory */
		total_len = field_len * field_count;
		if (total_len > *len) {
			*len = total_len;
			return -EINVAL;
		}

		*len = total_len;
		if (!total_len)
			return 0;

		IWI_DEBUG("addr = 0x%08x, total_len = %d, "
			      "field_info = 0x%08x\n",
			      addr, total_len, field_info);
		ipw_read_indirect( priv, addr, (u8*)val, total_len);
		break;

	default:
		IWI_DEBUG("Invalid ordinal!\n");
		return -EINVAL;

	}

	return 0;
}

void darwin_iwi2200::ipw_reset_stats(struct ipw_priv *priv)
{
	u32 len = sizeof(u32);

	priv->quality = 0;
	memset(&priv->average_missed_beacons, 0, sizeof(*&priv->average_missed_beacons));
	priv->exp_avg_rssi = -60;
	priv->exp_avg_noise = -85 + 0x100;

	priv->last_rate = 0;
	priv->last_missed_beacons = 0;
	priv->last_rx_packets = 0;
	priv->last_tx_packets = 0;
	priv->last_tx_failures = 0;

	/* Firmware managed, reset only when NIC is restarted, so we have to
	 * normalize on the current value */
	ipw_get_ordinal(priv, IPW_ORD_STAT_RX_ERR_CRC,
			&priv->last_rx_err, &len);
	ipw_get_ordinal(priv, IPW_ORD_STAT_TX_FAILURE,
			&priv->last_tx_failures, &len);

	/* Driver managed, reset with each association */
	priv->missed_adhoc_beacons = 0;
	priv->missed_beacons = 0;
	priv->tx_packets = 0;
	priv->rx_packets = 0;

}

void darwin_iwi2200::ipw_read_indirect(struct ipw_priv *priv, u32 addr, u8 * buf,
			       int num)
{
	u32 aligned_addr = addr & IPW_INDIRECT_ADDR_MASK;	/* dword align */
	u32 dif_len = addr - aligned_addr;
	u32 i;

	IWI_DEBUG("addr = %d, buf = %p, num = %d\n", addr, buf, num);

	if (num <= 0) {
		return;
	}

	/* Read the first dword (or portion) byte by byte */
	if (unlikely(dif_len)) {
		ipw_write32( IPW_INDIRECT_ADDR, aligned_addr);
		/* Start reading at aligned_addr + dif_len */
		for (i = dif_len; ((i < 4) && (num > 0)); i++, num--)
			*buf++ = _ipw_read8(memBase, IPW_INDIRECT_DATA + i);
		aligned_addr += 4;
	}

	/* Read all of the middle dwords as dwords, with auto-increment */
	ipw_write32( IPW_AUTOINC_ADDR, aligned_addr);
	for (; num >= 4; buf += 4, aligned_addr += 4, num -= 4)
		*(u32 *) buf = ipw_read32( IPW_AUTOINC_DATA);

	/* Read the last dword (or portion) byte by byte */
	if (unlikely(num)) {
		ipw_write32( IPW_INDIRECT_ADDR, aligned_addr);
		for (i = 0; num > 0; i++, num--)
			*buf++ = _ipw_read8(memBase, IPW_INDIRECT_DATA + i);
	}
}

void darwin_iwi2200::ipw_link_up(struct ipw_priv *priv)
{
	priv->last_seq_num = -1;
	priv->last_frag_num = -1;
	priv->last_packet_time = 0;
	

	//ifaddr_t tt=(ifaddr_t)priv->bssid;
	
	//ifaddr_t tt=(ifaddr_t)priv->mac_addr;
	//setHardwareAddress(&tt, ETHER_ADDR_LEN);
	
	//configu(priv);
	//fNetif->attachToDataLinkLayer(0,0);
	//ifnet_set_lladdr(fifnet, tt, ETHER_ADDR_LEN);
	enable(fNetif);
	fTransmitQueue->start();
	//fTransmitQueue->flush();
	/*netif_carrier_on(priv->net_dev);
	if (netif_queue_stopped(priv->net_dev)) {
		IWI_DEBUG("waking queue\n");
		netif_wake_queue(priv->net_dev);
	} else {
		IWI_DEBUG("starting queue\n");
		netif_start_queue(priv->net_dev);
	}*/
	queue_td(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan));	
	queue_td(4,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan_check));
	ipw_reset_stats(priv);
	/* Ensure the rate is updated immediately */
	priv->last_rate = ipw_get_current_rate(priv);
	//ipw_gather_stats(priv);
	ipw_led_link_on(priv);
	//notify_wx_assoc_event(priv);

	if (priv->config & CFG_BACKGROUND_SCAN)
		queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,3,true);
}

void darwin_iwi2200::average_add(struct average *avg, s16 val)
{
	avg->sum -= avg->entries[avg->pos];
	avg->sum += val;
	avg->entries[avg->pos++] = val;
	if (unlikely(avg->pos == AVG_ENTRIES)) {
		avg->init = 1;
		avg->pos = 0;
	}
}

void darwin_iwi2200::ipw_gather_stats(struct ipw_priv *priv)
{
	u32 rx_err, rx_err_delta, rx_packets_delta;
	u32 tx_failures, tx_failures_delta, tx_packets_delta;
	u32 missed_beacons_percent, missed_beacons_delta;
	u32 quality = 0;
	u32 len = sizeof(u32);
	s16 rssi;
	u32 beacon_quality, signal_quality, tx_quality, rx_quality,
	    rate_quality;
	u32 max_rate;

	if (!(priv->status & STATUS_ASSOCIATED)) {
		priv->quality = 0;
		return;
	}

	if (priv->ieee->iw_mode == IW_MODE_INFRA && priv->assoc_network)
	if (priv->assoc_network->exclude==1)
	{
		/*int i,p=-1,ok=0;
		for (i=0;i<20;i++) 
		{
			if (nonets[i].bssid)
			if (!memcmp(nonets[i].bssid, priv->bssid, ETH_ALEN)) ok=1;
			else if (p==-1) p=i;
		}
		if (ok==0 && p!=-1) memcpy(&nonets[p],priv->assoc_network,sizeof(struct ieee80211_network));
		priv->assoc_network->exclude=0;
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		ipw_disassociate(priv);
		return;*/
	}
	
	/* Update the statistics */
	ipw_get_ordinal(priv, IPW_ORD_STAT_MISSED_BEACONS,
			&priv->missed_beacons, &len);
	missed_beacons_delta = priv->missed_beacons - priv->last_missed_beacons;
	priv->last_missed_beacons = priv->missed_beacons;
	if (priv->assoc_request.beacon_interval) {
		missed_beacons_percent = missed_beacons_delta *
		    (HZ * priv->assoc_request.beacon_interval) /
		    (IPW_STATS_INTERVAL * 10);
	} else {
		missed_beacons_percent = 0;
	}
	average_add(&priv->average_missed_beacons, missed_beacons_percent);

	ipw_get_ordinal(priv, IPW_ORD_STAT_RX_ERR_CRC, &rx_err, &len);
	rx_err_delta = rx_err - priv->last_rx_err;
	priv->last_rx_err = rx_err;

	ipw_get_ordinal(priv, IPW_ORD_STAT_TX_FAILURE, &tx_failures, &len);
	tx_failures_delta = tx_failures - priv->last_tx_failures;
	priv->last_tx_failures = tx_failures;

	rx_packets_delta = priv->rx_packets - priv->last_rx_packets;
	priv->last_rx_packets = priv->rx_packets;

	tx_packets_delta = priv->tx_packets - priv->last_tx_packets;
	priv->last_tx_packets = priv->tx_packets;

	/* Calculate quality based on the following:
	 *
	 * Missed beacon: 100% = 0, 0% = 70% missed
	 * Rate: 60% = 1Mbs, 100% = Max
	 * Rx and Tx errors represent a straight % of total Rx/Tx
	 * RSSI: 100% = > -50,  0% = < -80
	 * Rx errors: 100% = 0, 0% = 50% missed
	 *
	 * The lowest computed quality is used.
	 *
	 */

	beacon_quality = 100 - missed_beacons_percent;
	if (beacon_quality < BEACON_THRESHOLD)
		beacon_quality = 0;
	else
		beacon_quality = (beacon_quality - BEACON_THRESHOLD) * 100 /
		    (100 - BEACON_THRESHOLD);
	IWI_DEBUG("Missed beacon: %3d%% (%d%%)\n",
			beacon_quality, missed_beacons_percent);

	priv->last_rate = ipw_get_current_rate(priv);
	max_rate = ipw_get_max_rate(priv);
	rate_quality = priv->last_rate * 40 / max_rate + 60;
	IWI_DEBUG("Rate quality : %3d%% (%dMbs)\n",
			rate_quality, priv->last_rate / 1000000);

	if (rx_packets_delta > 100 && rx_packets_delta + rx_err_delta)
		rx_quality = 100 - (rx_err_delta * 100) /
		    (rx_packets_delta + rx_err_delta);
	else
		rx_quality = 100;
	IWI_DEBUG("Rx quality   : %3d%% (%u errors, %u packets)\n",
			rx_quality, rx_err_delta, rx_packets_delta);

	if (tx_packets_delta > 100 && tx_packets_delta + tx_failures_delta)
		tx_quality = 100 - (tx_failures_delta * 100) /
		    (tx_packets_delta + tx_failures_delta);
	else
		tx_quality = 100;
	IWI_DEBUG("Tx quality   : %3d%% (%u errors, %u packets)\n",
			tx_quality, tx_failures_delta, tx_packets_delta);

	rssi = priv->exp_avg_rssi;
	signal_quality =
	    (100 *
	     (priv->ieee->perfect_rssi - priv->ieee->worst_rssi) *
	     (priv->ieee->perfect_rssi - priv->ieee->worst_rssi) -
	     (priv->ieee->perfect_rssi - rssi) *
	     (15 * (priv->ieee->perfect_rssi - priv->ieee->worst_rssi) +
	      62 * (priv->ieee->perfect_rssi - rssi))) /
	    ((priv->ieee->perfect_rssi - priv->ieee->worst_rssi) *
	     (priv->ieee->perfect_rssi - priv->ieee->worst_rssi));
	if (signal_quality > 100)
		signal_quality = 100;
	else if (signal_quality < 1)
		signal_quality = 0;

	IWI_DEBUG("Signal level : %3d%% (%d dBm)\n",
			signal_quality, rssi);

	quality = min(beacon_quality,
		      min(rate_quality,
			  min(tx_quality, min(rx_quality, signal_quality))));
	if (quality == beacon_quality)
		IWI_DEBUG("Quality (%d%%): Clamped to missed beacons.\n",
				quality);
	if (quality == rate_quality)
		IWI_DEBUG("Quality (%d%%): Clamped to rate quality.\n",
				quality);
	if (quality == tx_quality)
		IWI_DEBUG("Quality (%d%%): Clamped to Tx quality.\n",
				quality);
	if (quality == rx_quality)
		IWI_DEBUG("Quality (%d%%): Clamped to Rx quality.\n",
				quality);
	if (quality == signal_quality)
		IWI_DEBUG("Quality (%d%%): Clamped to signal quality.\n",
				quality);

	priv->quality = quality;

	//queue_delayed_work(priv->workqueue, &priv->gather_stats, IPW_STATS_INTERVAL);
	queue_te(6,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_gather_stats),priv,2,true);
}

u32 darwin_iwi2200::ipw_get_max_rate(struct ipw_priv *priv)
{
	u32 i = 0x80000000;
	u32 mask = priv->rates_mask;
	/* If currently associated in B mode, restrict the maximum
	 * rate match to B rates */
	if (priv->assoc_request.ieee_mode == IPW_B_MODE)
		mask &= IEEE80211_CCK_RATES_MASK;

	/* TODO: Verify that the rate is supported by the current rates
	 * list. */

	while (i && !(mask & i))
		i >>= 1;
	switch (i) {
	case IEEE80211_CCK_RATE_1MB_MASK:
		return 1000000;
	case IEEE80211_CCK_RATE_2MB_MASK:
		return 2000000;
	case IEEE80211_CCK_RATE_5MB_MASK:
		return 5500000;
	case IEEE80211_OFDM_RATE_6MB_MASK:
		return 6000000;
	case IEEE80211_OFDM_RATE_9MB_MASK:
		return 9000000;
	case IEEE80211_CCK_RATE_11MB_MASK:
		return 11000000;
	case IEEE80211_OFDM_RATE_12MB_MASK:
		return 12000000;
	case IEEE80211_OFDM_RATE_18MB_MASK:
		return 18000000;
	case IEEE80211_OFDM_RATE_24MB_MASK:
		return 24000000;
	case IEEE80211_OFDM_RATE_36MB_MASK:
		return 36000000;
	case IEEE80211_OFDM_RATE_48MB_MASK:
		return 48000000;
	case IEEE80211_OFDM_RATE_54MB_MASK:
		return 54000000;
	}

	if (priv->ieee->mode == IEEE_B)
		return 11000000;
	else
		return 54000000;
}

u32 darwin_iwi2200::ipw_get_current_rate(struct ipw_priv *priv)
{
	u32 rate, len = sizeof(rate);
	int err;

	if (!(priv->status & STATUS_ASSOCIATED))
		return 0;

	if (priv->tx_packets > IPW_REAL_RATE_RX_PACKET_THRESHOLD) {
		err = ipw_get_ordinal(priv, IPW_ORD_STAT_TX_CURR_RATE, &rate,
				      &len);
		if (err) {
			IWI_DEBUG("failed querying ordinals.\n");
			return 0;
		}
	} else
		return ipw_get_max_rate(priv);

	switch (rate) {
	case IPW_TX_RATE_1MB:
		return 1000000;
	case IPW_TX_RATE_2MB:
		return 2000000;
	case IPW_TX_RATE_5MB:
		return 5500000;
	case IPW_TX_RATE_6MB:
		return 6000000;
	case IPW_TX_RATE_9MB:
		return 9000000;
	case IPW_TX_RATE_11MB:
		return 11000000;
	case IPW_TX_RATE_12MB:
		return 12000000;
	case IPW_TX_RATE_18MB:
		return 18000000;
	case IPW_TX_RATE_24MB:
		return 24000000;
	case IPW_TX_RATE_36MB:
		return 36000000;
	case IPW_TX_RATE_48MB:
		return 48000000;
	case IPW_TX_RATE_54MB:
		return 54000000;
	}

	return 0;
}

void darwin_iwi2200::ipw_link_down(struct ipw_priv *priv)
{

	priv->status |= STATUS_ASSOCIATED;
	ipw_led_link_down(priv);
	IWI_DEBUG("link down\n");
	fTransmitQueue->stop();

	fTransmitQueue->flush();
	fNetif->clearInputQueue();
	priv->status &= ~(STATUS_ASSOCIATED);

	disable(fNetif);
	//netif_carrier_off(priv->net_dev);
	//netif_stop_queue(priv->net_dev);
	//notify_wx_assoc_event(priv);
	/* Cancel any queued work ... */
	queue_td(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan));
	queue_td(4,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan_check));
	queue_td(6,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_gather_stats));
	queue_td(8,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adhoc_check));
	ipw_reset_stats(priv);
	if (!(priv->status & STATUS_EXIT_PENDING)) {
		/* Queue up another scan... */
		queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,NULL,true);
		queue_te(2,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_led_link_on),priv,NULL,true);
	}
}

const char* darwin_iwi2200::ipw_get_status_code(u16 status)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(ipw_status_codes); i++)
		if (ipw_status_codes[i].status == (status & 0xff))
			return ipw_status_codes[i].reason;
	return "Unknown status value.";
}

void darwin_iwi2200::ipw_adhoc_check(void *data)
{
	struct ipw_priv *priv = (struct ipw_priv*)data;

	if (priv->missed_adhoc_beacons++ > priv->disassociate_threshold &&
	    !(priv->config & CFG_ADHOC_PERSIST)) {
		IWI_DEBUG("Missed beacon: %d - disassociate\n",
			  priv->missed_adhoc_beacons);
		ipw_remove_current_network(priv);
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		ipw_disassociate(priv);
		return;
	}

	//queue_delayed_work(priv->workqueue, &priv->adhoc_check,  priv->assoc_request.beacon_interval);
	if (priv->status & STATUS_ASSOCIATED)
	{
		IWI_DEBUG("ipw_adhoc_check in %d\n",priv->assoc_request.beacon_interval/10);
		queue_te(8,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adhoc_check),priv,priv->assoc_request.beacon_interval/10, true);
	}
	
}

void darwin_iwi2200::ipw_handle_missed_beacon(struct ipw_priv *priv, int missed_count)
{
	//if (!(ifnet_flags(fifnet) & IFF_RUNNING)) return;
	priv->notif_missed_beacons = missed_count;

	if (missed_count > priv->disassociate_threshold &&
	    priv->status & STATUS_ASSOCIATED) {
		/* If associated and we've hit the missed
		 * beacon threshold, disassociate, turn
		 * off roaming, and abort any active scans */
		IWI_DEBUG( "Missed beacon: %d - disassociate\n", missed_count);
		priv->status &= ~STATUS_ROAMING;

		if (priv->status & STATUS_SCANNING) {
			IWI_DEBUG("Aborting scan with missed beacon.\n");
			//queue_work(priv->workqueue, &priv->abort_scan);
			ipw_abort_scan(priv);
		}
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		ipw_disassociate(priv);
		//queue_work(priv->workqueue, &priv->disassociate);
		return;
	}
	
	if (priv->status & STATUS_ROAMING) {
		/* If we are currently roaming, then just
		 * print a debug statement... */
		IWI_DEBUG( "Missed beacon: %d - roam in progress\n",
			  missed_count);
		return;
	}

	if (roaming &&
	    (missed_count > priv->roaming_threshold &&
	     missed_count <= priv->disassociate_threshold)) {
		/* If we are not already roaming, set the ROAM
		 * bit in the status and kick off a scan.
		 * This can happen several times before we reach
		 * disassociate_threshold. */
		IWI_DEBUG(  "Missed beacon: %d - initiate "
			  "roaming\n", missed_count);
		if (!(priv->status & STATUS_ROAMING)) {
			priv->status |= STATUS_ROAMING;
			if (!(priv->status & STATUS_SCANNING))
				queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,3,true);
				//queue_work(priv->workqueue,  &priv->request_scan);
		}
		return;
	}

	if (priv->status & STATUS_SCANNING) {
		/* Stop scan to keep fw from getting
		 * stuck (only if we aren't roaming --
		 * otherwise we'll never scan more than 2 or 3
		 * channels..) */
		IWI_DEBUG(    "Aborting scan with missed beacon.\n");
		ipw_abort_scan(priv);
		//queue_work(priv->workqueue, &priv->abort_scan);
	}

	IWI_DEBUG("Missed beacon: %d\n", missed_count);
}

void darwin_iwi2200::notifIntr(struct ipw_priv *priv,
				struct ipw_rx_notification *notif)
{
	notif->size = le16_to_cpu(notif->size);

	IWI_DEBUG_FULL("type = %d (%d bytes)\n", notif->subtype, notif->size);

	switch (notif->subtype) {
	case HOST_NOTIFICATION_STATUS_ASSOCIATED:{
			struct notif_association *assoc = &notif->u.assoc;

			switch (assoc->state) {
			case CMAS_ASSOCIATED:{
					IWI_LOG(						  "associated: '%s' %02x:%02x:%02x:%02x:%02x:%02x \n",
						  escape_essid((const char*)priv->essid,
							       priv->essid_len),
						  MAC_ARG(priv->bssid));

					switch (priv->ieee->iw_mode) {
					case IW_MODE_INFRA:
						memcpy(priv->ieee->bssid,
						       priv->bssid, ETH_ALEN);
						break;

					case IW_MODE_ADHOC:
						memcpy(priv->ieee->bssid,
						       priv->bssid, ETH_ALEN);

						/* clear out the station table */
						priv->num_stations = 0;

						IWI_DEBUG("queueing adhoc check in %d\n", priv->assoc_request.beacon_interval/10);
						queue_te(8,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_adhoc_check),priv,priv->assoc_request.beacon_interval/10, true);
						/*queue_delayed_work(priv->
								   workqueue,
								   &priv->
								   adhoc_check,
								   priv->
								   assoc_request.
								   beacon_interval);*/
						break;
					}

					priv->status &= ~STATUS_ASSOCIATING;
					priv->status |= STATUS_ASSOCIATED;
					//queue_work(priv->workqueue, &priv->system_config);
					ipw_send_cmd_pdu(priv, IPW_CMD_SYSTEM_CONFIG,sizeof(priv->sys_config), &priv->sys_config);
					//sendCommand(IWI_CMD_SET_CONFIG, &priv->sys_config, sizeof(priv->sys_config), 1);

#ifdef CONFIG_IPW2200_QOS
					if ((priv->status & STATUS_AUTH) &&
					    (IPW_GET_PACKET_STYPE(&notif->u.raw)
					     == IEEE80211_STYPE_ASSOC_RESP)) {
						if ((sizeof
						     (struct
						      ieee80211_assoc_response)
						     <= notif->size)
						    && (notif->size <= 2314)) {
							struct ieee80211_rx_stats stats;
								stats.len =notif->size - 1;

							IWI_DEBUG
							    ("QoS Associate "
							     "size %d\n",
							     notif->size);
							ieee80211_rx_mgt(priv->
									 ieee,
									 (struct
									  ieee80211_hdr_4addr
									  *)
									 &notif->u.raw, &stats);
						}
					}
#endif
					ipw_link_up(priv);

					break;
				}

			case CMAS_AUTHENTICATED:{
					if (priv->
					    status & (STATUS_ASSOCIATED |
						      STATUS_AUTH)) {
						struct notif_authenticate *auth
						    = &notif->u.auth;
						IWI_LOG(							  "deauthenticated: '%s' "
							  MAC_FMT
							  ": (0x%04X) - %s \n",
							  escape_essid((const char*)priv->
								       essid,
								       priv->
								       essid_len),
							  MAC_ARG(priv->bssid),
							  ntohs(auth->status),
							  ipw_get_status_code
							  (ntohs
							   (auth->status)));

						priv->status &=
						    ~(STATUS_ASSOCIATING |
						      STATUS_AUTH |
						      STATUS_ASSOCIATED);

						ipw_link_down(priv);
						break;
					}

					IWI_LOG(						  "authenticated: '%s' %02x:%02x:%02x:%02x:%02x:%02x\n",
						  escape_essid((const char*)priv->essid,
							       priv->essid_len),
						  MAC_ARG(priv->bssid));
					break;
				}

			case CMAS_INIT:{
					if (priv->status & STATUS_AUTH) {
						struct
						    ieee80211_assoc_response
						*resp;
						resp =
						    (struct
						     ieee80211_assoc_response
						     *)&notif->u.raw;
						IWI_LOG(							  "association failed (0x%04X): %s\n",
							  ntohs(resp->status),
							  ipw_get_status_code
							  (ntohs
							   (resp->status)));
					}

					IWI_LOG(  "disassociated: '%s' %02x:%02x:%02x:%02x:%02x:%02x \n",
						  escape_essid((const char*)priv->essid,
							       priv->essid_len),
						  MAC_ARG(priv->bssid));

					priv->status &=
					    ~(STATUS_DISASSOCIATING |
					      STATUS_ASSOCIATING |
					      STATUS_ASSOCIATED | STATUS_AUTH);
					if (priv->assoc_network
					    && (priv->assoc_network->
						capability &
						WLAN_CAPABILITY_IBSS))
						ipw_remove_current_network
						    (priv);

					ipw_link_down(priv);

					break;
				}

			case CMAS_RX_ASSOC_RESP:
				break;

			default:
				IWI_DEBUG("assoc: unknown (%d)\n",
					  assoc->state);
				break;
			}

			break;
		}

	case HOST_NOTIFICATION_STATUS_AUTHENTICATE:{
			struct notif_authenticate *auth = &notif->u.auth;
			switch (auth->state) {
			case CMAS_AUTHENTICATED:
				IWI_LOG("AUthenticated: '%s' %02x:%02x:%02x:%02x:%02x:%02x \n",
					  escape_essid((const char*)priv->essid,
						       priv->essid_len),
					  MAC_ARG(priv->bssid));
				priv->status |= STATUS_AUTH;
				
				break;

			case CMAS_INIT:
				
				if (priv->status & STATUS_AUTH) {
					IWI_LOG(						  "AUthentication failed (0x%04X): %s\n",
						  ntohs(auth->status),
						  ipw_get_status_code(ntohs
								      (auth->
								       status)));
				}
				IWI_LOG(					  "DEauthenticated: '%s' %02x:%02x:%02x:%02x:%02x:%02x\n",
					  escape_essid((const char*)priv->essid,
						       priv->essid_len),
					  MAC_ARG(priv->bssid));

				priv->status &= ~(STATUS_ASSOCIATING |
						  STATUS_AUTH |
						  STATUS_ASSOCIATED);

				ipw_link_down(priv);
				break;

			case CMAS_TX_AUTH_SEQ_1:
				IWI_DEBUG("AUTH_SEQ_1\n");
				break;
			case CMAS_RX_AUTH_SEQ_2:
				IWI_DEBUG("AUTH_SEQ_2\n");
				break;
			case CMAS_AUTH_SEQ_1_PASS:
				IWI_DEBUG("AUTH_SEQ_1_PASS\n");
				break;
			case CMAS_AUTH_SEQ_1_FAIL:
				IWI_DEBUG("AUTH_SEQ_1_FAIL\n");
				break;
			case CMAS_TX_AUTH_SEQ_3:
				IWI_DEBUG("AUTH_SEQ_3\n");
				break;
			case CMAS_RX_AUTH_SEQ_4:
				IWI_DEBUG("RX_AUTH_SEQ_4\n");
				break;
			case CMAS_AUTH_SEQ_2_PASS:
				IWI_DEBUG("AUTH_SEQ_2_PASS\n");
				break;
			case CMAS_AUTH_SEQ_2_FAIL:
				IWI_DEBUG("AUT_SEQ_2_FAIL\n");
				break;
			case CMAS_TX_ASSOC:
				IWI_DEBUG("TX_ASSOC\n");
				break;
			case CMAS_RX_ASSOC_RESP:
				IWI_DEBUG("RX_ASSOC_RESP\n");

				break;
			case CMAS_ASSOCIATED:
				IWI_DEBUG("ASSOCIATED\n");
				break;
			default:
				IWI_DEBUG("auth: failure - %d\n",
						auth->state);
				break;
			}
			break;
		}

	case HOST_NOTIFICATION_STATUS_SCAN_CHANNEL_RESULT:{
			struct notif_channel_result *x =
			    &notif->u.channel_result;

			if (notif->size == sizeof(*x)) {
				IWI_DEBUG("Scan result for channel %d\n",
					       x->channel_num);
			} else {
				IWI_DEBUG("Scan result of wrong size %d "
					       "(should be %zd)\n",
					       notif->size, sizeof(*x));
			}
			break;
		}

	case HOST_NOTIFICATION_STATUS_SCAN_COMPLETED:{
			struct notif_scan_complete *x = &notif->u.scan_complete;
			if (notif->size == sizeof(*x)) {
				IWI_LOG
				    ("Scan completed: %d type %d, %d channels, "
				     "%d status\n", priv->ieee->scans++, x->scan_type,
				     x->num_channels, x->status);
			} else {
				IWI_DEBUG("Scan completed of wrong size %d "
					  "(should be %zd)\n",
					  notif->size, sizeof(*x));
			}

			priv->status &=
			    ~(STATUS_SCANNING | STATUS_SCAN_ABORTING);

			//wake_up_interruptible(&priv->wait_state);
			queue_td(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan));
			queue_td(4,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan_check));
			
			if (priv->status & STATUS_EXIT_PENDING)
				break;

			
			//priv->ieee->scans++;
#ifdef CONFIG_IPW2200_MONITOR			
			if (priv->ieee->iw_mode == IW_MODE_MONITOR) {
				priv->status |= STATUS_SCAN_FORCED;
				queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,3,true);
				break;
			}
			priv->status &= ~STATUS_SCAN_FORCED;

#endif				/* CONFIG_IPW2200_MONITOR */			
			if (!(priv->status & (STATUS_ASSOCIATED |
					      STATUS_ASSOCIATING |
					      STATUS_ROAMING |
					      STATUS_DISASSOCIATING)))
				queue_te(5,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_associate),priv,NULL,true);
			else if (priv->status & STATUS_ROAMING) {
				if (x->status == SCAN_COMPLETED_STATUS_COMPLETE)
					/* If a scan completed and we are in roam mode, then
					 * the scan that completed was the one requested as a
					 * result of entering roam... so, schedule the
					 * roam work */
					//queue_work(priv->workqueue,  &priv->roam);
					IWI_DEBUG("TODO: roam\n");
				else
					/* Don't schedule if we aborted the scan */
					priv->status &= ~STATUS_ROAMING;
			} else if (priv->status & STATUS_SCAN_PENDING)
				queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,3,true);
			else if (priv->config & CFG_BACKGROUND_SCAN
				 && priv->status & STATUS_ASSOCIATED)
				queue_te(0,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_scan),priv,3,true);

			if (x->status == SCAN_COMPLETED_STATUS_COMPLETE) {
				/*
				union iwreq_data wrqu;

				wrqu.data.length = 0;
				wrqu.data.flags = 0;
				//wireless_send_event(priv->net_dev, SIOCGIWSCAN, &wrqu, NULL);
				ieee80211_wx_get_scan(priv->ieee, info, wrqu, extra);*/
			}
			break;
		}

	case HOST_NOTIFICATION_STATUS_FRAG_LENGTH:{
			struct notif_frag_length *x = &notif->u.frag_len;

			if (notif->size == sizeof(*x))
				IWI_DEBUG_FULL("Frag length: %d\n",
					  le16_to_cpu(x->frag_length));
			else
				IWI_DEBUG_FULL("Frag length of wrong size %d "
					  "(should be %zd)\n",
					  notif->size, sizeof(*x));
			break;
		}

	case HOST_NOTIFICATION_STATUS_LINK_DETERIORATION:{
			struct notif_link_deterioration *x =
			    &notif->u.link_deterioration;

			if (notif->size == sizeof(*x)) {
				IWI_DEBUG_FULL(					  "link deterioration: type %d, cnt %d\n",
					  x->silence_notification_type,
					  x->silence_count);
				memcpy(&priv->last_link_deterioration, x,
				       sizeof(*x));
			} else {
				IWI_DEBUG_FULL("Link Deterioration of wrong size %d "
					  "(should be %zd)\n",
					  notif->size, sizeof(*x));
			}
			break;
		}

	case HOST_NOTIFICATION_DINO_CONFIG_RESPONSE:{
			IWI_DEBUG("Dino config\n");
			if (priv->hcmd
			    && priv->hcmd->cmd != HOST_CMD_DINO_CONFIG)
				IWI_DEBUG("Unexpected DINO_CONFIG_RESPONSE\n");

			break;
		}

	case HOST_NOTIFICATION_STATUS_BEACON_STATE:{
			struct notif_beacon_state *x = &notif->u.beacon_state;
			if (notif->size != sizeof(*x)) {
				IWI_DEBUG
				    ("Beacon state of wrong size %d (should "
				     "be %zd)\n", notif->size, sizeof(*x));
				break;
			}
			//IWI_DEBUG("ipw_handle_missed_beacon\n");
			if (le32_to_cpu(x->state) ==
			    HOST_NOTIFICATION_STATUS_BEACON_MISSING)
				ipw_handle_missed_beacon(priv, le32_to_cpu(x->number));

			break;
		}

	case HOST_NOTIFICATION_STATUS_TGI_TX_KEY:{
			struct notif_tgi_tx_key *x = &notif->u.tgi_tx_key;
			if (notif->size == sizeof(*x)) {
				IWI_DEBUG("TGi Tx Key: state 0x%02x sec type "
					  "0x%02x station %d\n",
					  x->key_state, x->security_type,
					  x->station_index);
				break;
			}

			IWI_DEBUG
			    ("TGi Tx Key of wrong size %d (should be %zd)\n",
			     notif->size, sizeof(*x));
			break;
		}

	case HOST_NOTIFICATION_CALIB_KEEP_RESULTS:{
			struct notif_calibration *x = &notif->u.calibration;
			if (notif->size == sizeof(*x)) {
				memcpy(&priv->calib, x, sizeof(*x));
				IWI_DEBUG("TODO: Calibration\n");
				break;
			}

			IWI_DEBUG
			    ("Calibration of wrong size %d (should be %zd)\n",
			     notif->size, sizeof(*x));
			break;
		}

	case HOST_NOTIFICATION_NOISE_STATS:{
			IWI_DEBUG_FULL("HOST_NOTIFICATION_NOISE_STATS\n");
			if (notif->size == sizeof(u32)) {
				priv->exp_avg_noise =exponential_average(priv->exp_avg_noise,(u8) (le32_to_cpu(notif->u.noise.value) & 0xff),DEPTH_NOISE);
				break;
			}

			IWI_DEBUG_FULL
			    ("Noise stat is wrong size %d (should be %zd)\n",
			     notif->size, sizeof(u32));
			break;
		}

	default:
		IWI_DEBUG_FULL("Unknown notification: "
				"subtype=%d,flags=0x%2x,size=%d\n",
				notif->subtype, notif->flags, notif->size);
	}
}

int darwin_iwi2200::ipw_get_tx_queue_number(struct ipw_priv *priv, u16 priority)
{
	if (priority > 7 || !priv->qos_data.qos_enable)
		priority = 0;

	return from_priority_to_tx_queue[priority] - 1;
}



int darwin_iwi2200::ipw_net_is_queue_full(struct net_device *dev, int pri)
{
#ifdef CONFIG_IPW2200_QOS
	int tx_id = ipw_get_tx_queue_number(priv, pri);
	struct clx2_tx_queue *txq = &priv->txq[tx_id];
#else
	struct clx2_tx_queue *txq = &priv->txq[0];
#endif				/* CONFIG_IPW2200_QOS */

	if (ipw_queue_space(&txq->q) < txq->q.high_mark)
		return 1;

	return 0;
}

u8 darwin_iwi2200::ipw_find_station(struct ipw_priv *priv, u8 * bssid)
{
	int i;

	for (i = 0; i < priv->num_stations; i++)
		if (!memcmp(priv->stations[i], bssid, ETH_ALEN))
			return i;

	return IPW_INVALID_STATION;
}

struct ieee80211_txb *darwin_iwi2200::ieee80211_alloc_txb(int nr_frags, int txb_size,
						 int headroom, int gfp_mask)
{
	struct ieee80211_txb *txb;
	int i;
	(void*)txb = kmalloc(sizeof(struct ieee80211_txb) + (sizeof(u8 *) * nr_frags),
		      gfp_mask);
	if (!txb)
		return NULL;

	memset(txb, 0, sizeof(struct ieee80211_txb));
	txb->nr_frags = nr_frags;
	txb->frag_size = txb_size;

	for (i = 0; i < nr_frags; i++) {
		txb->fragments[i] = alloc_skb(txb_size + headroom,gfp_mask);
		//__dev_alloc_skb(txb_size + headroom,						    gfp_mask);
		if (unlikely(!txb->fragments[i])) {
			i--;
			break;
		}
		// default m_len is alocated size in mbuf
		// must set 0 m_len , pkthdr.len . 
		//mbuf_setlen(txb->fragments[i],0);
		//mbuf_pkthdr_setlen(txb->fragments[i],0);

		skb_reserve(txb->fragments[i], headroom);
		// fix me: should check later
		//mbuf_setlen(txb->fragments[i], headroom);
		//mbuf_pkthdr_setlen(txb->fragments[i],headroom);
	}
	if (unlikely(i != nr_frags)) {
		while (i >= 0)
		{
			i--;
			if (txb->fragments[i]!=NULL){
				if (!(txb->fragments[i]) == MBUF_TYPE_FREE) freePacket(txb->fragments[i]);
				 txb->fragments[i]=NULL;
			}
			//txb->fragments[i--]=NULL;
		//	dev_kfree_skb_any(txb->fragments[i--]);
		}
		kfree(txb);
		txb=NULL;
		return NULL;
	}
	return txb;
}

int darwin_iwi2200::ipw_is_qos_active(struct net_device *dev, mbuf_t skb)
{
	//struct ipw_priv *priv = ieee80211_priv(dev);
	struct ieee80211_qos_data *qos_data = NULL;
	int active, supported;
	u8 *daddr = ((UInt8*)mbuf_data(skb) + ETH_ALEN);
	int unicast = !ipw_is_multicast_ether_addr(daddr);
	
	if (!(priv->status & STATUS_ASSOCIATED))
		return 0;

	qos_data = &priv->assoc_network->qos_data;

	if (priv->ieee->iw_mode == IW_MODE_ADHOC) {
		if (unicast == 0)
			qos_data->active = 0;
		else
			qos_data->active = qos_data->supported;
	}
	active = qos_data->active;
	supported = qos_data->supported;
	IWI_DEBUG("QoS  %d network is QoS active %d  supported %d  "
		      "unicast %d\n",
		      priv->qos_data.qos_enable, active, supported, unicast);
	if (active && priv->qos_data.qos_enable)
		return 1;

	return 0;

}

int darwin_iwi2200::ipw_net_hard_start_xmit(struct ieee80211_txb *txb,
				   struct net_device *dev, int pri)
{
	//struct ipw_priv *priv = ieee80211_priv(dev);
	unsigned long flags;
	int ret;
	//IOInterruptState	instate;

	IWI_DEBUG("dev->xmit(%d bytes)\n", txb->payload_size);
	//spin_lock_irqsave(&priv->lock, flags);
	// this is checking of race conditions
	if (test_lock > 0) {
		IWI_WARNING("race condition\n");
		return kIOReturnOutputStall;
	}
	test_lock++;
	//instate = IOSimpleLockLockDisableInterrupt( spin);
	
	if (!(priv->status & STATUS_ASSOCIATED)) {
		IWI_ERR("Tx attempt while not associated.\n");
		priv->ieee->stats.tx_carrier_errors++;
		//netif_stop_queue(dev);
		fTransmitQueue->stop();
		// fix me:  flush is not requuired?
		//fTransmitQueue->flush();
		goto fail_unlock;
	}
	// check after.
	// fix me : ipw_tx_skb is not require?
	//            the reason of copyed mbuf at first?
	//            set a like a frame_length in ipw_tx_skb... 
#if 1	/* enable by xkazu  in test */
	if (txb->payload_size==0) goto fail_unlock;
	
	ret = ipw_tx_skb(priv, txb, pri);
#else	/* javaloui's routine */
	struct clx2_tx_queue *txq = &priv->txq[0];
	struct clx2_queue *q = &txq->q;
	txq->txb[q->first_empty] = txb;
	q->first_empty = ipw_queue_inc_wrap(q->first_empty, q->n_bd);
	ipw_write32( q->reg_w, q->first_empty);

	if (ipw_queue_space(q) < q->high_mark){
		IWI_DEBUG("ipw_queue_space(q) < q->high_mark \n");
		// FIXME
		//fTransmitQueue->stop();
		//fTransmitQueue->flush();
	}
	//	netif_stop_queue(priv->net_dev);
	__ipw_led_activity_on(priv);
	return kIOReturnOutputSuccess;//NETDEV_TX_OK;
#endif	
	if (ret == kIOReturnOutputSuccess)//NETDEV_TX_OK)
		__ipw_led_activity_on(priv);
	//spin_unlock_irqrestore(&priv->lock, flags);
	//IOSimpleLockUnlockEnableInterrupt( spin, instate );
	test_lock--;
	
	return ret;

      fail_unlock:
	//spin_unlock_irqrestore(&priv->lock, flags);
	//IOSimpleLockUnlockEnableInterrupt( spin, instate );
	test_lock--;
	return kIOReturnOutputStall;
}

int darwin_iwi2200::ieee80211_xmit(mbuf_t skb, struct net_device *dev)
{
	struct ieee80211_device *ieee = priv->ieee;//netdev_priv(dev);
	struct ieee80211_txb *txb = NULL;
	struct ieee80211_hdr_3addrqos *frag_hdr;
	int i, bytes_per_frag, nr_frags, bytes_last_frag, frag_size,
	    rts_required;
	unsigned long flags;
	struct net_device_stats *stats = &ieee->stats;
	int ether_type, encrypt, host_encrypt, host_encrypt_msdu, host_build_iv;
	int bytes, fc, hdr_len;
	mbuf_t skb_frag;
	struct ieee80211_hdr_3addrqos header;/* Ensure zero initialized */
		header.duration_id = 0;
		header.seq_ctl = 0;
		header.qos_ctl = 0;
	u8 dest[ETH_ALEN], src[ETH_ALEN];
	struct ieee80211_crypt_data *crypt;
	int priority = 0;//skb->priority;
	int snapped = 0;
          int call_count=0;
	int ret;	  
		  
		  
	IWI_DEBUG_FN("%d \n",call_count++);
	if (/* ieee->is_queue_full  && */ ipw_net_is_queue_full(dev, priority))
	{
		IWI_WARNING( " tx queue is full \n");
		return kIOReturnOutputStall;//NETDEV_TX_BUSY;
	}
	//spin_lock_irqsave(&ieee->lock, flags);
	/* If there is no driver handler to take the TXB, dont' bother
	 * creating it... */
	/*if (!ieee->hard_start_xmit) {
		printk(KERN_WARNING "%s: No xmit handler.\n", ieee->dev->name);
		goto success;
	}*/

	if (unlikely(mbuf_len(skb) < SNAP_SIZE + sizeof(u16))) {
		IWI_DEBUG_FULL( "%s: skb too small (%d).\n",
		       ieee->dev->name, mbuf_len(skb));
		goto success;
	}
	
	ether_type = ntohs(((struct ethhdr *)(mbuf_data(skb)))->h_proto);

	crypt = ieee->crypt[ieee->tx_keyidx];

	encrypt = !(ether_type == ETH_P_PAE && ieee->ieee802_1x) &&
	    ieee->sec.encrypt;

	host_encrypt = ieee->host_encrypt && encrypt && crypt;
	host_encrypt_msdu = ieee->host_encrypt_msdu && encrypt && crypt;
	host_build_iv = ieee->host_build_iv && encrypt && crypt;

	if (!encrypt && ieee->ieee802_1x &&
	    ieee->drop_unencrypted && ether_type != ETH_P_PAE) {
		stats->tx_dropped++;
		goto success;
	}

	/* Save source and destination addresses */
	memcpy(dest, mbuf_data(skb), ETH_ALEN);
	memcpy(src, ((UInt8*)mbuf_data(skb) + ETH_ALEN), ETH_ALEN);

	if (host_encrypt || host_build_iv)
		fc = IEEE80211_FTYPE_DATA | IEEE80211_STYPE_DATA |
		    IEEE80211_FCTL_PROTECTED;
	else
		fc = IEEE80211_FTYPE_DATA | IEEE80211_STYPE_DATA;

	if (ieee->iw_mode == IW_MODE_INFRA) {
		fc |= IEEE80211_FCTL_TODS;
		/* To DS: Addr1 = BSSID, Addr2 = SA, Addr3 = DA */
		memcpy(header.addr1, ieee->bssid, ETH_ALEN);
		memcpy(header.addr2, src, ETH_ALEN);
		memcpy(header.addr3, dest, ETH_ALEN);
	} else if (ieee->iw_mode == IW_MODE_ADHOC) {
		/* not From/To DS: Addr1 = DA, Addr2 = SA, Addr3 = BSSID */
		memcpy(header.addr1, dest, ETH_ALEN);
		memcpy(header.addr2, src, ETH_ALEN);
		memcpy(header.addr3, ieee->bssid, ETH_ALEN);
	}
	hdr_len = IEEE80211_3ADDR_LEN;

	if (qos_enable)
	if (ieee->is_qos_active && ipw_is_qos_active(dev, skb)) {
		fc |= IEEE80211_STYPE_QOS_DATA;
		hdr_len += 2;

		//skb->priority = ieee80211_classify(skb);
		//header.qos_ctl |= cpu_to_le16(skb->priority & IEEE80211_QCTL_TID);
	}
	header.frame_ctl = cpu_to_le16(fc);
	
	/* Advance the SKB to the start of the payload */
	skb_pull(skb, sizeof(struct ethhdr));
	
	//mbuf_adj(skb, sizeof(struct ethhdr));
	/* Determine total amount of storage required for TXB packets */
	// fix me : mbuf_pkthdr_len ?
	bytes = mbuf_len(skb) + SNAP_SIZE + sizeof(u16);
	//bytes = mbuf_pkthdr_len(skb) + SNAP_SIZE + sizeof(u16);

	/* Encrypt msdu first on the whole data packet. */
	if ((host_encrypt || host_encrypt_msdu) &&
	    crypt && crypt->ops && crypt->ops->encrypt_msdu) {
		IWI_DEBUG_FN("BUG dont support encrypt");
		goto failed;
		int res = 0;
		int len = bytes + hdr_len + crypt->ops->extra_msdu_prefix_len +
		    crypt->ops->extra_msdu_postfix_len;
		mbuf_t skb_new = alloc_skb(len,0);

		if (unlikely(!skb_new))
			goto failed;

		skb_reserve(skb_new, crypt->ops->extra_msdu_prefix_len);
		//mbuf_setlen(skb_new, crypt->ops->extra_msdu_prefix_len);
		memcpy(skb_put(skb_new, hdr_len), &header, hdr_len);
		//memcpy(((UInt8*)mbuf_data(skb_new)+hdr_len), &header, hdr_len);
		snapped = 1;
		ieee80211_copy_snap((u8*)skb_put(skb_new, SNAP_SIZE + sizeof(u16)),
		//ieee80211_copy_snap(((UInt8*)mbuf_data(skb_new) +SNAP_SIZE + sizeof(u16)),
				    ether_type);
		memcpy(skb_put(skb_new, mbuf_len(skb)), mbuf_data(skb), mbuf_len(skb));
		//memcpy(((UInt8*)mbuf_data(skb_new)+mbuf_len(skb)), mbuf_data(skb), mbuf_len(skb));
		IWI_DEBUG("TODO: msdu encryption\n");
		res = -1;//crypt->ops->encrypt_msdu(skb_new, hdr_len, crypt->priv);
		if (res < 0) {
			IWI_DEBUG("msdu encryption failed\n");
			//dev_kfree_skb_any(skb_new);
			//mbuf_freem_list(skb);
			if (skb_new!=NULL) 
			if (!(skb_new) == MBUF_TYPE_FREE) freePacket(skb_new);
			skb_new=NULL;
			goto failed;
		}
		//dev_kfree_skb_any(skb);
		 if (skb!=NULL) 
		 if (!(skb) == MBUF_TYPE_FREE) freePacket(skb);
		skb=NULL;
		
		skb = skb_new;
		bytes += crypt->ops->extra_msdu_prefix_len +
		    crypt->ops->extra_msdu_postfix_len;
		skb_pull(skb, hdr_len);
		//mbuf_adj(skb, hdr_len);
	}

	if (host_encrypt || ieee->host_open_frag) {
		/* Determine fragmentation size based on destination (multicast
		 * and broadcast are not fragmented) */
		if (is_multicast_ether_addr(dest) ||
		    is_broadcast_ether_addr(dest))
			frag_size = MAX_FRAG_THRESHOLD;
		else
			frag_size = ieee->fts;

		/* Determine amount of payload per fragment.  Regardless of if
		 * this stack is providing the full 802.11 header, one will
		 * eventually be affixed to this fragment -- so we must account
		 * for it when determining the amount of payload space. */
		bytes_per_frag = frag_size - IEEE80211_3ADDR_LEN;
		if (ieee->config &
		    (CFG_IEEE80211_COMPUTE_FCS | CFG_IEEE80211_RESERVE_FCS))
			bytes_per_frag -= IEEE80211_FCS_LEN;

		/* Each fragment may need to have room for encryptiong
		 * pre/postfix */
		if (host_encrypt)
			bytes_per_frag -= crypt->ops->extra_mpdu_prefix_len +
			    crypt->ops->extra_mpdu_postfix_len;

		/* Number of fragments is the total
		 * bytes_per_frag / payload_per_fragment */
		nr_frags = bytes / bytes_per_frag;
		bytes_last_frag = bytes % bytes_per_frag;
		if (bytes_last_frag)
			nr_frags++;
		else
			bytes_last_frag = bytes_per_frag;
	} else {
		nr_frags = 1;
		bytes_per_frag = bytes_last_frag = bytes;
		frag_size = bytes + IEEE80211_3ADDR_LEN;
	}

	rts_required = (frag_size > ieee->rts
			&& ieee->config & CFG_IEEE80211_RTS);
	if (rts_required)
		nr_frags++;
	
	/* When we allocate the TXB we allocate enough space for the reserve
	 * and full fragment bytes (bytes_per_frag doesn't include prefix,
	 * postfix, header, FCS, etc.) */
	
	txb = ieee80211_alloc_txb(nr_frags, frag_size,
				  ieee->tx_headroom, NULL);
	
	if (unlikely(!txb)) {
		IWI_DEBUG( "%s: Could not allocate TXB\n",
		       ieee->dev->name);
		goto failed;
	}
	
	txb->encrypted = encrypt;
	if (host_encrypt)
		txb->payload_size = frag_size * (nr_frags - 1) +
		    bytes_last_frag;
	else
		txb->payload_size = bytes;

	if (rts_required) {
		skb_frag = txb->fragments[0];
		//frag_hdr =(struct ieee80211_hdr_3addrqos *)((UInt8*)mbuf_data(skb_frag)+hdr_len);
		 frag_hdr =  (struct ieee80211_hdr_3addrqos *)skb_put(skb_frag, hdr_len);

		/*
		 * Set header frame_ctl to the RTS.
		 */
		header.frame_ctl =
		    cpu_to_le16(IEEE80211_FTYPE_CTL | IEEE80211_STYPE_RTS);
		memcpy(frag_hdr, &header, hdr_len);

		/*
		 * Restore header frame_ctl to the original data setting.
		 */
		header.frame_ctl = cpu_to_le16(fc);

		if (ieee->config &
		   (CFG_IEEE80211_COMPUTE_FCS | CFG_IEEE80211_RESERVE_FCS))
		   //mbuf_adj(skb_frag, 4);
		       skb_put(skb_frag, 4);

		txb->rts_included = 1;
		i = 1;
	} else
		i = 0;
	
	
	for (; i < nr_frags; i++) {
	
		skb_frag = txb->fragments[i];

		if (host_encrypt || host_build_iv)
			skb_reserve(skb_frag,
				crypt->ops->extra_mpdu_prefix_len);
			//mbuf_setlen(skb_frag,crypt->ops->extra_mpdu_prefix_len);
			//frag_hdr =(struct ieee80211_hdr_3addrqos *)((UInt8*)mbuf_data(skb_frag)+hdr_len);
		frag_hdr  = (struct ieee80211_hdr_3addrqos *)skb_put(skb_frag, hdr_len);
		memcpy(frag_hdr, &header, hdr_len);
		
		IWI_DEBUG_FULL("src " MAC_FMT "desc " MAC_FMT  " bssid " MAC_FMT "\n" ,  
			MAC_ARG(frag_hdr->addr2), MAC_ARG(frag_hdr->addr3) , MAC_ARG(frag_hdr->addr1)  );
		/* If this is not the last fragment, then add the MOREFRAGS
		 * bit to the frame control */
		if (i != nr_frags - 1) {
			frag_hdr->frame_ctl =
			    cpu_to_le16(fc | IEEE80211_FCTL_MOREFRAGS);
			bytes = bytes_per_frag;
		} else {
			/* The last fragment takes the remaining length */
			bytes = bytes_last_frag;
		}
	
	
		if (i == 0 && !snapped) {
			//ieee80211_copy_snap(((UInt8*)mbuf_data(skb_frag)+ SNAP_SIZE + sizeof(u16)),
			ieee80211_copy_snap((u8*)skb_put
						 (skb_frag, SNAP_SIZE + sizeof(u16)),
						ether_type);
			bytes -= SNAP_SIZE + sizeof(u16);
		}
		
		IWI_DUMP_MBUF(3,skb,bytes); 
		IWI_DUMP_MBUF(4,skb_frag,bytes);
		if ( mbuf_trailingspace(skb_frag) < bytes  ) {
			IWI_DEBUG("freespace is not enough.\n");
			goto failed; // kazu test
		}
		// FIXME: this routine only copy first mbuf in changes
		//            when mbuf with n_next , must copy next mbuf in chains? 
		memcpy(skb_put(skb_frag, bytes), mbuf_data(skb), bytes);
		
		//memcpy(((UInt8*)mbuf_data(skb_frag)+bytes), mbuf_data(skb), bytes);
		/* Advance the SKB... */
		skb_pull(skb, bytes);
		//mbuf_adj(skb, bytes);
		/* Encryption routine will move the header forward in order
		 * to insert the IV between the header and the payload */
		 
#if 0 /* fix me: encrption */		 
		if (host_encrypt)
			ieee80211_encrypt_fragment(ieee, skb_frag, hdr_len);
		else if (host_build_iv) {
			struct ieee80211_crypt_data *crypt;

			crypt = ieee->crypt[ieee->tx_keyidx];
			atomic_inc(&crypt->refcnt);
			if (crypt->ops->build_iv)
				crypt->ops->build_iv(skb_frag, hdr_len,
				      ieee->sec.keys[ieee->sec.active_key],
				      ieee->sec.key_sizes[ieee->sec.active_key],
				      crypt->priv);
			atomic_dec(&crypt->refcnt);
		}
#endif
		if (ieee->config &
		   (CFG_IEEE80211_COMPUTE_FCS | CFG_IEEE80211_RESERVE_FCS))
			//mbuf_adj(skb_frag, 4);
			skb_put(skb_frag, 4);
	}

      success:
	//spin_unlock_irqrestore(&ieee->lock, flags);
	//dev_kfree_skb_any(skb);
	//skb=NULL;
	if (skb!=NULL) 
	{
	     if (!(skb) == MBUF_TYPE_FREE) freePacket(skb);
               skb=NULL;
	}
	if (txb) {
		//int ret = ipw_tx_skb(priv,txb, priority);
		// test comment out.
		ret=ipw_net_hard_start_xmit(txb,dev,priority); 
		
		
		if (ret == kIOReturnOutputSuccess) {
			stats->tx_packets++;
			stats->tx_bytes += txb->payload_size;
			return kIOReturnOutputSuccess;
		}

		ieee80211_txb_free(txb);
	}

	return ret;

      failed:
	//spin_unlock_irqrestore(&ieee->lock, flags);
	//netif_stop_queue(dev);
	IWI_DEBUG_FULL("TX drop\n");
	//fTransmitQueue->stop();
	//fTransmitQueue->flush();
	stats->tx_errors++;
	return kIOReturnOutputDropped;
}

int darwin_iwi2200::ipw_tx_skb(struct ipw_priv *priv, struct ieee80211_txb *txb, int pri)
{
	struct ieee80211_hdr_3addrqos *hdr = (struct ieee80211_hdr_3addrqos*)(mbuf_data(txb->fragments[0]));
	int i = 0;
	struct tfd_frame *tfd;
#ifdef CONFIG_IPW2200_QOS
	int tx_id = ipw_get_tx_queue_number(priv,pri);
	struct clx2_tx_queue *txq = &priv->txq[tx_id];
#else
	struct clx2_tx_queue *txq = &priv->txq[0];
#endif
	struct clx2_queue *q = &txq->q;
	u8 id, hdr_len, unicast;
	u16 remaining_bytes;
	int fc;

	hdr_len = ieee80211_get_hdrlen(le16_to_cpu(hdr->frame_ctl));
	switch (priv->ieee->iw_mode) {
	case IW_MODE_ADHOC:
		unicast = !ipw_is_multicast_ether_addr(hdr->addr1);
		id = ipw_find_station(priv, hdr->addr1);
		if (id == IPW_INVALID_STATION) {
			id = ipw_add_station(priv, hdr->addr1);
			if (id == IPW_INVALID_STATION) {
				IWI_WARNING("Attempt to send data to "
					    "invalid cell: " MAC_FMT "\n",
					    MAC_ARG(hdr->addr1));
				goto drop;
			}
		}
		break;

	case IW_MODE_INFRA:
	default:
		unicast = !ipw_is_multicast_ether_addr(hdr->addr3);
		id = 0;
		break;
	}

	tfd = &txq->bd[q->first_empty];
	txq->txb[q->first_empty] = txb;
	memset(tfd, 0, sizeof(*tfd));
	tfd->u.data.station_number = id;

	tfd->control_flags.message_type = TX_FRAME_TYPE;
	tfd->control_flags.control_bits = TFD_NEED_IRQ_MASK;

	tfd->u.data.cmd_id = DINO_CMD_TX;
	tfd->u.data.len = cpu_to_le16(txb->payload_size);
	remaining_bytes = txb->payload_size;

	if (priv->assoc_request.ieee_mode == IPW_B_MODE)
		tfd->u.data.tx_flags_ext |= DCT_FLAG_EXT_MODE_CCK;
	else
		tfd->u.data.tx_flags_ext |= DCT_FLAG_EXT_MODE_OFDM;

	if (priv->assoc_request.preamble_length == DCT_FLAG_SHORT_PREAMBLE)
		tfd->u.data.tx_flags |= DCT_FLAG_SHORT_PREAMBLE;

	fc = le16_to_cpu(hdr->frame_ctl);
	hdr->frame_ctl = cpu_to_le16(fc & ~IEEE80211_FCTL_MOREFRAGS);

	memcpy(&tfd->u.data.tfd.tfd_24.mchdr, hdr, hdr_len);

	if (!unlikely(unicast))
		tfd->u.data.tx_flags |= DCT_FLAG_ACK_REQD;

	if (txb->encrypted && !priv->ieee->host_encrypt) {
		switch (priv->ieee->sec.level) {
		case SEC_LEVEL_3:
			tfd->u.data.tfd.tfd_24.mchdr.frame_ctl |=
			    cpu_to_le16(IEEE80211_FCTL_PROTECTED);
			/* XXX: ACK flag must be set for CCMP even if it
			 * is a multicast/broadcast packet, because CCMP
			 * group communication encrypted by GTK is
			 * actually done by the AP. */
			if (!unicast)
				tfd->u.data.tx_flags |= DCT_FLAG_ACK_REQD;

			tfd->u.data.tx_flags &= ~DCT_FLAG_NO_WEP;
			tfd->u.data.tx_flags_ext |= DCT_FLAG_EXT_SECURITY_CCM;
			tfd->u.data.key_index = 0;
			tfd->u.data.key_index |= DCT_WEP_INDEX_USE_IMMEDIATE;
			break;
		case SEC_LEVEL_2:
			tfd->u.data.tfd.tfd_24.mchdr.frame_ctl |=
			    cpu_to_le16(IEEE80211_FCTL_PROTECTED);
			tfd->u.data.tx_flags &= ~DCT_FLAG_NO_WEP;
			tfd->u.data.tx_flags_ext |= DCT_FLAG_EXT_SECURITY_TKIP;
			tfd->u.data.key_index = DCT_WEP_INDEX_USE_IMMEDIATE;
			break;
		case SEC_LEVEL_1:
			tfd->u.data.tfd.tfd_24.mchdr.frame_ctl |=
			    cpu_to_le16(IEEE80211_FCTL_PROTECTED);
			tfd->u.data.key_index = priv->ieee->tx_keyidx;
			if (priv->ieee->sec.key_sizes[priv->ieee->tx_keyidx] <=
			    40)
				tfd->u.data.key_index |= DCT_WEP_KEY_64Bit;
			else
				tfd->u.data.key_index |= DCT_WEP_KEY_128Bit;
			break;
		case SEC_LEVEL_0:
			break;
		default:
			IWI_DEBUG("Unknow security level %d\n",
			       priv->ieee->sec.level);
			break;
		}
	} else
		/* No hardware encryption */
		tfd->u.data.tx_flags |= DCT_FLAG_NO_WEP;

#ifdef CONFIG_IPW2200_QOS
	if (qos_enable)
	if (fc & IEEE80211_STYPE_QOS_DATA)
		ipw_qos_set_tx_queue_command(priv, pri, &(tfd->u.data));
#endif				/* CONFIG_IPW2200_QOS */
	//goto kick;
	/* payload */
	tfd->u.data.num_chunks = cpu_to_le32(min((u8) (NUM_TFD_CHUNKS - 2),
						 txb->nr_frags));
	IWI_DEBUG("%d fragments being sent as %d chunks.\n",
		       txb->nr_frags, le32_to_cpu(tfd->u.data.num_chunks));
			   
	for (i = 0; i < le32_to_cpu(tfd->u.data.num_chunks); i++) {
		IWI_DEBUG("Adding fragment %d of %d (%d bytes).\n",
			       i, le32_to_cpu(tfd->u.data.num_chunks),
			       mbuf_len(txb->fragments[i]) - hdr_len);
		IWI_DEBUG("Dumping TX packet frag %d of %d (%d bytes):\n",
			     i, tfd->u.data.num_chunks,
			     mbuf_len(txb->fragments[i]) - hdr_len);
	/*
		IWI_DEBUG(IPW_DL_TX, txb->fragments[i]->data + hdr_len,
			   mbuf_len(txb->fragments[i]) - hdr_len);
	*/		   
		// fix me: this should be chcked ?
		
		
#if 0		
		struct IOPhysicalSegment vector;
		IOMbufLittleMemoryCursor *_mbufCursor;
		
		_mbufCursor = IOMbufLittleMemoryCursor::withSpecification(3000, 1);
		//mbuf_adj(txb->fragments[i],hdr_len);
		int rm = _mbufCursor->getPhysicalSegmentsWithCoalesce(txb->fragments[i], &vector, 1);
		if (rm!=0 && (mbuf_len(txb->fragments[i]) > hdr_len))
		{
		    IWI_DEBUG_FULL(" pkt_len %d vector.len %d \n",mbuf_pkthdr_len(txb->fragments[i]),vector.length);
		    tfd->u.data.chunk_ptr[i] = vector.location;
		    tfd->u.data.chunk_len[i] = vector.length; 
		    //  tfd->u.data.chunk_len[i] = cpu_to_le16(txb->fragments[i]->len - hdr_len); 
		}
#else		
		// no mbuf cursor version by xkazu
		//mbuf_adj(txb->fragments[i],hdr_len);
		//
		mbuf_t mn;
		UInt8 *             des_ptr_ub;
		
		//long pkt_len = mbuf_pkthdr_len(txb->fragments[i])
		for ( mn = txb->fragments[i]; mn; mn = mbuf_next(mn) )
		{
			if(  mn = txb->fragments[i] ){
				IWI_DEBUG_FULL("kick only one\n");
				mbuf_adj(txb->fragments[i],hdr_len);
				IWI_DEBUG("txb->fragments[%d] = %08x \n",i ,cpu_to_le32(
								mbuf_data_to_physical(
									mbuf_data(txb->fragments[i]) ) ) );
				// FIXME: this routine is not required					
				if (  cpu_to_le16(mbuf_pkthdr_len(txb->fragments[i]) )  > 2048) {
					IWI_ERR("output packet > 2048\n");
					goto drop;
				}					
				tfd->u.data.chunk_ptr[i] =
					cpu_to_le32(mbuf_data_to_physical
					(mbuf_data(txb->fragments[i])));
				IWI_DEBUG(" pkt_len %d mbuf_len %d \n",
					mbuf_pkthdr_len(txb->fragments[i]), mbuf_len(txb->fragments[i]) );
				tfd->u.data.chunk_len[i] = cpu_to_le16(mbuf_pkthdr_len(txb->fragments[i]) );
				skb_push(txb->fragments[i],hdr_len);
			}else{
				IWI_ERR("BUG: mbuf is not single but chains\n mbuf_len(%d) mbuf_pkthdr_len(%d)\n",
					mbuf_len(txb->fragments[i]), mbuf_pkthdr_len(txb->fragments[i])
				);
				goto frg;
				//bcopy( mbuf_data(mn)
			}
		}
		    //cpu_to_le32( 
			/*pci_map_single
				(priv->pci_dev,
				 txb->fragments[i]->data + hdr_len,
				 txb->fragments[i]->len - hdr_len,
				 PCI_DMA_TODEVICE));*/
		/*tfd->u.data.chunk_len[i] = cpu_to_le16(mbuf_pkthdr_len(txb->fragments[i]) );
			skb_push(txb->fragments[i],hdr_len); */
		
#endif
		IWI_DEBUG_FULL("chunk_len %d\n",tfd->u.data.chunk_len[i]);
	}
frg:
	if (i != txb->nr_frags) {
		IWI_ERR("BUGS: nr_frags(%d) != i(%d)  \n", txb->nr_frags,i);
		mbuf_t skb;
		u16 remaining_bytes = 0;
		int j;

		// fix me: check mbuf_len
		for (j = i; j < txb->nr_frags; j++)
			remaining_bytes += mbuf_len(txb->fragments[j]) - hdr_len;

		IWI_DEBUG("Trying to reallocate for %d bytes\n",
		       remaining_bytes);
		skb = alloc_skb(remaining_bytes, GFP_ATOMIC);
		if (skb != NULL) {
			tfd->u.data.chunk_len[i] = cpu_to_le16(remaining_bytes);
			for (j = i; j < txb->nr_frags; j++) {
				// fix me: mbuf_len
				int size = mbuf_len(txb->fragments[j]) - hdr_len;

				IWI_DEBUG( "Adding frag %d %d...\n",
				       j, size);
				//mbuf_copydata(txb->fragments[j], hdr_len, size, skb);
				memcpy(skb_put(skb, size), (UInt8*)mbuf_data(txb->fragments[j])+ hdr_len , size);
				
			}
			//dev_kfree_skb_any(txb->fragments[i]);
			 if (txb->fragments[i]!=NULL)
			 {
				 if (!(txb->fragments[i]) == MBUF_TYPE_FREE) freePacket(txb->fragments[i]);
				 txb->fragments[i]=NULL;
			 }
			
			txb->fragments[i] = skb;
			tfd->u.data.chunk_ptr[i] = cpu_to_le32(mbuf_data_to_physical(mbuf_data(skb)));
			   /* cpu_to_le32(
				pci_map_single
					(priv->pci_dev, skb->data,
					 tfd->u.data.chunk_len[i],
					 PCI_DMA_TODEVICE));*/

			tfd->u.data.num_chunks =
			    cpu_to_le32(le32_to_cpu(tfd->u.data.num_chunks) +
					1);
		}
	}

   kick:
	/* kick DMA */
	IWI_DEBUG("kick DMA before q->first_empty %d\n",q->first_empty );
	q->first_empty = ipw_queue_inc_wrap(q->first_empty, q->n_bd);
	IWI_DEBUG("kick DMA after q->first_empty %d\n",q->first_empty );
	ipw_write32( q->reg_w, q->first_empty);
	
#ifdef TX_QUEUE_CHECK
	if (ipw_queue_space(q) < q->high_mark){ 
		// FIXME
		IWI_ERR("no TransmitQueue space\n ");
		fTransmitQueue->stop();
	}
#endif	
	//	netif_stop_queue(priv->net_dev);
	return kIOReturnOutputSuccess;//NETDEV_TX_OK;

      drop:
	IWI_DEBUG("Silently dropping Tx packet.\n");
	ieee80211_txb_free(txb);
	return kIOReturnOutputSuccess;//NETDEV_TX_OK;
}

/* 
 *  merge mbuf packet chains to single mbuf.
 *	if source mbuf is single chain,fail to merge, this method return copyed mbuf.
 * @m: source mbuf.
 *   
 * return: merged or copyed mbuf.
 *
 */

mbuf_t darwin_iwi2200::mergePacket(mbuf_t m)
{
	mbuf_t nm,nm2;
	if(!mbuf_next(m)){
		IWI_DEBUG_FULL("this packet dont has mbuf_next, this  is not required copy\n");
		goto copy_packet;
	}
	
	/* allocate and Initialize New mbuf */
	nm = allocatePacket(mbuf_pkthdr_len(m));
	if  (mbuf_pkthdr_len(nm) <= 0 ) {
		/* should not be reached */
		IWI_WARNING("output packet is not single chain but cannot merge to single\n");
	} else {
		mbuf_setlen(nm,0);
		mbuf_pkthdr_setlen(nm,0);
	}
	
	/* merging chains to single mbuf */
	for (nm2 = m; nm2;  nm2 = mbuf_next(nm2)) {
		memcpy (skb_put (nm, mbuf_len(nm2)), (UInt8*)mbuf_data(nm2), mbuf_len(nm2));
	}
	
	/* checking if merged or not. */
	if( mbuf_len(nm) == mbuf_pkthdr_len(m) ) 
		return nm;

	/* merging is not completed. */
	IWI_WARNING("mergePacket is failed: data copy dont work collectly\n");
	IWI_WARNING("orig_len %d orig_pktlen %d new_len  %d new_pktlen  %d\n",
					mbuf_len(m),mbuf_pkthdr_len(m),
					mbuf_len(nm),mbuf_pkthdr_len(nm) );
	if (nm)
	if (!(nm) == MBUF_TYPE_FREE) freePacket(nm);

copy_packet: 
		return copyPacket(m, 0); 
}
UInt32 darwin_iwi2200::outputPacket(mbuf_t m, void * param)
{
	int offset = (4 - ((int)(mbuf_data(m)) & 3)) % 4;    //packet needs to be 4 byte aligned
	mbuf_t nm;
	//mbuf_t nm2;
	size_t psize = 0;
	int ret = kIOReturnOutputDropped;

	//checking supported packet
	
	//IWI_LOG("t: %d f:%04x\n",mbuf_type(m),mbuf_flags(m));
	
	//drop mbuf is not PKTHDR
	if (!(mbuf_flags(m) & MBUF_PKTHDR) ){
		IWI_ERR("BUG: dont support mbuf without pkthdr and dropped \n");
		goto finish;
	}
	
	if(mbuf_type(m) == MBUF_TYPE_FREE){
		IWI_ERR("BUG: this is freed packet and dropped \n");
		goto finish;
	}
	
#if 0
	if (offset) { 
		IWI_DEBUG("required copyPacket\n");
		nm = copyPacket(m, 0);
		if (!(m) == MBUF_TYPE_FREE) freePacket(m);
	}	
		else    nm=m;
#else
	nm = mergePacket(m);
#endif
	if(mbuf_next(nm)){
		IWI_ERR("BUG: dont support chains mbuf\n");
		IWI_ERR("BUG: tx packet is not single mbuf mbuf_len(%d) mbuf_pkthdr_len(%d)\n",mbuf_len(nm) , mbuf_pkthdr_len(nm) );
		IWI_ERR("BUG: next mbuf size %d\n",mbuf_len(mbuf_next(nm)));
	}
	
#if 0	
	for (nm2 = nm; nm2; nm2 = mbuf_next(nm2)){
		 psize+= mbuf_len(nm2);
	}
	
	IWI_DEBUG(" %s single packet size %d  all %d \n",__FUNCTION__ ,mbuf_len(nm),psize);
#else
	//if(mbuf_len(nm) != mbuf_pkthdr_len(nm)){
	//	IWI_ERR("BUG: tx packet is not single mbuf mbuf_len(%d) mbuf_pkthdr_len(%d)\n",mbuf_len(nm) , mbuf_pkthdr_len(nm) );
	//}
#endif	
	//
	if(!(fNetif->getFlags() & IFF_RUNNING) || mbuf_len(nm)==0 || nm==NULL)
	//if (!(ifnet_flags(fifnet) & IFF_RUNNING) || mbuf_len(nm)==0 || nm==NULL)
	{
		//return kIOReturnOutputDropped;
		goto finish;
	}

	// if p_mode=0 (bss) and dhcp gives ip=169.254.xxx there's no internet connection
	// the network should be excluded from list to allow other networks to associate
	// this could also hapen if the link goes down.
#if 0	
	ifaddr_t *addresses;
	struct sockaddr *out_addr;
	if (ifnet_get_address_list_family(fifnet, &addresses, AF_INET)==0)
	{
		if (ifaddr_address(addresses[0], out_addr, sizeof(*out_addr))==0)
		{
			IWI_DEBUG("my ip address: " IP_FORMAT "\n",IP_LIST(out_addr->sa_data));
			if (priv->ieee->iw_mode == IW_MODE_INFRA)
			if ((int)(IP_CH(out_addr->sa_data)[2])==169 && (int)(IP_CH(out_addr->sa_data)[3])==254)
			{
				IWI_DEBUG("no internet connection!\n");// dissasociate , invalidade this network, re-scan
				countnonet++; // give time to get a valid ip address
				if (countnonet>10 && priv->assoc_network)
				{
					countnonet=0;
					priv->assoc_network->exclude=1;
				}
			}
		}
		ifnet_free_address_list(addresses);
	}
#endif		
	IWI_DEBUG_FULL("call ieee80211_xmit\n");
	ret  = ieee80211_xmit(nm,priv->net_dev);

finish:	
	
	/* free finished packet */

	
	return ret;	
}

int darwin_iwi2200::ipw_qos_set_tx_queue_command(struct ipw_priv *priv,
					u16 priority, struct tfd_data *tfd)
{
	int tx_queue_id = 0;

	tx_queue_id = from_priority_to_tx_queue[priority] - 1;
	tfd->tx_flags_ext |= DCT_FLAG_EXT_QOS_ENABLED;

	if (priv->qos_data.qos_no_ack_mask & (1UL << tx_queue_id)) {
		tfd->tx_flags &= ~DCT_FLAG_ACK_REQD;
		tfd->tfd.tfd_26.mchdr.qos_ctrl |= cpu_to_le16(CTRL_QOS_NO_ACK);
	}
	return 0;
}

struct ieee80211_frag_entry *darwin_iwi2200::ieee80211_frag_cache_find(struct
							      ieee80211_device
							      *ieee,
							      unsigned int seq,
							      unsigned int frag,
							      u8 * src,
							      u8 * dst)
{
	struct ieee80211_frag_entry *entry;
	int i;

	for (i = 0; i < IEEE80211_FRAG_CACHE_LEN; i++) {
		entry = &ieee->frag_cache[i];
		if (entry->skb != NULL &&
		    time_after(jiffies, entry->first_frag_time + 2 * HZ)) {
			IWI_DEBUG("expiring fragment cache entry "
					     "seq=%u last_frag=%u\n",
					     entry->seq, entry->last_frag);
			//dev_kfree_skb_any(entry->skb);
			if (!(entry->skb) == MBUF_TYPE_FREE) freePacket(entry->skb);
			entry->skb = NULL;
		}

		if (entry->skb != NULL && entry->seq == seq &&
		    (entry->last_frag + 1 == frag || frag == -1) &&
		    !compare_ether_addr(entry->src_addr, src) &&
		    !compare_ether_addr(entry->dst_addr, dst))
			return entry;
	}

	return NULL;
}

mbuf_t darwin_iwi2200::ieee80211_frag_cache_get(struct ieee80211_device *ieee,
						struct ieee80211_hdr_4addr *hdr)
{
	mbuf_t skb = NULL;
	u16 sc;
	unsigned int frag, seq;
	struct ieee80211_frag_entry *entry;

	sc = le16_to_cpu(hdr->seq_ctl);
	frag = WLAN_GET_SEQ_FRAG(sc);
	seq = WLAN_GET_SEQ_SEQ(sc);

	if (frag == 0) {
		/* Reserve enough space to fit maximum frame length */
		skb = alloc_skb(ieee->dev->mtu +
				    sizeof(struct ieee80211_hdr_4addr) +
				    8 /* LLC */  +
				    2 /* alignment */  +
				    8 /* WEP */  + ETH_ALEN /* WDS */ ,0);
		if (skb == NULL)
			return NULL;

		entry = &ieee->frag_cache[ieee->frag_next_idx];
		ieee->frag_next_idx++;
		if (ieee->frag_next_idx >= IEEE80211_FRAG_CACHE_LEN)
			ieee->frag_next_idx = 0;

		if (entry->skb != NULL)
		{
			if (!(entry->skb) == MBUF_TYPE_FREE) freePacket(entry->skb);
			entry->skb = NULL;
		}
		entry->first_frag_time = jiffies;
		entry->seq = seq;
		entry->last_frag = frag;
		entry->skb = skb;
		memcpy(entry->src_addr, hdr->addr2, ETH_ALEN);
		memcpy(entry->dst_addr, hdr->addr1, ETH_ALEN);
	} else {
		/* received a fragment of a frame for which the head fragment
		 * should have already been received */
		entry = ieee80211_frag_cache_find(ieee, seq, frag, hdr->addr2,
						  hdr->addr1);
		if (entry != NULL) {
			entry->last_frag = frag;
			skb = entry->skb;
		}
	}

	return skb;
}

int darwin_iwi2200::ieee80211_frag_cache_invalidate(struct ieee80211_device *ieee,
					   struct ieee80211_hdr_4addr *hdr)
{
	u16 sc;
	unsigned int seq;
	struct ieee80211_frag_entry *entry;

	sc = le16_to_cpu(hdr->seq_ctl);
	seq = WLAN_GET_SEQ_SEQ(sc);

	entry = ieee80211_frag_cache_find(ieee, seq, -1, hdr->addr2,
					  hdr->addr1);

	if (entry == NULL) {
		IWI_DEBUG("could not invalidate fragment cache "
				     "entry (seq=%u)\n", seq);
		return -1;
	}

	entry->skb = NULL;
	return 0;
}

int darwin_iwi2200::ieee80211_is_eapol_frame(struct ieee80211_device *ieee,
				    mbuf_t skb)
{
	struct net_device *dev = ieee->dev;
	u16 fc, ethertype;
	struct ieee80211_hdr_3addr *hdr;
	u8 *pos;

	if (mbuf_len(skb) < 24)
		return 0;

	hdr = (struct ieee80211_hdr_3addr *)(mbuf_data(skb));
	fc = le16_to_cpu(hdr->frame_ctl);

	/* check that the frame is unicast frame to us */
	if ((fc & (IEEE80211_FCTL_TODS | IEEE80211_FCTL_FROMDS)) ==
	    IEEE80211_FCTL_TODS &&
	    !compare_ether_addr(hdr->addr1, dev->dev_addr) &&
	    !compare_ether_addr(hdr->addr3, dev->dev_addr)) {
		/* ToDS frame with own addr BSSID and DA */
	} else if ((fc & (IEEE80211_FCTL_TODS | IEEE80211_FCTL_FROMDS)) ==
		   IEEE80211_FCTL_FROMDS &&
		   !compare_ether_addr(hdr->addr1, dev->dev_addr)) {
		/* FromDS frame with own addr as DA */
	} else
		return 0;

	if (mbuf_len(skb) < 24 + 8)
		return 0;

	/* check for port access entity Ethernet type */
	pos = ((UInt8*)mbuf_data(skb) + 24);
	ethertype = (pos[6] << 8) | pos[7];
	if (ethertype == ETH_P_PAE)
		return 1;

	return 0;
}

int darwin_iwi2200::ieee80211_rx(struct ieee80211_device *ieee, mbuf_t skb,
		 struct ieee80211_rx_stats *rx_stats)
{
	struct net_device *dev = ieee->dev;
	struct ieee80211_hdr_4addr *hdr;
	size_t hdrlen;
	u16 fc, type, stype, sc;
	struct net_device_stats *stats;
	unsigned int frag;
	u8 *payload;
	u16 ethertype;
#ifdef NOT_YET
	struct net_device *wds = NULL;
	struct sk_buff *skb2 = NULL;
	struct net_device *wds = NULL;
	int frame_authorized = 0;
	int from_assoc_ap = 0;
	void *sta = NULL;
#endif	
	
	u8 dst[ETH_ALEN];
	u8 src[ETH_ALEN];
	struct ieee80211_crypt_data *crypt = NULL;
	int keyidx = 0;
	int can_be_decrypted = 0;

	hdr = (struct ieee80211_hdr_4addr *)mbuf_data(skb);
	stats = &ieee->stats;

	if (mbuf_len(skb) < 10) {
		IWI_DEBUG( "%s: SKB length < 10\n", dev->name);
		goto rx_dropped;
	}

	fc = le16_to_cpu(hdr->frame_ctl);
	type = WLAN_FC_GET_TYPE(fc);
	stype = WLAN_FC_GET_STYPE(fc);
	sc = le16_to_cpu(hdr->seq_ctl);
	frag = WLAN_GET_SEQ_FRAG(sc);
	hdrlen = ieee80211_get_hdrlen(fc);

	/* Put this code here so that we avoid duplicating it in all
	 * Rx paths. - Jean II */
#ifdef CONFIG_WIRELESS_EXT
#ifdef IW_WIRELESS_SPY		/* defined in iw_handler.h */
	/* If spy monitoring on */
	if (ieee->spy_data.spy_number > 0) {
		struct iw_quality wstats;

		wstats.updated = 0;
		if (rx_stats->mask & IEEE80211_STATMASK_RSSI) {
			wstats.level = rx_stats->rssi;
			wstats.updated |= IW_QUAL_LEVEL_UPDATED;
		} else
			wstats.updated |= IW_QUAL_LEVEL_INVALID;

		if (rx_stats->mask & IEEE80211_STATMASK_NOISE) {
			wstats.noise = rx_stats->noise;
			wstats.updated |= IW_QUAL_NOISE_UPDATED;
		} else
			wstats.updated |= IW_QUAL_NOISE_INVALID;

		if (rx_stats->mask & IEEE80211_STATMASK_SIGNAL) {
			wstats.qual = rx_stats->signal;
			wstats.updated |= IW_QUAL_QUAL_UPDATED;
		} else
			wstats.updated |= IW_QUAL_QUAL_INVALID;

		/* Update spy records */
		wireless_spy_update(ieee->dev, hdr->addr2, &wstats);
	}
#endif				/* IW_WIRELESS_SPY */
#endif				/* CONFIG_WIRELESS_EXT */

#ifdef NOT_YET
	hostap_update_rx_stats(local->ap, hdr, rx_stats);
#endif

	if (ieee->iw_mode == IW_MODE_MONITOR) {
		stats->rx_packets++;
		stats->rx_bytes += mbuf_len(skb);
		//ieee80211_monitor_rx(ieee, skb, rx_stats);
		return 1;
	}

	/*can_be_decrypted = (is_multicast_ether_addr(hdr->addr1) ||
			    is_broadcast_ether_addr(hdr->addr2)) ?
	    ieee->host_mc_decrypt : ieee->host_decrypt;

	if (can_be_decrypted) {
		int idx = 0;
		if (mbuf_len(skb) >= hdrlen + 3) {
			idx = ((UInt8*)(mbuf_data(skb)))[hdrlen + 3] >> 6;
		}


		crypt = ieee->crypt[idx];

		if (crypt && (crypt->ops == NULL ||
			      crypt->ops->decrypt_mpdu == NULL))
			crypt = NULL;

		if (!crypt && (fc & IEEE80211_FCTL_PROTECTED)) {
			IWI_DEBUG("Decryption failed (not set)"
					     " (SA=" MAC_FMT ")\n",
					     MAC_ARG(hdr->addr2));
			ieee->ieee_stats.rx_discards_undecryptable++;
			goto rx_dropped;
		}
	}*/
#ifdef NOT_YET
	if (type != WLAN_FC_TYPE_DATA) {
		if (type == WLAN_FC_TYPE_MGMT && stype == WLAN_FC_STYPE_AUTH &&
		    fc & IEEE80211_FCTL_PROTECTED && ieee->host_decrypt &&
		    (keyidx = hostap_rx_frame_decrypt(ieee, skb, crypt)) < 0) {
			printk(KERN_DEBUG "%s: failed to decrypt mgmt::auth "
			       "from " MAC_FMT "\n", dev->name,
			       MAC_ARG(hdr->addr2));
			/* TODO: could inform hostapd about this so that it
			 * could send auth failure report */
			goto rx_dropped;
		}

		if (ieee80211_rx_frame_mgmt(ieee, skb, rx_stats, type, stype))
			goto rx_dropped;
		else
			goto rx_exit;
	}
#endif

	/* Data frame - extract src/dst addresses */
	if (mbuf_len(skb) < IEEE80211_3ADDR_LEN)
		goto rx_dropped;

	switch (fc & (IEEE80211_FCTL_FROMDS | IEEE80211_FCTL_TODS)) {
	case IEEE80211_FCTL_FROMDS:
		memcpy(dst, hdr->addr1, ETH_ALEN);
		memcpy(src, hdr->addr3, ETH_ALEN);
		break;
	case IEEE80211_FCTL_TODS:
		memcpy(dst, hdr->addr3, ETH_ALEN);
		memcpy(src, hdr->addr2, ETH_ALEN);
		break;
	case IEEE80211_FCTL_FROMDS | IEEE80211_FCTL_TODS:
		if (mbuf_len(skb) < IEEE80211_4ADDR_LEN)
			goto rx_dropped;
		memcpy(dst, hdr->addr3, ETH_ALEN);
		memcpy(src, hdr->addr4, ETH_ALEN);
		break;
	case 0:
		memcpy(dst, hdr->addr1, ETH_ALEN);
		memcpy(src, hdr->addr2, ETH_ALEN);
		break;
	}
	
#ifdef NOT_YET
	if (hostap_rx_frame_wds(ieee, hdr, fc, &wds))
		goto rx_dropped;
	if (wds) {
		skb->dev = dev = wds;
		stats = hostap_get_stats(dev);
	}

	if (ieee->iw_mode == IW_MODE_MASTER && !wds &&
	    (fc & (IEEE80211_FCTL_TODS | IEEE80211_FCTL_FROMDS)) ==
	    IEEE80211_FCTL_FROMDS && ieee->stadev
	    && !compare_ether_addr(hdr->addr2, ieee->assoc_ap_addr)) {
		/* Frame from BSSID of the AP for which we are a client */
		skb->dev = dev = ieee->stadev;
		stats = hostap_get_stats(dev);
		from_assoc_ap = 1;
	}
#endif

	dev->last_rx = jiffies;

#ifdef NOT_YET
	if ((ieee->iw_mode == IW_MODE_MASTER ||
	     ieee->iw_mode == IW_MODE_REPEAT) && !from_assoc_ap) {
		switch (hostap_handle_sta_rx(ieee, dev, skb, rx_stats,
					     wds != NULL)) {
		case AP_RX_CONTINUE_NOT_AUTHORIZED:
			frame_authorized = 0;
			break;
		case AP_RX_CONTINUE:
			frame_authorized = 1;
			break;
		case AP_RX_DROP:
			goto rx_dropped;
		case AP_RX_EXIT:
			goto rx_exit;
		}
	}
#endif

	/* Nullfunc frames may have PS-bit set, so they must be passed to
	 * hostap_handle_sta_rx() before being dropped here. */

	stype &= ~IEEE80211_STYPE_QOS_DATA;

	if (stype != IEEE80211_STYPE_DATA &&
	    stype != IEEE80211_STYPE_DATA_CFACK &&
	    stype != IEEE80211_STYPE_DATA_CFPOLL &&
	    stype != IEEE80211_STYPE_DATA_CFACKPOLL) {
		if (stype != IEEE80211_STYPE_NULLFUNC)
			IWI_DEBUG("RX: dropped data frame "
					     "with no data (type=0x%02x, "
					     "subtype=0x%02x, len=%d)\n",
					     type, stype, mbuf_len(skb));
		goto rx_dropped;
	}

	/* skb: hdr + (possibly fragmented, possibly encrypted) payload */

	if ((fc & IEEE80211_FCTL_PROTECTED) && can_be_decrypted  /* &&
	    (keyidx = ieee80211_rx_frame_decrypt(ieee, skb, crypt)) < 0 */ )
		goto rx_dropped;

	//hdr = (struct ieee80211_hdr_4addr *)mbuf_data(skb);

	// skb: hdr + (possibly fragmented) plaintext payload 
	// PR: FIXME: hostap has additional conditions in the "if" below:
	// ieee->host_decrypt && (fc & IEEE80211_FCTL_PROTECTED) &&
	if ((frag != 0) || (fc & IEEE80211_FCTL_MOREFRAGS)) {
		int flen;
		mbuf_t frag_skb = ieee80211_frag_cache_get(ieee, hdr);
		IWI_DEBUG_FULL("Rx Fragment received (%u)\n", frag);

		if (!frag_skb) {
			IWI_DEBUG("Rx cannot get skb from fragment "
					"cache (morefrag=%d seq=%u frag=%u)\n",
					(fc & IEEE80211_FCTL_MOREFRAGS) != 0,
					WLAN_GET_SEQ_SEQ(sc), frag);
			goto rx_dropped;
		}

		flen = mbuf_len(skb);
		if (frag != 0)
			flen -= hdrlen;
		/*
		if (frag_skb->tail + flen > frag_skb->end) { */
		// skb->tail  := mbuf_data(skb)+mbuf_len(skb) 
		// skb->end := mbuf_datastart(skb)+mbuf_maxlen(skb) 
		if (  (UInt8*)mbuf_data(frag_skb) + mbuf_len(frag_skb)  + flen > (UInt8*)mbuf_datastart(frag_skb) + mbuf_maxlen(frag_skb)  ) {
			IWI_DEBUG( "%s: host decrypted and "
			       "reassembled frame did not fit skb\n",
			       dev->name);
			ieee80211_frag_cache_invalidate(ieee, hdr);
			goto rx_dropped;
		}

		if (frag == 0) {
			// copy first fragment (including full headers) into
			 // beginning of the fragment cache skb 
			memcpy(skb_put(frag_skb, flen), (UInt8*)mbuf_data(skb), flen);
		} else {
			// append frame payload to the end of the fragment
			 // cache skb 
			memcpy(skb_put(frag_skb, flen), (UInt8*)mbuf_data(skb) + hdrlen,
			       flen);
		}
		//dev_kfree_skb_any(skb);
		if (skb != NULL) {
			if (!(skb) == MBUF_TYPE_FREE) freePacket(skb);
		}
		skb = NULL;

		if (fc & IEEE80211_FCTL_MOREFRAGS) {
			// more fragments expected - leave the skb in fragment
			 // cache for now; it will be delivered to upper layers
			 //after all fragments have been received 
			goto rx_exit;
		}

		// this was the last fragment and the frame will be
		 // delivered, so remove skb from fragment cache 
		skb = frag_skb;
		hdr = (struct ieee80211_hdr_4addr *)(mbuf_data(skb));
		ieee80211_frag_cache_invalidate(ieee, hdr);
	}

	/* skb: hdr + (possible reassembled) full MSDU payload; possibly still
	  encrypted/authenticated  */
	if ((fc & IEEE80211_FCTL_PROTECTED) && can_be_decrypted /*&&
	    ieee80211_rx_frame_decrypt_msdu(ieee, skb, keyidx, crypt) */   )
		goto rx_dropped;

	hdr = (struct ieee80211_hdr_4addr *)(mbuf_data(skb));
	if (crypt && !(fc & IEEE80211_FCTL_PROTECTED) && !ieee->open_wep) {
		if (		
			   ieee80211_is_eapol_frame(ieee, skb)) {
			// pass unencrypted EAPOL frames even if encryption is
			 // configured 
		} else {
			IWI_DEBUG("encryption configured, but RX "
					     "frame not encrypted (SA=" MAC_FMT
					     ")\n", MAC_ARG(hdr->addr2));
			goto rx_dropped;
		}
	}

	if (crypt && !(fc & IEEE80211_FCTL_PROTECTED) && !ieee->open_wep &&
	    !ieee80211_is_eapol_frame(ieee, skb)) {
		IWI_DEBUG("dropped unencrypted RX data "
				     "frame from " MAC_FMT
				     " (drop_unencrypted=1)\n",
				     MAC_ARG(hdr->addr2));
		goto rx_dropped;
	}

	/* skb: hdr + (possible reassembled) full plaintext payload */

	payload = ((UInt8*)mbuf_data(skb) + hdrlen);
	ethertype = (payload[6] << 8) | payload[7];

#ifdef NOT_YET
	/* If IEEE 802.1X is used, check whether the port is authorized to send
	 * the received frame. */
	if (ieee->ieee802_1x && ieee->iw_mode == IW_MODE_MASTER) {
		if (ethertype == ETH_P_PAE) {
			printk(KERN_DEBUG "%s: RX: IEEE 802.1X frame\n",
			       dev->name);
			if (ieee->hostapd && ieee->apdev) {
				/* Send IEEE 802.1X frames to the user
				 * space daemon for processing */
				prism2_rx_80211(ieee->apdev, skb, rx_stats,
						PRISM2_RX_MGMT);
				ieee->apdevstats.rx_packets++;
				ieee->apdevstats.rx_bytes += skb->len;
				goto rx_exit;
			}
		} else if (!frame_authorized) {
			printk(KERN_DEBUG "%s: dropped frame from "
			       "unauthorized port (IEEE 802.1X): "
			       "ethertype=0x%04x\n", dev->name, ethertype);
			goto rx_dropped;
		}
	}
#endif

	/* convert hdr + possible LLC headers into Ethernet header */
	if ( mbuf_len(skb) - hdrlen >= 8 &&
	    ((memcmp(payload, rfc1042_header, SNAP_SIZE) == 0 &&
	      ethertype != ETH_P_AARP && ethertype != ETH_P_IPX) ||
	     memcmp(payload, bridge_tunnel_header, SNAP_SIZE) == 0)) {
		/* remove RFC1042 or Bridge-Tunnel encapsulation and
		 * replace EtherType */
		skb_pull(skb, hdrlen + SNAP_SIZE);
		//mbuf_adj(skb, hdrlen + SNAP_SIZE);
		memcpy(skb_push(skb, ETH_ALEN), src, ETH_ALEN);
		memcpy(skb_push(skb, ETH_ALEN), dst, ETH_ALEN);
		//memcpy(((UInt8*)mbuf_data(skb) + ETH_ALEN), src, ETH_ALEN);
		//memcpy(((UInt8*)mbuf_data(skb) +ETH_ALEN), dst, ETH_ALEN);
	} else {
		u16 len;
		/* Leave Ethernet header part of hdr and full payload */
		skb_pull(skb, hdrlen);
		//mbuf_adj(skb, hdrlen);
		len = htons(mbuf_len(skb));
		memcpy(skb_push(skb, 2), &len, 2);
		memcpy(skb_push(skb, ETH_ALEN), src, ETH_ALEN);
		memcpy(skb_push(skb, ETH_ALEN), dst, ETH_ALEN);
		//memcpy(((UInt8*)mbuf_data(skb) + 2), &len, 2);
		//memcpy(((UInt8*)mbuf_data(skb) + ETH_ALEN), src, ETH_ALEN);
		//memcpy(((UInt8*)mbuf_data(skb) + ETH_ALEN), dst, ETH_ALEN);
	}


	stats->rx_packets++;
	stats->rx_bytes += mbuf_len(skb);


	if (skb) {
		//skb->protocol = eth_type_trans(skb, dev);
		//memset(skb->cb, 0, sizeof(skb->cb));
		//skb->dev = dev;
		//skb->ip_summed = CHECKSUM_NONE;	/* 802.11 crc not sufficient */
		if( mbuf_flags(skb) & MBUF_PKTHDR){
			fNetif->inputPacket(skb,mbuf_pkthdr_len(skb),IONetworkInterface::kInputOptionQueuePacket);
		}else{
			IWI_ERR("this packet dont have MBUF_PKTHDR\n");
			fNetif->inputPacket(skb,mbuf_len(skb),IONetworkInterface::kInputOptionQueuePacket);
		}
		//if (netif_rx(skb) == NET_RX_DROP) {
			/* netif_rx always succeeds, but it might drop
			 * the packet.  If it drops the packet, we log that
			 * in our stats. */
		//	IWI_DEBUG ("RX: netif_rx dropped the packet\n");
		//	stats->rx_dropped++;
		//}
	}

      rx_exit:

	return 1;

      rx_dropped:
	IWI_DEBUG("rx dropped %d\n",stats->rx_dropped);
	stats->rx_dropped++;

	/* Returning 0 indicates to caller that we have not handled the SKB--
	 * so it is still allocated and can be used again by underlying
	 * hardware as a DMA target */
	return 0;
}

int darwin_iwi2200::ieee80211_handle_assoc_resp(struct ieee80211_device *ieee, struct ieee80211_assoc_response
				       *frame, struct ieee80211_rx_stats *stats)
{
	struct ieee80211_network network_resp; 
		network_resp.ibss_dfs = NULL;
	
	struct ieee80211_network *network = &network_resp;
	struct net_device *dev = ieee->dev;

	network->flags = 0;
	network->qos_data.active = 0;
	network->qos_data.supported = 0;
	network->qos_data.param_count = 0;
	network->qos_data.old_param_count = 0;

	//network->atim_window = le16_to_cpu(frame->aid) & (0x3FFF);
	network->atim_window = le16_to_cpu(frame->aid);
	network->listen_interval = le16_to_cpu(frame->status);
	memcpy(network->bssid, frame->header.addr3, ETH_ALEN);
	network->capability = le16_to_cpu(frame->capability);
	network->last_scanned = jiffies;
	network->rates_len = network->rates_ex_len = 0;
	network->last_associate = 0;
	network->ssid_len = 0;
	network->erp_value =
	    (network->capability & WLAN_CAPABILITY_IBSS) ? 0x3 : 0x0;

	if (stats->freq == IEEE80211_52GHZ_BAND) {
		/* for A band (No DS info) */
		network->channel = stats->received_channel;
	} else
		network->flags |= NETWORK_HAS_CCK;

	network->wpa_ie_len = 0;
	network->rsn_ie_len = 0;

	if (ieee80211_parse_info_param
	    (frame->info_element, stats->len - sizeof(*frame), network))
		return 1;

	network->mode = 0;
	if (stats->freq == IEEE80211_52GHZ_BAND)
		network->mode = IEEE_A;
	else {
		if (network->flags & NETWORK_HAS_OFDM)
			network->mode |= IEEE_G;
		if (network->flags & NETWORK_HAS_CCK)
			network->mode |= IEEE_B;
	}

	if (ieee80211_is_empty_essid((const char*)network->ssid, network->ssid_len))
		network->flags |= NETWORK_EMPTY_ESSID;

	memcpy(&network->stats, stats, sizeof(network->stats));

	//if (ieee->handle_assoc_response != NULL)
	//	ieee->handle_assoc_response(dev, frame, network);
	ipw_handle_assoc_response(dev, frame, network);
	return 0;
}

int darwin_iwi2200::ipw_qos_association_resp(struct ipw_priv *priv,
				    struct ieee80211_network *network)
{
	int ret = 0;
	unsigned long flags;
	u32 size = sizeof(struct ieee80211_qos_parameters);
	int set_qos_param = 0;

	if ((priv == NULL) || (network == NULL) ||
	    (priv->assoc_network == NULL))
		return ret;

	if (!(priv->status & STATUS_ASSOCIATED))
		return ret;

	if ((priv->ieee->iw_mode != IW_MODE_INFRA))
		return ret;

	//spin_lock_irqsave(&priv->ieee->lock, flags);
	if (network->flags & NETWORK_HAS_QOS_PARAMETERS) {
		memcpy(&priv->assoc_network->qos_data, &network->qos_data,
		       sizeof(struct ieee80211_qos_data));
		priv->assoc_network->qos_data.active = 1;
		if ((network->qos_data.old_param_count !=
		     network->qos_data.param_count)) {
			set_qos_param = 1;
			network->qos_data.old_param_count =
			    network->qos_data.param_count;
		}

	} else {
		if ((network->mode == IEEE_B) || (priv->ieee->mode == IEEE_B))
			memcpy(&priv->assoc_network->qos_data.parameters,
			       &def_parameters_CCK, size);
		else
			memcpy(&priv->assoc_network->qos_data.parameters,
			       &def_parameters_OFDM, size);
		priv->assoc_network->qos_data.active = 0;
		priv->assoc_network->qos_data.supported = 0;
		set_qos_param = 1;
	}

	//spin_unlock_irqrestore(&priv->ieee->lock, flags);

	if (set_qos_param == 1)
		queue_te(9,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_bg_qos_activate),priv,NULL,true);
		//schedule_work(&priv->qos_activate);
	return ret;
}

int darwin_iwi2200::ipw_handle_assoc_response(struct net_device *dev,
				     struct ieee80211_assoc_response *resp,
				     struct ieee80211_network *network)
{
#ifdef CONFIG_IPW2200_QOS
	//struct ipw_priv *priv = ieee80211_priv(dev);
	if (qos_enable)
	ipw_qos_association_resp(priv, network);
#endif
	return 0;
}

struct ipw_frame *darwin_iwi2200::ipw_get_free_frame(struct ipw_priv *priv)
{
	struct ipw_frame *frame;
	struct list_head *element;
	//if (list_empty(&priv->free_frames)) {
		(void*)frame = kzalloc(sizeof(*frame), NULL);
		if (!frame) {
			IWI_DEBUG("Could not allocate frame!\n");
			return 0;
		}

		/*priv->frames_count++;

		if (priv->frames_count >= FREE_FRAME_THRESHOLD) {
			IPW_WARNING("%d frames allocated.  "
				    "Are we losing them?\n",
				    priv->frames_count);
		}*/

		return frame;
//	}

//	element = priv->free_frames.next;
//	list_del(element);
//	return list_entry(element, struct ipw_frame, list);
}

void *darwin_iwi2200::ieee80211_next_info_element(struct ieee80211_info_element
					 *info_element)
{
	return &info_element->data[info_element->len];
}

int darwin_iwi2200::ipw_fill_beacon_frame(struct ipw_priv *priv,
				 struct ieee80211_hdr *hdr, u8 * dest, int left)
{
	struct ieee80211_probe_response *frame = NULL;
	struct ieee80211_info_element *info_element = NULL;
	int len = 0;
	u16 ret_rates;

	if (!(priv->status & (STATUS_ASSOCIATED | STATUS_ASSOCIATING))
	    || (priv->ieee->iw_mode != IW_MODE_ADHOC))
		return 0;

	frame = (struct ieee80211_probe_response *)hdr;
	frame->header.frame_ctl = IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_BEACON;

	memcpy(frame->header.addr1, dest, ETH_ALEN);
	memcpy(frame->header.addr2, priv->mac_addr, ETH_ALEN);
	memcpy(frame->header.addr3, priv->assoc_request.bssid, ETH_ALEN);

	frame->header.seq_ctl = 0;

	/* fill the probe fields */
	frame->capability = priv->assoc_request.capability;
	frame->time_stamp[0] = priv->assoc_request.assoc_tsf_lsw;
	frame->time_stamp[1] = priv->assoc_request.assoc_tsf_msw;

	frame->beacon_interval = priv->assoc_request.beacon_interval;
	frame->capability |= WLAN_CAPABILITY_IBSS;
	if (priv->capability & CAP_PRIVACY_ON)
		frame->capability |= WLAN_CAPABILITY_PRIVACY;
	else
		frame->capability &= ~WLAN_CAPABILITY_PRIVACY;
	frame->capability &= ~WLAN_CAPABILITY_SHORT_SLOT_TIME;

	info_element = frame->info_element;

	/* fill in ssid broadcast */
	info_element->id = MFIE_TYPE_SSID;
	info_element->len = priv->essid_len;
	memcpy(info_element->data, priv->essid, priv->essid_len);

	/* Account for the size we know... */
	len = sizeof(struct ieee80211_probe_response) +
	    sizeof(struct ieee80211_info_element) + priv->essid_len;

	/* Advance to next IE... */
	(void*)info_element = ieee80211_next_info_element(info_element);

	/*  atim window */
	info_element->id = MFIE_TYPE_IBSS_SET;
	info_element->data[0] = (u8) priv->assoc_request.atim_window;
	info_element->data[1] = 0x0;
	info_element->len = 2;

	len += sizeof(struct ieee80211_info_element) + info_element->len;

	/* Advance to next IE... */
	(void*)info_element = ieee80211_next_info_element(info_element);

	info_element->id = MFIE_TYPE_RATES;
	info_element->len = 0;
	/*ret_rates =
	    ipw_supported_rate_to_ie(info_element, priv->active_rate,
				     priv->active_rate_basic,
				     IPW_SUPPORTED_RATES_IE_LEN);

	ret_rates = ~ret_rates & priv->active_rate;*/

	len += sizeof(struct ieee80211_info_element) + info_element->len;

	if (ret_rates != 0) {

		/* Now see how much space is left.... */
		if (left < sizeof(struct ieee80211_info_element) +
		    IPW_SUPPORTED_RATES_IE_LEN)
			return 0;

		(void*)info_element = ieee80211_next_info_element(info_element);
		info_element->id = MFIE_TYPE_RATES_EX;
		info_element->len = 0;
		/*ipw_supported_rate_to_ie(info_element,
					 ret_rates,
					 priv->active_rate_basic,
					 IPW_SUPPORTED_RATES_IE_LEN);*/
		if (info_element->len > 0)
			/* account for this IE */
			len += sizeof(struct ieee80211_info_element) +
			    info_element->len;
	}

	/* add ERP present IE if not 11a or 11b */
	if (priv->assoc_request.ieee_mode == IPW_G_MODE) {
		(void*)info_element = ieee80211_next_info_element(info_element);

		info_element->id = MFIE_TYPE_ERP_INFO;
		info_element->len = 1;

		//info_element->data[0] = priv->assoc_request.erp_value;

		if (info_element->data[0] & IEEE80211_ERP_BARKER_PREAMBLE_MODE) {
			frame->capability &= ~WLAN_CAPABILITY_SHORT_PREAMBLE;
		}
		len += sizeof(struct ieee80211_info_element) +
		    info_element->len;
	}
	/* add DS present IE */
	if (priv->assoc_request.ieee_mode != IPW_A_MODE) {
		(void*)info_element = ieee80211_next_info_element(info_element);

		info_element->id = MFIE_TYPE_DS_SET;
		info_element->len = 1;

		info_element->data[0] = priv->channel;

		len += sizeof(struct ieee80211_info_element) +
		    info_element->len;

	}

	return len;
}

int darwin_iwi2200::ipw_handle_probe_request(struct net_device *dev, struct ieee80211_probe_request
				    *frame, struct ieee80211_rx_stats *stats)
{
	int rc = 0;
	u16 left;
	struct ieee80211_info_element *info_element;
	struct ipw_frame *out_frame;
	int len;

	if (priv->status & STATUS_EXIT_PENDING)
		return 0;

	if (((priv->status & STATUS_ASSOCIATED) == 0)
	    || (priv->ieee->iw_mode != IW_MODE_ADHOC))
		return rc;

	if (stats->received_channel != priv->channel)
		return rc;
	info_element = frame->info_element;
	left = stats->len - sizeof(*frame);
	while (left >= sizeof(*info_element)) {
		if (sizeof(*info_element) + info_element->len > left) {
			break;
		}

		if (info_element->id == MFIE_TYPE_SSID) {
			if (info_element->len == 0) {
				break;
			}

			if (priv->essid_len != info_element->len)
				return rc;
			if (memcmp
			    (priv->essid, info_element->data,
			     info_element->len) != 0)
				return rc;
			break;
		}
		left -= sizeof(struct ieee80211_info_element) +
		    info_element->len;
		info_element = (struct ieee80211_info_element *)
		    &info_element->data[info_element->len];
	}

	out_frame = ipw_get_free_frame(priv);

	if (!out_frame) {
		IWI_DEBUG("Could not allocate frame for auth work.\n");
		return -ENOMEM;
	}

	len = ipw_fill_beacon_frame(priv, &out_frame->u.frame,
				    frame->header.addr2, sizeof(out_frame->u));

	if (len) {
		IWI_DEBUG("Sending %d bytes.\n", len);
		out_frame->u.frame.frame_ctl = IEEE80211_FTYPE_MGMT |
		    IEEE80211_STYPE_PROBE_RESP;
		//ieee80211_tx_frame(priv->ieee, &out_frame->u.frame, 0, len, 0);
		mbuf_t temp=alloc_skb(len,0);
		memcpy(temp, &out_frame->u.frame, len);
		outputPacket(temp,0);
		
	}

	//ipw_free_frame(priv, out_frame);
	memset(frame, 0, sizeof(*frame));
	return 0;
}

// linux ipw3945
/*static int ipw_handle_auth(struct net_device *dev, struct ieee80211_auth *auth)
{
	struct ipw_priv *priv = ieee80211_priv(dev);
	int size = sizeof(struct ieee80211_auth);

	IPW_DEBUG_INFO
	    ("auth frame algorithm %d  transaction %d  status %d \n",
	     auth->algorithm, auth->transaction, auth->status);

	if (!priv->assoc_sequence_frame) {
		IPW_DEBUG_INFO("Auth received out of sequence.  Ignoring.\n");
		return 0;
	}

	if (priv->status & STATUS_EXIT_PENDING)
		return 0;

	if (!(priv->status & STATUS_ASSOCIATED) &&
	    !(priv->status & STATUS_ASSOCIATING))
		return 0;

	if (priv->auth_state == AUTH_SUCCESS || priv->auth_state == AUTH_INIT) {
		IPW_DEBUG_INFO
		    ("AUTH frame received while not authenticating...\n");
		return 0;
	}

	if (auth->transaction == 2 && auth->algorithm == WLAN_AUTH_SHARED_KEY)
		size += 138;

	priv->assoc_sequence_frame->len = size;
	memcpy(&priv->assoc_sequence_frame->u.frame, auth, size);

	cancel_delayed_work(&priv->assoc_state_retry);

	queue_work(priv->workqueue, &priv->auth_work);

	return 0;
}*/

/*static int ipw_handle_disassoc(struct net_device *dev,
			       struct ieee80211_disassoc *disassoc)
{
	struct ipw_priv *priv = ieee80211_priv(dev);
	IPW_DEBUG_INFO("disassociation reasoncode %d: %s\n",
		       disassoc->reason, ipw_get_status_code(disassoc->reason));
	queue_work(priv->workqueue, &priv->disassociate);
	return 0;
}*/

/*static int ipw_handle_deauth(struct net_device *dev,
			     struct ieee80211_deauth *deauth)
{
	struct ipw_priv *priv = ieee80211_priv(dev);

	IPW_DEBUG_INFO("deauthentication reasoncode %x\n", deauth->reason);
	queue_work(priv->workqueue, &priv->disassociate);
	return 0;
}*/

void darwin_iwi2200::ieee80211_rx_mgt(struct ieee80211_device *ieee, 
        struct ieee80211_hdr_4addr *header,struct ieee80211_rx_stats *stats)
{
	// copy from ieee80211_rx.c ieee80211_rx_mgt
	switch (WLAN_FC_GET_STYPE(le16_to_cpu(header->frame_ctl))) {
	case IEEE80211_STYPE_ASSOC_RESP:
		IWI_DEBUG("received ASSOCIATION RESPONSE (%d)\n",
				WLAN_FC_GET_STYPE(le16_to_cpu(header->frame_ctl)));
		ieee80211_handle_assoc_resp(ieee,
				                    (struct ieee80211_assoc_response *)
									header, stats); 
		break;
	case IEEE80211_STYPE_REASSOC_RESP:
			IEEE80211_DEBUG_MGMT("received REASSOCIATION RESPONSE (%d)\n",
								 WLAN_FC_GET_STYPE(le16_to_cpu
													(header->frame_ctl)));
		break;
	case IEEE80211_STYPE_PROBE_REQ:
			IEEE80211_DEBUG_MGMT("received auth (%d)\n",
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));
			IWI_DEBUG("but not impletented \n");										   
            /*
			if (ieee->handle_probe_request != NULL)
                        ieee->handle_probe_request(ieee->dev,
                                                   (struct
                                                    ieee80211_probe_request *)
                                                   header, stats); */
		break;
	case IEEE80211_STYPE_PROBE_RESP:
			IEEE80211_DEBUG_MGMT("received PROBE RESPONSE (%d)\n",
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));	
			/*ipw_handle_probe_request(ieee->dev, (struct
                                                    ieee80211_probe_request *)
                                                   header, stats);*/
                ieee80211_process_probe_response(ieee,
                                                 (struct
                                                  ieee80211_probe_response *)
                                                 header, stats); 
		break;
	case IEEE80211_STYPE_BEACON:
                IEEE80211_DEBUG_MGMT("received BEACON (%d)\n",
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));
                ieee80211_process_probe_response(ieee,
                                                 (struct
                                                  ieee80211_probe_response *)
                                                 header, stats); 
                break;
	case IEEE80211_STYPE_AUTH:

                IEEE80211_DEBUG_MGMT("received auth (%d)\n",
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));
                IWI_DEBUG("but not impletented \n"); 
				/*
                if (ieee->handle_auth != NULL)
                        ieee->handle_auth(ieee->dev,
                                          (struct ieee80211_auth *)header); */
                break;

	case IEEE80211_STYPE_DISASSOC:
		        IWI_DEBUG("DISASSOC: not impletented \n");
				/* 
                if (ieee->handle_disassoc != NULL)
                        ieee->handle_disassoc(ieee->dev,
                                              (struct ieee80211_disassoc *)
                                              header); */
                break;
	case IEEE80211_STYPE_ACTION:
                IEEE80211_DEBUG_MGMT("ACTION\n");
				IWI_DEBUG("ACTION: but not impletented \n");
				/* 
                if (ieee->handle_action)
                        ieee->handle_action(ieee->dev,
                                            (struct ieee80211_action *)
                                            header, stats); */
                break;

	case IEEE80211_STYPE_REASSOC_REQ:
                IEEE80211_DEBUG_MGMT("received reassoc (%d)\n",
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));

                IEEE80211_DEBUG_MGMT("%s: IEEE80211_REASSOC_REQ received\n",
									 ieee->dev->name);
				IWI_DEBUG("REASSOC: but not impletented \n");
				/*
                if (ieee->handle_reassoc_request != NULL)
                        ieee->handle_reassoc_request(ieee->dev,
                                                    (struct ieee80211_reassoc_request *)
                                                     header); */
                break;
	case IEEE80211_STYPE_ASSOC_REQ:
                IEEE80211_DEBUG_MGMT("received assoc (%d)\n",
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));
				ieee80211_handle_assoc_resp(ieee,
				                    (struct ieee80211_assoc_response *)
									header, stats); 
				/*ieee80211_process_probe_response(ieee,
                                                 (struct
                                                  ieee80211_probe_response *)
                                                 header, stats);*/
                /* if (ieee->handle_assoc_request != NULL)
                        ieee->handle_assoc_request(ieee->dev); */
                break;

	case IEEE80211_STYPE_DEAUTH:
                IEEE80211_DEBUG_MGMT("DEAUTH\n");
				IWI_DEBUG("DEAUTH: but not impletented \n");
                /*if (ieee->handle_deauth != NULL)
                        ieee->handle_deauth(ieee->dev,
                                            (struct ieee80211_deauth *)
                                            header); */
                break;
	default:
                IEEE80211_DEBUG_MGMT("received UNKNOWN (%d)\n",
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));
                IEEE80211_DEBUG_MGMT("%s: Unknown management packet: %d\n",
									ieee->dev->name,
                                     WLAN_FC_GET_STYPE(le16_to_cpu
                                                       (header->frame_ctl)));
                break;
	}
}

int darwin_iwi2200::ieee80211_qos_convert_ac_to_parameters(struct
						  ieee80211_qos_parameter_info
						  *param_elm, struct
						  ieee80211_qos_parameters
						  *qos_param)
{
	int rc = 0;
	int i;
	struct ieee80211_qos_ac_parameter *ac_params;
	u32 txop;
	u8 cw_min;
	u8 cw_max;

	for (i = 0; i < QOS_QUEUE_NUM; i++) {
		ac_params = &(param_elm->ac_params_record[i]);

		qos_param->aifs[i] = (ac_params->aci_aifsn) & 0x0F;
		qos_param->aifs[i] -= (qos_param->aifs[i] < 2) ? 0 : 2;

		cw_min = ac_params->ecw_min_max & 0x0F;
		qos_param->cw_min[i] = (u16) ((1 << cw_min) - 1);

		cw_max = (ac_params->ecw_min_max & 0xF0) >> 4;
		qos_param->cw_max[i] = (u16) ((1 << cw_max) - 1);

		qos_param->flag[i] =
		    (ac_params->aci_aifsn & 0x10) ? 0x01 : 0x00;

		txop = le16_to_cpu(ac_params->tx_op_limit) * 32;
		qos_param->tx_op_limit[i] = (u16) txop;
	}
	return rc;
}

int darwin_iwi2200::ieee80211_read_qos_param_element(struct ieee80211_qos_parameter_info
					    *element_param, struct ieee80211_info_element
					    *info_element)
{
	int ret = 0;
	u16 size = sizeof(struct ieee80211_qos_parameter_info) - 2;

	if ((info_element == NULL) || (element_param == NULL))
		return -1;

	if (info_element->id == QOS_ELEMENT_ID && info_element->len == size) {
		memcpy(element_param->info_element.qui, info_element->data,
		       info_element->len);
		element_param->info_element.elementID = info_element->id;
		element_param->info_element.length = info_element->len;
	} else
		ret = -1;
	if (ret == 0)
		ret = ieee80211_verify_qos_info(&element_param->info_element,
						QOS_OUI_PARAM_SUB_TYPE);
	return ret;
}

int darwin_iwi2200::ieee80211_verify_qos_info(struct ieee80211_qos_information_element
				     *info_element, int sub_type)
{

	if (info_element->qui_subtype != sub_type)
		return -1;
	if (memcmp(info_element->qui, qos_oui, QOS_OUI_LEN))
		return -1;
	if (info_element->qui_type != QOS_OUI_TYPE)
		return -1;
	if (info_element->version != QOS_VERSION_1)
		return -1;

	return 0;
}

int darwin_iwi2200::ieee80211_read_qos_info_element(struct
					   ieee80211_qos_information_element
					   *element_info, struct ieee80211_info_element
					   *info_element)
{
	int ret = 0;
	u16 size = sizeof(struct ieee80211_qos_information_element) - 2;

	if (element_info == NULL)
		return -1;
	if (info_element == NULL)
		return -1;

	if ((info_element->id == QOS_ELEMENT_ID) && (info_element->len == size)) {
		memcpy(element_info->qui, info_element->data,
		       info_element->len);
		element_info->elementID = info_element->id;
		element_info->length = info_element->len;
	} else
		ret = -1;

	if (ret == 0)
		ret = ieee80211_verify_qos_info(element_info,
						QOS_OUI_INFO_SUB_TYPE);
	return ret;
}

int darwin_iwi2200::ieee80211_parse_qos_info_param_IE(struct ieee80211_info_element
					     *info_element,
					     struct ieee80211_network *network)
{
	int rc = 0;
	struct ieee80211_qos_parameters *qos_param = NULL;
	struct ieee80211_qos_information_element qos_info_element;

	rc = ieee80211_read_qos_info_element(&qos_info_element, info_element);

	if (rc == 0) {
		network->qos_data.param_count = qos_info_element.ac_info & 0x0F;
		network->flags |= NETWORK_HAS_QOS_INFORMATION;
	} else {
		struct ieee80211_qos_parameter_info param_element;

		rc = ieee80211_read_qos_param_element(&param_element,
						      info_element);
		if (rc == 0) {
			qos_param = &(network->qos_data.parameters);
			ieee80211_qos_convert_ac_to_parameters(&param_element,
							       qos_param);
			network->flags |= NETWORK_HAS_QOS_PARAMETERS;
			network->qos_data.param_count =
			    param_element.info_element.ac_info & 0x0F;
		}
	}

	if (rc == 0) {
		IWI_DEBUG("QoS is supported\n");
		network->qos_data.supported = 1;
	}
	return rc;
}

int darwin_iwi2200::ieee80211_parse_info_param(struct ieee80211_info_element
				      *info_element, u16 length,
				      struct ieee80211_network *network)
{
	u8 i;

	while (length >= sizeof(*info_element)) {
		if (sizeof(*info_element) + info_element->len > length) {
			IWI_DEBUG("ERROR: Info elem: parse failed: "
					"info_element->len + 2 > left : "
					"info_element->len+2=%zd left=%d, id=%d.\n",
					info_element->len +
					sizeof(*info_element),
					length, info_element->id);
			/* We stop processing but don't return an error here
			 * because some misbehaviour APs break this rule. ie.
			 * Orinoco AP1000. */
			break;
		}

		switch (info_element->id) {
		case MFIE_TYPE_SSID:
			if (ieee80211_is_empty_essid((const char*)info_element->data,
						     info_element->len)) {
				network->flags |= NETWORK_EMPTY_ESSID;
				break;
			}

			network->ssid_len = min(info_element->len,
						(u8) IW_ESSID_MAX_SIZE);
			memcpy(network->ssid, info_element->data,
			       network->ssid_len);
			if (network->ssid_len < IW_ESSID_MAX_SIZE)
				memset(network->ssid + network->ssid_len, 0,
				       IW_ESSID_MAX_SIZE - network->ssid_len);

			IWI_DEBUG("MFIE_TYPE_SSID: '%s' len=%d.\n",
					     escape_essid((const char*)network->ssid, network->ssid_len), network->ssid_len);
			
			break;

		case MFIE_TYPE_RATES:
			network->rates_len = min(info_element->len,
						 MAX_RATES_LENGTH);
			for (i = 0; i < network->rates_len; i++) {
				network->rates[i] = info_element->data[i];
				if (ieee80211_is_ofdm_rate
				    (info_element->data[i])) {
					network->flags |= NETWORK_HAS_OFDM;
					if (info_element->data[i] &
					    IEEE80211_BASIC_RATE_MASK)
						network->flags &=
						    ~NETWORK_HAS_CCK;
				}
			}
			break;

		case MFIE_TYPE_RATES_EX:
			network->rates_ex_len = min(info_element->len,
						    MAX_RATES_EX_LENGTH);
			for (i = 0; i < network->rates_ex_len; i++) {
				network->rates_ex[i] = info_element->data[i];
				if (ieee80211_is_ofdm_rate
				    (info_element->data[i])) {
					network->flags |= NETWORK_HAS_OFDM;
					if (info_element->data[i] &
					    IEEE80211_BASIC_RATE_MASK)
						network->flags &=
						    ~NETWORK_HAS_CCK;
				}
			}

			break;

		case MFIE_TYPE_DS_SET:
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_DS_SET: %d\n",
					     info_element->data[0]);
			network->channel = info_element->data[0];
			break;

		case MFIE_TYPE_FH_SET:
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_FH_SET: ignored\n");
			break;

		case MFIE_TYPE_CF_SET:
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_CF_SET: ignored\n");
			break;

		case MFIE_TYPE_TIM:
			network->tim.tim_count = info_element->data[0];
			network->tim.tim_period = info_element->data[1];
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_TIM: partially ignored\n");
			break;

		case MFIE_TYPE_ERP_INFO:
			network->erp_value = info_element->data[0];
			network->flags |= NETWORK_HAS_ERP_VALUE;
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_ERP_SET: %d\n",
					     network->erp_value);
			break;

		case MFIE_TYPE_IBSS_SET:
			network->atim_window = info_element->data[0];
			//IEEE80211_DEBUG_MGMT("MFIE_TYPE_IBSS_SET: %d\n",network->atim_window);
			IWI_DEBUG("MFIE_TYPE_IBSS_SET (%02x:%02x:%02x:%02x:%02x:%02x)\n",
					MAC_ARG(network->atim_window));
			break;

		case MFIE_TYPE_CHALLENGE:
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_CHALLENGE: ignored\n");
			break;

		case MFIE_TYPE_GENERIC:
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_GENERIC: %d bytes\n",
					     info_element->len);
						 
			if (qos_enable)
			if (!ieee80211_parse_qos_info_param_IE(info_element,
							       network))
				break;

			if (info_element->len >= 4 &&
			    info_element->data[0] == 0x00 &&
			    info_element->data[1] == 0x50 &&
			    info_element->data[2] == 0xf2 &&
			    info_element->data[3] == 0x01) {
				network->wpa_ie_len = min(info_element->len + 2,
							  MAX_WPA_IE_LEN);
				memcpy(network->wpa_ie, info_element,
				       network->wpa_ie_len);
			}
			break;

		case MFIE_TYPE_RSN:
			IEEE80211_DEBUG_MGMT("MFIE_TYPE_RSN: %d bytes\n",
					     info_element->len);
			network->rsn_ie_len = min(info_element->len + 2,
						  MAX_WPA_IE_LEN);
			memcpy(network->rsn_ie, info_element,
			       network->rsn_ie_len);
			break;

		case MFIE_TYPE_QOS_PARAMETER:
			IWI_DEBUG(
			       "ERROR: QoS Error need to parse QOS_PARAMETER IE\n");
			break;
			/* 802.11h */
		case MFIE_TYPE_POWER_CONSTRAINT:
			network->power_constraint = info_element->data[0];
			network->flags |= NETWORK_HAS_POWER_CONSTRAINT;
			break;

		case MFIE_TYPE_CSA:
			network->power_constraint = info_element->data[0];
			network->flags |= NETWORK_HAS_CSA;
			break;

		case MFIE_TYPE_QUIET:
			network->quiet.count = info_element->data[0];
			network->quiet.period = info_element->data[1];
			network->quiet.duration = info_element->data[2];
			network->quiet.offset = info_element->data[3];
			network->flags |= NETWORK_HAS_QUIET;
			break;

		case MFIE_TYPE_IBSS_DFS:
		    IEEE80211_DEBUG_MGMT("MFIE_TYPE_IBSS_DFS:\n");
			 
			if (network->ibss_dfs)
				break;
			(void*)network->ibss_dfs =
			    kmalloc(info_element->len, NULL);
			if (!network->ibss_dfs)
				return 1;
			memcpy(network->ibss_dfs, info_element->data,
			       info_element->len);
			network->flags |= NETWORK_HAS_IBSS_DFS;
			
			break;

		case MFIE_TYPE_TPC_REPORT:
			network->tpc_report.transmit_power =
			    info_element->data[0];
			network->tpc_report.link_margin = info_element->data[1];
			network->flags |= NETWORK_HAS_TPC_REPORT;
			break;

		default:
			IEEE80211_DEBUG_MGMT
			    ("Unsupported info element: %d\n",
			     get_info_element_string(info_element->id),
			     info_element->id);
			break;
		}

		length -= sizeof(*info_element) + info_element->len;
		info_element =
		    (struct ieee80211_info_element *)&info_element->
		    data[info_element->len];
	}

	return 0;
}

int darwin_iwi2200::ieee80211_network_init(struct ieee80211_device *ieee, struct ieee80211_probe_response
					 *beacon,
					 struct ieee80211_network *network,
					 struct ieee80211_rx_stats *stats)
{
    // add by kazu expire qos routine
	network->qos_data.active = 0;
	network->qos_data.supported = 0;
	network->qos_data.param_count = 0;
	network->qos_data.old_param_count = 0;
    

	/* Pull out fixed field data */
	memcpy(network->bssid, beacon->header.addr3, ETH_ALEN);
	network->capability = le16_to_cpu(beacon->capability);
	network->last_scanned = ieee->scans;
	network->time_stamp[0] = le32_to_cpu(beacon->time_stamp[0]);
	network->time_stamp[1] = le32_to_cpu(beacon->time_stamp[1]);
	network->beacon_interval = le16_to_cpu(beacon->beacon_interval);
	/* Where to pull this? beacon->listen_interval; */
	network->listen_interval = 0x0A;
	network->rates_len = network->rates_ex_len = 0;
	network->last_associate = 0;
	network->ssid_len = 0;
	network->flags = 0;
	network->atim_window = 0;
	network->erp_value = (network->capability & WLAN_CAPABILITY_IBSS) ?
	    0x3 : 0x0;

	if (stats->freq == IEEE80211_52GHZ_BAND) {
		/* for A band (No DS info) */
		network->channel = stats->received_channel;
	} else
		network->flags |= NETWORK_HAS_CCK;

	network->wpa_ie_len = 0;
	network->rsn_ie_len = 0;

     
	if (ieee80211_parse_info_param
	    (beacon->info_element, stats->len - sizeof(*beacon), network))
		return 1;
    
	
	
	network->mode = 0;
	if (stats->freq == IEEE80211_52GHZ_BAND)
		network->mode = IEEE_A;
	else {
		if (network->flags & NETWORK_HAS_OFDM)
			network->mode |= IEEE_G;
		if (network->flags & NETWORK_HAS_CCK)
			network->mode |= IEEE_B;
	}

	if (network->mode == 0) {
		IEEE80211_DEBUG_SCAN("Filtered out '%s (" MAC_FMT ")' "
				     "network.\n",
				     escape_essid((const char *)network->ssid,
						  network->ssid_len),
				     MAC_ARG(network->bssid));
		return 1;
	}
   
	if (ieee80211_is_empty_essid((const char *)network->ssid, network->ssid_len))
		network->flags |= NETWORK_EMPTY_ESSID;

	memcpy(&network->stats, stats, sizeof(network->stats));

	return 0;
}
void darwin_iwi2200::ieee80211_process_probe_response(struct ieee80211_device *ieee,
        struct ieee80211_probe_response *beacon,
        struct ieee80211_rx_stats *stats)
{
	struct net_device *dev = ieee->dev;
	struct ieee80211_network network;
		network.ibss_dfs = NULL;
	struct ieee80211_network *target;
	struct ieee80211_network *oldest = NULL;
	unsigned long flags;
	struct ieee80211_info_element *info_element = beacon->info_element;
	
	IEEE80211_DEBUG_SCAN("'%s' (" MAC_FMT
			     "): %c%c%c%c %c%c%c%c-%c%c%c%c %c%c%c%c\n",
			     escape_essid((const char*)info_element->data,
					  info_element->len),
			     MAC_ARG(beacon->header.addr3),
			     (beacon->capability & (1 << 0xf)) ? '1' : '0',
			     (beacon->capability & (1 << 0xe)) ? '1' : '0',
			     (beacon->capability & (1 << 0xd)) ? '1' : '0',
			     (beacon->capability & (1 << 0xc)) ? '1' : '0',
			     (beacon->capability & (1 << 0xb)) ? '1' : '0',
			     (beacon->capability & (1 << 0xa)) ? '1' : '0',
			     (beacon->capability & (1 << 0x9)) ? '1' : '0',
			     (beacon->capability & (1 << 0x8)) ? '1' : '0',
			     (beacon->capability & (1 << 0x7)) ? '1' : '0',
			     (beacon->capability & (1 << 0x6)) ? '1' : '0',
			     (beacon->capability & (1 << 0x5)) ? '1' : '0',
			     (beacon->capability & (1 << 0x4)) ? '1' : '0',
			     (beacon->capability & (1 << 0x3)) ? '1' : '0',
			     (beacon->capability & (1 << 0x2)) ? '1' : '0',
			     (beacon->capability & (1 << 0x1)) ? '1' : '0',
			     (beacon->capability & (1 << 0x0)) ? '1' : '0');

	if (ieee80211_network_init(ieee, beacon, &network, stats)) {
		IEEE80211_DEBUG_SCAN("Dropped '%s' (" MAC_FMT ") via %s.\n",
				     escape_essid((const char*)info_element->data,
						  info_element->len),
				     MAC_ARG(beacon->header.addr3),
				     is_beacon(beacon->header.frame_ctl) ?
				     "BEACON" : "PROBE RESPONSE");
		return;
	}

	/* The network parsed correctly -- so now we scan our known networks
	 * to see if we can find it in our list.
	 *
	 * NOTE:  This search is definitely not optimized.  Once its doing
	 *        the "right thing" we'll optimize it for efficiency if
	 *        necessary */

	/* Search for this entry in the list and update it if it is
	 * already there. */

	//spin_lock_irqsave(&ieee->lock, flags);

	list_for_each_entry(target, &ieee->network_list, list) {
		if (is_same_network(target, &network))
			break;

		if ((oldest == NULL) ||
		    (target->last_scanned < oldest->last_scanned))
			oldest = target;
	}

	/* If we didn't find a match, then get a new network slot to initialize
	 * with this beacon's information */
	if (&target->list == &ieee->network_list) {
		if (list_empty(&ieee->network_free_list)) {
			/* If there are no more slots, expire the oldest */
			list_del(&oldest->list);
			target = oldest;
			IEEE80211_DEBUG_SCAN("Expired '%s' (" MAC_FMT ") from "
					     "network list.\n",
					     escape_essid((const char*)target->ssid,
							  target->ssid_len),
					     MAC_ARG(target->bssid));
			ieee80211_network_reset(target);
		} else {
			/* Otherwise just pull from the free list */
			target = list_entry(ieee->network_free_list.next,
					    struct ieee80211_network, list);
			list_del(ieee->network_free_list.next);
		}

		memcpy(target, &network, sizeof(*target));
		network.ibss_dfs = NULL;
		list_add_tail(&target->list, &ieee->network_list);
	} else {
		IEEE80211_DEBUG_SCAN("Updating '%s' (" MAC_FMT ") via %s.\n",
				     escape_essid((const char*)target->ssid,
						  target->ssid_len),
				     MAC_ARG(target->bssid),
				     is_beacon(beacon->header.frame_ctl) ?
				     "BEACON" : "PROBE RESPONSE");
		update_network(target, &network);
		network.ibss_dfs = NULL;
	}

	//spin_unlock_irqrestore(&ieee->lock, flags);

	if (is_beacon(beacon->header.frame_ctl)) {
				ipw_handle_probe_response(dev, beacon, target);
		//if (ieee->handle_beacon != NULL)
		//	ieee->handle_beacon(dev, beacon, target);
	} else {
				ipw_handle_beacon(dev, beacon, target);
		//if (ieee->handle_probe_response != NULL)
		//	ieee->handle_probe_response(dev, beacon, target);
	}
}

void darwin_iwi2200::ipw_bg_qos_activate(void *data)
{
#ifdef CONFIG_IPW2200_QOS
	struct ipw_priv *priv = (struct ipw_priv*)data;

	if (priv == NULL)
		return;

	//mutex_lock(&priv->mutex);

	if (priv->status & STATUS_ASSOCIATED)
		ipw_qos_activate(priv, &(priv->assoc_network->qos_data));

	//mutex_unlock(&priv->mutex);
#endif	
}

int darwin_iwi2200::ipw_qos_handle_probe_response(struct ipw_priv *priv,
					 int active_network,
					 struct ieee80211_network *network)
{
	u32 size = sizeof(struct ieee80211_qos_parameters);

	if (network->capability & WLAN_CAPABILITY_IBSS)
		network->qos_data.active = network->qos_data.supported;

	if (network->flags & NETWORK_HAS_QOS_MASK) {
		if (active_network &&
		    (network->flags & NETWORK_HAS_QOS_PARAMETERS))
			network->qos_data.active = network->qos_data.supported;

		if ((network->qos_data.active == 1) && (active_network == 1) &&
		    (network->flags & NETWORK_HAS_QOS_PARAMETERS) &&
		    (network->qos_data.old_param_count !=
		     network->qos_data.param_count)) {
			network->qos_data.old_param_count =
			    network->qos_data.param_count;
				queue_te(9,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_bg_qos_activate),priv,NULL,true);
			//schedule_work(&priv->qos_activate);
			IWI_DEBUG("QoS parameters change call "
				      "qos_activate\n");
		}
	} else {
		if ((priv->ieee->mode == IEEE_B) || (network->mode == IEEE_B))
			memcpy(&network->qos_data.parameters,
			       &def_parameters_CCK, size);
		else
			memcpy(&network->qos_data.parameters,
			       &def_parameters_OFDM, size);

		if ((network->qos_data.active == 1) && (active_network == 1)) {
			IWI_DEBUG("QoS was disabled call qos_activate \n");
			queue_te(9,OSMemberFunctionCast(thread_call_func_t,this,&darwin_iwi2200::ipw_bg_qos_activate),priv,NULL,true);
			//schedule_work(&priv->qos_activate);
		}

		network->qos_data.active = 0;
		network->qos_data.supported = 0;
	}

	if ((priv->status & STATUS_ASSOCIATED) &&
	    (priv->ieee->iw_mode == IW_MODE_ADHOC) && (active_network == 0)) {
		if (memcmp(network->bssid, priv->bssid, ETH_ALEN))
			if ((network->capability & WLAN_CAPABILITY_IBSS) &&
			    !(network->flags & NETWORK_EMPTY_ESSID))
				if ((network->ssid_len ==
				     priv->assoc_network->ssid_len) &&
				    !memcmp(network->ssid,
					    priv->assoc_network->ssid,
					    network->ssid_len)) {
					//queue_work(priv->workqueue,&priv->merge_networks);
					ipw_merge_adhoc_network(priv);
				}
	}

	return 0;
}

int darwin_iwi2200::ipw_find_adhoc_network(struct ipw_priv *priv,
				  struct ipw_network_match *match,
				  struct ieee80211_network *network,
				  int roaming)
{
	struct ipw_supported_rates rates;

	/* Verify that this network's capability is compatible with the
	 * current mode (AdHoc or Infrastructure) */
	if ((priv->ieee->iw_mode == IW_MODE_ADHOC &&
	     !(network->capability & WLAN_CAPABILITY_IBSS))) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded due to "
				"capability mismatch.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	/* If we do not have an ESSID for this AP, we can not associate with
	 * it */
	if (network->flags & NETWORK_EMPTY_ESSID) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because of hidden ESSID.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	if (unlikely(roaming)) {
		/* If we are roaming, then ensure check if this is a valid
		 * network to try and roam to */
		if ((network->ssid_len != match->network->ssid_len) ||
		    memcmp(network->ssid, match->network->ssid,
			   network->ssid_len)) {
			IWI_DEBUG("Netowrk '%s (" MAC_FMT ")' excluded "
					"because of non-network ESSID.\n",
					escape_essid((const char*)network->ssid,
						     network->ssid_len),
					MAC_ARG(network->bssid));
			return 0;
		}
	} else {
		/* If an ESSID has been configured then compare the broadcast
		 * ESSID to ours */
		if ((priv->config & CFG_STATIC_ESSID) &&
		    ((network->ssid_len != priv->essid_len) ||
		     memcmp(network->ssid, priv->essid,
			    min(network->ssid_len, priv->essid_len)))) {
			char escaped[IW_ESSID_MAX_SIZE * 2 + 1];

			strncpy(escaped,
				escape_essid((const char*)network->ssid, network->ssid_len),
				sizeof(escaped));
			IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
					"because of ESSID mismatch: '%s'.\n",
					escaped, MAC_ARG(network->bssid),
					escape_essid((const char*)priv->essid,
						     priv->essid_len));
			return 0;
		}
	}

	/* If the old network rate is better than this one, don't bother
	 * testing everything else. */

	if (network->time_stamp[0] < match->network->time_stamp[0]) {
		IWI_DEBUG("Network '%s excluded because newer than "
				"current network.\n",
				escape_essid((const char*)match->network->ssid,
					     match->network->ssid_len));
		return 0;
	} else if (network->time_stamp[1] < match->network->time_stamp[1]) {
		IWI_DEBUG("Network '%s excluded because newer than "
				"current network.\n",
				escape_essid((const char*)match->network->ssid,
					     match->network->ssid_len));
		return 0;
	}

	/* Now go through and see if the requested network is valid... */
	if (priv->ieee->scan_age != 0 &&
	    time_after(jiffies, network->last_scanned + priv->ieee->scan_age)) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because of age: %ums.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid),
				jiffies_to_msecs(jiffies -
						 network->last_scanned));
		return 0;
	}

	if ((priv->config & CFG_STATIC_CHANNEL) &&
	    (network->channel != priv->channel)) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because of channel mismatch: %d != %d.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid),
				network->channel, priv->channel);
		return 0;
	}

	/* Verify privacy compatability */
	if (((priv->capability & CAP_PRIVACY_ON) ? 1 : 0) !=
	    ((network->capability & WLAN_CAPABILITY_PRIVACY) ? 1 : 0)) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because of privacy mismatch: %s != %s.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid),
				priv->
				capability & CAP_PRIVACY_ON ? "on" : "off",
				network->
				capability & WLAN_CAPABILITY_PRIVACY ? "on" :
				"off");
		return 0;
	}

	if (!memcmp(network->bssid, priv->bssid, ETH_ALEN)) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because of the same BSSID match: " MAC_FMT
				".\n", escape_essid((const char*)network->ssid,
						    network->ssid_len),
				MAC_ARG(network->bssid), MAC_ARG(priv->bssid));
		return 0;
	}

	/* Filter out any incompatible freq / mode combinations */
	if (!ieee80211_is_valid_mode(priv->ieee, network->mode)) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because of invalid frequency/mode "
				"combination.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	/* Ensure that the rates supported by the driver are compatible with
	 * this AP, including verification of basic rates (mandatory) */
	if (!ipw_compatible_rates(priv, network, &rates)) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because configured rate mask excludes "
				"AP mandatory rate.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	if (rates.num_rates == 0) {
		IWI_DEBUG("Network '%s (" MAC_FMT ")' excluded "
				"because of no compatible rates.\n",
				escape_essid((const char*)network->ssid, network->ssid_len),
				MAC_ARG(network->bssid));
		return 0;
	}

	/* TODO: Perform any further minimal comparititive tests.  We do not
	 * want to put too much policy logic here; intelligent scan selection
	 * should occur within a generic IEEE 802.11 user space tool.  */

	/* Set up 'new' AP to this network */
	ipw_copy_rates(&match->rates, &rates);
	match->network = network;
	IWI_DEBUG("Network '%s (" MAC_FMT ")' is a viable match.\n",
			escape_essid((const char*)network->ssid, network->ssid_len),
			MAC_ARG(network->bssid));

	return 1;
}

void darwin_iwi2200::ipw_merge_adhoc_network(void *data)
{
	//struct ipw_priv *priv = data;
	struct ieee80211_network *network = NULL;
	struct ipw_network_match match;
		match.network = priv->assoc_network;


	if ((priv->status & STATUS_ASSOCIATED) &&
	    (priv->ieee->iw_mode == IW_MODE_ADHOC)) {
		/* First pass through ROAM process -- look for a better
		 * network */
		unsigned long flags;

		//spin_lock_irqsave(&priv->ieee->lock, flags);
		list_for_each_entry(network, &priv->ieee->network_list, list) {
			if (network != priv->assoc_network)
				ipw_find_adhoc_network(priv, &match, network,
						       1);
		}
		//spin_unlock_irqrestore(&priv->ieee->lock, flags);

		if (match.network == priv->assoc_network) {
			IWI_DEBUG("No better ADHOC in this network to "
					"merge to.\n");
			return;
		}

		//mutex_lock(&priv->mutex);
		if ((priv->ieee->iw_mode == IW_MODE_ADHOC)) {
			IWI_DEBUG("remove network %s\n",
					escape_essid((const char*)priv->essid,
						     priv->essid_len));
			ipw_remove_current_network(priv);
		}
		fNetif->setLinkState(kIO80211NetworkLinkDown);
		ipw_disassociate(priv);
		priv->assoc_network = match.network;
		//mutex_unlock(&priv->mutex);
		return;
	}
}

int darwin_iwi2200::ipw_handle_probe_response(struct net_device *dev,
				     struct ieee80211_probe_response *resp,
				     struct ieee80211_network *network)
{
	//struct ipw_priv *priv = ieee80211_priv(dev);

	int active_network = ((priv->status & STATUS_ASSOCIATED) &&
			      (network == priv->assoc_network));

	if (qos_enable)
	ipw_qos_handle_probe_response(priv, active_network, network);

	return 0;
}

int darwin_iwi2200::ipw_handle_beacon(struct net_device *dev,
			     struct ieee80211_beacon *resp,
			     struct ieee80211_network *network)
{
	//struct ipw_priv *priv = ieee80211_priv(dev);
	int active_network = ((priv->status & STATUS_ASSOCIATED) &&
			      (network == priv->assoc_network));

	if (qos_enable)
	ipw_qos_handle_probe_response(priv, active_network, network);

	return 0;
}

void darwin_iwi2200::update_network(struct ieee80211_network *dst,
				  struct ieee80211_network *src)
{
	int qos_active;
	u8 old_param;

	ieee80211_network_reset(dst);
	dst->ibss_dfs = src->ibss_dfs;

	/* We only update the statistics if they were created by receiving
	 * the network information on the actual channel the network is on.
	 *
	 * This keeps beacons received on neighbor channels from bringing
	 * down the signal level of an AP. */
	if (dst->channel == src->stats.received_channel)
		memcpy(&dst->stats, &src->stats,
		       sizeof(struct ieee80211_rx_stats));
	else
		IEEE80211_DEBUG_SCAN("Network " MAC_FMT " info received "
			"off channel (%d vs. %d)\n", MAC_ARG(src->bssid),
			dst->channel, src->stats.received_channel);

	dst->capability = src->capability;
	memcpy(dst->rates, src->rates, src->rates_len);
	dst->rates_len = src->rates_len;
	memcpy(dst->rates_ex, src->rates_ex, src->rates_ex_len);
	dst->rates_ex_len = src->rates_ex_len;

	dst->mode = src->mode;
	dst->flags = src->flags;
	dst->time_stamp[0] = src->time_stamp[0];
	dst->time_stamp[1] = src->time_stamp[1];

	dst->beacon_interval = src->beacon_interval;
	dst->listen_interval = src->listen_interval;
	dst->atim_window = src->atim_window;
	dst->erp_value = src->erp_value;
	dst->tim = src->tim;

	memcpy(dst->wpa_ie, src->wpa_ie, src->wpa_ie_len);
	dst->wpa_ie_len = src->wpa_ie_len;
	memcpy(dst->rsn_ie, src->rsn_ie, src->rsn_ie_len);
	dst->rsn_ie_len = src->rsn_ie_len;

	dst->last_scanned = jiffies;
	qos_active = src->qos_data.active;
	old_param = dst->qos_data.old_param_count;
	if (dst->flags & NETWORK_HAS_QOS_MASK)
		memcpy(&dst->qos_data, &src->qos_data,
		       sizeof(struct ieee80211_qos_data));
	else {
		dst->qos_data.supported = src->qos_data.supported;
		dst->qos_data.param_count = src->qos_data.param_count;
	}

	if (dst->qos_data.supported == 1) {
		if (dst->ssid_len)
			IEEE80211_DEBUG_QOS
			    ("QoS the network %s is QoS supported\n",
			     dst->ssid);
		else
			IEEE80211_DEBUG_QOS
			    ("QoS the network is QoS supported\n");
	}
	dst->qos_data.active = qos_active;
	dst->qos_data.old_param_count = old_param;

	/* dst->last_associate is not overwritten */
}

void darwin_iwi2200::getPacketBufferConstraints(IOPacketBufferConstraints * constraints) const {
    constraints->alignStart  = kIOPacketBufferAlign4;	// even word aligned.
    constraints->alignLength = kIOPacketBufferAlign4;	// no restriction.
}

mbuf_t darwin_iwi2200::alloc_skb(unsigned int size, UInt32 priority)
{
	mbuf_t skb;
	
	skb = allocatePacket(size);
	if (skb == 0){ /* fail to allocatePacket */
		IWI_WARNING("failed to allocatePacket  of size[%d]\n", size);
		return NULL;
	}
	//  m_len is alocated size in default.
	// but length  is initialized as 0  in linux'skb
	mbuf_setlen(skb,0);
	mbuf_pkthdr_setlen(skb,0);
	
	return skb;
}

