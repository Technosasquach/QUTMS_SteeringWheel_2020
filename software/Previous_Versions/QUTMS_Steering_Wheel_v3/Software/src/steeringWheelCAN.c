/*****************************************************************************
* @file    Software/src/steeringWheelCAN.c 
* @author  Zoe Goodward
* @version V1.0.0
* @date    28-April-2018
* @brief   
*****************************************************************************/

//External libraries
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <math.h>

#include "steeringWheelCAN.h"

/*============================================================================
Function:   can_send_test()
------------------------------------------------------------------------------
Purpose :   
Input   :   
Returns :   
Notes   :
============================================================================*/
int8_t can_send_test()
{
	uint8_t tData [2] = {111,111};
	uint8_t mob = CAN_findFreeTXMOB();
	if(mob>=0)can_tx_mob(mob, 2, tData, 0, 2); //transmit registration and do not wait for finish
	return mob;
}

/*============================================================================
Function:   can_init()
------------------------------------------------------------------------------
Purpose :   initialises CAN controller
Input   :   
Returns :   
Notes   :
============================================================================*/
uint8_t can_init()
{	
	CANGCON = (1 << SWRES);     // Reset the CAN controller hardware and general registers.
	for(uint8_t mobCount = 0; mobCount <= 5; mobCount++)
	{
		CANPAGE = (mobCount << 4);		//select the message object to modify
		CANCDMOB = 0;					//make sure we do not do any actions on this mob.
		CANSTMOB = 0;					//make sure no interrupts are set.
		CANIDM1 = 0;
		CANIDM2 = 0;
		CANIDM3 = 0;
		CANIDM4 = 0;					//these registers are used to control the mask which filters incoming messages
	}
	CANBT1 = 0x0E;		//these registers control speed of communication
	CANBT2 = 0x04;		//currently with these values, it is 250kbps
	CANBT3 = 0x13;		//with 8 TQ per bit.

	CANIE2 = (1 << IEMOB4)|(1 << IEMOB5);		//enable interrupts on MOB 4 and 5 for receiving
	CANGIE = (1 << ENRX)|(1 << ENIT);		//enable receive interrupt; enable global CAN interrupt (all interrupts)
	//put other initialisation functions here.

	CANGCON = (1 << ENASTB);    // Enable the CAN. 
	_delay_ms(50);
	if(!(CANGSTA & (1<<ENFG)))return 1;
	return 0;
}

/*============================================================================
Function:   can_rx_init()
------------------------------------------------------------------------------
Purpose :   
Input   :   
Returns :   
Notes   :
============================================================================*/
void can_rx_init(int8_t mob, uint8_t numBytes, uint32_t IDmsk, uint32_t ID)
{
	CANPAGE = ( mob << 4);		//use the mobth mob for receiving.
	//IDEMSK is sent with the CAN packet, we choose to not require that it be set, and instead focus on ID match
	CANIDM4 = (IDmsk<<03) & 0xF8;	//shifts the value sets RTRMSK to zero and IDEMSK to 0
	CANIDM3 = (IDmsk>>05) & 0xFF;
	CANIDM2 = (IDmsk>>13) & 0xFF;	
	CANIDM1 = (IDmsk>>21) & 0xFF;
	
	CANIDT4 = (ID<<03) & 0xF8;	//shifts the value sets RTRTAG, RB1TAG and RB0TAG to 0
	CANIDT3 = (ID>>05) & 0xFF;
	CANIDT2 = (ID>>13) & 0xFF;
	CANIDT1 = (ID>>21) & 0xFF;
	
	CANCDMOB = (numBytes << DLC0)|(2<<CONMOB0)|(1 << IDE);		//we are expecting only numBytes bytes; also set the mob to receive mode.
}

/*============================================================================
Function:   can_tx_mob()
------------------------------------------------------------------------------
Purpose :   
Input   :   
Returns :   
Notes   :
============================================================================*/
void can_tx_mob(int8_t mob, uint8_t numBytes, uint8_t * data, uint32_t ID, uint8_t ms_loop_until_TXOK)
{
	CANPAGE = ( mob << 4);		//use the mobth mob
	//IDEMSK is sent with the CAN packet, we choose to not set it, and instead the receiver will focus on ID match
	CANSTMOB &= ~(1<<TXOK);
	CANIDM4 = 0;
	
	CANIDT4 = (ID<<03) & 0xF8;	//shifts the value sets RTRTAG, RB1TAG and RB0TAG
	CANIDT3 = (ID>>05) & 0xFF;
	CANIDT2 = (ID>>13) & 0xFF;
	CANIDT1 = (ID>>21) & 0xFF;
	for(uint8_t i = 0; i < numBytes; i++)
	{
		CANMSG = data[i];
	}
	CANCDMOB = (numBytes << DLC0)|(1<<CONMOB0)|(1 << IDE);		//we are expecting only numBytes bytes; also set the mob to receive mode.

	for(uint8_t i = 0; i < ms_loop_until_TXOK; i++)	//loop until specified wait time is up
	{
		if((CANSTMOB & (1 << TXOK)))break;		//check for transmission complete
		_delay_ms(1);								//do nothing for 1 ms
	}
}

/*============================================================================
Function:   CAN_findFreeTXMOB()
------------------------------------------------------------------------------
Purpose :   
Input   :   
Returns :   
Notes   :
============================================================================*/
int8_t CAN_findFreeTXMOB()
{
	for(uint8_t i = 0; i < 4; i++) //tx mobs are 0 to 3 (<4)
	{
		CANPAGE = ( i << 4);		//use the mobth mob
		if((CANSTMOB & (1<<TXOK)) || !(CANCDMOB & (1<<CONMOB0))) //if the transmission is complete or the mob has not been setup yet
		{
			return i;			//send back the free mob
		}
	}
	return -1;		//otherwise, none are free
}