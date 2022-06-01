include (LibAddMacros)

add_plugin (
	simplelogger
	SOURCES simplelogger.h simplelogger.c
	ADD_TEST TEST_README COMPONENT libelektra${SO_VERSION})
