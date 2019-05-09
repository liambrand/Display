#include <MK64F12.h>
#include <fsl_flexcan.h>
#include <assert.h>
#include <string.h>
#include "can.h"

#define TX_BUF_ID 8
#define RX_BUF_ID 9

static flexcan_frame_t rxFrame;
static flexcan_handle_t fHandle;
static flexcan_mb_transfer_t xfer;
static void (*user_handler)(void);

static void enterFreezeMode(CAN_Type *base)
{
    /* Set Freeze, Halt bits. */
    base->MCR |= CAN_MCR_HALT_MASK;

    /* Wait until the FlexCAN Module enter freeze mode. */
    while (!(base->MCR & CAN_MCR_FRZACK_MASK))
    {
    }
}

static void exitFreezeMode(CAN_Type *base)
{
    /* Clear Freeze, Halt bits. */
    base->MCR &= ~CAN_MCR_HALT_MASK;

    /* Wait until the FlexCAN Module exit freeze mode. */
    while (base->MCR & CAN_MCR_FRZACK_MASK)
    {
    }
}

void canInit(baudRate_t baudRate, bool loopback) {
    flexcan_config_t flexcanConfig;
    flexcan_rx_mb_config_t mbconfig;

    /* Enable the clock to the FLEXCAN module */
    SIM->SCGC6 |= SIM_SCGC6_FLEXCAN0_MASK;

    /* Enable the clock to PORT B */
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;

    /* Select the CAN function (Alternative 2) for pins 18 and 19 of PORT B */
    PORTB->PCR[18] |= PORT_PCR_MUX(2) | PORT_PCR_PS(1) | PORT_PCR_PE(1);
    PORTB->PCR[19] |= PORT_PCR_MUX(2) | PORT_PCR_PS(1) | PORT_PCR_PE(1);

    FLEXCAN_GetDefaultConfig(&flexcanConfig);
    flexcanConfig.baudRate = baudRate;
    if (loopback) {
        flexcanConfig.enableLoopBack = true;
    }
    FLEXCAN_Init(CAN0, &flexcanConfig, CLOCK_GetOsc0ErClkFreq());

    /* Prevent reception of own messages unless we're in loopback mode */
    if (!loopback) {
        enterFreezeMode(CAN0);
        CAN0->MCR |= CAN_MCR_SRXDIS_MASK;
        exitFreezeMode(CAN0);
    }

    FLEXCAN_SetTxMbConfig(CAN0, TX_BUF_ID, true);
    mbconfig.type = kFLEXCAN_FrameTypeData;
    mbconfig.format = kFLEXCAN_FrameFormatStandard;
    mbconfig.id = FLEXCAN_ID_STD(0x0);
    FLEXCAN_SetRxMbConfig(CAN0, RX_BUF_ID, &mbconfig, true);
    FLEXCAN_SetRxMbGlobalMask(CAN0, 0UL);
}

bool canWrite(canMessage_t *message) {
    flexcan_frame_t txFrame;

    txFrame.type = kFLEXCAN_FrameTypeData; 
    txFrame.format = kFLEXCAN_FrameFormatStandard;
    txFrame.id = FLEXCAN_ID_STD(message->id);
    txFrame.length = message->len;
    txFrame.dataWord0 = message->dataA;
    txFrame.dataWord1 = message->dataB;

    if (kStatus_Success == FLEXCAN_WriteTxMb(CAN0, TX_BUF_ID, &txFrame)) {
        return true;
    }
    else {
        return false;
    }
}

/*void canRead(canMessage_t *message) {*/
    /*if (kStatus_Success == FLEXCAN_TransferReceiveBlocking(CAN0, RX_BUF_ID, &rxFrame)) {*/
        /*message->id = (rxFrame.id >> CAN_ID_STD_SHIFT);*/
	/*message->len = rxFrame.length;*/
	/*message->dataA = rxFrame.dataWord0;*/
	/*message->dataB = rxFrame.dataWord1;*/
    /*}*/
/*}*/

void canRead(canMessage_t *message) {
    flexcan_frame_t rxFrame;
    status_t status;

    FLEXCAN_ClearMbStatusFlags(CAN0, 1 << RX_BUF_ID);
    memset(&rxFrame, 0, sizeof(rxFrame));
    status = FLEXCAN_ReadRxMb(CAN0, RX_BUF_ID, &rxFrame);
    assert(kStatus_Success == status);
    memset(message, 0, sizeof(canMessage_t));
    message->id = (rxFrame.id >> CAN_ID_STD_SHIFT);
    message->len = rxFrame.length;
    message->dataA = rxFrame.dataWord0;
    message->dataB = rxFrame.dataWord1;
}

void canTransferRxFrame(volatile canMessage_t *message) {
    memset((canMessage_t *)message, 0, sizeof(canMessage_t));
    message->id = (rxFrame.id >> CAN_ID_STD_SHIFT);
    message->len = rxFrame.length;
    message->dataA = rxFrame.dataWord0;
    message->dataB = rxFrame.dataWord1;
}

bool canReady(void) {
    if (FLEXCAN_GetMbStatusFlags(CAN0, 1 << RX_BUF_ID)) {
        return true;
    }
    else {
        return false;
    }
}

uint32_t canStatus(void) {
    return 0;
}

static void flexcan_callback(CAN_Type *base, flexcan_handle_t *handle, 
                             status_t status, uint32_t result, void *userData) {

    if ((kStatus_FLEXCAN_RxIdle == status) && (RX_BUF_ID == result)) {
        FLEXCAN_TransferReceiveNonBlocking(CAN0, &fHandle, &xfer);
        user_handler();
    }
}
    
void canRxInterrupt(void (*handler)(void)) {
    user_handler = handler;
    FLEXCAN_TransferCreateHandle(CAN0, &fHandle, flexcan_callback, NULL);
    xfer.frame = &rxFrame; 
    xfer.mbIdx = RX_BUF_ID;
    FLEXCAN_TransferReceiveNonBlocking(CAN0, &fHandle, &xfer); 
}

