/*_____ I N C L U D E S ____________________________________________________*/
// #include <stdio.h>
#include <string.h>
#include "r_smc_entry.h"

#include "misc_config.h"
#include "custom_func.h"
#include "retarget.h"

#include "fdl_app.h"
/*_____ D E C L A R A T I O N S ____________________________________________*/

volatile struct flag_32bit flag_PROJ_CTL;
#define FLAG_PROJ_TIMER_PERIOD_1000MS                 	(flag_PROJ_CTL.bit0)
#define FLAG_PROJ_TIMER_PERIOD_SPECIFIC           	    (flag_PROJ_CTL.bit1)
#define FLAG_PROJ_REVERSE2                 	        	(flag_PROJ_CTL.bit2)
#define FLAG_PROJ_REVERSE3                    		    (flag_PROJ_CTL.bit3)
#define FLAG_PROJ_REVERSE4                              (flag_PROJ_CTL.bit4)
#define FLAG_PROJ_REVERSE5                              (flag_PROJ_CTL.bit5)
#define FLAG_PROJ_REVERSE6                              (flag_PROJ_CTL.bit6)
#define FLAG_PROJ_REVERSE7                              (flag_PROJ_CTL.bit7)


#define FLAG_PROJ_TRIG_1                                (flag_PROJ_CTL.bit8)
#define FLAG_PROJ_TRIG_2                                (flag_PROJ_CTL.bit9)
#define FLAG_PROJ_TRIG_3                                (flag_PROJ_CTL.bit10)
#define FLAG_PROJ_TRIG_4                                (flag_PROJ_CTL.bit11)
#define FLAG_PROJ_TRIG_5                                (flag_PROJ_CTL.bit12)
#define FLAG_PROJ_REVERSE13                             (flag_PROJ_CTL.bit13)
#define FLAG_PROJ_REVERSE14                             (flag_PROJ_CTL.bit14)
#define FLAG_PROJ_REVERSE15                             (flag_PROJ_CTL.bit15)

/*_____ D E F I N I T I O N S ______________________________________________*/

volatile unsigned short counter_tick = 0U;
volatile unsigned long ostmr_tick = 0U;

#define BTN_PRESSED_LONG                                (2500U)

#pragma section privateData

const unsigned char dummy_3 = 0x5AU;

volatile unsigned char dummy_2 = 0xFFU;

volatile unsigned char dummy_1;

#pragma section default

volatile unsigned long g_u32_counter = 0U;

volatile UART_MANAGER_T UART0Manager = 
{
	.g_uart0rxbuf = 0U,                                         /* UART0 receive buffer */
	.g_uart0rxerr = 0U,                                         /* UART0 receive error status */
};


/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/



void ostmr_tick_counter(void)
{
	ostmr_tick++;
}

void ostmr_1ms_IRQ(void)
{
	ostmr_tick_counter();
}

void ostimer_dealyms(unsigned long ms)
{
    R_Config_OSTM0_Start();
    ostmr_tick = 0U;

    while(ostmr_tick < ms);

    R_Config_OSTM0_Stop();

}

unsigned short get_tick(void)
{
	return (counter_tick);
}

void set_tick(unsigned short t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000U)
    {
        set_tick(0U);
    }
}

void delay_ms(unsigned long ms)
{
    unsigned long tickstart = get_tick();
    unsigned long wait = ms;
	unsigned long tmp = 0U;
	
    while (1)
    {
		if (get_tick() > tickstart)	// tickstart = 59000 , tick_counter = 60000
		{
			tmp = get_tick() - tickstart;
		}
		else // tickstart = 59000 , tick_counter = 2048
		{
			tmp = 60000U -  tickstart + get_tick();
		}		
		
		if (tmp > wait)
			break;
    }
}

unsigned char R_PORT_GetGPIOLevel(unsigned short n,unsigned char Pin)
{
    unsigned short PortLevel;

    switch(n)
    {
        case 0U:
            PortLevel = PORT.PPR0;
            break;
        case 8U:
            PortLevel = PORT.PPR8;
            break;
        case 9U:
            PortLevel = PORT.PPR9;
            break;
        case 10U:
            PortLevel = PORT.PPR10;
            break;
        case 11U:
            PortLevel = PORT.PPR11;
            break;
        case 0x2C8U:
            PortLevel = PORT.APPR0;
            break;
    }
    PortLevel &= 1U<<Pin;
    
    if(PortLevel == 0U)
    {
        return 0U;
    }
    else
    {
        return 1U;
    }
}
void tmr_1ms_IRQ(void)
{
    tick_counter();

    if ((get_tick() % 1000U) == 0U)
    {
        FLAG_PROJ_TIMER_PERIOD_1000MS = 1U;
    }

    if ((get_tick() % 250U) == 0U)
    {
        FLAG_PROJ_TIMER_PERIOD_SPECIFIC = 1U;
    }

    if ((get_tick() % 50U) == 0U)
    {

    }	

}

void LED_Toggle(void)
{
    static unsigned char flag_gpio = 0U;
		
    GPIO_TOGGLE(0,14);//PORT.PNOT0 |= 1u<<14;
	
	if (!flag_gpio)
	{
		flag_gpio = 1U;
        GPIO_HIGH(P8,5);//PORT.P8 |= 1u<<5;
	}
	else
	{
		flag_gpio = 0U;
		GPIO_LOW(P8,5);//PORT.P8 &= ~(1u<<5);
	}	
}

void loop(void)
{
	// static unsigned long LOG1 = 0U;

    if (FLAG_PROJ_TIMER_PERIOD_1000MS)
    {
        FLAG_PROJ_TIMER_PERIOD_1000MS = 0U;

        g_u32_counter++;
        LED_Toggle();   
        // tiny_printf("timer:%4d\r\n",LOG1++);
    }

    if (FLAG_PROJ_TIMER_PERIOD_SPECIFIC)
    {
        FLAG_PROJ_TIMER_PERIOD_SPECIFIC = 0U;
    }

    if (FLAG_PROJ_TRIG_4)
    {
        DF_Flash_test_process(4);
        FLAG_PROJ_TRIG_4 = 0U;
    }
    if (FLAG_PROJ_TRIG_3)
    {
        DF_Flash_test_process(3);
        FLAG_PROJ_TRIG_3 = 0U;
    }
    if (FLAG_PROJ_TRIG_2)
    {
        DF_Flash_test_process(2);
        FLAG_PROJ_TRIG_2 = 0U;
    }
    if (FLAG_PROJ_TRIG_1)
    {
        DF_Flash_test_process(1);
        FLAG_PROJ_TRIG_1 = 0U;
    }
    
}

void UARTx_ErrorCheckProcess(unsigned char err)
{
    if (err)          /* Check reception error */
    {   
        /* Reception error */
        switch(err)
        {
            case _UART_PARITY_ERROR_FLAG:   /* Parity error */
                tiny_printf("uart rx:Parity Error Flag\r\n");
                break;
            case _UART_FRAMING_ERROR_FLAG:  /* Framing error */
                tiny_printf("uart rx:Framing Error Flag\r\n");
                break;
            case _UART_OVERRUN_ERROR_FLAG:  /* Overrun error */
                tiny_printf("uart rx:Overrun Error Flag\r\n");
                break;
            case _UART_BIT_ERROR_FLAG:      /* Bit error */
                tiny_printf("uart rx:Bit Error Flag\r\n");
                break;
        }
        UART0Manager.g_uart0rxerr = 0U;
    }
}

void UARTx_Process(unsigned char rxbuf)
{    
    if (rxbuf == 0x00U)
    {
        return;
    }

    if (rxbuf > 0x7FU)
    {
        tiny_printf("invalid command\r\n");
    }
    else
    {
        tiny_printf("press:%c(0x%02X)\r\n" , rxbuf,rxbuf);   // %c :  C99 libraries.
        switch(rxbuf)
        {
            case '1':
                FLAG_PROJ_TRIG_1 = 1U;
                break;
            case '2':
                FLAG_PROJ_TRIG_2 = 1U;
                break;
            case '3':
                FLAG_PROJ_TRIG_3 = 1U;
                break;
            case '4':
                FLAG_PROJ_TRIG_4 = 1U;
                break;
            case '5':
                FLAG_PROJ_TRIG_5 = 1U;
                break;

            case 'X':
            case 'x':
            case 'Z':
            case 'z':
                RH850_software_reset();
                break;

            default:       
                // exception
                break;                
        }
    }
}

void RH850_software_reset(void)
{
    unsigned long  reg32_value;

    reg32_value = 0x00000001UL;
    WPROTR.PROTCMD0 = _WRITE_PROTECT_COMMAND;
    RESCTL.SWRESA = reg32_value;
    RESCTL.SWRESA = (unsigned long) ~reg32_value;
    RESCTL.SWRESA = reg32_value;
    while (WPROTR.PROTS0 != reg32_value)
    {
        NOP();
    }
}

void RLIN3_UART_SendChar(unsigned char c)
{
    /*
        UTS : 0 - transmission is not in progress    
    */
    while (((RLN30.LST & _UART_TRANSMISSION_OPERATED) != 0U));    
    RLN30.LUTDR.UINT16 = c;
    // RLN30.LUTDR.UINT8[L] = (unsigned char) c;  
}

void SendChar(unsigned char ch)
{
    RLIN3_UART_SendChar(ch);
}

void hardware_init(void)
{
    EI();

    R_Config_TAUJ0_0_Start();
    R_Config_OSTM0_Start();

    /*
        LED : 
            - LED18 > P0_14
            - LED17 > P8_5 
        UART : 
            - TX > P10_10
            - RX > P10_9    
    */
    R_Config_UART0_Receive((uint8_t *)&UART0Manager.g_uart0rxbuf, 1U);
    R_Config_UART0_Start();

    DF_Flash_init();
   
    tiny_printf("\r\nhardware_init rdy\r\n");

}
