#include "Portfolio.h"
#include "Event.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

long mapPadDataToOldJoyStick( ControlPadEventData padData )
	{
	long rtnval = 0;
	
	if ( padData.cped_ButtonBits & ControlDown )
		rtnval |= JOYDOWN;
	if ( padData.cped_ButtonBits & ControlUp )
		rtnval |= JOYUP;
	if ( padData.cped_ButtonBits & ControlRight )
		rtnval |= JOYRIGHT;
	if ( padData.cped_ButtonBits & ControlLeft )
		rtnval |= JOYLEFT;
	if ( padData.cped_ButtonBits & ControlA )
		rtnval |= JOYFIREA;
	if ( padData.cped_ButtonBits & ControlB )
		rtnval |= JOYFIREB;
	if ( padData.cped_ButtonBits & ControlC )
		rtnval |= JOYFIREC;
	if ( padData.cped_ButtonBits & ControlStart )
		rtnval |= JOYSTART;
	return( rtnval);
	}

bool EventUtilityInited;

long ReadControlPad(long lControlMask)
	{
	static long oldjoy;
	long newjoy, returnjoy;
	Err err;
	ControlPadEventData padData;

	if ( EventUtilityInited );
	else {
		newjoy = 0;
		oldjoy = 0;
		if ( ( err = InitEventUtility( 1, 0, 1 ) ) < 0 ) {
			PrintfSysErr( err );
			goto Done;
			}
		else {
//			kprintf( "ReadControlPad Inited\n" );
			EventUtilityInited = true;
			}
		}
		
	if ( ( err = GetControlPad( 1, 0, &padData ) ) < 0 ) {
		PrintfSysErr( err );
		newjoy = oldjoy;
		}
	else if ( err == 1 ) {
		 newjoy = mapPadDataToOldJoyStick( padData );
//		 kprintf( "padData=%8.8X newjoy=%8.8X oldjoy %8.8X\n", padData.cped_ButtonBits, newjoy, oldjoy );
		 }
		 else {
//		 	kprintf( "oldjoy & lControlMask=%8.8X\n", oldjoy & lControlMask );
		 	return( oldjoy & lControlMask );
			}

Done:		
	returnjoy = newjoy ^ oldjoy;

	returnjoy &= newjoy;
	returnjoy |= (newjoy & lControlMask);

	oldjoy = newjoy;
	return ( returnjoy );
	}
