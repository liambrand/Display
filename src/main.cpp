/*
 * Simple program for FRDM-K64F with CAN loopback 
 *
 * Implemented using mbed OS 5 and threads scheduled according to RMS
 *
 * DK - 02-Feb-2019
 */


#include <stdbool.h>
#include <stdint.h>
#include <mbed.h>
#include <stdio.h>
#include <assert.h>
#include "can.h"

static DigitalOut red(LED_RED);
static Serial pc(USBTX, USBRX, 115200);
static uint32_t txCount;
static uint32_t rxCount;
static volatile canMessage_t rxMsg;
Semaphore rxDone;
Thread request(osPriorityRealtime);
Thread display(osPriorityRealtime1);

static void led1Toggle(void);
static void requestTask(void);
static void displayTask(void);
static void canHandler(void);

int main () {
    osStatus status;

    red = 0;
    canInit(BD125000, true);
    pc.printf("Display -- Loopback test\n");

		status = request.start(requestTask);
    assert(osOK == status);
		status = display.start(displayTask);
    assert(osOK == status);

    while (true) {
      led1Toggle();
      wait_ms(500);
    }
}

static void led1Toggle(void) {
  red = 1 - red;
}

/* Transmit CAN message requesting temperature
 * reading from EngineMonitor
*/
static void requestTask(void) {
	static canMessage_t txMsg = {0x23, 8, 0, 0};
	bool txOk;

	while(true) {
		txOk = canWrite(&txMsg);
		wait_ms(500);
	}
}

/* Pend on a semaphore released by an RX Interrupt
 * When released, display the recieved temperature reading to a terminal
*/
static void displayTask(void) {
	canRxInterrupt(canHandler);
	while(true) {
		rxDone.wait();
		pc.printf("ID: 0x%lx LEN: 0x%01lx DATA_A: 0x%08lx DATA_B: 0x%08lx\n", rxMsg.id, rxMsg.len, rxMsg.dataA, rxMsg.dataB);
		wait_ms(500);
	}
}

/* A simple interrupt handler for CAN message reception */
void canHandler(void) {
    canTransferRxFrame(&rxMsg);
    rxDone.release();
}
