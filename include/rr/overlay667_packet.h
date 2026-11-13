#ifndef RR_OVERLAY667_PACKET_H
#define RR_OVERLAY667_PACKET_H
#include "rr/menu_choice.h"
#include "rr/overlay667_events.h"
#include "rr/replay_types.h"
/* Twelve-byte serial record: four header bytes followed by the sender's two
 * car inputs. XOR covers the entire eight-byte payload. */
typedef struct {
    u8 flags00,checksum01,status02,phase03;
    CarInput4 first04,second08;
} Overlay667SerialPacket;
/* Initialization clears aligned words; serial transfer and payload copy use
 * the byte-aligned record view. Both views belong to the same full owner. */
typedef union {
    Overlay667SerialPacket packet;
    u32 words[3];
} Overlay667SerialStorage;
typedef union {
    Overlay667SerialPacket packets[8];
    u32 words[24];
} Overlay667SerialRing;
extern Overlay667SerialStorage ovl667_tx_staging_packet,ovl667_rx_staging_packet;
extern u8 ovl667_tx_slot_index,ovl667_checksum_error_flag;
extern s32 RaceState;
extern u8 ovl667_pending_pause_bits;
/* BIOS B0 service35: write count bytes, returning the transferred count. */
#include "rr/bios_events.h"
s32 ovl667_send_race_packet(void);
/* Eight stored serial records, from 80118F20 through 80118F7F. Ring indices
 * used for payload access must remain in0..7, including after service calls. */
extern Overlay667SerialRing ovl667_rx_packet_ring;
extern s32 ovl667_peer_race_state,ovl667_link_shutdown_stage;
extern s32 ovl667_sync_retry_state,ovl667_rx_idle_flag;
extern s32 ovl667_rx_packet_count,ovl667_rx_miss_count;
extern s32 ovl667_tx_packet_count,ovl667_tx_miss_count,ovl667_tx_stage;
extern u8 ovl667_rx_slot_index,ovl667_active_slot_index,ovl667_remote_pause_bits,ovl667_local_pause_bits;
extern u8 ovl667_consumed_slot_index;
extern u8 ovl667_remote_error_code;
void ovl667_init_sio_session(u32 configuration);
/* An event-ready packet's flags byte must be a valid ring index0..7: the
 * original uses the full byte as the index, without stripping control bits.
 * Serial services may mutate globals; every later ring access stays in0..7. */
s32 ovl667_poll_race_packet(void);
/* Receive selection and transmit retry state machines. Counter increments wrap
 * modulo2^32. Payloads and state are reread after the serial service returns. */
void ovl667_receive_peer_inputs(void);
void ovl667_transmit_local_inputs(void);
RR_ASSERT_SIZE(Overlay667SerialPacket,12);
RR_ASSERT_OFFSET(Overlay667SerialPacket,first04,4);
RR_ASSERT_OFFSET(Overlay667SerialPacket,second08,8);
RR_ASSERT_SIZE(Overlay667SerialStorage,12);
RR_ASSERT_SIZE(Overlay667SerialRing,96);
#endif
