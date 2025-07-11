#ifndef __FDL_APP_H__
#define __FDL_APP_H__

/*_____ I N C L U D E S ____________________________________________________*/

/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/

/*  
	template
	typedef struct _peripheral_manager_t
	{
		uint8_t u8Cmd;
		uint8_t au8Buf[33];
		uint8_t u8RecCnt;
		uint8_t bByPass;
		uint16_t* pu16Far;
	}PERIPHERAL_MANAGER_T;

	volatile PERIPHERAL_MANAGER_T g_PeripheralManager = 
	{
		.u8Cmd = 0,
		.au8Buf = {0},		//.au8Buf = {100U, 200U},
		.u8RecCnt = 0,
		.bByPass = FALSE,
		.pu16Far = NULL,	//.pu16Far = 0	
	};
	extern volatile PERIPHERAL_MANAGER_T g_PeripheralManager;
*/

// data flash , 1 block = 64 bytes = 16 words
#define DATA_FLASH_BLOCK_SIZE_BYTES                     (64U)
#define DATA_FLASH_BLOCK_SIZE_WORDS                     (16U)
#define DATA_FLASH_WORD_SIZE_BYTES                      (4U)
#define DATA_FLASH_BLOCK_WORDS                          DATA_FLASH_BLOCK_SIZE_WORDS  // alias


/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/
extern unsigned long rd_buffer[DATA_FLASH_BLOCK_WORDS];
extern unsigned long wr_buffer[DATA_FLASH_BLOCK_WORDS];
extern unsigned long counter;
extern const unsigned long array[];

void DF_Flash_data_write(unsigned long start_addr , unsigned short numbers_of_words , unsigned long* write_buff);
void DF_Flash_data_read(unsigned long start_addr , unsigned short numbers_of_words , unsigned long* read_buff);
void DF_Flash_erase(unsigned short number_of_first_block , unsigned short numbers_of_blocks);

void DF_Flash_init(void);
void DF_Flash_test_process(unsigned char idx);

#endif //__FDL_APP_H__
