/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <syscfg/syscfg.h>

#if MYNEWT_VAL(BLE_LL_DIRECT_TEST_MODE) == 1

#include "assert.h"
#include "os/os.h"
#include "controller/ble_ll.h"
#include "controller/ble_phy.h"
#include "controller/ble_ll_sched.h"
#include "ble_ll_dtm_priv.h"

struct dtm_ctx {
    uint8_t payload_packet;
    uint8_t itvl_rem_usec;
    uint16_t num_of_packets;
    uint32_t itvl_ticks;
    int active;
    int chan;
    int phy_mode;
    struct os_mbuf *om;
    struct os_event evt;
    struct ble_ll_sched_item sch;
};

static struct dtm_ctx g_ble_ll_dtm_ctx;

static const uint8_t g_ble_ll_dtm_prbs9_data[] =
{
    0xff, 0xc1, 0xfb, 0xe8, 0x4c, 0x90, 0x72, 0x8b,
    0xe7, 0xb3, 0x51, 0x89, 0x63, 0xab, 0x23, 0x23,
    0x02, 0x84, 0x18, 0x72, 0xaa, 0x61, 0x2f, 0x3b,
    0x51, 0xa8, 0xe5, 0x37, 0x49, 0xfb, 0xc9, 0xca,
    0x0c, 0x18, 0x53, 0x2c, 0xfd, 0x45, 0xe3, 0x9a,
    0xe6, 0xf1, 0x5d, 0xb0, 0xb6, 0x1b, 0xb4, 0xbe,
    0x2a, 0x50, 0xea, 0xe9, 0x0e, 0x9c, 0x4b, 0x5e,
    0x57, 0x24, 0xcc, 0xa1, 0xb7, 0x59, 0xb8, 0x87,
    0xff, 0xe0, 0x7d, 0x74, 0x26, 0x48, 0xb9, 0xc5,
    0xf3, 0xd9, 0xa8, 0xc4, 0xb1, 0xd5, 0x91, 0x11,
    0x01, 0x42, 0x0c, 0x39, 0xd5, 0xb0, 0x97, 0x9d,
    0x28, 0xd4, 0xf2, 0x9b, 0xa4, 0xfd, 0x64, 0x65,
    0x06, 0x8c, 0x29, 0x96, 0xfe, 0xa2, 0x71, 0x4d,
    0xf3, 0xf8, 0x2e, 0x58, 0xdb, 0x0d, 0x5a, 0x5f,
    0x15, 0x28, 0xf5, 0x74, 0x07, 0xce, 0x25, 0xaf,
    0x2b, 0x12, 0xe6, 0xd0, 0xdb, 0x2c, 0xdc, 0xc3,
    0x7f, 0xf0, 0x3e, 0x3a, 0x13, 0xa4, 0xdc, 0xe2,
    0xf9, 0x6c, 0x54, 0xe2, 0xd8, 0xea, 0xc8, 0x88,
    0x00, 0x21, 0x86, 0x9c, 0x6a, 0xd8, 0xcb, 0x4e,
    0x14, 0x6a, 0xf9, 0x4d, 0xd2, 0x7e, 0xb2, 0x32,
    0x03, 0xc6, 0x14, 0x4b, 0x7f, 0xd1, 0xb8, 0xa6,
    0x79, 0x7c, 0x17, 0xac, 0xed, 0x06, 0xad, 0xaf,
    0x0a, 0x94, 0x7a, 0xba, 0x03, 0xe7, 0x92, 0xd7,
    0x15, 0x09, 0x73, 0xe8, 0x6d, 0x16, 0xee, 0xe1,
    0x3f, 0x78, 0x1f, 0x9d, 0x09, 0x52, 0x6e, 0xf1,
    0x7c, 0x36, 0x2a, 0x71, 0x6c, 0x75, 0x64, 0x44,
    0x80, 0x10, 0x43, 0x4e, 0x35, 0xec, 0x65, 0x27,
    0x0a, 0xb5, 0xfc, 0x26, 0x69, 0x3f, 0x59, 0x99,
    0x01, 0x63, 0x8a, 0xa5, 0xbf, 0x68, 0x5c, 0xd3,
    0x3c, 0xbe, 0x0b, 0xd6, 0x76, 0x83, 0xd6, 0x57,
    0x05, 0x4a, 0x3d, 0xdd, 0x81, 0x73, 0xc9, 0xeb,
    0x8a, 0x84, 0x39, 0xf4, 0x36, 0x0b, 0xf7
};

static const uint8_t g_ble_ll_dtm_prbs15_data[] =
{
    0xff, 0x7f, 0xf0, 0x3e, 0x3a, 0x13, 0xa4, 0xdc,
    0xe2, 0xf9, 0x6c, 0x54, 0xe2, 0xd8, 0xea, 0xc8,
    0x88, 0x00, 0x21, 0x86, 0x9c, 0x6a, 0xd8, 0xcb,
    0x4e, 0x14, 0x6a, 0xf9, 0x4d, 0xd2, 0x7e, 0xb2,
    0x32, 0x03, 0xc6, 0x14, 0x4b, 0x7f, 0xd1, 0xb8,
    0xa6, 0x79, 0x7c, 0x17, 0xac, 0xed, 0x06, 0xad,
    0xaf, 0x0a, 0x94, 0x7a, 0xba, 0x03, 0xe7, 0x92,
    0xd7, 0x15, 0x09, 0x73, 0xe8, 0x6d, 0x16, 0xee,
    0xe1, 0x3f, 0x78, 0x1f, 0x9d, 0x09, 0x52, 0x6e,
    0xf1, 0x7c, 0x36, 0x2a, 0x71, 0x6c, 0x75, 0x64,
    0x44, 0x80, 0x10, 0x43, 0x4e, 0x35, 0xec, 0x65,
    0x27, 0x0a, 0xb5, 0xfc, 0x26, 0x69, 0x3f, 0x59,
    0x99, 0x01, 0x63, 0x8a, 0xa5, 0xbf, 0x68, 0x5c,
    0xd3, 0x3c, 0xbe, 0x0b, 0xd6, 0x76, 0x83, 0xd6,
    0x57, 0x05, 0x4a, 0x3d, 0xdd, 0x81, 0x73, 0xc9,
    0xeb, 0x8a, 0x84, 0x39, 0xf4, 0x36, 0x0b, 0xf7,
    0xf0, 0x1f, 0xbc, 0x8f, 0xce, 0x04, 0x29, 0xb7,
    0x78, 0x3e, 0x1b, 0x95, 0x38, 0xb6, 0x3a, 0x32,
    0x22, 0x40, 0x88, 0x21, 0xa7, 0x1a, 0xf6, 0xb2,
    0x13, 0x85, 0x5a, 0x7e, 0x93, 0xb4, 0x9f, 0xac,
    0xcc, 0x80, 0x31, 0xc5, 0xd2, 0x5f, 0x34, 0xae,
    0x69, 0x1e, 0xdf, 0x05, 0x6b, 0xbb, 0x41, 0xeb,
    0xab, 0x02, 0xa5, 0x9e, 0xee, 0xc0, 0xb9, 0xe4,
    0x75, 0x45, 0xc2, 0x1c, 0x7a, 0x9b, 0x85, 0x7b,
    0xf8, 0x0f, 0xde, 0x47, 0x67, 0x82, 0x94, 0x5b,
    0x3c, 0x9f, 0x8d, 0x4a, 0x1c, 0x5b, 0x1d, 0x19,
    0x11, 0x20, 0xc4, 0x90, 0x53, 0x0d, 0x7b, 0xd9,
    0x89, 0x42, 0x2d, 0xbf, 0x49, 0xda, 0x4f, 0x56,
    0x66, 0xc0, 0x98, 0x62, 0xe9, 0x2f, 0x1a, 0xd7,
    0x34, 0x8f, 0xef, 0x82, 0xb5, 0xdd, 0xa0, 0xf5,
    0x55, 0x81, 0x52, 0x4f, 0x77, 0xe0, 0x5c, 0xf2,
    0xba, 0x22, 0x61, 0x0e, 0xbd, 0xcd, 0xc2
};

#define BLE_DTM_SYNC_WORD          (0x71764129)
#define BLE_DTM_CRC                (0x555555)

static void
ble_ll_dtm_set_next(struct dtm_ctx *ctx)
{
    struct ble_ll_sched_item *sch = &ctx->sch;

    sch->start_time += ctx->itvl_ticks;
    sch->remainder += ctx->itvl_rem_usec;
    if (sch->remainder > 30) {
       sch->start_time++;
       sch->remainder -= 30;
    }

    sch->start_time -= g_ble_ll_sched_offset_ticks;
}

static void
ble_ll_dtm_event(struct os_event *evt) {
    /* It is called in LL context */
    struct dtm_ctx *ctx = evt->ev_arg;
    int rc;
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);
    if (!ctx->active || !ctx->om) {
        OS_EXIT_CRITICAL(sr);
        return;
    }
    OS_EXIT_CRITICAL(sr);

    ble_ll_dtm_set_next(ctx);
    rc = ble_ll_sched_dtm(&ctx->sch);
    assert(rc == 0);
}

static void
ble_ll_dtm_tx_done(void *arg)
{
    struct dtm_ctx *ctx;

    ctx = arg;
    if (!ctx->evt.ev_cb) {
        return;
    }
    ctx->num_of_packets++;
    /* Reschedule event in LL context */
    os_eventq_put(&g_ble_ll_data.ll_evq, &ctx->evt);

    ble_ll_state_set(BLE_LL_STATE_STANDBY);
}

static int
ble_ll_dtm_tx_sched_cb(struct ble_ll_sched_item *sch)
{
    struct dtm_ctx *ctx = sch->cb_arg;
    int rc;

    if (!ctx->active) {
        return BLE_LL_SCHED_STATE_DONE;
    }

    rc = ble_phy_setchan(ctx->chan, BLE_DTM_SYNC_WORD, BLE_DTM_CRC);
    if (rc != 0) {
        assert(0);
        return BLE_LL_SCHED_STATE_DONE;
    }

    ble_phy_mode_set(ctx->phy_mode, ctx->phy_mode);
    ble_phy_set_txend_cb(ble_ll_dtm_tx_done, ctx);
    ble_phy_txpwr_set(0);

    sch->start_time += g_ble_ll_sched_offset_ticks;

    /*XXX Maybe reschedule if too late */
    rc = ble_phy_tx_set_start_time(sch->start_time, sch->remainder);
    assert(rc == 0);

    rc = ble_phy_tx(ctx->om, BLE_PHY_TRANSITION_NONE);
    assert(rc == 0);

    ble_ll_state_set(BLE_LL_STATE_DTM);

    return BLE_LL_SCHED_STATE_DONE;
}

static void
ble_ll_dtm_calculate_itvl(struct dtm_ctx *ctx, uint8_t len, int phy_mode)
{
    uint32_t l;
    uint32_t itvl_usec;
    uint32_t itvl_ticks;

    /* Calculate interval as per spec Bluetooth 5.0 Vol 6. Part F, 4.1.6 */
    l = ble_ll_pdu_tx_time_get(len + BLE_LL_PDU_HDR_LEN, phy_mode);
    itvl_usec = ((l + 249 + 624) / 625) * 625;

    itvl_ticks = os_cputime_usecs_to_ticks(itvl_usec);
    ctx->itvl_rem_usec = (itvl_usec - os_cputime_ticks_to_usecs(itvl_ticks));
    if (ctx->itvl_rem_usec == 31) {
        ctx->itvl_rem_usec = 0;
        ++itvl_ticks;
    }
    ctx->itvl_ticks = itvl_ticks;
}

static int
ble_ll_dtm_tx_create_ctx(uint8_t packet_payload, uint8_t len, int chan,
                         int phy_mode)
{
    int rc = 0;
    uint8_t byte_pattern;
    struct ble_ll_sched_item *sch = &g_ble_ll_dtm_ctx.sch;

    /* MSYS is big enough to get continues memory */
    g_ble_ll_dtm_ctx.om = os_msys_get_pkthdr(len, sizeof(struct ble_mbuf_hdr));
    assert(g_ble_ll_dtm_ctx.om);

    g_ble_ll_dtm_ctx.phy_mode = phy_mode;
    g_ble_ll_dtm_ctx.chan = chan;

    /* Set BLE transmit header */
    ble_ll_mbuf_init(g_ble_ll_dtm_ctx.om, len, packet_payload);

    switch(packet_payload) {
    case 0x00:
        memcpy(g_ble_ll_dtm_ctx.om->om_data, &g_ble_ll_dtm_prbs9_data, len);
        goto schedule;
    case 0x01:
        byte_pattern = 0x0F;
        break;
    case 0x02:
        byte_pattern = 0x55;
        break;
    case 0x03:
        memcpy(g_ble_ll_dtm_ctx.om->om_data, &g_ble_ll_dtm_prbs15_data, len);
        goto schedule;
    case 0x04:
        byte_pattern = 0xFF;
        break;
    case 0x05:
        byte_pattern = 0x00;
        break;
    case 0x06:
        byte_pattern = 0xF0;
        break;
    case 0x07:
        byte_pattern = 0xAA;
        break;
    default:
        return 1;
    }

    memset(g_ble_ll_dtm_ctx.om->om_data, byte_pattern, len);

schedule:

    sch->sched_cb = ble_ll_dtm_tx_sched_cb;
    sch->cb_arg = &g_ble_ll_dtm_ctx;
    sch->sched_type = BLE_LL_SCHED_TYPE_DTM;
    sch->start_time =  os_cputime_get32() +
                                       os_cputime_usecs_to_ticks(5000);

    /* Prepare os_event */
    g_ble_ll_dtm_ctx.evt.ev_cb = ble_ll_dtm_event;
    g_ble_ll_dtm_ctx.evt.ev_arg = &g_ble_ll_dtm_ctx;

    ble_ll_dtm_calculate_itvl(&g_ble_ll_dtm_ctx, len, phy_mode);

    /* Set some start point for TX packets */
    rc = ble_ll_sched_dtm(sch);
    assert(rc == 0);

    g_ble_ll_dtm_ctx.active = 1;
    return 0;
}

static int
ble_ll_dtm_rx_start(void)
{
    int rc;

    rc = ble_phy_setchan(g_ble_ll_dtm_ctx.chan, BLE_DTM_SYNC_WORD,
                         BLE_DTM_CRC);
    assert(rc == 0);

    ble_phy_mode_set(g_ble_ll_dtm_ctx.phy_mode, g_ble_ll_dtm_ctx.phy_mode);

    rc = ble_phy_rx_set_start_time(os_cputime_get32() +
                                   g_ble_ll_sched_offset_ticks, 0);
    assert(rc == 0);

    ble_ll_state_set(BLE_LL_STATE_DTM);

    return 0;
}

static int
ble_ll_dtm_rx_create_ctx(int chan, int phy_mode)
{
    g_ble_ll_dtm_ctx.phy_mode = phy_mode;
    g_ble_ll_dtm_ctx.chan = chan;
    g_ble_ll_dtm_ctx.active = 1;

    if (ble_ll_dtm_rx_start() != 0) {
        assert(0);
        return 1;
    }

    return 0;
}

static void
ble_ll_dtm_ctx_free(struct dtm_ctx * ctx)
{
    os_sr_t sr;

    OS_ENTER_CRITICAL(sr);
    if (!ctx->active) {
        OS_EXIT_CRITICAL(sr);
        return;
    }
    OS_EXIT_CRITICAL(sr);

    ble_phy_disable();
    ble_ll_state_set(BLE_LL_STATE_STANDBY);
#ifdef BLE_XCVR_RFCLK
    ble_ll_xcvr_rfclk_stop();
#endif

    ble_ll_sched_rmv_elem(&ctx->sch);
    os_mbuf_free_chain(ctx->om);
    memset(ctx, 0, sizeof(*ctx));
}

int
ble_ll_dtm_tx_test(uint8_t *cmdbuf, bool enhanced)
{
    uint8_t tx_chan = cmdbuf[0];
    uint8_t len = cmdbuf[1];
    uint8_t packet_payload = cmdbuf[2];
    uint8_t phy_mode = BLE_PHY_MODE_1M;

    if (g_ble_ll_dtm_ctx.active) {
        return BLE_ERR_CTLR_BUSY;
    }

    if (enhanced) {
        switch (cmdbuf[3]) {
        case BLE_HCI_LE_PHY_1M:
            phy_mode = BLE_PHY_MODE_1M;
            break;
        case BLE_HCI_LE_PHY_2M:
            phy_mode = BLE_PHY_MODE_2M;
            break;
        case BLE_HCI_LE_PHY_CODED_S8:
            phy_mode = BLE_PHY_MODE_CODED_125KBPS;
            break;
        case BLE_HCI_LE_PHY_CODED_S2:
            phy_mode = BLE_PHY_MODE_CODED_500KBPS;
            break;
        default:
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }
    }

    if (tx_chan > 0x27 || packet_payload > 0x07) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (ble_ll_dtm_tx_create_ctx(packet_payload, len, tx_chan, phy_mode)) {
        return BLE_ERR_UNSPECIFIED;
    }

    return BLE_ERR_SUCCESS;
}

int ble_ll_dtm_rx_test(uint8_t *cmdbuf, bool enhanced)
{
    uint8_t rx_chan = cmdbuf[0];
    uint8_t phy_mode = BLE_PHY_MODE_1M;

    if (g_ble_ll_dtm_ctx.active) {
        return BLE_ERR_CTLR_BUSY;
    }

    /*XXX What to do with modulation cmdbuf[2]? */

    if (enhanced) {
        switch (cmdbuf[1]) {
        case BLE_HCI_LE_PHY_1M:
            phy_mode = BLE_PHY_MODE_1M;
            break;
        case BLE_HCI_LE_PHY_2M:
            phy_mode = BLE_PHY_MODE_2M;
            break;
        case BLE_HCI_LE_PHY_CODED:
            phy_mode = BLE_PHY_MODE_CODED_500KBPS;
            break;
        default:
            return BLE_ERR_INV_HCI_CMD_PARMS;
        }
    }

    if (rx_chan > 0x27) {
        return BLE_ERR_INV_HCI_CMD_PARMS;
    }

    if (ble_ll_dtm_rx_create_ctx(rx_chan, phy_mode)) {
        return BLE_ERR_UNSPECIFIED;
    }

    return BLE_ERR_SUCCESS;
}

int ble_ll_dtm_end_test(uint8_t *cmdbuf, uint8_t *rsp, uint8_t *rsplen)
{
    put_le16(rsp, g_ble_ll_dtm_ctx. num_of_packets);
    *rsplen = 2;

    ble_ll_dtm_ctx_free(&g_ble_ll_dtm_ctx);
    return BLE_ERR_SUCCESS;
}

int ble_ll_dtm_rx_isr_start(struct ble_mbuf_hdr *rxhdr, uint32_t aa)
{
    return 0;
}

void
ble_ll_dtm_rx_pkt_in(struct os_mbuf *rxpdu, struct ble_mbuf_hdr *hdr)
{
    if (BLE_MBUF_HDR_CRC_OK(hdr)) {
        /* XXX Compare data. */
        g_ble_ll_dtm_ctx.num_of_packets++;
    }

    if (ble_ll_dtm_rx_start() != 0) {
        assert(0);
    }
}

int
ble_ll_dtm_rx_isr_end(uint8_t *rxbuf, struct ble_mbuf_hdr *rxhdr)
{
    struct os_mbuf *rxpdu;

    if (!g_ble_ll_dtm_ctx.active) {
        return -1;
    }

    rxpdu = ble_ll_rxpdu_alloc(rxbuf[1] + BLE_LL_PDU_HDR_LEN);

    /* Copy the received pdu and hand it up */
    if (rxpdu) {
        ble_phy_rxpdu_copy(rxbuf, rxpdu);
        ble_ll_rx_pdu_in(rxpdu);
    }

    return 0;
}

void
ble_ll_dtm_wfr_timer_exp(void)
{
    /* Should not be needed */
    assert(0);
}


void
ble_ll_dtm_reset(void)
{
    ble_ll_dtm_ctx_free(&g_ble_ll_dtm_ctx);
}
#endif
