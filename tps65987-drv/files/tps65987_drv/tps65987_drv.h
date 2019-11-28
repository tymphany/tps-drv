/**
*  @file      tps65987-drv.c
*  @brief     tps65987
*  @author    Link Lin
*  @date      11 -2019
*  @copyright
*/

#include<stdio.h>
#include<stdlib.h>

#define  REG_MODE               0x03
#define  REG_Version            0x0F
#define  REG_Status             0x1A
#define  REG_PORTCONFIG         0x28
#define  REG_BootFlags          0x2D

typedef struct
{
    unsigned int  PatchHeaderErr        :1;
    unsigned int  Reserved0             :1;
    unsigned int  DeadBatteryFlag       :1;
    unsigned int  SpiFlashPresent       :1;
    unsigned int  Region0               :1;
    unsigned int  Region1               :1;
    unsigned int  Region0Invalid        :1;
    unsigned int  Region1Invalid        :1;

    unsigned int  Region0FlashErr       :1;
    unsigned int  Region1FlashErr       :1;
    unsigned int  PatchDownloadErr      :1;
    unsigned int  Reserved1             :1;
    unsigned int  Region0CrcFail        :1;
    unsigned int  Region1CrcFail        :1;
    unsigned int  CustomerOTPInvalid    :1;
    unsigned int  Reserved2             :1;

    unsigned int  Reserved3             :1;
    unsigned int  PP1Switch             :1;
    unsigned int  PP2Switch             :1;
    unsigned int  PP3Switch             :1;
    unsigned int  PP4Switch             :1;
    unsigned int  Reserved4             :11;
} s_TPS_bootflag;


typedef struct
{
    unsigned int  TypeCStateMachine         :2;
    unsigned int  Reserved0                 :1;
    unsigned int  ReceptacleType            :3;
    unsigned int  AudioAccessorySupport     :1;
    unsigned int  DebugAccessorySupport     :1;

    unsigned int  SupportTypeCOptions       :2;
    unsigned int  Reserved1                 :1;
    unsigned int  VCONNsupported            :2;
    unsigned int  USB3rate                  :2;
    unsigned int  Reserved2                 :1;

} s_TPS_portconfig;


typedef struct
{
    unsigned int  PlugPresent               :1;
    unsigned int  ConnState                 :3;
    unsigned int  PlugOrientation           :1;
    unsigned int  PortRole                  :1;
    unsigned int  DataRole                  :1;

    unsigned int  Reserved0                 :13;
    unsigned int  VbusStatus                :2;
    unsigned int  UsbHostPresent            :2;
    unsigned int  ActingAsLegacy            :2;
    unsigned int  Reserved1                 :1;
    unsigned int  BIST                      :1;
    unsigned int  HighVoltageWarning        :1;
    unsigned int  LowVoltageWarning         :1;
    unsigned int  Ack_Timeout               :1;
    unsigned int  Reserved2                 :1;

    //Bytes 5-6:
    unsigned int  AMStatus                  :2;
    unsigned int  Reserved3                 :14;

    //Bytes 7-8:
    unsigned int  Reserved4                 :16;

} s_TPS_status;


typedef  struct
{
    unsigned int  regionnum         :1;
    unsigned int  Reserved          :7;
} s_TPS_flrr;

typedef  struct
{
    unsigned int   flashaddr;
    unsigned char  numof4ksector;
} s_TPS_flem;

typedef  struct
{
    unsigned int   flashaddr;
} s_TPS_flad;

typedef  struct
{
    unsigned int   flashaddr;
} s_TPS_flvy;

#define  REGION_0   0
#define  REGION_1   1

#define  DISABLE_PORT   0x03

int tps65987_i2c_write(unsigned char dev_addr, unsigned char reg, unsigned char *val, unsigned char data_len);
int tps65987_i2c_read(unsigned char addr, unsigned char reg, unsigned char *val, unsigned char data_len);
int tps65987_exec_4CC_Cmd(unsigned char *cmd_ptr, unsigned char *cmd_data_in_ptr, unsigned char cmd_data_in_length, unsigned char *cmd_data_out_ptr, unsigned char cmd_data_out_length);
int ResetPDController();
int tps65987_ext_flash_upgrade(void);

