#	ChunkMatcher.make
#
#	Copyright (c) 1994 The 3DO Company. All Rights Reserved.
#
#	10/25/94	ec		move tool to 'apps & data:chunkmatcher:' when built.
#	10/11/94	ec		move tool to 'apps & data' when built.
#	9/28/94		crm		Link against ToolLib.o to get SpinCursor function
#


#####################################
#		Symbol definitions
#####################################
Program			=	ChunkMatcher
ObjectDir		=	:Objects:
CC				=	CPlus
ASM				=	
LINK			=	Link
Common			=	:
AppsDir			=	'::Apps & Data':{Program}:
StackSize		=	
StackSizeBytes	=	
DebugOption		=	-d DEBUG=1


#####################################
#	Default compiler options
#####################################
NormCOptions	= {DebugOption} -model far
COptions		= {NormCOptions}

SOptions		= 

NormLOptions	= -c "MPS " -t "MPST" -d -model far
LOptions		= {NormLOptions}


#####################################
#		Object files
#####################################

LIBS		=												--
				"{MPW}Libraries:Libraries:MacRuntime.o"		--
	 			"{MPW}Libraries:Libraries:IntEnv.o"			--
	 			"{MPW}Libraries:CLibraries:CPluslib.o"		--
				"{MPW}Libraries:CLibraries:StdCLib.o"		--
	 			"{MPW}Libraries:Libraries:Interface.o"		--
				"{MPW}Libraries:Libraries:ToolLibs.o"


OBJECTS		=												--
				"{ObjectDir}Options.cp.o"					--
				"{ObjectDir}Main.cp.o"						--
				"{ObjectDir}TypeID.cp.o"					--
				"{ObjectDir}AnimExtractor.cp.o"				--
				"{ObjectDir}Chunk.cp.o"						--
				"{ObjectDir}ChunkFile.cp.o"					--
				"{ObjectDir}ChunkQueue.cp.o"				--
				"{ObjectDir}ChunkSequence.cp.o"				--
				"{ObjectDir}ChunkSequenceHandler.cp.o"		--
				"{ObjectDir}Form3DOChunks.cp.o"				--
				"{ObjectDir}MultiPartFileName.cp.o"			--
				"{ObjectDir}SAAAnimExtractor.cp.o"			--
				"{ObjectDir}SAnimExtractor.cp.o"			--
				"{ObjectDir}SCelExtractor.cp.o"				--
				"{ObjectDir}StreamChunks.cp.o"				--
				"{ObjectDir}StreamAnalyzer.cp.o"


#####################################
#	Default build rules
#####################################
All				A	{Program}

{ObjectDir}		A	:

.cp.o			A	.cp
	{CC} {COptions} -o {TargDir}{Default}.cp.o {DepDir}{Default}.cp

.s.o			A	.s
	{ASM} {SOptions} -o {TargDir}{Default}.s.o {DepDir}{Default}.s


#####################################
#	Target build rules
#####################################

Options.cp				A	 Options.h
TypeID.cp				A	 TypeID.h
AnimExtractor.cp		A	 AnimExtractor.h
Chunk.cp				A	 Chunk.h
ChunkFile.cp			A	 ChunkFile.h
ChunkQueue.cp			A	 ChunkQueue.h
ChunkSequence.cp		A	 ChunkSequence.h
ChunkSequenceHandler.cp	A	 ChunkSequenceHandler.h
Form3DOChunks.cp		A	 Form3DOChunks.h
MultiPartFileName.cp	A	 MultiPartFileName.h
SAAAnimExtractor.cp		A	 SAAAnimExtractor.h
SAnimExtractor.cp		A	 SAnimExtractor.h
SCelExtractor.cp		A	 SCelExtractor.h
StreamChunks.cp			A	 StreamChunks.h
StreamAnalyzer.cp		A	 StreamAnalyzer.h


{Program}		AA	{Program}.make {Program}.r {OBJECTS}
	Rez  -o {Program} {Program}.r -a -ov
	{LINK}	{LOptions}						--
			-o {Program}					--
			{OBJECTS}						--
			{LIBS}
	if not `exists {AppsDir}`
		newFolder {AppsDir}
	end
	move -y {Program} {AppsDir}{Program}
	if `exists "{Program}.sym"`
		move -y {Program}.sym {AppsDir}{Program}.sym
	end

