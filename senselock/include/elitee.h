/**************************************************************************************
* Copyright (c) 2011 Beijing Senselock Software Technology Co.,Ltd.
* All rights reserved.
*
* filename: Elitee.h
*
* brief: Library interface declaration, return value and some constant definition.
* 
* history:
*   2011,03,04 created 2.3
***************************************************************************************/



#ifndef __ELITEE_H__
#define __ELITEE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int                 BOOL;
typedef void                *HANDLE;
  
#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else
#define NULL    ((void *)0)
#endif
#endif
  
#if defined(_WIN64)
    typedef __int64 LONG_PTR, *PLONG_PTR;
#else
	typedef long LONG_PTR, *PLONG_PTR;
#endif

typedef char                CHAR8_T;
typedef unsigned char       UCHAR8_T;
typedef unsigned short      USHORT16_T;
typedef unsigned long       ULONG32_T;
typedef unsigned int        UINT32_T;
typedef int                 INT32_T;
typedef long                LONG32_T;


#ifndef ELEAPI
#if defined WIN32 || defined _WIN32
#define ELEAPI __stdcall
#else
#define ELEAPI
#endif
#endif


#ifndef FALSE
#define FALSE               0
#endif
  
#ifndef TRUE
#define TRUE                1
#endif
  
#ifndef IN
#define IN
#endif
  
#ifndef OUT
#define OUT
#endif

  
// open mode
#define ELE_EXCLUSIVE_MODE                    0x00000000// open dongle with exclusive mode
#define ELE_SHARE_MODE                        0x00000001// open dongle with sharing mode

// communication mode
#define ELE_COMM_USB_MODE                     0x00000000// USB mode
#define ELE_COMM_HID_MODE                     0x000000AA// HID mode

// dongle version
#define ELE_V0201                             0x00000201
#define ELE_V0220                             0x00000220

// dongle type
#define ELE_LOCAL_DEVICE                      0x00000000// desktop dongle
#define ELE_NET_DEVICE                        0x00000001// network dongle
#define ELE_NORMAL_DEVICE                     0x00000000// non-RTC dongle(dongle without clock function)
#define ELE_RTL_DEVICE                        0x00000002// RTC dongle(dongle with clock function)
#define ELE_USER_DEVICE                       0x00000000// user dongle
#define ELE_MASTER_DEVICE                     0x00000080// control dongle

// device information
#define ELE_GET_DEVICE_SERIAL                 0x00000001// device serial number, 8bytes
#define ELE_GET_VENDOR_DESC                   0x00000002// manufacturer descritpion, 8bytes
#define ELE_GET_CURRENT_TIME                  0x00000004// current time from dongle, only for RTC dongle
#define ELE_GET_DEVICE_VERSION                0x00000007// device verion
#define ELE_GET_DEVICE_TYPE                   0x00000008// device type
#define ELE_GET_MODIFY_TIME                   0x0000000A// production date of the dongle
#define ELE_GET_COMM_MODE                     0x0000000B// communication mode, USB or HID
#define ELE_GET_DEVELOPER_NUMBER              0x00000012// developer number
#define ELE_GET_MODULE_COUNT                  0x00000013// module amount
#define ELE_GET_MODULE_SIZE                   0x00000014// module size


// control code
#define ELE_RESET_DEVICE                      0x00000016// reset device
#define ELE_SET_LED_UP                        0x00000017// LED up
#define ELE_SET_LED_DOWN                      0x00000018// LED down
#define ELE_SET_LED_FLASH                     0x00000019// LED blinking
#define ELE_SET_COMM_MODE                     0x0000001A// set communication mode
#define ELE_SET_VENDOR_DESC                   0x0000001B// set manufacturer description


// error code
#define ELE_SUCCESS                           0x00000000// successful
#define ELE_INVALID_PARAMETER                 0x00000001// invalid parameter
#define ELE_INSUFFICIENT_BUFFER               0x00000002// insufficient buffer
#define ELE_NOT_ENOUGH_MEMORY                 0x00000003// insufficient memory
#define ELE_INVALID_DEVICE_HANDLE             0x00000004// invalid device handle
#define ELE_COMM_ERROR                        0x00000005// communication error

#define ELE_INVALID_SHARE_MODE                0x00000006// open device with invalid share mode
#define ELE_UNSUPPORTED_OS                    0x00000007// no supported operating system
#define ELE_ENUMERATE_DEVICE_FAILED           0x00000008// failed to enumerate device
#define ELE_NO_MORE_DEVICE                    0x00000009// no matched device

#define ELE_VERSION_NOT_MACTH                 0x00000022// hardware version does not match
#define ELE_UNSUPPORTED_DEVICE_VERSION        0x00000023// unsupported hardware version
#define ELE_NOT_RTL_DEVICE                    0x00000024// not RTC device
#define ELE_NOT_MASTER_DEVICE                 0x00000025// not control dongle
#define ELE_MODULE_NOT_FOUND                  0x00000027// the module doesn't exist
#define ELE_MODULE_SIZE_BEYOND                0x00000028// out of module range
#define ELE_INVALID_PACKAGE                   0x0000002C// invalid package
#define ELE_INVALID_STRUCTURE_SIZE            0x00000066// invalid size field
#define ELE_CLOSE_DEVICE_FAILED               0x00000067// faield to close device
#define ELE_VERIFY_SIGNATURE_ERROR            0x00000068// failed to verify signature
#define ELE_INVALID_CODE_SIZE				          0x00000069// the size of code section isn't 3K
#define ELE_INVALID_CODE_SIZE_6k				      0x00000070// the size of code section isn't 6K

//Hardware's error

#define ELE_WRITE_FLASH_ERROR                 0x00008001// error occurs when writing eeprom
#define ELE_UNSUPPORTED_FUNCTION              0x00008002// function not support                   
#define ELE_RTC_ERROR                         0x00008003// read clock module error                
#define ELE_RTC_POWER_OFF                     0x00008004// the clock module is power off   
#define ELE_WRONG_COMMAND_INS                 0x00008101// invalid INS
#define ELE_WRONG_COMMAND_CLA                 0x00008102// invalid CLA
#define ELE_WRONG_COMMAND_LC                  0x00008301// invalid LC
#define ELE_WRONG_COMMAND_DATA                0x00008302// invalid DATA
#define ELE_WRONG_COMMAND_LE                  0x00008303// invalid LE
#define ELE_WRONG_COMMAND_PP                  0x00008306// invalid PP
#define ELE_INVALID_OFFSET                    0x00008307// invalid offset
#define ELE_INVALID_MODULE                    0x00008501// invalid module
#define ELE_MACHINE_CODE_MISMATCH             0x00008502// machine code mismatch
#define ELE_INVALID_HEAD_SIZE                 0x00008503// invalid optional head size
#define ELE_INVALID_SECTION_NUMBER            0x00008504// invalid section number
#define ELE_SECTION_NOT_FOUND                 0x00008505// section doesn't exist
#define ELE_INVALID_SECTION_OFFSET            0x00008506// invalid section offset
#define ELE_INVALID_SECTION_SIZE              0x00008507// invalid section size
#define ELE_SECTION_TYPE_MISMATCH             0x00008508// section type mismach
#define ELE_ANONYMOUS_USER                    0x00008800// anonymous user
#define ELE_DEVICE_STATE_MISMATCH             0x00008801// device state mismatch
#define ELE_SECURITY_STATE_MISMATCH           0x00008802// security state mismatch
#define ELE_DEVICE_PIN_BLOCK                  0x00008803// PIN is locked
#define ELE_SECURITY_MESSAGE_ERROR            0x00008808// invalid security message in package
#define ELE_DEVICE_PIN_ERROR                  0x000088c0// 88cx is PIN verification error, x times left to retry
#define ELE_OBJECT_NOT_FOUND                  0x00008901// object doesn't exist
#define ELE_NO_CURRENT_MODULE                 0x00008902// no current module
#define ELE_OBJECT_NAME_ALREADY_EXIST         0x00008903// object name is existent
#define ELE_BEYOND_OBJECT_SIZE                0x00008905// exceed object's size
#define ELE_INVALID_OBJECT_HANDLE             0x00008906// invalid object handle
#define ELE_DEVICE_NOT_IN_WHITE_LIST          0x00008a01// the updating device isn't existent in whitelist
#define ELE_BLOCK_CIPHER_ERROR                0x00008a02// updating block's key is incorrect
#define ELE_BLOCK_SIGNATURE_ERROR             0x00008a03// updating block's signature is incorrect
#define ELE_BLOCK_MISMATCH                    0x00008a04// updating block mismatch
#define ELE_ERROR_UNKNOWN                     0x00008fff// unknown error

#define ELE_ELC_ERROR_SES_EEPROM              0x00009001// write eeprom failed                    
#define ELE_ELC_ERROR_SES_UNSUPPORT           0x00009002// function not support                   
#define ELE_ELC_ERROR_SES_RTC                 0x00009003// read clock module error                
#define ELE_ELC_ERROR_SES_RTC_POWER           0x00009004// the clock module has been power down   
#define ELE_ELC_ERROR_SES_MEMORY              0x00009201// memory error                                                                      
#define ELE_ELC_ERROR_SES_PARAM               0x00009204// parameter error                        
#define ELE_ELC_ERROR_SES_OBJECT              0x00009901// object error
                                                        
                                                                                                     
#define ELE_VM_W_CODE_RANGE   	              0x0000A001// exceed ROM address space   
#define ELE_VM_W_INST_RSV     	              0x0000A002// invalid instruction         
#define ELE_VM_W_IDATA_RANGE  	              0x0000A003// exceed RAM address space   
#define ELE_VM_W_BIT_RANGE    	              0x0000A004// exceed BIT address space   
#define ELE_VM_W_SFR_RANGE    	              0x0000A005// exceed SFR address space   
#define ELE_VM_W_XRAM_RANGE   	              0x0000A006// exceed XRAM address sapce  

//Hardware Version
#define ELE_HARDWARE_V0200                    0x00000200// Hardware Version 2.0
#define ELE_HARDWARE_V0201                    0x00000201// Hardware Version 2.1
#define ELE_HARDWARE_V0213                    0x00000213// Hardware Version 2.1.3
#define ELE_HARDWARE_V0220                    0x00000220// Hardware Version 2.2

#pragma pack(push, 1)

typedef  struct _ELE_DEVICE_CONTEXT
{
  ULONG32_T   ulSize;                   
  ULONG32_T   ulFinger;                 
  ULONG32_T   ulMask;                   
  UCHAR8_T    ucDevNumber[8];           
  UCHAR8_T    ucDesp[8];                
  UCHAR8_T    ucSerialNumber[8];        
  ULONG32_T   ulShareMode;              
  ULONG32_T   ulIndex;                  
  ULONG32_T   ulDriverMode;             
  HANDLE	    hDevice;                  
  HANDLE	    hMutex;                   
} ELE_DEVICE_CONTEXT,*PELE_DEVICE_CONTEXT;

typedef struct _ELE_LIBVERSIONINFO {
  ULONG32_T ulVersionInfoSize;          
  ULONG32_T  ulMajorVersion;            
  ULONG32_T  ulMinorVersion;            
  ULONG32_T  ulClientID;                
  CHAR8_T    acDesp[128];               
} ELE_LIBVERSIONINFO;

#pragma pack(pop)

BOOL  ELEAPI  EleSign (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,
  IN    UCHAR8_T                *pucSerial,
  IN    CHAR8_T                 *pcModuleName,
  IN    UCHAR8_T                *pucModuleContent,
  IN    ULONG32_T               ulModuleSize,
  OUT   UCHAR8_T                *pPkgBuffer,
  IN    ULONG32_T               ulPkgBufferLen,
  OUT   ULONG32_T               *pulActualPkgLen,
  IN    ULONG32_T               ulHardwareVersion
  );

BOOL  ELEAPI  EleUpdate (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,
  IN    UCHAR8_T                *pucPkgContent,
  IN    ULONG32_T               ulPkgLen
  );

BOOL  ELEAPI  EleVerifyPin (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,
  IN    UCHAR8_T                *pucPin
  );

BOOL  ELEAPI  EleChangePin (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,                              
  IN    UCHAR8_T                *pucOldPin,
  IN	  UCHAR8_T                *pucNewPin
  );

BOOL  ELEAPI  EleReadModule (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,                              
  IN    CHAR8_T                 *pcModuleName,
  OUT   UCHAR8_T                *pucModuleBuffer,
  IN    ULONG32_T               ulModuleBufferLen,
  OUT   ULONG32_T               *pulActualModuleLen
  );

BOOL  ELEAPI  EleWriteModule (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,                              
  IN    CHAR8_T                 *pcModuleName,
  IN    UCHAR8_T                *pucModuleContent,
  IN    ULONG32_T               ulModuleContentLen,
  OUT   ULONG32_T               *pulActualWrittenLen
  );

BOOL  ELEAPI  EleChangeModuleName (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,                              
  IN    CHAR8_T                 *pcOldModuleName,
  IN    CHAR8_T                 *pcNewModuleName
  );
                        

BOOL  ELEAPI  EleExecute (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,                              
  IN    CHAR8_T                 *pcModuleName,
  IN    UCHAR8_T                *pucInput,
  IN    ULONG32_T               ulInputLen,
  OUT   UCHAR8_T                *pucOutput,
  IN    ULONG32_T               ulOutputLen,
  OUT   ULONG32_T               *pulActualOutputLen
  );

BOOL  ELEAPI  EleGetFirstModuleName(
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,                              
  OUT   CHAR8_T                 *pcModuleNameBuffer,
  IN    ULONG32_T               ulModuleNameBufferLen,
  OUT   ULONG32_T               *pulModuleNameLen,
  OUT   ULONG32_T               *pulIndex
  );

BOOL  ELEAPI  EleGetNextModuleName(
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,                              
  OUT   CHAR8_T                 *pcModuleNameBuffer,
  IN    ULONG32_T               ulModuleNameBufferLen,
  OUT   ULONG32_T               *pulModuleNameLen,
  IN OUT ULONG32_T              *pulIndex
  );

BOOL  ELEAPI EleOpenFirstDevice(
  IN    UCHAR8_T                *pucDevNumber,
  IN    UCHAR8_T                *pucDesp,
  IN    UCHAR8_T                *pucSerialNumber,
  IN    ULONG32_T               ulShareMode,
  OUT   ELE_DEVICE_CONTEXT      *pDeviceContext
  );

BOOL ELEAPI  EleOpenNextDevice(
  IN OUT ELE_DEVICE_CONTEXT     *pDeviceContext
  );

BOOL  ELEAPI  EleClose (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext
  );

BOOL  ELEAPI  EleControl (
  IN    ELE_DEVICE_CONTEXT      *pDeviceContext,
  IN    ULONG32_T               ulCtrlCode,
  IN    UCHAR8_T                *pucInput,
  IN    ULONG32_T               ulInputLen,
  OUT   UCHAR8_T                *pucOutput,
  IN    ULONG32_T               ulOutputLen,
  OUT   ULONG32_T               *pulActualLen
  );

ULONG32_T  ELEAPI  EleGetLastError(void);

BOOL   ELEAPI   EleGetVersion(
  OUT   ELE_LIBVERSIONINFO      *pEleLibVersionInfo
  );


#ifdef __cplusplus
}
#endif
#endif





















