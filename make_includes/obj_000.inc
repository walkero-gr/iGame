##########################################################################
# object files (generic 000)
##########################################################################

src/funcs.o: src/funcs.c src/funcs.h src/iGame_strings.h \
	src/iGameExtern.h src/strfuncs.h src/fsfuncs.h src/funcs.h \
	src/iGameGUI.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ src/funcs.c

src/iGameGUI.o: src/iGameGUI.c src/iGameGUI.h src/iGame_strings.h \
	src/version.h src/iGameExtern.h src/fsfuncs.h src/funcs.h \
	src/strfuncs.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ src/iGameGUI.c

src/iGameMain.o: src/iGameMain.c src/sqldb.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ src/iGameMain.c

src/strfuncs.o: src/strfuncs.c src/strfuncs.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ src/strfuncs.c

src/fsfuncs.o: src/fsfuncs.c src/fsfuncs.h src/iGame_strings.h \
	src/iGameExtern.h src/strfuncs.h src/funcs.h src/sqldb.h \
	src/fsfuncs.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ src/fsfuncs.c

src/sqldb.o: src/sqldb.c src/sqldb.h
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ src/sqldb.c
