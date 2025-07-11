/*_____ I N C L U D E S ____________________________________________________*/
// #include <stdio.h>
#include <string.h>
#include "r_smc_entry.h"

#include "misc_config.h"
#include "custom_func.h"
#include "retarget.h"

// FDL header files include
// #define EEELIB_INTDEF
// #include "r_typedefs.h"
#include "target.h"
#include "fdl_user.h"
#include "r_fdl.h"
#include "fdl_descriptor.h"

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

// data flash , 1 block = 64 bytes = 16 words
#define DATA_FLASH_BLOCK_SIZE_BYTES                     (64U)
#define DATA_FLASH_BLOCK_SIZE_WORDS                     (16U)
#define DATA_FLASH_WORD_SIZE_BYTES                      (4U)
#define DATA_FLASH_BLOCK_WORDS                          DATA_FLASH_BLOCK_SIZE_WORDS  // alias
unsigned long rd_buffer[DATA_FLASH_BLOCK_WORDS] = {0};
unsigned long wr_buffer[DATA_FLASH_BLOCK_WORDS] = {0};
unsigned long counter = 0;

const unsigned long array[] = 
{
  0x5A1EAD72, 0x3B99EA52, 0xA0681756, 0xFEFEB801,
  0x5B8E3333, 0xBB174F9E, 0xB9C71DF8, 0x7A492502,
  0x5CF9A33A, 0xD42E717F, 0xB2ED2A92, 0xAF2AF203,
  0x5D43B5B7, 0x52D6FB3C, 0x51B973CE, 0xEED3D104
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

void DF_Flash_blank_check(unsigned short start_addr , unsigned short numbers_of_words)
{
    r_fdl_request_t           req;

    DI();

    /*
        Blank Check addresses from 0x10 to 0x17.
        myRequest.idx_u32          = 0x10; 
        myRequest.cnt_u16          = 2; 
    */
    req.command_enu     = R_FDL_CMD_BLANKCHECK;
    /*
        The virtual start address for performing 
        blank check in data flash. Must be word (4 
        bytes) aligned
    */
    req.idx_u32         = start_addr;
    /*
        Number of words (4 bytes) to check
    */
    req.cnt_u16         = numbers_of_words;
    req.accessType_enu  = R_FDL_ACCESS_USER;
    R_FDL_Execute( &req );
    
    while( R_FDL_BUSY == req.status_enu )
    {
        R_FDL_Handler();
    }
    
    if( R_FDL_OK != req.status_enu )
    {   
        /* 
            The half word is blank... we may not read 
            R_FDL_ERR_BLANKCHECK
        */
        /* Error handler */
        #if 1
        tiny_printf("\r\n>>>>R_FDL_CMD_BLANKCHECK error(0x%02X)\r\n",req.status_enu);
        return;
        #else
        while( 1 )
            ;
        #endif
    }
    
    EI();
        
    tiny_printf("R_FDL_CMD_BLANKCHECK rdy\r\n");
}

/*
    len : 1 words = 4 bytes = uint32_t 
    write_buff : 32-bit aligned
*/
void DF_Flash_data_write(unsigned long start_addr , unsigned short numbers_of_words , unsigned long* write_buff)
{
    r_fdl_request_t           req;

    DI();

    req.command_enu     = R_FDL_CMD_WRITE;
    /*    
        The virtual start address for writing in Data 
        Flash aligned to word size (4 bytes). 
    */
    req.idx_u32         = start_addr;
    /*
        Number of words to write. 
    */
    req.cnt_u16         = numbers_of_words;
    /*
        Address of the buffer containing the source 
        data to be written.     
    */
    req.bufAddr_u32     = (uint32_t)( &write_buff[0] );
    req.accessType_enu  = R_FDL_ACCESS_USER;
    R_FDL_Execute( &req );
    while( R_FDL_BUSY == req.status_enu )
    {
        R_FDL_Handler();
    }
    if( R_FDL_OK != req.status_enu )
    {   
        /* Error handler */
        #if 1
        tiny_printf("\r\n>>>>R_FDL_CMD_WRITE err(0x%02X)\r\n",req.status_enu);
        return;
        #else
        while( 1 )
            ;
        #endif
    }

    EI();
        
    tiny_printf("R_FDL_CMD_WRITE rdy\r\n");
}

/*
    len : 1 words = 4 bytes = uint32_t 
    read_buff : 32-bit aligned
*/
void DF_Flash_data_read(unsigned long start_addr , unsigned short numbers_of_words , unsigned long* read_buff)
{
    r_fdl_request_t           req;

    DI();

    req.command_enu     = R_FDL_CMD_READ;
    /*
        Data Flash virtual address from where to 
        read. Must be word (4 bytes) aligned.     
    */
    req.idx_u32         = start_addr;
    /*
        Numbers of words (4 bytes) to read
    */
    req.cnt_u16         = numbers_of_words;
    /*
        Data destination buffer address in RAM. 
        Note: The buffer must be 32-bit aligned!     
    */
    req.bufAddr_u32     = (uint32_t)( &read_buff[0] );
    req.accessType_enu  = R_FDL_ACCESS_USER;
    R_FDL_Execute( &req );
    while( R_FDL_BUSY == req.status_enu )
    {
        R_FDL_Handler();
    }
    if( R_FDL_OK != req.status_enu )
    {   
        /* Error handler */
        #if 1
        if (R_FDL_ERR_ECC_SED == req.status_enu)
        {
            tiny_printf("\r\n>>>>R_FDL_CMD_READ error-single bit ECC error(0x%02X)\r\n",req.status_enu);
            
        }
        else if (R_FDL_ERR_ECC_DED == req.status_enu)
        {
            tiny_printf("\r\n>>>>R_FDL_CMD_READ error-double bit ECC error(0x%02X)\r\n",req.status_enu);            
        }
        else
        {
            tiny_printf("\r\n>>>>R_FDL_CMD_READ error(0x%02X)\r\n",req.status_enu);
        }

        return;
        #else
        while( 1 )
            ;
        #endif
    }

    EI();
        
    tiny_printf("R_FDL_CMD_READ rdy\r\n");
}

/*
    RH850/F1KM-S1 Data Flash Memory Size 64 KB , 

    check [Mapping of the Data Flash Memory] in hardware manual
    total 1024 blocks , 1 block = 64 bytes

    FF20 FFFF H
    FF20 FFC0 H     Block 1023 (64 bytes)
    ...
    FF20 803F H
    FF20 8000 H     Block 512 (64 bytes)
    FF20 7FFF H
    FF20 7FC0 H     Block 511 (64 bytes)
    ...
    FF20 007F H
    FF20 0040 H     Block 1 (64 bytes)
    FF20 003F H
    FF20 0000 H     Block 0 (64 bytes)
*/

void DF_Flash_erase(unsigned short number_of_first_block , unsigned short numbers_of_blocks)
{
    r_fdl_request_t           req;

    DI();

    /* -----------------------------------------------------------------------
       Example...
       Erase Flash block 0
       ----------------------------------------------------------------------- */   
    req.command_enu     = R_FDL_CMD_ERASE;
    req.idx_u32         = number_of_first_block;    // from block xx 
    req.cnt_u16         = numbers_of_blocks;        // how many blocks
    req.accessType_enu  = R_FDL_ACCESS_USER;
    R_FDL_Execute( &req );
    
    while( R_FDL_BUSY == req.status_enu )
    {
        R_FDL_Handler();
    }
    if( R_FDL_OK != req.status_enu )
    {   
        /* Error handler */
        #if 1
        tiny_printf("\r\n>>>>R_FDL_CMD_ERASE err(0x%02X)\r\n",req.status_enu);
        return;
        #else
        while( 1 )
            ;
        #endif
    }

    EI();
        
    tiny_printf("R_FDL_CMD_ERASE rdy\r\n");
}

void DF_Flash_process(void)
{
    unsigned char i = 0;
    unsigned short start_address = 0;

    if (FLAG_PROJ_TRIG_4)   // data block 2 write array
    {
        start_address = 0x40;
        reset_buffer(rd_buffer,0x00,DATA_FLASH_BLOCK_WORDS);

        DF_Flash_erase(1,1);
        DF_Flash_blank_check(start_address,DATA_FLASH_BLOCK_WORDS);
        DF_Flash_data_write(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)array);
        DF_Flash_data_read(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&rd_buffer);
        compare_buffer(array,rd_buffer,DATA_FLASH_BLOCK_WORDS);
        dump_buffer32(rd_buffer,DATA_FLASH_BLOCK_WORDS);

        FLAG_PROJ_TRIG_4 = 0;
    }
    if (FLAG_PROJ_TRIG_3)   // data block 1 write ram buffer
    {
        for(i = 0;i < DATA_FLASH_BLOCK_WORDS;i++)
        {
            wr_buffer[i] = 0x1FFF7000 + i + counter;
        }
        
        start_address = 0x00;
        reset_buffer(rd_buffer,0x00,DATA_FLASH_BLOCK_WORDS);

        DF_Flash_erase(0,1);
        DF_Flash_blank_check(start_address,DATA_FLASH_BLOCK_WORDS);
        DF_Flash_data_write(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&wr_buffer);
        DF_Flash_data_read(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&rd_buffer);
        compare_buffer(wr_buffer,rd_buffer,DATA_FLASH_BLOCK_WORDS);
        dump_buffer32(rd_buffer,DATA_FLASH_BLOCK_WORDS);

        FLAG_PROJ_TRIG_3 = 0;
        counter += 0x100;
    }
    if (FLAG_PROJ_TRIG_2)   // read block 0 ~ 1
    {
        start_address = 0x00;
        reset_buffer(rd_buffer,0x00,DATA_FLASH_BLOCK_WORDS);

        DF_Flash_data_read(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&rd_buffer);
        tiny_printf("block 0\r\n");
        dump_buffer32(rd_buffer,DATA_FLASH_BLOCK_WORDS);

        start_address = 0x40;
        reset_buffer(rd_buffer,0x00,DATA_FLASH_BLOCK_WORDS);

        DF_Flash_data_read(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&rd_buffer);
        tiny_printf("block 1\r\n");
        dump_buffer32(rd_buffer,DATA_FLASH_BLOCK_WORDS);

        FLAG_PROJ_TRIG_2 = 0;
    }
    if (FLAG_PROJ_TRIG_1)   // erase 0 ~ 31
    {
        DF_Flash_erase(0,32);
        DF_Flash_blank_check(start_address,DATA_FLASH_BLOCK_WORDS*32);
        FLAG_PROJ_TRIG_1 = 0;
    }
}

void DF_Flash_init(void)
{
    r_fdl_status_t            fdlRet;
    r_fdl_request_t           req;

    DI();

    /*****************************************************************************************************************
     * Open the FDL / Data Flash access
     *****************************************************************************************************************/
    /* Initialize the data FLash is required to be able to access the data Flash. As this is considered to be a user
       function, it is not part of the libraries, but part of the application sample */
    FDL_Open ();

    /* 1st initialize the FDL */
    fdlRet = R_FDL_Init( &sampleApp_fdlConfig_enu );
    if( R_FDL_OK != fdlRet )
    {   
        /* Error handler */
        #if 1
        tiny_printf("\r\n>>>>R_FDL_Init err(0x%02X)\r\n",fdlRet);
        return;
        #else
        while( 1 )
            ;
        #endif
    }
        
    tiny_printf("R_FDL_Init rdy\r\n");

    #ifndef R_FDL_LIB_V1_COMPATIBILITY
        /* Prepare the environment */
        req.command_enu     = R_FDL_CMD_PREPARE_ENV;
        req.idx_u32         = 0;
        req.cnt_u16         = 0;
        req.accessType_enu  = R_FDL_ACCESS_NONE;
        R_FDL_Execute( &req );
        
        while( R_FDL_BUSY == req.status_enu )
        {
            R_FDL_Handler();
        }
        if( R_FDL_OK != req.status_enu )
        {   
            /* Error handler */
            #if 1
            tiny_printf("\r\n>>>>R_FDL_CMD_PREPARE_ENV err(0x%02X)\r\n",req.status_enu);
            return;
            #else
            while( 1 )
                ;
            #endif
        }
        
        tiny_printf("R_FDL_CMD_PREPARE_ENV rdy\r\n");
    #endif

    /*****************************************************************************************************************
     * Close the FDL / Data Flash access
     *****************************************************************************************************************/
    // FDL_Close ();

    EI();
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

    DF_Flash_process();
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
