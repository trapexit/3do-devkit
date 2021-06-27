;---------------------------------------------------
;
;  Stack type macros.
;
;---------------------------------------------------

	MACRO	
$label		SWPREG	$reg1,$reg2
$label		eor	$reg1,$reg1,$reg2
		eor	$reg2,$reg1,$reg2
		eor	$reg1,$reg1,$reg2
	MEND

	MACRO
$label		ldfar	$reg,$addr
$label		ADR	$reg,$addr
		ldr	$reg,[$reg]
	MEND

	MACRO
$label		ENTER	$expr
$label
	IF	DEBUG=1
			mov	ip,sp
		IF	"$expr"=""
			stmdb	sp!,{fp,ip,lr,pc}
		ELSE
			stmdb	sp!,{$expr,fp,ip,lr,pc}
		ENDIF
			sub	fp,ip,#4
	ELSE
		IF	"$expr"=""
			stmfd	sp!,{lr}
		ELSE
			stmfd	sp!,{$expr,lr}
		ENDIF
	ENDIF
	MEND

	MACRO
$label		EXIT	$expr
$label
	IF	DEBUG=1
		IF	"$expr"=""
			ldmdb	fp,{fp,sp,pc}
		ELSE
			ldmdb	fp,{$expr,fp,sp,pc}
		ENDIF
	ELSE
		IF	"$expr"=""
			ldmfd	sp!,{pc}
		ELSE
			ldmfd	sp!,{$expr,pc}
		ENDIF
	ENDIF
	MEND

	MACRO
$label		PUSH	$expr
		stmfd	sp!,{$expr}
	MEND

	MACRO
$label		POP	$expr
		ldmfd	sp!,{$expr}
	MEND

	END
