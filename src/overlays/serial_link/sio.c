#include "rr/overlay_module.h"
#include "rr/overlay667_status.h"
#include "rr/overlay667_events.h"
#include "rr/serial_handles.h"
#include "rr/overlay667_packet.h"
#include "types.h"
#include "rr/driver_control.h"

s32 ovl667_check_sio_timeout(s32 arg0, u32 arg1) {
    if (arg0 == 2) {
        if (arg1 >= 25000U) {
            return 0;
        }
    }
    return 1;
}

void ovl667_reset_sio_ports(void) {
    _sio_control(0, 0, 0);
    _sio_control(0, 1, 0);
}

s32 ovl667_open_sio_events(void) {
    ovl667_sio_read_event = OpenEvent(0xF000000B, 0x400, 0x2000, 0);
    ovl667_sio_error_event = OpenEvent(0xF000000B, 0x8000, 0x1000, ovl667_on_sio_error_event);
    LinkWriteHandle = FileOpen(ovl667_sio_device_name, 2);
    LinkReadHandle = FileOpen(ovl667_sio_device_name, 0x8001);
    EnableEvent(ovl667_sio_error_event);
    EnableEvent(ovl667_sio_read_event);
    return 0;
}

void ovl667_close_sio_session(void){
 _sio_control(2,3,0);_sio_control(2,0,0);
 DisableEvent(ovl667_sio_read_event);DisableEvent(ovl667_sio_error_event);
 FileClose(LinkWriteHandle);FileClose(LinkReadHandle);
 CloseEvent(ovl667_sio_read_event);CloseEvent(ovl667_sio_error_event);
 _sio_control(4,0,0);_sio_control(1,1,1);
}

void ovl667_init_sio_session(u32 configuration) {
    s32 work;
    u32 *base;
    /* Three word lanes in the genuine24-word ring. Keep cursor arithmetic
     * at full pointer width: after the last store, the unused second/third
     * integer cursors advance beyond the ring without forming C pointers. */
    uintptr_t first;
    uintptr_t second;
    uintptr_t third;
    SetDispMask(0);
    _sio_control(1,3,configuration);
    _sio_control(1,4,4);
    _sio_control(4,0,(uintptr_t)ovl667_check_sio_timeout);
    work=0;
    base=ovl667_rx_packet_ring.words;
    third=(uintptr_t)base+8;
    second=(uintptr_t)base+4;
    first=(uintptr_t)base;
    ovl667_checksum_error_flag=0;
    ovl667_active_slot_index=0;
    ovl667_consumed_slot_index=7;
    ovl667_rx_slot_index=0;
    ovl667_tx_slot_index=0;
    ovl667_rx_miss_count=0;
    ovl667_tx_miss_count=0;
    ovl667_rx_packet_count=0;
    ovl667_tx_packet_count=0;
    ovl667_read_pending_flag=0;
    ovl667_sync_retry_state=0;
    ovl667_remote_pause_bits=0;
    ovl667_local_pause_bits=0;
    ovl667_pending_pause_bits=0;
    do {
        *(u32 *)first=0;
        *(u32 *)second=0;
        *(u32 *)third=0;
        third+=12;
        second+=12;
        work++;
        first+=12;
    } while (work<8);
    ovl667_tx_staging_packet.words[0]=0;
    ovl667_tx_staging_packet.words[1]=0;
    ovl667_tx_staging_packet.words[2]=0;
    ovl667_rx_staging_packet.words[0]=0;
    ovl667_rx_staging_packet.words[1]=0;
    ovl667_rx_staging_packet.words[2]=0;
    LinkPendingCarInputs.words[0]=0;
    LinkPendingCarInputs.words[1]=0;
    LinkLocalCarInputs.words[0]=0;
    LinkLocalCarInputs.words[1]=0;
    LinkRemoteCarInputs.words[0]=0;
    LinkRemoteCarInputs.words[1]=0;
    ovl667_peer_race_state=0;
    ovl667_link_shutdown_stage=0;
    if (_sio_control(3,1,0) != 0) {
        _sio_control(3,0,1);
    } else {
        _sio_control(3,0,1);
        while (_sio_control(3,1,0)==0) {
            work=((u32)_sio_control(0,0,0)>>7)&3;
            if (work==0 || work==3) break;
        }
    }
    _sio_control(5,0,0);
    VSync(0);
    _sio_control(3,0,0);
}

s32 ovl667_on_sio_error_event(void) {
    union {u32 bits;s32 value;} count;
    count.bits=ovl667_sio_error_count+1U;
    ovl667_sio_error_count=count.value;
    ovl667_read_pending_flag = 0;
    return 0;
}

s32 ovl667_send_race_packet(void) {
    u8 csum;
    u32 i;
    if (_sio_control(3, 1, 0) == 0) {
        return 0;
    }
    ovl667_tx_staging_packet.packet.flags00 = ovl667_tx_slot_index;
    if (ovl667_checksum_error_flag == 1) {
        ovl667_tx_staging_packet.packet.flags00 |= 0x80;
    }
    csum = 0;
    i = 0;
    ovl667_tx_staging_packet.packet.first04 = LinkPendingCarInputs.cars[0];
    ovl667_tx_staging_packet.packet.second08 = LinkPendingCarInputs.cars[1];
    do {
        csum ^= ((u8 *)&ovl667_tx_staging_packet)[i + 4];
        i++;
    } while (i < 8);
    ovl667_tx_staging_packet.packet.checksum01 = csum;
    ovl667_tx_staging_packet.packet.status02 = RaceState;
    ovl667_tx_staging_packet.packet.phase03 = ovl667_pending_pause_bits;
    if (FileWrite(LinkWriteHandle, &ovl667_tx_staging_packet, 12) != 12) {
        return 2;
    }
    return 1;
}

/* The ring slot both ends step with: the other end's latest packet, the
 * one before it when that end is a slot ahead, and a slot of our own back
 * when we are two ahead. Host fix: the original compared the "ahead" cases
 * without wrapping the slot numbers modulo 8, so across the wrap (7 to 0)
 * they fell through to the latest packet and the two ends stepped with
 * different slots: the race desynced. On the console a cable and aligned
 * vblanks keep the ends in step and these cases do not come up; on the
 * host, thread scheduling and the network make them common. */
static void ovl667_select_active_slot(void) {
    if (ovl667_tx_slot_index == ovl667_rx_slot_index) {
        ovl667_active_slot_index = (ovl667_rx_slot_index + 7) & 7;
    } else if (ovl667_rx_slot_index == ((ovl667_tx_slot_index + 1) & 7)) {
        ovl667_active_slot_index = ovl667_tx_slot_index;
        ovl667_sync_retry_state = 3;
    } else if (ovl667_tx_slot_index == ((ovl667_rx_slot_index + 2) & 7)) {
        ovl667_active_slot_index = ovl667_rx_slot_index;
        ovl667_tx_slot_index = (ovl667_rx_slot_index + 9) & 7;
    } else {
        ovl667_active_slot_index = ovl667_rx_slot_index;
    }
}

s32 ovl667_poll_race_packet(void) {
    u8 checksum;
    u32 i;
    if (TestEvent(ovl667_sio_read_event) == 1) {
        ovl667_rx_idle_flag = 0;
        ovl667_checksum_error_flag = 0;
        checksum = 0;
        i = 0;
        do {
            checksum ^= ((u8 *)&ovl667_rx_staging_packet.packet)[i+4];
            i++;
        } while (i < 8);
        if (checksum != ovl667_rx_staging_packet.packet.checksum01) ovl667_checksum_error_flag = 1;
        if (ovl667_rx_staging_packet.packet.flags00 >> 7) {
            if ((ovl667_rx_staging_packet.packet.flags00 >> 6) & 1) ovl667_remote_error_code = 2;
            else ovl667_remote_error_code = 1;
        }
        ovl667_rx_slot_index = ovl667_rx_staging_packet.packet.flags00;
        ovl667_select_active_slot();
        ovl667_rx_packet_ring.packets[ovl667_rx_slot_index] = ovl667_rx_staging_packet.packet;
        FileRead(LinkReadHandle, &ovl667_rx_staging_packet.packet, 12);
        return 1;
    }
    if (ovl667_read_pending_flag != 0) return -1;
    _sio_control(2,0,0);
    FileRead(LinkReadHandle, &ovl667_rx_staging_packet.packet, 12);
    ovl667_read_pending_flag = 1;
    ovl667_rx_packet_count = (ovl667_rx_packet_count + 1U);
    return 0;
}

extern s32 ovl667_sio_error_count;
s32 ovl667_get_sio_error_count(void) { return ovl667_sio_error_count; }

s32 ovl667_get_sio_status_flag(void) { return ovl667_sio_status_flag; }

void ovl667_receive_peer_inputs(void) {
    if (ovl667_link_shutdown_stage > 0) {
        if (ovl667_link_shutdown_stage == 1) {
            ovl667_link_shutdown_stage = 2;
        }
    } else {
        ovl667_tx_stage = 1;
        if (ovl667_poll_race_packet() == 1) {
            ovl667_rx_miss_count = 0;
            ovl667_rx_idle_flag = 0;
            ovl667_rx_packet_count = (ovl667_rx_packet_count + 1U);
        } else {
            ovl667_rx_miss_count = (ovl667_rx_miss_count + 1U);
            ovl667_select_active_slot();
        }
        LinkRemoteCarInputs.cars[0] = ovl667_rx_packet_ring.packets[ovl667_active_slot_index].first04;
        LinkRemoteCarInputs.cars[1] = ovl667_rx_packet_ring.packets[ovl667_active_slot_index].second08;
        ovl667_remote_pause_bits = ovl667_rx_packet_ring.packets[ovl667_active_slot_index].phase03;
    }
}

void ovl667_transmit_local_inputs(void) {
    s32 state;
    if (ovl667_peer_race_state == RaceState && ovl667_peer_race_state >= 4) {
        if (ovl667_link_shutdown_stage == 0) ovl667_link_shutdown_stage = 1;
    } else if (ovl667_link_shutdown_stage <= 0) {
        state = ovl667_tx_stage;
        if (state == 1) {
            if (ovl667_send_race_packet() == state) {
                ovl667_tx_miss_count = 0;
                LinkLocalCarInputs.cars[0] = LinkPendingCarInputs.cars[0];
                LinkLocalCarInputs.cars[1] = LinkPendingCarInputs.cars[1];
                ovl667_tx_packet_count = (ovl667_tx_packet_count + 1U);
                ovl667_tx_slot_index = (ovl667_tx_slot_index + 1) & 7;
                ovl667_local_pause_bits = ovl667_pending_pause_bits;
                if (ovl667_sync_retry_state == state) ovl667_sync_retry_state = 0;
                ovl667_tx_stage = 0;
            } else ovl667_tx_stage = 2;
        } else if (state == 2) {
            if (ovl667_send_race_packet() == 1) {
                ovl667_tx_miss_count = 0;
                LinkLocalCarInputs.cars[0] = LinkPendingCarInputs.cars[0];
                LinkLocalCarInputs.cars[1] = LinkPendingCarInputs.cars[1];
                ovl667_tx_packet_count = (ovl667_tx_packet_count + 1U);
                ovl667_tx_slot_index = (ovl667_tx_slot_index + 1) & 7;
                ovl667_local_pause_bits = ovl667_pending_pause_bits;
                if (ovl667_sync_retry_state == 1) ovl667_sync_retry_state = 0;
                ovl667_tx_stage = 0;
            } else {
                ovl667_tx_stage = 0;
                ovl667_tx_miss_count = (ovl667_tx_miss_count + 1U);
                if (ovl667_sync_retry_state == 0) ovl667_sync_retry_state = 1;
                else if (ovl667_sync_retry_state == 1) ovl667_sync_retry_state = state;
            }
        }
    }
}

void ovl667_abort_on_link_timeout(void){
 if(ovl667_tx_miss_count>=7 || ovl667_rx_miss_count>=7 || ovl667_checksum_error_flag==1){
  ovl667_tx_miss_count=0;ovl667_rx_miss_count=0;ovl667_rx_packet_count=0;
  ovl667_tx_packet_count=0;ovl667_read_pending_flag=0;ModeFrame=0;
  ovl667_close_sio_session();
  ModeGroup=1;
  ModeIndex=44;
  SubmodeState=0;
 }
}

void ovl667_sync_link_display(void)
{
    ResetGraph(1);
    _sio_control(3,0,1);
    while(_sio_control(3,1,0)==0){}
    _sio_control(5,0,0);
    _sio_control(3,0,0);
    VSync(0);
    SetDispMask(1);
}

/* Initial values of the overlay globals this module owns. */
const char ovl667_sio_device_name[8] = "sio:";
s32 ovl667_sync_retry_state;
s32 ovl667_peer_race_state;
s32 ovl667_link_shutdown_stage;
u32 ovl667_sio_read_event;
u32 ovl667_sio_error_event;
s32 ovl667_sio_error_count;
s32 ovl667_sio_status_flag;
s32 ovl667_read_pending_flag;
u8 ovl667_checksum_error_flag;
u8 ovl667_remote_error_code;
s32 ovl667_rx_idle_flag;
s32 ovl667_rx_packet_count;
s32 ovl667_rx_miss_count;
s32 ovl667_tx_packet_count;
s32 ovl667_tx_miss_count;
u8 ovl667_rx_slot_index;
u8 ovl667_tx_slot_index;
s32 ovl667_tx_stage;
Overlay667SerialStorage ovl667_rx_staging_packet;
Overlay667SerialRing ovl667_rx_packet_ring;
Overlay667SerialStorage ovl667_tx_staging_packet;
u8 ovl667_active_slot_index;

/* Writable state of this unit; ovl667_reset_module restores it. */
static const OverlayStateRegion ovl667_sio_regions[] = {
    RR_OVERLAY_STATE(ovl667_active_slot_index),
    RR_OVERLAY_STATE(ovl667_checksum_error_flag),
    RR_OVERLAY_STATE(ovl667_link_shutdown_stage),
    RR_OVERLAY_STATE(ovl667_peer_race_state),
    RR_OVERLAY_STATE(ovl667_read_pending_flag),
    RR_OVERLAY_STATE(ovl667_remote_error_code),
    RR_OVERLAY_STATE(ovl667_rx_idle_flag),
    RR_OVERLAY_STATE(ovl667_rx_miss_count),
    RR_OVERLAY_STATE(ovl667_rx_packet_count),
    RR_OVERLAY_STATE(ovl667_rx_packet_ring),
    RR_OVERLAY_STATE(ovl667_rx_slot_index),
    RR_OVERLAY_STATE(ovl667_rx_staging_packet),
    RR_OVERLAY_STATE(ovl667_sio_error_count),
    RR_OVERLAY_STATE(ovl667_sio_error_event),
    RR_OVERLAY_STATE(ovl667_sio_read_event),
    RR_OVERLAY_STATE(ovl667_sio_status_flag),
    RR_OVERLAY_STATE(ovl667_sync_retry_state),
    RR_OVERLAY_STATE(ovl667_tx_miss_count),
    RR_OVERLAY_STATE(ovl667_tx_packet_count),
    RR_OVERLAY_STATE(ovl667_tx_slot_index),
    RR_OVERLAY_STATE(ovl667_tx_stage),
    RR_OVERLAY_STATE(ovl667_tx_staging_packet)};
const OverlayStateTable ovl667_sio_state = {ovl667_sio_regions, sizeof(ovl667_sio_regions) / sizeof(ovl667_sio_regions[0])};

/* Module reset: the state a fresh load of the serial_link image gave
 * (initialized data and zeroed BSS); see rr/overlay_module.h. */
extern const OverlayStateTable ovl667_car_selection_state, ovl667_race_state, ovl667_results_state, ovl667_sio_state;
static const OverlayStateTable *const ovl667_state_tables[] = {&ovl667_car_selection_state, &ovl667_race_state, &ovl667_results_state, &ovl667_sio_state};
static OverlayStateImage ovl667_state_image = {ovl667_state_tables, sizeof(ovl667_state_tables) / sizeof(ovl667_state_tables[0]), NULL};

void ovl667_reset_module(void) {
    OverlayStateRestore(&ovl667_state_image);
}

/* Initial values of the globals this module owns. */
LinkCarInputPair LinkRemoteCarInputs;
LinkCarInputPair LinkLocalCarInputs;
s32 LinkReadHandle;
s32 LinkWriteHandle;
LinkCarInputPair LinkPendingCarInputs;
