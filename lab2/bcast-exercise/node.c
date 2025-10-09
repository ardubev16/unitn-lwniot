/*---------------------------------------------------------------------------*/
#include "dev/button-sensor.h"
#include "lib/sensors.h"
#include "net/packetbuf.h"
#include "net/rime/broadcast.h"
#include "random.h"
#include "sys/etimer.h"
#include "sys/process.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
PROCESS(broadcast_process, "Broadcast Process");
AUTOSTART_PROCESSES(&broadcast_process);
/*---------------------------------------------------------------------------*/
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  size_t len = packetbuf_datalen();
  char rx_msg[len + 1];
  memcpy(rx_msg, packetbuf_dataptr(), len);
  rx_msg[len] = '\0';

  printf("Recv from %02X:%02X - Message: '%s'\n", from->u8[0], from->u8[1],
         rx_msg);
}

static void broadcast_sent(struct broadcast_conn *conn, int status,
                           int num_tx) {
  printf("Status: %d, Num TX: %d\n", status, num_tx);
}

static const struct broadcast_callbacks broadcast_cb = {.recv = broadcast_recv,
                                                        .sent = broadcast_sent};
static struct broadcast_conn broadcast;
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_process, ev, data) {
  static struct etimer et;
  static int message_selector = 0;
  static char *msg_0 = "Hello world!";
  static char *msg_1 = "Bye bye world!";

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  broadcast_open(&broadcast, 169, &broadcast_cb);

  /* Print node link layer address */
  printf("Node Link Layer Address: %02X:%02X\n", linkaddr_node_addr.u8[0],
         linkaddr_node_addr.u8[1]);

  while (1) {
    /* Delay 5-8 seconds */
    etimer_set(&et, CLOCK_SECOND * 5 + random_rand() % (CLOCK_SECOND * 3));

    /* Wait for the timer to expire or button press */
    while (1) {
      PROCESS_WAIT_EVENT();
      if (ev == PROCESS_EVENT_TIMER && etimer_expired(&et))
        break;

      else if (ev == sensors_event && data == &button_sensor)
        message_selector = !message_selector;
    }

    char *msg_to_send;
    if (message_selector)
      msg_to_send = msg_1;
    else
      msg_to_send = msg_0;

    packetbuf_clear();
    packetbuf_copyfrom(msg_to_send, strlen(msg_to_send));
    broadcast_send(&broadcast);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
