######################################################################
#	MakeFile for Borland C/C++ Compiler
#

CC  = bcc32
LN  = bcc32
RL  = brc32
RC  = brcc32

CFLAG = -c -O1 -O2 -Oc -Oi -Ov
LFLAG = -tWD -O1 -O2
RFLAG = 

EXE1 = field_sp.auf
EXE2 = field_wv.auf
OBJ1 = field_separate.obj
OBJ2 = field_weave.obj
RES1 = field_sp.res
RES2 = field_wv.res


all: $(EXE1) $(EXE2)


$(EXE1): $(OBJ1) $(RES1)
	$(LN) $(LFLAG) -e$(EXE1) $(OBJ1)
	$(RL) -fe$(EXE1) $(RES1)

$(EXE2): $(OBJ2) $(RES2)
	$(LN) $(LFLAG) -e$(EXE2) $(OBJ2)
	$(RL) -fe$(EXE2) $(RES2)

field_separate.obj: field_separate.cpp filter.h
	$(CC) $(CFLAG) field_separate.cpp

field_weave.obj: field_weave.cpp filter.h
	$(CC) $(CFLAG) field_weave.cpp

$(RES1): field_sp.rc
	$(RC) $(RFLAG) field_sp.rc

$(RES2): field_wv.rc
	$(RC) $(RFLAG) field_wv.rc
