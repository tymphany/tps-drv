/**
*  @file      tps65987-drv.c
*  @brief     tps65987
*  @author    Link Lin
*  @date      11 -2019
*  @copyright
*/

#include<stdio.h>
#include<stdlib.h>

#define  REG_MODE                       0x03
#define  REG_Version                    0x0F
#define  REG_Status                     0x1A
#define  REG_PORTCONFIG                 0x28
#define  REG_BootFlags                  0x2D
#define  REG_RX_Source_Capabilities     0x30
#define  REG_Power_Status               0x3F



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

enum
{
    SINK = 0,
    SOURCE = 1,

} TPS_Port_Role;


typedef struct
{
    unsigned int  PowerConnection           :1;
    unsigned int  SourceSink                :1;
    unsigned int  TypeC_Current             :2;
    unsigned int  Charger_Detect_Status     :4;

    unsigned int  Charger_AdvertiseStatus   :2;
    unsigned int  Reserved0                 :6;

} s_TPS_Power_Status;

enum
{
    USB_Default_Current = 0,
    C_1d5A_Current = 1,
    C_3A_Current = 2,
    PD_contract_negotiated = 3,

} TPS_TypeC_Current_Type;


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


int i2c_open_tps65987(unsigned char i2c_addr, char *i2c_file_name);
int ResetPDController();
int tps65987_ext_flash_upgrade(char *ota_file_name);
int tps65987_get_Status(s_TPS_status *p_tps_status);
int tps65987_get_RXSourceNumValidPDOs(void);
int tps65987_get_TypeC_Current(void);

