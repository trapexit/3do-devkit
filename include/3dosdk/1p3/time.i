 IF :DEF:|_TIME_I|
 ELSE
	GBLL	|_TIME_I|

;*****

; $Id: time.i,v 1.2 1992/10/29 02:35:42 dale Exp $

; $Log: time.i,v $
;Revision 1.2  1992/10/29  02:35:42  dale
;added Id and Log
;

;	Copyright (C) 1992, New Technologies Group, Inc.
;	All Rights Reserved
;	Confidential and Proprietary
;

	INCLUDE	structs.i

	BEGINSTRUCT	TIMEVAL
		INT32	tv_sec
		INT32	tv_usec
	ENDSTRUCT

 ENDIF	; |_TIME_I|

	END
