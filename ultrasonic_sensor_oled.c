#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif
#include <cr_section_macros.h>
#include "oled.h"

#define BARPOS1 48
#define BARPOS2 24
#define BARSIZE 128
#define BARMODE PB_SIDE

#define INTERVAL 10000 /* 100 [ms] */
#define GPIO_PININT_PINA 14
#define GPIO_PININT_PINB 13
#define GPIO_PININT_PORT 1
#define DEADTIME 87
#define MAXDISTANCE 4000
#define N 128
#define Y1 50


char sbuffer[30];
volatile uint32_t raw = 0, distance = 0;

uint8_t x_buff[N] = {0};
uint8_t wr = 0;

//float32_t *dtabptr = disttab;

/*--------------------------------------------------------------------*/
void PIN_INT0_IRQHandler(void) {
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH0);
	raw = (Chip_TIMER_ReadCount(LPC_TIMER16_0) - DEADTIME);
//distance [mm] = ( high level time × 34 / 100 ) / 2
	distance = raw * 1.7;
	if (distance > MAXDISTANCE)
		distance = MAXDISTANCE;

	wr++;
	if(wr > N-1) wr = 0;
	x_buff[wr] = distance/80;
}

/*--------------------------------------------------------------------*/
int main(void) {
#if defined (__USE_LPCOPEN)
// Read clock settings and update SystemCoreClock variable
	SystemCoreClockUpdate();
#if !defined(NO_BOARD_LIB)
// Set up and initialize all required blocks and
// functions related to the board hardware
	Board_Init();
// Set the LED to the state of "On"
	Board_LED_Set(0, true);
#endif
#endif
// TODO: insert code here
	/*--------------------- OLED Init -----------------------*/
	OLED_Init();
	/*-------------------- INT0 Init ------------------------*/
	Chip_IOCON_PinMuxSet(LPC_IOCON, GPIO_PININT_PORT, GPIO_PININT_PINA,
			(IOCON_FUNC0 | IOCON_MODE_PULLUP | IOCON_HYS_EN));
	/* Enable PININT clock */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PINT);
	/* Configure interrupt channel for the GPIO pin in SysCon block */
	Chip_SYSCTL_SetPinInterrupt(0, GPIO_PININT_PORT, GPIO_PININT_PINA);
	/* Configure channel interrupt as edge sensitive and falling edge interrupt */
	Chip_PININT_ClearIntStatus(LPC_PININT, PININTCH0);
	Chip_PININT_SetPinModeEdge(LPC_PININT, PININTCH0);
    Chip_PININT_EnableIntLow(LPC_PININT, PININTCH0);
	//Chip_PININT_EnableIntHigh(LPC_PININT, PININTCH0); /* calibration */
	/* Enable interrupt in the NVIC */
	NVIC_SetPriority(PIN_INT0_IRQn, 0);
	NVIC_ClearPendingIRQ(PIN_INT0_IRQn);
	NVIC_EnableIRQ(PIN_INT0_IRQn);
	/*----------------- Timer16_0 Init ------------------*/
	Chip_IOCON_PinMuxSet(LPC_IOCON, GPIO_PININT_PORT, GPIO_PININT_PINB,
			IOCON_FUNC2); /* PIO1_13 connected to MAT0 */
	Chip_TIMER_Init(LPC_TIMER16_0);
	Chip_TIMER_PrescaleSet(LPC_TIMER16_0, 720 - 1); /* 10 [us] */
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 0, INTERVAL);
	Chip_TIMER_SetMatch(LPC_TIMER16_0, 3, INTERVAL);
	Chip_TIMER_ResetOnMatchEnable(LPC_TIMER16_0, 3);
// __IO uint32_t PWMC; //brak w strukturze LPC_TIMER_T
	LPC_TIMER16_0->PWMC = 1;
	Chip_TIMER_Reset(LPC_TIMER16_0);
	Chip_TIMER_Enable(LPC_TIMER16_0);
	/*-------------------- End of Init ----------------------*/
	//OLED_Puts(10, 0, "Ultrasonic Sensor");
	while (1) {
		sprintf(sbuffer, "RAW data: %4d ", raw);
		OLED_Puts(0, 3, sbuffer);
		for(int i=0; i<N; i++)
			OLED_Draw_Line(i,Y1,i, 50-x_buff[(wr+i)%N]);
		sprintf(sbuffer, "Distance: %4d [mm] ", distance);
		OLED_Puts(0, 7, sbuffer);
		//OLED_Progressbar_Value(0,BARPOS1,BARSIZE,BARMODE,distance/4000.0);
		//Spectrum_Line_f32(pScr,128);
		OLED_Refresh_Gram();
		OLED_Clear_Screen(0);
	}
	return 0;
}
