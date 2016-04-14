/*Warning: this file is for debug purpose only*/
#ifndef  _SES_ERR_MSG_H_
#define  _SES_ERR_MSG_H_
	#ifdef _DEBUG
		const char* SesErrMsgs[] =
		{		
			"SES_SUCCESS==0x0000	/* ses function success */",
			"SES_ERROR_EEPROM==0x9001	/* write eeprom failed */",
			"SES_ERROR_UNSUPPORT==0x9002	/* function not support*/",
			"SES_ERROR_RTC==0x9003	/* read clock module error */",
			"SES_ERROR_RTC_POWER==0x9004	/* the clock module has been power down */",		
			"SES_ERROR_MEMORY==0x9201	/* memory error */",
			"SES_ERROR_PARAM==0x9204	/* parameter error */",
			
					
			"SES_ERROR_ADDRESS==0x9505	/* the address to be written is invalid */",
			"SES_ERROR_WRITE==0x9508	/* the address can't be write */",
			"SES_ERROR_LENGTH==0x9905	/* the data length error */"
		};
	#else
		const char* SesErrMsgs[0xa] = {"\0",};
	#endif

#endif //_SES_ERR_MSG_H_
