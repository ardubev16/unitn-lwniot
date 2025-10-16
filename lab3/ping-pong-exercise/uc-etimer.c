/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "random.h"
#include "sys/etimer.h"
#include "sys/node-id.h"
#include "sys/process.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/* Application Configuration */
#define UNICAST_CHANNEL 146
#define APP_TIMER_DELAY (CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2))
/*---------------------------------------------------------------------------*/
typedef struct ping_pong_msg {
  uint16_t sequence_number;
  int16_t noise_floor;
} __attribute__((packed)) ping_pong_msg_t;
/*---------------------------------------------------------------------------*/
static linkaddr_t receiver = {{0xAA, 0xBB}};
/*---------------------------------------------------------------------------*/
static uint16_t ping_pong_number =
    0; // You can use it to keep track of the current ping-pong sequence number.
/*---------------------------------------------------------------------------*/
process_event_t app_event;
/*---------------------------------------------------------------------------*/
/* Declaration of static functions */
static void print_rf_conf(void);
static void set_ping_pong_msg(ping_pong_msg_t *m);
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from);
static void sent_unicast(struct unicast_conn *c, int status, int num_tx);
/*---------------------------------------------------------------------------*/
static const struct unicast_callbacks uc_callbacks = {.recv = recv_unicast,
                                                      .sent = sent_unicast};
static struct unicast_conn uc;
/*---------------------------------------------------------------------------*/
PROCESS(unicast_process, "Unicast Process");
AUTOSTART_PROCESSES(&unicast_process);
/*---------------------------------------------------------------------------*/
static void print_rf_conf(void) {
  radio_value_t rfval = 0;

  printf("RF Configuration:\n");
  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &rfval);
  printf("\tRF Channel: %d\n", rfval);
  NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &rfval);
  printf("\tTX Power: %ddBm\n", rfval);
  NETSTACK_RADIO.get_value(RADIO_PARAM_CCA_THRESHOLD, &rfval);
  printf("\tCCA Threshold: %d\n", rfval);
}
/*---------------------------------------------------------------------------*/
static void set_ping_pong_msg(ping_pong_msg_t *m) {
  radio_value_t rfval;
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rfval);

  m->noise_floor = rfval;
  m->sequence_number = ping_pong_number;
}
/*---------------------------------------------------------------------------*/
static void app_send_msg(void) {
  ping_pong_msg_t msg; /* Message struct to be sent in unicast */

  set_ping_pong_msg(&msg);
  packetbuf_copyfrom(&msg, sizeof(msg));
  unicast_send(&uc, &receiver);
}
/*---------------------------------------------------------------------------*/
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from) {
  /* Local variables */
  ping_pong_msg_t msg;
  radio_value_t rssi;

  memcpy(&msg, packetbuf_dataptr(), sizeof(msg));

  NETSTACK_RADIO.get_value(RADIO_PARAM_LAST_RSSI, &rssi);
  printf("Last RSSI: %d\n", rssi);
  printf("Received ping_pong: %d, noise_floor: %d\n", msg.sequence_number,
         msg.noise_floor);

  ping_pong_number = msg.sequence_number + 1;
  process_post(&unicast_process, app_event, NULL);
}
/*---------------------------------------------------------------------------*/
static void sent_unicast(struct unicast_conn *c, int status, int num_tx) {
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if (linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }

  printf("Sent packet: ppn=%d, num_tx=%d\n", ping_pong_number, num_tx);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_process, ev, data) {
  static struct etimer et;

  PROCESS_BEGIN();

  // #if CONTIKI_TARGET_SKY
  if (node_id == 1) {
    receiver.u8[0] = 0x02;
    receiver.u8[1] = 0x00;
  } else if (node_id == 2) {
    receiver.u8[0] = 0x01;
    receiver.u8[1] = 0x00;
  }
  // #endif

  /* Print RF configuration */
  print_rf_conf();

  /* Print the node link layer address */
  printf("Node Link Layer Address: %02X:%02X\n", linkaddr_node_addr.u8[0],
         linkaddr_node_addr.u8[1]);

  /* Print the receiver link layer address */
  printf("Node: receiver address %02X:%02X\n", receiver.u8[0], receiver.u8[1]);

  /* Print the size of the ping_pong_msg_t struct */
  printf("Sizeof struct: %u\n", sizeof(ping_pong_msg_t));

  unicast_open(&uc, UNICAST_CHANNEL, &uc_callbacks);

  etimer_set(&et, 0);

  while (1) {
    /* Wait for an event */
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER && etimer_expired(&et)) {
      app_send_msg();
    } else if (ev == app_event) {
      etimer_set(&et, APP_TIMER_DELAY);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
