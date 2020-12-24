/**
*  @file      tps65987-drv.c
*  @brief     tps65987 i2c drv
*  @author    Link Lin
*  @date      11 -2019
*  @copyright
*/

#include<stdio.h>
#include<fcntl.h>
#include <error.h>
#include<unistd.h>
#include<sys/ioctl.h>
#include<string.h>
#include<stdlib.h>

#include<linux/i2c.h>
#include<linux/i2c-dev.h>

#include "tps65987_drv.h"

#define OTA_FILE_NAME "/data/ota-file/r1-low-region-flash-"
#define OTA_FILE_NAME1 ".bin"

/*the I2C addr will change, 0x38 or 0x20
  i2c1 cab be master/slave, address is 0x20
  i2c2 is only slave, address is 0x38
*/

unsigned int I2C_ADDR = 0x38;

//supposed to be Little-endian
int check_endian(void)
{
    unsigned int x;
    unsigned char x0,x1,x2,x3;

    x=0x12345678;

    x0=((unsigned char *)&x)[0];  //low byte
    x1=((unsigned char *)&x)[1];
    x2=((unsigned char *)&x)[2];
    x3=((unsigned char *)&x)[3];  //high byte

    if(x0 == 0x12)
    {
        printf("Big-endian, x0=0x%x,x1=0x%x,x2=0x%x,x3=0x%x\n",x0,x1,x2,x3);
    }

    if(x0 == 0x78)
    {
        printf("Little-endian, x0=0x%x,x1=0x%x,x2=0x%x,x3=0x%x\n",x0,x1,x2,x3);
    }

    return 0;
}


enum FLASH_UPGRADE_STATE
{
    OPEN_FILE,
    READ_FILE,
    VERIFY_IF_VALID,
    CLOSE_FILE,
};


struct FLASH_UPGRADE_PARA
{
    enum FLASH_UPGRADE_STATE flash_upgrade_state;

    unsigned char active_region;
    unsigned char inactive_region;

    unsigned char flash_upgrade_finish;

} flash_upgrade_para;


static int fd;


int i2c_open_tps65987(unsigned char i2c_addr,char *i2c_file_name)
{
    int ret;

    int val;

    fd = open(i2c_file_name, O_RDWR);

    if(fd < 0)
    {
        perror("Unable to open i2c control file");

        return -1;
    }

    printf("open i2c file success %d\n",fd);

    ret = ioctl(fd, I2C_SLAVE_FORCE, i2c_addr);
    if (ret < 0)
    {
        printf("i2c: Failed to set i2c device address 0x%x\n",i2c_addr);
        return -1;
    }

    printf("i2c: set i2c device address success\n");

    val = 3;
    ret = ioctl(fd, I2C_RETRIES, val);
    if(ret < 0)
    {
        printf("i2c: set i2c retry times err\n");
    }

    printf("i2c: set i2c retry times %d\n",val);

    /*
    * use I2C_TIMEOUT default setting, which is HZ, that means 1 second
    */

    return 0;
}

static int i2c_write(int fd, unsigned char dev_addr, unsigned char *val, unsigned char len)
{
    int ret;
    int i;

    struct i2c_rdwr_ioctl_data data;

    struct i2c_msg messages;


    messages.addr = dev_addr;  //device address
    messages.flags = 0;    //write
    messages.len = len;
    messages.buf = val;  //data

    data.msgs = &messages;
    data.nmsgs = 1;

    ret = ioctl(fd, I2C_RDWR, &data);

    if(ret < 0)
    {
        printf("write ioctl err %d\n",ret);
        return ret;
    }

    printf("i2c write buf = ");
    for(i=0; i< len; i++)
    {
        printf("%02x ",val[i]);
    }
    printf("\n");

    return 0;
}


static int i2c_read(int fd, unsigned char addr, unsigned char reg, unsigned char *val, unsigned char len)
{
    int ret;
    int i;

    struct i2c_rdwr_ioctl_data data;
    struct i2c_msg messages[2];

    messages[0].addr = addr;  //device address
    messages[0].flags = 0;    //write
    messages[0].len = 1;
    messages[0].buf = &reg;  //reg address

    messages[1].addr = addr;       //device address
    messages[1].flags = I2C_M_RD;  //read
    messages[1].len = len;
    messages[1].buf = val;

    data.msgs = messages;
    data.nmsgs = 2;

    ret = ioctl(fd, I2C_RDWR, &data);

    if(ret < 0)
    {
        printf("read ioctl err %d\n",ret);

        return ret;
    }

    printf("i2c read buf = ");
    for(i = 0; i < len; i++)
    {
        printf("%02x ",val[i]);
    }
    printf("\n");

    return 0;
}


int tps65987_i2c_write(unsigned char dev_addr, unsigned char reg, unsigned char *val, unsigned char data_len)
{
    unsigned char buf[80] = {0};
    int i;

    int ret;

    if(data_len + 2 >= 80)
    {
        printf("data_len_exceed\n");
        return -1;
    }

    buf[0] = reg;
    buf[1] = data_len;

    for(i = 0; i<data_len; i++)
    {
        buf[2+i] = val[i];
    }

    if(i2c_write(fd, dev_addr, buf, data_len+2) == 0)
    {
        return 0;
    }

    return -1;
}


int tps65987_i2c_read(unsigned char addr, unsigned char reg, unsigned char *val, unsigned char data_len)
{
    unsigned char buf[80] = {0};
    int i;

    if(data_len + 1 >= 80)
    {
        printf("data_len_exceed\n");
        return -1;
    }

    if(i2c_read(fd, addr, reg, buf, data_len+1) == 0)
    {
        printf("read reg 0x%x = ",reg);
        for(i = 0; i < data_len; i++)
        {
            val[i] = buf[1+i];
            printf("%02x ",val[i]);
        }
        printf("\n");

        return 0;
    }

    return -1;
}


static int tps65987_send_4CC_Cmd(unsigned char *cmd_ptr, unsigned char *cmd_data_ptr, unsigned char cmd_data_length)
{
    int ret;
    int i;

    unsigned char val[4] = {0};

    //first write 4CC Cmd Used Data(if any)
    if(cmd_data_ptr != NULL)
    {
        ret = tps65987_i2c_write(I2C_ADDR, 0x09, cmd_data_ptr, cmd_data_length);

        if(ret != 0)
        {
            printf("write 4CC Cmd Used Data err \n");
            return -1;
        }
    }

    val[0] = cmd_ptr[0];
    val[1] = cmd_ptr[1];
    val[2] = cmd_ptr[2];
    val[3] = cmd_ptr[3];

    printf("send 4CC Cmd : ");
    for(i=0; i<4; i++)
    {
        printf("%c",val[i]);
    }
    printf("\n");

    //write 4CC Cmd
    return tps65987_i2c_write(I2C_ADDR, 0x08, val, 4);
}


static int tps65987_check_4CC_Cmd_executed()
{
    int i;

    unsigned char buf[4] = {0,1,2,3}; //just a random value

    unsigned char Cmd_exec_success[4] = {0,0,0,0};

    unsigned char Cmd_exec_fail[4] = {'C','M','D',' '};
    unsigned char Cmd_unrecognized[4] = {'!','C','M','D'};

    for(i = 0; i< 50; i++)
    {
        usleep(10000);

        tps65987_i2c_read(I2C_ADDR, 0x08, buf, 4);

        if(memcmp(buf,Cmd_exec_success,4) == 0)
        {
            printf("4CC Cmd executed, %d\n", i);
            return 0;
        }

        if(memcmp(buf,Cmd_exec_fail,4) == 0)
        {
            printf("4CC Cmd exec fail, %d\n", i);
            return 1;
        }

        if(memcmp(buf,Cmd_unrecognized,4) == 0)
        {
            printf("4CC Cmd unrecognized, %d\n", i);
            return -1;
        }
    }

    printf("4CC Cmd exec timeout, %d\n", i);
    return -1;

}


static int tps65987_read_4CC_Cmd_exec_output(unsigned char *cmd_data_ptr, unsigned char cmd_data_length)
{
    return tps65987_i2c_read(I2C_ADDR, 0x09, cmd_data_ptr, cmd_data_length);
}


int tps65987_exec_4CC_Cmd(unsigned char *cmd_ptr, unsigned char *cmd_data_in_ptr, unsigned char cmd_data_in_length, unsigned char *cmd_data_out_ptr, unsigned char cmd_data_out_length)
{

    if(tps65987_send_4CC_Cmd(cmd_ptr, cmd_data_in_ptr, cmd_data_in_length) != 0)
    {
        printf("send_4CC_Cmd err\n");
        return -1;
    }

    if( strcmp(cmd_ptr,"Gaid") == 0 || strcmp(cmd_ptr,"GAID") == 0 )
    {
        //Technically this command never completes since the processor restarts
    }
    else
    {
        if(tps65987_check_4CC_Cmd_executed() != 0)
        {
            printf("4CC_Cmd exec err\n");
            return -1;
        }
    }


    if(cmd_data_out_ptr != NULL)
    {
        if(tps65987_read_4CC_Cmd_exec_output(cmd_data_out_ptr, cmd_data_out_length) != 0)
        {
            printf("read 4CC_Cmd exec output err\n");
            return -1;
        }
    }

    return 0;
}


int tps65987_host_patch_bundle(void)
{
    unsigned char buf[64] = {0};

    tps65987_i2c_read(I2C_ADDR, 0x14, buf, 11);
    tps65987_i2c_read(I2C_ADDR, 0x15, buf, 11);

    tps65987_send_4CC_Cmd("PTCq", 0, 0);
    tps65987_check_4CC_Cmd_executed();

    //test unvalid 4CC Cmd
    tps65987_send_4CC_Cmd("ABCD", 0, 0);
    tps65987_check_4CC_Cmd_executed();

    tps65987_send_4CC_Cmd("PTCr", 0, 0);
    tps65987_check_4CC_Cmd_executed();

    tps65987_send_4CC_Cmd("Gaid", 0, 0);
    tps65987_check_4CC_Cmd_executed();

    tps65987_i2c_read(I2C_ADDR, 0x14, buf, 11);
    tps65987_i2c_read(I2C_ADDR, 0x15, buf, 11);

    tps65987_send_4CC_Cmd("PTCs", 0, 0);
    tps65987_check_4CC_Cmd_executed();

    tps65987_i2c_read(I2C_ADDR, 0x14, buf, 11);
    tps65987_i2c_read(I2C_ADDR, 0x15, buf, 11);
}

/*
* function for flash upgrade
*/
static int PreOpsForFlashUpdate(void);
static int StartFlashUpdate(char *ota_file_name);
static int UpdateAndVerifyRegion(unsigned char region_number, char *ota_file_name);


static int PreOpsForFlashUpdate(void)
{
    unsigned char buf[64];
    int ret;

    s_TPS_bootflag *p_bootflags = NULL;
    s_TPS_portconfig *p_portconfig = NULL;

    tps65987_i2c_read(I2C_ADDR, REG_Version, buf, 4);

    printf("tps65987 check version\n");

    /*
    * Read BootFlags (0x2D) register:
    * - Note #1: Applications shouldn't proceed w/ flash-update if the device's
    * boot didn't succeed
    * - Note #2: Flash-update shall be attempted on the inactive region first
    */
    tps65987_i2c_read(I2C_ADDR, REG_BootFlags, buf, 12);

    p_bootflags = (s_TPS_bootflag *)&buf[0];

    printf("TPS_bootflag = 0x%08x\n", *((unsigned int *)p_bootflags));
    printf("test TPS_bootflag %x\n", p_bootflags->SpiFlashPresent);

    if(p_bootflags->PatchHeaderErr != 0)
    {
        printf("PatchHeaderErr\n");
        return -1;
    }

    /*
    * Note #2
    * Region1 = 0 indicates that device didn't attempt 'Region1',
    * which implicitly means that the content at Region0 is valid/active
    */
    if(p_bootflags->Region1 == 0)
    {
        flash_upgrade_para.active_region = REGION_0;
        flash_upgrade_para.inactive_region = REGION_1;

        printf("flash_upgrade_para inactive_region is REGION_1, %d\n", flash_upgrade_para.inactive_region);
    }
    else if ( (p_bootflags->Region1 == 1) && \
              (p_bootflags->Region0 == 1) && \
              ((p_bootflags->Region1CrcFail == 0) && \
               (p_bootflags->Region1FlashErr == 0) && \
               (p_bootflags->Region1Invalid == 0)) )
    {
        flash_upgrade_para.active_region = REGION_1;
        flash_upgrade_para.inactive_region = REGION_0;

        printf("flash_upgrade_para inactive_region is REGION_0, %d\n", flash_upgrade_para.inactive_region);
    }
    else
    {
        printf("Region Check Err\n");
        return -1;

        //need further debug
        /*printf("force upgrade REGION_0\n");
        flash_upgrade_para.active_region = REGION_1;
        flash_upgrade_para.inactive_region = REGION_0;*/
    }

    /*
    * Keep the port disabled during the flash-update
    */
    tps65987_i2c_read(I2C_ADDR, REG_PORTCONFIG, buf, 8);

    p_portconfig = (s_TPS_portconfig *)&buf[0];

    printf("TPS_portconfig = 0x%04x\n", *((unsigned int *)p_portconfig));
    printf("test TPS_portconfig %x\n", p_portconfig->TypeCStateMachine);

    p_portconfig->TypeCStateMachine = DISABLE_PORT;

    tps65987_i2c_write(I2C_ADDR, REG_PORTCONFIG, buf, 8);

    printf("DISABLE TYPE-C PORT\n");

    sleep(3);
    tps65987_i2c_read(I2C_ADDR, REG_PORTCONFIG, buf, 8); //just for check

    return 0;
}


static int StartFlashUpdate(char *ota_file_name)
{
    int retVal;

    printf("\n\rActive Region is [%d] - Region being updated is [%d]\n\r",
           flash_upgrade_para.active_region, flash_upgrade_para.inactive_region);

    /*
    * Region-0 is currently active, hence update Region-1
    */
    retVal = UpdateAndVerifyRegion(flash_upgrade_para.inactive_region,ota_file_name);
    if(retVal != 0)
    {
        printf("Region[%d] update failed.! Next boot will happen from Region[%d]\n\r",\
               flash_upgrade_para.inactive_region, flash_upgrade_para.active_region);
        retVal = -1;
        goto error;
    }

    /*
    * Region-1 is successfully updated.
    * To maintain a redundant copy for a fail-safe flash-update, copy the same
    * content at Region-0
    */
    printf("Region-%d is successfully updated.To maintain a redundant copy for a fail-safe flash-update, \
    copy the same content at Region-%d",flash_upgrade_para.inactive_region,flash_upgrade_para.active_region);

    retVal = UpdateAndVerifyRegion(flash_upgrade_para.active_region, ota_file_name);
    if(retVal != 0)
    {
        printf("Region[%d] update failed.! Next boot will happen from Region[%d]\n\r",\
               flash_upgrade_para.active_region, flash_upgrade_para.inactive_region);
        retVal = -1;
        goto error;
    }

error:
    //add some operation if need, maybe
    return retVal;
}


static int UpdateAndVerifyRegion(unsigned char region_number, char *ota_file_name)
{
    FILE *fp;

    unsigned char buf[64];
    int ret;

    int i;

    s_TPS_flrr flrrInData = {0};
    s_TPS_flem flemInData = {0};
    s_TPS_flad fladInData = {0};
    s_TPS_flvy flvyInData = {0};

    unsigned char outdata[64];

    int idx = -1;
    int retVal = -1;

    unsigned int regAddr = 0;

    /*
    * should first check whether the upgrade bin file is exist
    */
    fp = fopen(ota_file_name,"rb");
    if(fp == NULL)
    {
        printf("fail to open tps65987 upgrade bin file, ota file is %s.\n", ota_file_name);
        return -1;
    }

    printf("open tps65987 upgrade bin file success\n");

    /*
    * Get the location of the region 'region_number'
    */
    flrrInData.regionnum = region_number;
    retVal = tps65987_exec_4CC_Cmd("FLrr", (unsigned char *)&flrrInData, 1, outdata, 4);

    if(retVal != 0)
    {
        printf("4CC_Cmd FLrr FAILED.!\n\r");
        return -1;
    }

    regAddr = (outdata[3] << 24) | (outdata[2] << 16) | (outdata[1] << 8) | outdata[0];

    printf("regAddr = 0x%08x\n", regAddr);

    /*
    * Erase #'numof4ksector' sectors at address 'regAddr' of the sFLASH
    * - Note: The below snippet assumes that the total number of 4kB segments
    * required to hold the maximum size of the patch-bundle is 4.
    * Ensure its validity for the TPS6598x being used for your
    * application.
    */
    flemInData.flashaddr = regAddr;
    flemInData.numof4ksector = 4;
    retVal = tps65987_exec_4CC_Cmd("FLem", (unsigned char *)&flemInData, 5, outdata, 1);

    if(retVal != 0)
    {
        printf("4CC_Cmd FLem FAILED.!\n\r");
        return -1;
    }

    if(outdata[0] != 0)
    {
        printf("Flash Erase FAILED.! 0x%x\n\r",outdata[0]);
        return -1;
    }

    printf("Flash Erase Success.! 0x%x\n\r",outdata[0]);

    /*
    * Set the start address for the next write
    */
    fladInData.flashaddr = regAddr;
    retVal = tps65987_exec_4CC_Cmd("FLad", (unsigned char *)&fladInData, 4, outdata, 1);

    if(retVal != 0)
    {
        printf("4CC_Cmd FLad FAILED.!\n\r");
        return -1;
    }

    printf("Updating [%d] 4k chunks starting @ 0x%x \n\r", flemInData.numof4ksector, regAddr);

    flash_upgrade_para.flash_upgrade_finish = 0;
    flash_upgrade_para.flash_upgrade_state = OPEN_FILE;

    while(!flash_upgrade_para.flash_upgrade_finish)
    {
        switch(flash_upgrade_para.flash_upgrade_state)
        {
            case OPEN_FILE:
                //already opened
                /*fp = fopen("/tmp/low-region-flash.bin","rb");
                if(fp == NULL)
                {
                    printf("fail to open tps65987 upgrade bin file\n");
                    return 1;
                }

                printf("open tps65987 upgrade bin file success\n");*/

                flash_upgrade_para.flash_upgrade_state = READ_FILE;
                break;

            case READ_FILE:
                ret = fread(buf,1,64,fp);

                if(feof(fp))
                {
                    printf("read file finish %d:\n", ret);

                    flash_upgrade_para.flash_upgrade_state = VERIFY_IF_VALID;
                    break;
                }

                printf("read %d data from file:", ret);
                for(i=0; i<ret; i++)
                {
                    printf("%02x",buf[i]);
                }
                printf("\n");

                /*
                * Execute FLwd with PATCH_BUNDLE_SIZE bytes of patch-data
                * in each iteration
                */
                retVal = tps65987_exec_4CC_Cmd("FLwd", buf, ret, outdata, 1);

                if(retVal != 0)
                {
                    printf("4CC_Cmd FLwd FAILED.!\n\r");
                    fclose(fp);
                    return -1;
                }

                /*
                * 'outdata[1]' will contain the command's return code
                */
                if(outdata[0] != 0)
                {
                    printf("Flash Write FAILED.!\n\r");
                    fclose(fp);
                    return -1;
                }

                usleep(100000);

                break;

            case VERIFY_IF_VALID:
                /*
                * Write is through. Now verify if the content/copy is valid
                */
                flvyInData.flashaddr = regAddr;
                retVal = tps65987_exec_4CC_Cmd("FLvy", (unsigned char *)&flvyInData, 4, outdata, 1);

                if(outdata[0] != 0)
                {
                    printf("Flash Verify FAILED.!\n\r");
                    fclose(fp);
                    return -1;
                }

                flash_upgrade_para.flash_upgrade_state = CLOSE_FILE;
                break;

            case CLOSE_FILE:
                fclose(fp);
                printf("close tps65987 upgrade bin file\n");

                flash_upgrade_para.flash_upgrade_state = OPEN_FILE;
                flash_upgrade_para.flash_upgrade_finish = 1;
                break;
        }

    }

    return 0;
}

int ResetPDController()
{
    unsigned char buf[64] = {0};

    /*
    * Execute GAID, and wait for reset to complete
    */
    printf("Send GAID and Waiting for device to reset\n\r");
    tps65987_exec_4CC_Cmd("GAID", NULL, 0, NULL, 0);

    usleep(1000000);

    //read Mode
    tps65987_i2c_read(I2C_ADDR, REG_MODE, buf, 4);

    tps65987_i2c_read(I2C_ADDR, REG_Version, buf, 4);

    tps65987_i2c_read(I2C_ADDR, REG_BootFlags, buf, 12);

    return 0;
}


int tps65987_ext_flash_upgrade(char *ota_file_name)
{
    int retVal;

    if(PreOpsForFlashUpdate() != 0)
    {
        printf("Pre Ops For FlashUpdate fail\n\r");
        return -1;
    }

    if(StartFlashUpdate(ota_file_name) == 0)
    {
        retVal = 0;
        printf("FlashUpdate success\n\r");
    }
    else
    {
        retVal = -1;
        printf("FlashUpdate fail\n\r");
    }

    ResetPDController();

    return retVal;
}


int tps65987_get_Status(s_TPS_status *p_tps_status)
{
    unsigned char buf[64] = {0};

    if(tps65987_i2c_read(I2C_ADDR, REG_Status, (unsigned char*)p_tps_status, 8) == 0)
    {
        printf("get tps65987 Status: \n");
        printf("PlugPresent: %d\n", p_tps_status->PlugPresent);
        printf("ConnState: %d\n", p_tps_status->ConnState);
        printf("PlugOrientation: %d\n", p_tps_status->PlugOrientation);
        printf("PortRole: ");
        switch(p_tps_status->PortRole)
        {
            case 0:
                printf("Sink, %d\n", p_tps_status->PortRole);
                break;

            case 1:
                printf("Source, %d\n", p_tps_status->PortRole);
                break;
        }

        printf("DataRole: %d\n", p_tps_status->DataRole);
        switch(p_tps_status->DataRole)
        {
            case 0:
                printf("UFP, %d\n", p_tps_status->DataRole);
                break;

            case 1:
                printf("DFP, %d\n", p_tps_status->DataRole);
                break;
        }

        printf("VbusStatus: %d\n", p_tps_status->VbusStatus);
        printf("UsbHostPresent: %d\n", p_tps_status->UsbHostPresent);
        printf("HighVoltageWarning: %d\n", p_tps_status->HighVoltageWarning);
        printf("LowVoltageWarning: %d\n", p_tps_status->LowVoltageWarning);

        return 0;
    }

    return -1;
}


int tps65987_get_PortRole(void)
{
    s_TPS_status tps_status = {0};

    s_TPS_status *p_tps_status = NULL;

    p_tps_status = &tps_status;

    if(tps65987_i2c_read(I2C_ADDR, REG_Status, (unsigned char*)p_tps_status, 8) == 0)
    {
        printf("get tps65987 Status: \n");
        printf("PlugPresent: %d\n", p_tps_status->PlugPresent);
        printf("ConnState: %d\n", p_tps_status->ConnState);
        printf("PlugOrientation: %d\n", p_tps_status->PlugOrientation);
        printf("PortRole: ");
        switch(p_tps_status->PortRole)
        {
            case 0:
                printf("Sink, %d\n", p_tps_status->PortRole);
                break;

            case 1:
                printf("Source, %d\n", p_tps_status->PortRole);
                break;
        }

        printf("DataRole: %d\n", p_tps_status->DataRole);
        switch(p_tps_status->DataRole)
        {
            case 0:
                printf("UFP, %d\n", p_tps_status->DataRole);
                break;

            case 1:
                printf("DFP, %d\n", p_tps_status->DataRole);
                break;
        }

        printf("VbusStatus: %d\n", p_tps_status->VbusStatus);
        printf("UsbHostPresent: %d\n", p_tps_status->UsbHostPresent);
        printf("HighVoltageWarning: %d\n", p_tps_status->HighVoltageWarning);
        printf("LowVoltageWarning: %d\n", p_tps_status->LowVoltageWarning);

        //return PortRole
        switch(p_tps_status->PortRole)
        {
            case 0:
                printf("Sink, %d\n\n", p_tps_status->PortRole);
                return SINK;

            case 1:
                printf("Source, %d\n\n", p_tps_status->PortRole);
                return SOURCE;

            default:
                printf("value err, %d\n\n", p_tps_status->PortRole);
                return -1;
        }
    }

    return -1;
}


int tps65987_get_RXSourceNumValidPDOs(void)
{
    unsigned char buf[64] = {0};

    unsigned char valid_PDO_num = 0;

    if(tps65987_i2c_read(I2C_ADDR, REG_RX_Source_Capabilities, buf, 29) != 0)
    {
        printf("get RXSourceNumValidPDOs err \n");
        return -1;
    }

    valid_PDO_num = buf[0] & 0x03;

    printf("get RXSourceNumValidPDOs = %d\n\n", valid_PDO_num);

    return valid_PDO_num;
}


int tps65987_get_TypeC_Current(void)
{
    s_TPS_Power_Status tps_power_status = {0};

    s_TPS_Power_Status *p_tps_power_status = NULL;

    p_tps_power_status = &tps_power_status;

    if(tps65987_i2c_read(I2C_ADDR, REG_Power_Status, (unsigned char*)p_tps_power_status, 2) == 0)
    {
        printf("get tps65987 Power Status: \n");
        printf("PowerConnection: %d\n", p_tps_power_status->PowerConnection);
        printf("SourceSink: ");
        switch(p_tps_power_status->SourceSink)
        {
            case 0:
                printf("PD Controller as source, %d\n", p_tps_power_status->SourceSink);
                break;

            case 1:
                printf("PD Controller as sink, %d\n", p_tps_power_status->SourceSink);
                break;
        }

        printf("TypeC_Current: ");
        switch(p_tps_power_status->TypeC_Current)
        {
            case USB_Default_Current:
                printf("USB Default Current, %d\n", p_tps_power_status->TypeC_Current);
                break;

            case C_1d5A_Current:
                printf("1.5A, %d\n", p_tps_power_status->TypeC_Current);
                break;

            case C_3A_Current:
                printf("3A, %d\n", p_tps_power_status->TypeC_Current);
                break;

            case PD_contract_negotiated:
                printf("PD contract negotiated, %d\n", p_tps_power_status->TypeC_Current);
                break;
        }

        printf("Charger Detect Status: %d\n", p_tps_power_status->Charger_Detect_Status);
        printf("Charger_AdvertiseStatus: %d\n\n", p_tps_power_status->Charger_AdvertiseStatus);

        //return TypeC_Current
        return p_tps_power_status->TypeC_Current;

    }

    return -1;
}

int main(int argc, char* argv[])
{
    //FILE *fp;
    int i,j;
    unsigned char reg = 0;

    unsigned char buf[64] = {0};
    unsigned char buf_2[64] = {0};
    unsigned char val[64] = {0};
    unsigned char customeruse1[64] = {0};
    unsigned char customeruse[64] ={0};

    s_TPS_status tps_status = {0};

    int tps_port_role;
    memset(val, 0x55, sizeof(val));

    printf("start run tps65987-ota\n");
    freopen("/data/tps65987-log.txt", "w", stdout);

    if(argc > 1)
    {
        for(i = 0; i < argc; i++)
        {
            printf("Argument %d is %s\n", i, argv[i]);
        }

        if(strcmp(argv[1],"0x38") == 0)
        {
            I2C_ADDR = 0x38;
        }
        else if(strcmp(argv[1],"0x20") == 0)
        {
            I2C_ADDR = 0x20;
        }
    }

    printf("used i2c address is 0x%x\n",I2C_ADDR);

    check_endian();

    if(i2c_open_tps65987(I2C_ADDR,argv[2]) != 0)
    {
        return -1;
    }

    //test read
    tps65987_i2c_read(I2C_ADDR, 0x00, buf, 4);
    tps65987_i2c_read(I2C_ADDR, 0x05, buf, 16);
    tps65987_i2c_read(I2C_ADDR, 0x0f, buf, 4);


    tps65987_i2c_read(I2C_ADDR, 0x06, customeruse1, sizeof(customeruse1));
    sprintf(customeruse,"%s%02x%s",OTA_FILE_NAME,customeruse1[0],OTA_FILE_NAME1);
    printf("ota-file is %s\n",argv[3]);
    printf("local-file is %s\n",customeruse);

    printf("ota-file size is %ld\n",strlen(argv[3]));
    printf("local-file size is %ld\n",strlen(customeruse));

    printf("result is %d\n", strcmp(argv[3],customeruse));

    printf("customer use is %s", customeruse);
    printf("aragv3 is %s, len is %d", argv[3], strlen(argv[3]));

	if(strcmp(argv[3],customeruse) <= 0)
    {
	    printf("version is old,version is %s\n",argv[3]);

	    if(strcmp(argv[argc - 1], "-f") != 0)
	    {

		   return -1;

	    }else{
		    printf("Froced update  of tps65987 firmware.\n");
	    }
    }else{
	 //   strcpy(customeruse,argv[3]);
	    printf("Have new version,version is %s\n",argv[3]);
	}




    //test read and write
    val[0] = 0x04;
    tps65987_i2c_write(I2C_ADDR, 0x70, &val[0], 1);
    usleep(10000);
    tps65987_i2c_read(I2C_ADDR, 0x70, buf, 1);

    //tps65987_host_patch_bundle();

    tps65987_ext_flash_upgrade(argv[3]);

    tps65987_i2c_read(I2C_ADDR, REG_Version, buf, 4);
    tps65987_i2c_read(I2C_ADDR, REG_BootFlags, buf, 12);

    //buf[0] = 0x01;
    //tps65987_exec_4CC_Cmd("FLrr", buf, 1, buf_2, 4);

    //buf[0] = 0x00;
    //tps65987_exec_4CC_Cmd("FLrr", buf, 1, buf_2, 4);

    //ResetPDController();

    tps65987_get_Status(&tps_status);

    /*while(1)
    {
        tps_port_role = tps65987_get_PortRole();

        //tps65987_get_Status(&tps_status);

        tps65987_get_TypeC_Current();

        sleep(8);
    }*/
    freopen("/dev/tty","w",stdout);
    printf("end tps65987-ota\n");
    close(fd);

    return 0;
}



