
lib_LIBRARIES = libcrash.a

libcrash_a_SOURCES = sigsegv.c sigsegv.h
libcrash_a_CFLAGS  = -I../

noinst_PROGRAMS = crash_test crash_test1 crash_test2

crash_test_SOURCES = test1.c
crash_test_CFLAGS  = -I..
crash_test_LDFLAGS = -L.
crash_test_LDADD   = -lcrash -lpthread

crash_test1_SOURCES = test2.c
crash_test1_CFLAGS  = -I..
crash_test1_LDFLAGS = -L.
crash_test1_LDADD   = -lcrash -lpthread

crash_test2_SOURCES = test3.c
crash_test2_CFLAGS  = -I..
crash_test2_LDFLAGS = -L.
crash_test2_LDADD   = -lcrash -lpthread
