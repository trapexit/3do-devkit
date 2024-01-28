#pragma include_only_once
/*
** JoyPad Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1993, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

int32 InitJoypad (void);
int32 ReadJoypad (uint32 *Buttons);
int32 TermJoypad (void);
int32 WaitJoypad (uint32 Mask, uint32 *Buttons);
