#include "driverlib.h"

#define NUM_DATA 128

uint8_t DAC_TABLE[NUM_DATA] = {0x19,0x1a,0x1b,0x1d,0x1e,0x1f,0x20,0x21,
		0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,
		0x2b,0x2c,0x2c,0x2d,0x2e,0x2e,0x2f,0x30,
		0x30,0x31,0x31,0x31,0x32,0x32,0x32,0x32,
		0x32,0x32,0x32,0x32,0x32,0x31,0x31,0x31,
		0x30,0x30,0x2f,0x2e,0x2e,0x2d,0x2c,0x2c,
		0x2b,0x2a,0x29,0x28,0x27,0x26,0x25,0x24,
		0x23,0x21,0x20,0x1f,0x1e,0x1d,0x1b,0x1a,
		0x19,0x18,0x17,0x15,0x14,0x13,0x12,0x11,
		0xf,0xe,0xd,0xc,0xb,0xa,0x9,0x8,
		0x7,0x6,0x6,0x5,0x4,0x4,0x3,0x2,
		0x2,0x1,0x1,0x1,0x0,0x0,0x0,0x0,
		0x0,0x0,0x0,0x0,0x0,0x1,0x1,0x1,
		0x2,0x2,0x3,0x4,0x4,0x5,0x6,0x6,
		0x7,0x8,0x9,0xa,0xb,0xc,0xd,0xe,
		0xf,0x11,0x12,0x13,0x14,0x15,0x17,0x18};



/* DMA Control Table */
#ifdef ewarm
#pragma data_alignment=256
#else
#pragma DATA_ALIGN(controlTable, 256)
#endif
uint8_t controlTable[256];

void ADC_Init() {
	ADC14_enableModule();

    /* Configuring GPIOs (5.5 A0) */
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN5,
    GPIO_TERTIARY_MODULE_FUNCTION);

	ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_1, ADC_NOROUTE);

	ADC14_setResolution(ADC_14BIT);
	ADC14_setSampleHoldTrigger(ADC_TRIGGER_ADCSC, false);
	ADC14_setSampleHoldTime(ADC_PULSE_WIDTH_4, ADC_PULSE_WIDTH_4);

	ADC14_configureSingleSampleMode(ADC_MEM0, false);

	ADC14_configureConversionMemory(ADC_MEM0, ADC_VREFPOS_AVCC_VREFNEG_VSS, ADC_INPUT_A0, false);

	ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);
	ADC14_setPowerMode(ADC_UNRESTRICTED_POWER_MODE);
	ADC14_enableConversion();
//	ADC14_enableInterrupt(ADC_INT0);
//	ADC14_enableInterrupt(ADC_OV_INT);
}


void CS_Init() {
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_PJ,
            GPIO_PIN3 | GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);
    CS_setExternalClockSourceFrequency(32000,48000000);
    PCM_setPowerState(PCM_AM_LDO_VCORE1);
    PCM_setCoreVoltageLevel(PCM_VCORE1);
    FlashCtl_setWaitState(FLASH_BANK0, 2);
    FlashCtl_setWaitState(FLASH_BANK1, 2);
    CS_startHFXT(false);
    //TODO: test whether this makes a difference (probably not)
    CS_setDCOFrequency(CS_DCO_FREQUENCY_48);
    CS_initClockSignal(CS_MCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    CS_initClockSignal(CS_HSMCLK, CS_HFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
}

void GPIO_Init() {
	P4DIR = 0x3F; //make pins 0-5 out
	P4OUT = 0;
	P3DIR = 0x05;
	P3OUT = 0;
}

const Timer_A_UpModeConfig upConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,              // SMCLK Clock Source
        TIMER_A_CLOCKSOURCE_DIVIDER_1,          // SMCLK/1 = 3MHz
        5000,                            		// 5000 tick period
		TIMER_A_TAIE_INTERRUPT_DISABLE,         // Disable Timer interrupt
		TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE// ,
};

void TimerA_Init() {
	Timer_A_configureUpMode(TIMER_A0_MODULE, &upConfig);
}

const uint8_t* P4POINT = (uint8_t*) 0x40004C23;

uint8_t DacTable_32[32] = {0x19,0x1e,0x23,0x27,0x2b,0x2e,0x30,0x32,
		0x32,0x32,0x30,0x2e,0x2b,0x27,0x23,0x1e,
		0x19,0x14,0xf,0xb,0x7,0x4,0x2,0x0,
		0x0,0x0,0x2,0x4,0x7,0xb,0xf,0x14};

void DMA_Init() {
    DMA_enableModule();
    DMA_setControlBase(controlTable);
    DMA_assignChannel(DMA_CH0_TIMERA0CCR0);
    DMA_setChannelControl(UDMA_PRI_SELECT | DMA_CH0_TIMERA0CCR0,
            UDMA_SIZE_8 | UDMA_SRC_INC_8 | UDMA_DST_INC_NONE | UDMA_ARB_128);
    DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH0_TIMERA0CCR0, UDMA_MODE_AUTO, DAC_TABLE, P4POINT, 128);
    DMA_enableChannel(0);
    DMA_assignInterrupt(DMA_INT1, 0);
    Interrupt_enableInterrupt(INT_DMA_INT1);

    /* Setting Control Indexes */
    /*DMA_setChannelControl(UDMA_PRI_SELECT | DMA_CH7_ADC12C,
        UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 | UDMA_ARB_1);
    DMA_setChannelTransfer(UDMA_PRI_SELECT | DMA_CH7_ADC12C,
        UDMA_MODE_BASIC,
        ADC_MEM0, adcRecBuffer,1);
    DMA_assignInterrupt(DMA_INT2, 7);
    Interrupt_enableInterrupt(INT_DMA_INT2);*/
}

const eUSCI_UART_Config uartConfig = {
            EUSCI_A_UART_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
			312,                                      // BRDIV = 78
            8,                                       // UCxBRF = 2
            0,                                       // UCxBRS = 0
            EUSCI_A_UART_NO_PARITY,                  // No Parity
            EUSCI_A_UART_LSB_FIRST,                  // MSB First
            EUSCI_A_UART_ONE_STOP_BIT,               // One stop bit
            EUSCI_A_UART_MODE,                       // UART mode
			EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  // Oversampling
};

void UART_Init() {
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
            GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);
    UART_initModule(EUSCI_A0_MODULE, &uartConfig);
    UART_enableModule(EUSCI_A0_MODULE);
}

void sampleData();
void sendDataUART();
extern void dac_output(uint16_t*, uint8_t*);


int main(void) {
    WDT_A_holdTimer();

    ADC14CTL0 = 0;

    CS_Init();
    ADC_Init();
    GPIO_Init();
//    TimerA_Init();
    UART_Init();
//    DMA_Init();
//    Interrupt_enableInterrupt(INT_ADC14);
//    Interrupt_enableMaster();
//    Timer_A_startCounter(TIMER_A0_MODULE, TIMER_A_UP_MODE);

    uint16_t data_table[64];

    uint8_t DacTable_64[64] = {0x19,0x1b,0x1e,0x20,0x23,0x25,0x27,0x29,
    		0x2b,0x2c,0x2e,0x2f,0x30,0x31,0x32,0x32,
    		0x32,0x32,0x32,0x31,0x30,0x2f,0x2e,0x2c,
    		0x2b,0x29,0x27,0x25,0x23,0x20,0x1e,0x1b,
    		0x19,0x17,0x14,0x12,0xf,0xd,0xb,0x9,
    		0x7,0x6,0x4,0x3,0x2,0x1,0x0,0x0,
    		0x0,0x0,0x0,0x1,0x2,0x3,0x4,0x6,
    		0x7,0x9,0xb,0xd,0xf,0x12,0x14,0x17};

    uint8_t DacTable_32[32] = {0x19,0x1e,0x23,0x27,0x2b,0x2e,0x30,0x32,
    0x32,0x32,0x30,0x2e,0x2b,0x27,0x23,0x1e,
    0x19,0x14,0xf,0xb,0x7,0x4,0x2,0x0,
    0x0,0x0,0x2,0x4,0x7,0xb,0xf,0x14};

    while(1) {
    	dac_output(data_table, DacTable_32);
//    	sendDataUART(data_table);
    }

}

void sendDataUART(uint16_t* data_buff) {
	int dataIdx = 0;
	for(dataIdx = 0; dataIdx < 32; dataIdx++) {
		uint32_t value = data_buff[dataIdx];
		int i;
		UART_transmitData(EUSCI_A0_MODULE, 'a');
		for(i=0;i<4;i++) {
	    	uint8_t data_byte = value>>(i*8);
	    	UART_transmitData(EUSCI_A0_MODULE, data_byte);
		}
	}
}

void adc14_isr(void) {
//	ADC14_clearInterruptFlag(ADC_INT0);
}


void timer_a_0_isr(void) {

}

void dma_1_interrupt(void) {
}

void dma_2_interrupt(void) {

}

