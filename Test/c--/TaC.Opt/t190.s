.L1	WS	10
.a	DW	.L1
.main	PUSH	FP
	LD	FP,SP
	PUSH	G3
	PUSH	G4
	PUSH	G5
	CALL	__stkChk
	LD	G0,G3
	ADD	G0,#1
	LD	G3,G0
	LD	G0,G4
	ADD	G0,#65535
	LD	G4,G0
	LD	G0,.a
	LD	G3,0,G0
	LD	G0,.a
	ADDS	G0,#1
	LD	G4,0,G0
	LD	G3,0,G5
	LD	G0,G5
	ADDS	G0,#1
	LD	G4,0,G0
	POP	G5
	POP	G4
	POP	G3
	POP	FP
	RET