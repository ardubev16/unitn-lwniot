#include "contiki.h"
#include "dev/leds.h"
#include "leds.h"
#include "node-id.h"
#include <stdio.h>

#define ON_PERIOD CLOCK_SECOND / 10
#define OFF_PERIOD CLOCK_SECOND / 10 * 9
/*---------------------------------------------------------------------------*/
// Declare a process
PROCESS(hello_world_process, "Hello world process");
// List processes to start at boot
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
// Implement the process thread function
PROCESS_THREAD(hello_world_process, ev, data) {
  // Timer object
  static struct etimer timer; // ALWAYS use static variables in processes!

  PROCESS_BEGIN(); // All processes should start with PROCESS_BEGIN()

  while (1) {
    printf("Hello, world! I am 0x%04x\n", node_id);

    leds_on(LEDS_RED | LEDS_GREEN); // See also leds_on(), leds_off()
    etimer_set(&timer, ON_PERIOD);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    leds_off(LEDS_RED | LEDS_GREEN); // See also leds_on(), leds_off()
    etimer_set(&timer, OFF_PERIOD);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  }

  PROCESS_END(); // All processes should end with PROCESS_END()
}
/*---------------------------------------------------------------------------*/
