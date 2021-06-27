#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

void MapP2Cel(CCB* ccb, Point* quad)
	{
	/*
		This version performs the math more efficiently, based on the assumption that both the width and
		the height are powers of two. The most inefficient part of the entire function occurs right up
		front, when we attempt to get the base-2 logarithms of the width and height. A better algorithm
		would speed this up, as would remembering the values for more than one frame.
	*/

	int LogWidth = 0, LogHeight = 0;
	int32 temp;
	
	if (ccb->ccb_Width < ccb->ccb_Height)
		{
		temp = ccb->ccb_Width;
		while (temp >>= 1)
			LogWidth++;
			
		LogHeight = LogWidth;
		temp = ccb->ccb_Height >> LogWidth;
		while (temp >>= 1)
			LogHeight++;
		}
	else
		{
		temp = ccb->ccb_Height;
		while (temp >>= 1)
			LogHeight++;
			
		LogWidth = LogHeight;
		temp = ccb->ccb_Width >> LogHeight;
		while (temp >>= 1)
			LogWidth++;
		}
		
	ccb->ccb_XPos = quad[0].pt_X << 16;
	ccb->ccb_YPos = quad[0].pt_Y << 16;
	
	ccb->ccb_HDX = ((quad[1].pt_X-quad[0].pt_X)<<20) >> LogWidth;
	ccb->ccb_HDY = ((quad[1].pt_Y-quad[0].pt_Y)<<20) >> LogWidth;
	
	ccb->ccb_VDX = ((quad[3].pt_X-quad[0].pt_X)<<16) >> LogHeight;
	ccb->ccb_VDY = ((quad[3].pt_Y-quad[0].pt_Y)<<16) >> LogHeight;

	temp = 20 - (int32) LogWidth - (int32) LogHeight;
	ccb->ccb_HDDX = (quad[2].pt_X-quad[3].pt_X-quad[1].pt_X+quad[0].pt_X) << temp;
	ccb->ccb_HDDY = (quad[2].pt_Y-quad[3].pt_Y-quad[1].pt_Y+quad[0].pt_Y) << temp;
	}
