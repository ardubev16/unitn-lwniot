#include "contiki.h"
#include "dev/leds.h"
#include "leds.h"
#include "node-id.h"
#include "sys/ctimer.h"
#include <stdio.h>

#define ON_PERIOD CLOCK_SECOND / 10
#define OFF_PERIOD CLOCK_SECOND / 10 * 9

static struct ctimer timer; // Timer object
/*---------------------------------------------------------------------------*/
static void timer_on_cb(void *ptr);
static void timer_off_cb(void *ptr);

static void timer_on_cb(void *ptr) {
  printf("Hello, world! I am 0x%04x (callback timer)\n", node_id);
  leds_on(LEDS_RED | LEDS_GREEN);
  ctimer_set(&timer, ON_PERIOD, timer_off_cb, ptr); // set the same timer again
}

static void timer_off_cb(void *ptr) {
  leds_off(LEDS_RED | LEDS_GREEN);
  ctimer_set(&timer, OFF_PERIOD, timer_on_cb, ptr); // set the same timer again
}
/*---------------------------------------------------------------------------*/
// Declare a process
PROCESS(hello_world_process, "Hello world process");
// List processes to start at boot
AUTOSTART_PROCESSES(&hello_world_process);
/*---------------------------------------------------------------------------*/
// Implement the process thread function
PROCESS_THREAD(hello_world_process, ev, data) {
  PROCESS_BEGIN();

  // Set the callback timer
  ctimer_set(&timer, 0, timer_on_cb, NULL);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
