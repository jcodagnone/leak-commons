#ifndef Z1D68CAAB09B185B97C296A961F4B6BDC
#define Z1D68CAAB09B185B97C296A961F4B6BDC

void *
sigsegv_set_print( int (* fnc)(const char *format, ...), int _needs_cr);

void
sigsegv_handler_fnc(int signal);

void
sigsegv_handler_bt_full_fnc(int signal);

#endif
