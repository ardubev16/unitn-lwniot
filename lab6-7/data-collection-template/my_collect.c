#include "my_collect.h"
#include "contiki.h"
#include "core/net/linkaddr.h"
#include "leds.h"
#include "lib/random.h"
#include "net/linkaddr.h"
#include "net/netstack.h"
#include "net/rime/rime.h"
#include "sys/ctimer.h"
#include <stdbool.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
/* Time the sink should wait before rebuilding the tree from scratch. [Lab 7]
 * Try to change this period to analyse how it affects the radio-on time (i.e.,
 * energy consumption) of you solution and ContikiMac. */
#define BEACON_INTERVAL (CLOCK_SECOND * 60)
#define BEACON_FORWARD_DELAY (random_rand() % CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
// Links with RSSI < RSSI_THRESHOLD should be neglected!
#define RSSI_THRESHOLD -95
/*---------------------------------------------------------------------------*/
/* Callback function declarations */
void bc_recv(struct broadcast_conn *conn, const linkaddr_t *sender);
void uc_recv(struct unicast_conn *c, const linkaddr_t *from);
void beacon_timer_cb(void *ptr);
/*---------------------------------------------------------------------------*/
/* Initilization of Rime broadcast and unicast callback structures */
struct broadcast_callbacks bc_cb = {.recv = bc_recv, .sent = NULL};
struct unicast_callbacks uc_cb = {.recv = uc_recv, .sent = NULL};
/*---------------------------------------------------------------------------*/
void my_collect_open(struct my_collect_conn *conn, uint16_t channels,
                     bool is_sink,
                     const struct my_collect_callbacks *callbacks) {

  linkaddr_copy(&conn->parent, &linkaddr_null);
  conn->is_sink = is_sink;

  conn->metric = UINT16_MAX;

  conn->beacon_seqn = 0;

  conn->callbacks = callbacks;

  /* Open the underlying Rime primitives */
  broadcast_open(&conn->bc, channels, &bc_cb);
  unicast_open(&conn->uc, channels + 1, &uc_cb);

  if (conn->is_sink) {
    conn->metric = 0;
    ctimer_set(&conn->beacon_timer, CLOCK_SECOND, beacon_timer_cb, conn);
  }
}
/*---------------------------------------------------------------------------*/
/*                              Beacon Handling                              */
/*---------------------------------------------------------------------------*/
/* Beacon message structure */
struct beacon_msg {
  uint16_t seqn;
  uint16_t metric;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Send beacon using the current seqn and metric */
void send_beacon(struct my_collect_conn *conn) {
  /* Prepare the beacon message */
  struct beacon_msg beacon = {.seqn = conn->beacon_seqn,
                              .metric = conn->metric};

  /* Send the beacon message in broadcast */
  packetbuf_clear();
  packetbuf_copyfrom(&beacon, sizeof(beacon));
  printf("my_collect: sending beacon: seqn %d metric %d\n", conn->beacon_seqn,
         conn->metric);
  broadcast_send(&conn->bc);
}
/*---------------------------------------------------------------------------*/
/* Beacon timer callback */
void beacon_timer_cb(void *ptr) {
  struct my_collect_conn *conn = (struct my_collect_conn *)ptr;

  send_beacon(conn);
  if (conn->is_sink) {
    conn->beacon_seqn++;
    ctimer_set(&conn->beacon_timer, BEACON_INTERVAL, beacon_timer_cb, conn);
  }
}
/*---------------------------------------------------------------------------*/
/* Beacon receive callback */
void bc_recv(struct broadcast_conn *bc_conn, const linkaddr_t *sender) {
  struct beacon_msg beacon;
  int16_t rssi;

  /* Get the pointer to the overall structure my_collect_conn from its field bc
   */
  struct my_collect_conn *conn =
      (struct my_collect_conn *)(((uint8_t *)bc_conn) -
                                 offsetof(struct my_collect_conn, bc));

  /* Check if the received broadcast packet looks legitimate */
  if (packetbuf_datalen() != sizeof(struct beacon_msg)) {
    printf("my_collect: broadcast of wrong size\n");
    return;
  }
  memcpy(&beacon, packetbuf_dataptr(), sizeof(struct beacon_msg));

  rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

  printf("my_collect: recv beacon from %02x:%02x seqn %u metric %u rssi %d\n",
         sender->u8[0], sender->u8[1], beacon.seqn, beacon.metric, rssi);

  if (rssi < RSSI_THRESHOLD)
    return;

  if (beacon.seqn > conn->beacon_seqn ||
      (beacon.seqn == conn->beacon_seqn && beacon.metric + 1 < conn->metric)) {

    conn->beacon_seqn = beacon.seqn;
    conn->metric = beacon.metric + 1;
    linkaddr_copy(&conn->parent, sender);
    printf("my_collect: new parent %02x:%02x, my metric %d, my seqn %d\n",
           sender->u8[0], sender->u8[1], conn->metric, conn->beacon_seqn);

    ctimer_set(&conn->beacon_timer, BEACON_FORWARD_DELAY, beacon_timer_cb,
               conn);
  }
}
/*---------------------------------------------------------------------------*/
/*                     Data Handling --- LAB 7                               */
/*---------------------------------------------------------------------------*/
/* Header structure for data packets */
struct collect_header {
  linkaddr_t source;
  uint8_t hops;
} __attribute__((packed));
/*---------------------------------------------------------------------------*/
/* Data Collection: send function */
int my_collect_send(struct my_collect_conn *conn) {
  /* TODO 5:
   * 1. Check if the node is connected (has a parent), IF NOT return -1;
   * 2. If possible, allocate space for the data collection header. If this is
   *    not possible, return -2;
   * 3. Prepare and insert the header in the packet buffer;
   *    Tip: The Rime address of a node is stored in linkaddr_node_addr!
   *         (check contiki/core/net/linkaddr.h for additional details);
   * 4. Send the packet to the parent using unicast and return the status
   *    of unicast_send() to the application.
   */

  return 0; /* TO BE REMOVED WHEN TODO 5 IS IMPLEMENTED !!! */
}
/*---------------------------------------------------------------------------*/
/* Data receive callback */
void uc_recv(struct unicast_conn *uc_conn, const linkaddr_t *from) {
  /* Get the pointer to the overall structure my_collect_conn from its field uc
   */
  struct my_collect_conn *conn =
      (struct my_collect_conn *)(((uint8_t *)uc_conn) -
                                 offsetof(struct my_collect_conn, uc));

  struct collect_header hdr;

  /* Check if the received unicast message looks legitimate */
  if (packetbuf_datalen() < sizeof(struct collect_header)) {
    printf("my_collect: too short unicast packet %d\n", packetbuf_datalen());
    return;
  }

  /* TODO 6:
   * 1. Extract the header;
   * 2. On the sink, remove the header and call the application callback;
   *    [TBC] - Should we update any field of hdr?
   *          - What about packetbuf_dataptr() and packetbuf_hdrptr()? Does the
   *            application recv callback rely on any of them? Should we take
   * any action?
   * 3. On a forwarder, update the header and forward the packet to the parent
   * (IF ANY) using unicast.
   */
}
/*---------------------------------------------------------------------------*/
