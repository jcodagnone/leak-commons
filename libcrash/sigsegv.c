#ifdef HAVE_CONFIG_H_
  #include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include <unistd.h>

#ifdef HAVE_PTHREAD_H
  #include <pthread.h>
#endif

/*
 * http://www.gnu.org/manual/glibc-2.2.3/html_chapter/libc_33.html
 */
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__) && __GLIBC__ == 2 && __GLIBC_MINOR__ >= 1
  #define HAVE_BACKTRACE
  #include <execinfo.h>
#endif
#include <sys/wait.h>


#include "sigsegv.h"

static int (* print)(const char *format, ...) = NULL;
static int needs_cr = 1;

void *
sigsegv_set_print( int (* fnc)(const char *format, ...), int _needs_cr)
{	void *ret = print;
	
	ret = fnc;
	needs_cr = _needs_cr;

	return ret;
}

/**
 * intenta lanzar el gdb, y mostrar el backtrace
 */
static int
dump_pid_son(pid_t pid, const char *binary, int full_bt, 
             int (* myprint)(const char *format,
...))
{	char tmp[]="/tmp/mrbug-crash-XXXXXX";
	int ret = 0;
	int fd;

	fd = mkstemp(tmp);
	if( fd == -1 )
	{	(*myprint)("opening gdb command (tempory) file `%s'%c", tmp,
		           needs_cr ? '\n' : '\0');
		ret = -1;
	}
	else
	{	char gdb_cmd[]="bt\nquit";
		char gdb_cmd_full[]="bt full\nquit";
		char cmd[128];
		FILE *fp;
		
		if( full_bt )
			write(fd, gdb_cmd_full, strlen(gdb_cmd_full));
		else
			write(fd, gdb_cmd, strlen(gdb_cmd));
		close(fd);
	
		sprintf(cmd, "gdb -nw -n -batch -x \"%s\" %s %d", tmp, binary,
		        pid);
		(*myprint)("trying to dump pid: %d (%s)...%c", pid, binary,
		            needs_cr ? '\n' : '\0');

		fflush(NULL);
		fp = popen(cmd, "r");
		if( fp == NULL )
		{	(*myprint)("err. couldn't exec `%s'%c", cmd,
			           needs_cr ? '\n' : '\0');
			ret = -1;
		}
		else
		{	char buff[4096];
			size_t len;

			while(fgets(buff, sizeof(buff), fp))
			{	len = strlen(buff);
				if( buff[len-1] == '\n')
					buff[len-1]=0;
					
				(*myprint)("%s%c", buff,needs_cr ? '\n' : '\0');
			}
			fclose(fp);
		}
		if( remove(tmp) == -1 )
			(*myprint)("removing `%s` (@;@)%c", tmp,
			           needs_cr ? '\n' : '\0');
	}

	return ret;
}

static int
dump_pid(pid_t pid, const char *binary, int full_bt )
{	pid_t mpid;
	int (* myprint)(const char *format, ...);

	myprint = print ? print : printf;

	/*
	 * clone the process, so we don't make the bt bigger.
	 */
	mpid = fork();
	if( mpid == 0 )
	{	dump_pid_son(pid, binary, full_bt,  myprint);
		exit(0);
	}
	else if( mpid == -1 )
		(*myprint)("lunching son: `%s' %c", strerror(errno),
		           needs_cr ? '\n' : '\0');
	else
	{	/* father */
		int status;

		alarm(0);
		waitpid(0, &status, 0);
		if( WIFEXITED(status) && WEXITSTATUS(status)==0 )
			;
	}
	
	return 0;
}

/** 
 * obtiene el path del programa que creo el proceso con pid `pid'
 *
 * \param buff     buffer donde se escribe el path
 * \param nbuff    tamaño maximo del buffer
 * \param pid      pid del proceso a buscar
 *
 * \note Esta funcion solo funciona en Linux
 *
 * \return el parametro buff
 */
static char *
get_path_from_pid(char *buff, size_t nbuff, pid_t pid)
{	char proc[256];
	char *ret = NULL;
	int n;
	
	sprintf(proc, "/proc/%d/exe", pid);
	if( (n=readlink(proc, buff, nbuff)) == -1 )
		ret = NULL;
	else
	{	buff[n]=0;
		ret = buff;
	}

	return ret;
}


static void
sigsegv_libc_dump( int (* myprint)(const char *format, ...) )
{ 	void *array[48] = {0};
	unsigned short i;
	int n;
	char **res;

#ifdef HAVE_BACKTRACE
	(*myprint)("Backtrace:%c", needs_cr ? '\n' : '\0');
 	n  = backtrace(array, sizeof(array)/(sizeof(*array)));
	res =  backtrace_symbols(array, n);
	for (i = 0; i < n; i++)
		(*myprint)("%s%c", res[i], needs_cr ? '\n' : '\0');

	(*myprint)("Attempting to generate core file%c",
	           needs_cr ? '\n' : '\0');
	#endif

}

static void 
sigsegv_handler_generic(int signal, int full_bt)
{	char binary[2048];
	int pid = getpid();
	int (* myprint)(const char *format, ...);

	myprint = print ? print : printf;
	if( get_path_from_pid(binary, sizeof(binary), pid) == NULL)
               	(*myprint)("pid %d does not seems to exist", pid);
        else 
	{ 	(*myprint)("Segmentation Violation Detected.%c", 
		           needs_cr ? '\n':'\0');
		dump_pid(pid, binary, full_bt);
		sigsegv_libc_dump(myprint);
		
	}
	
	#ifdef HAVE_PTHREAD_H
		pthread_kill_other_threads_np();
	#endif
	fflush(NULL);
	abort();
}

void
sigsegv_handler_fnc(int signal)
{
	sigsegv_handler_generic(signal, 0);
}

void
sigsegv_handler_bt_full_fnc(int signal)
{
	sigsegv_handler_generic(signal, 1);
}

