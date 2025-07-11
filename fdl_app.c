/*_____ I N C L U D E S ____________________________________________________*/
// #include <stdio.h>
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

#include "fdl_app.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/

/*_____ D E F I N I T I O N S ______________________________________________*/
static unsigned char fdl_cmd_busy_flag = 0;

unsigned long rd_buffer[DATA_FLASH_BLOCK_WORDS] = {0};
unsigned long wr_buffer[DATA_FLASH_BLOCK_WORDS] = {0};
unsigned long counter = 0;

typedef enum
{
    EXECUTE_TIMEOUT = -1,
    EXECUTE_OK = 0,
    EXECUTE_ERROR = 1,
    EXECUTE_BUSY = 2,

    EXECUTE_DEFAULT,
}execute_result;

const char* cmdStr[] = 
{
    "R_FDL_CMD_ERASE",
    "R_FDL_CMD_WRITE",
    "R_FDL_CMD_BLANKCHECK",
    "R_FDL_CMD_READ",
    "R_FDL_CMD_RESERVED",
    "R_FDL_CMD_PREPARE_ENV",
};

const unsigned long array[] = 
{
  0x5A1EAD72, 0x3B99EA52, 0xA0681756, 0xFEFEB801,
  0x5B8E3333, 0xBB174F9E, 0xB9C71DF8, 0x7A492502,
  0x5CF9A33A, 0xD42E717F, 0xB2ED2A92, 0xAF2AF203,
  0x5D43B5B7, 0x52D6FB3C, 0x51B973CE, 0xEED3D104
};

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

execute_result __df_flash_low_level_drv(r_fdl_command_t cmd , unsigned long idx_u32 , unsigned short cnt_u16 , unsigned long* ram_buffer)
{
    unsigned long timeout = 0xFFFFFFFF;
    r_fdl_request_t           req;

    /*
        Blank Check addresses from 0x10 to 0x17.
        myRequest.idx_u32          = 0x10; 
        myRequest.cnt_u16          = 2; 
    */

    DI();

    fdl_cmd_busy_flag = 1;  
    
    req.command_enu     = cmd;

    if ((req.command_enu == R_FDL_CMD_WRITE) ||
        (req.command_enu == R_FDL_CMD_READ))
    {
        /*
            [bufAddr_u32]
            R_FDL_CMD_WRITE
                Address of the buffer containing the source data to be written.    
            R_FDL_CMD_READ
                Data destination buffer address in RAM.
                Note: The buffer must be 32-bit aligned!  
        */
        req.bufAddr_u32     = (uint32_t)( &ram_buffer[0] );
    }
    /*
        [idx_u32]
        R_FDL_CMD_BLANKCHECK
            The virtual start address for performing blank check in data flash. Must be word (4 bytes) aligned
        R_FDL_CMD_WRITE    
            The virtual start address for writing in Data Flash aligned to word size (4 bytes). 
        R_FDL_CMD_READ
            Data Flash virtual address from where to read. Must be word (4 bytes) aligned.  
        R_FDL_CMD_ERASE
            from block xx   
        R_FDL_CMD_PREPARE_ENV
            not used 

        [cnt_u16]
        R_FDL_CMD_BLANKCHECK
            Number of words (4 bytes) to check
        R_FDL_CMD_WRITE    
            Number of words to write. 
        R_FDL_CMD_READ
            Numbers of words (4 bytes) to read
        R_FDL_CMD_ERASE
            how many blocks   
        R_FDL_CMD_PREPARE_ENV
            not used 
    */
    if (req.command_enu == R_FDL_CMD_PREPARE_ENV)
    {
        req.idx_u32         = 0;
        req.cnt_u16         = 0;
        req.accessType_enu  = R_FDL_ACCESS_NONE;
    }
    else
    {
        req.idx_u32         = idx_u32;
        req.cnt_u16         = cnt_u16;
        req.accessType_enu  = R_FDL_ACCESS_USER;
    }

    R_FDL_Execute( &req );
    
    while( R_FDL_BUSY == req.status_enu )
    {
        R_FDL_Handler();
        
        if (timeout-- == 0)
        {
            fdl_cmd_busy_flag = 0;
            return EXECUTE_TIMEOUT;
        }
    }
    
    if( R_FDL_OK != req.status_enu )
    {   
        /* Error handler */

        if (req.command_enu == R_FDL_CMD_READ)
        {
            if (R_FDL_ERR_ECC_SED == req.status_enu)
            {
                tiny_printf("\r\n>>>> [%s] error-single bit ECC error(0x%02X)\r\n",cmdStr[cmd],req.status_enu);
                
            }
            else if (R_FDL_ERR_ECC_DED == req.status_enu)
            {
                tiny_printf("\r\n>>>> [%s] error-double bit ECC error(0x%02X)\r\n",cmdStr[cmd],req.status_enu);            
            }
            else
            {
                tiny_printf("\r\n>>>> [%s] error(0x%02X)\r\n",cmdStr[cmd],req.status_enu);
            }
        }
        else
        {
            tiny_printf("\r\n>>>> [%s] error(0x%02X)\r\n",cmdStr[cmd],req.status_enu);
        }

        fdl_cmd_busy_flag = 0;
        return EXECUTE_ERROR;
    }

    EI();

    tiny_printf("%s rdy\r\n",cmdStr[cmd]);
    
    fdl_cmd_busy_flag = 0;
    return EXECUTE_OK;
}

void __df_flash_low_level_result_check(execute_result res,r_fdl_command_t cmd)
{
    if (res != EXECUTE_OK)
    {
        tiny_printf(">>>> df_flash execute result error(%s:%d)\r\n\r\n",cmdStr[cmd],res);
    }
}

execute_result __df_flash_low_level_execute_wrapper(r_fdl_command_t cmd , unsigned long idx_u32 , unsigned short cnt_u16 , unsigned long* ram_buffer)
{    
    execute_result res = EXECUTE_DEFAULT;

    if (fdl_cmd_busy_flag)
    {
        return EXECUTE_BUSY;
    }
   
    res = __df_flash_low_level_drv(cmd,idx_u32,cnt_u16,ram_buffer);   
    __df_flash_low_level_result_check(res,cmd);

    return res;
}

/*
    len : 1 words = 4 bytes = uint32_t 
    write_buff : 32-bit aligned
*/
void DF_Flash_data_write(unsigned long start_addr , unsigned short numbers_of_words , unsigned long* write_buff)
{
    r_fdl_command_t cmd = R_FDL_CMD_WRITE;

    __df_flash_low_level_execute_wrapper(cmd,start_addr,numbers_of_words,write_buff); 
}

/*
    len : 1 words = 4 bytes = uint32_t 
    read_buff : 32-bit aligned
*/
void DF_Flash_data_read(unsigned long start_addr , unsigned short numbers_of_words , unsigned long* read_buff)
{
    r_fdl_command_t cmd = R_FDL_CMD_READ;

    __df_flash_low_level_execute_wrapper(cmd,start_addr,numbers_of_words,read_buff);  
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
    /* -----------------------------------------------------------------------
       Example...
       Erase Flash block 0
       ----------------------------------------------------------------------- */   
    
    execute_result res = EXECUTE_DEFAULT;
    r_fdl_command_t cmd = R_FDL_CMD_BLANKCHECK;

    // if blank , no need to execute erase
    cmd = R_FDL_CMD_BLANKCHECK;
    res = __df_flash_low_level_execute_wrapper(cmd,number_of_first_block*DATA_FLASH_BLOCK_SIZE_BYTES,numbers_of_blocks*DATA_FLASH_BLOCK_WORDS,NULL);  
    if (res == EXECUTE_OK)
    {
        return;
    }

    // not blank , execute erase    
    cmd = R_FDL_CMD_ERASE;
    __df_flash_low_level_execute_wrapper(cmd,number_of_first_block,numbers_of_blocks,NULL);
  
    // after erase , check blank again
    cmd = R_FDL_CMD_BLANKCHECK;
    __df_flash_low_level_execute_wrapper(cmd,number_of_first_block*DATA_FLASH_BLOCK_SIZE_BYTES,numbers_of_blocks*DATA_FLASH_BLOCK_WORDS,NULL);

}

void DF_Flash_init(void)
{
    r_fdl_status_t            fdlRet;
    r_fdl_command_t cmd = R_FDL_CMD_PREPARE_ENV;

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
    __df_flash_low_level_execute_wrapper(cmd,0UL,0U,NULL);
    #endif

    /*****************************************************************************************************************
     * Close the FDL / Data Flash access
     *****************************************************************************************************************/
    // FDL_Close ();

}


void DF_Flash_test_process(unsigned char idx)
{
    unsigned char i = 0;
    unsigned short start_address = 0;

    switch(idx)
    {
        case 4:    // data block 2 write array
            start_address = 0x40;
            reset_buffer(rd_buffer,0x00,DATA_FLASH_BLOCK_WORDS);

            DF_Flash_erase(1,1);
            DF_Flash_data_write(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)array);
            DF_Flash_data_read(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&rd_buffer);
            compare_buffer(array,rd_buffer,DATA_FLASH_BLOCK_WORDS);
            dump_buffer32(rd_buffer,DATA_FLASH_BLOCK_WORDS);        
            break;
        case 3:   // data block 1 write ram buffer
            for(i = 0;i < DATA_FLASH_BLOCK_WORDS;i++)
            {
                wr_buffer[i] = 0x1FFF7000 + i + counter;
            }
            
            start_address = 0x00;
            reset_buffer(rd_buffer,0x00,DATA_FLASH_BLOCK_WORDS);

            DF_Flash_erase(0,1);
            DF_Flash_data_write(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&wr_buffer);
            DF_Flash_data_read(start_address,DATA_FLASH_BLOCK_WORDS,(unsigned long *)&rd_buffer);
            compare_buffer(wr_buffer,rd_buffer,DATA_FLASH_BLOCK_WORDS);
            dump_buffer32(rd_buffer,DATA_FLASH_BLOCK_WORDS);

            counter += 0x100;        
            break;
        case 2:   // read block 0 ~ 1
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
            break;
        case 1:   // erase 0 ~ 31
            DF_Flash_erase(0,32);
            break;
    }

}


