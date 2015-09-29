/*
 * --------------------------
 *    vmpmemory:
 *    Author: Colm Quinn  
 *    quinncolm@optonline.net
 * ----------------------------
 *
 *  Core logic for libvmpmemory
 *  This source front ends the C library memory functions.
 *  The key is not to use any library methods that allocate or free
 *  memory.  Trickier than it seems.  vmputils.c contains some very 
 *  simple (and not safe outside this limited scope ) substitues for
 *  the C library methods used here. 
 *
 *  Example usage:
 *
 *   //Start the reporting server
 *    ./vmpserver.pl 7777
 *
 *   //run the application (tmemory in this example)
 *
 *    export VMPMEMORY_REPORTHOST=127.0.0.1:7777
 *    export VMPMEMORY_STDERR=1
 *    export VMPMEMORY_REPORTFILE=$PWD/vmpdata
 *    LD_PRELOAD=$PWD/libvmpmemory.so.0.1 ./tmemory
 *
 */



#define _GNU_SOURCE 
#include <dlfcn.h>

#ifndef RTLD_NEXT 
# define RTLD_NEXT      ((void *) -1l)
#endif


#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <execinfo.h> 

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include <pthread.h>
pthread_mutexattr_t mutattr;
pthread_mutex_t mut;


/* config settings */

#define BTDEPTH 5
#define BSIZE   1024

/* ---------------- */


#include "vmputils.h"

enum {MALLOC, CALLOC, REALLOC, FREE};


void*(*real_malloc)(size_t)         = 0;
void*(*real_calloc)(size_t, size_t) = 0;
void (*real_free)(void *)           = 0;
void*(*real_realloc)(void*, size_t) = 0;



bool initializationdone = false;
bool inInit             = true;


bool  config_includeBacktrace = false;
int   config_abortsize = -1;
char* config_host      = 0;
char* config_filename  = 0;
bool  config_stderr    = false;

/*
 * reporting client
 */
int    socketfd = -1;
struct addrinfo* res = 0;
struct sockaddr_storage remote_sockaddr;
socklen_t               remote_sockaddr_len;

FILE* fh = 0;


static void initreportingclient(char* hostport);
static void formatjson(int method, size_t size, void* ptr, char* bt, char* buffer, size_t* jsonsize);
static void formatjson_realloc(size_t size, void *oldptr, void* ptr, char* bt, char* buffer, size_t* jsonsize);
static void formatjson_free(void* ptr, char* bt, char* buffer, size_t* jsonsize);
static void report(char* buffer, size_t jsonsize);
static void error(const char* buffer);

/*
 * 	Library initialization and deoinitialization
 *
 */
 
void __attribute__ ((constructor)) my_init(void) {
  
  fprintf (stderr,  "VAPPS: vmemory - init  \n");

  // read environment variables

  // abort if size equals
  char *x = getenv("VPMMEMORY_ABORTSIZE");
  if (x != 0) {
     int xx = atoi(x);
     if (xx > 0)
        config_abortsize = xx;
  }

  x = getenv("VMPMEMORY_BACKTRACE");
  if (x != 0) {
     config_includeBacktrace = true;
  }

  x = getenv("VMPMEMORY_STDERR");
  if (x != 0) {
     config_stderr = true;
  }

  x = getenv("VMPMEMORY_REPORTHOST");
  if (x != 0) {
     fprintf(stderr, "report host found %s\n", x);
     size_t l = strlen(x);
     config_host = malloc(l);
     strcpy(config_host, x);
     fprintf(stderr, "report host %s\n", config_host);
     initreportingclient(config_host);
  }

  x = getenv("VMPMEMORY_REPORTFILENAME");
  if (x != 0) {
     size_t l = strlen(x);
     config_filename = malloc(l);
     strcpy(config_filename, x);
  }
}

void __attribute__ ((destructor)) my_fini(void) {
  if (config_host != 0 && real_free != 0) {
     (*real_free)(config_host);
  }
  if (config_filename != 0 && real_free != 0) {
     (*real_free)(config_filename);
  }

}


static void initreportingclient(char* hostport) {
 
   // get remote address 
   struct addrinfo ainfo;
   memset(&ainfo, 0, sizeof(ainfo));
   ainfo.ai_family=AF_UNSPEC;
   ainfo.ai_protocol=SOCK_DGRAM;
   ainfo.ai_flags=AI_ADDRCONFIG;

   size_t h = strlen(hostport);
   size_t hix=0;
   int port = 7999; 
   char* host = hostport;

   fprintf(stderr, "initreportclient %s\n", hostport);

   for (;hix<h; ++hix) {
      if (hostport[hix] == ':') {
          hostport[hix] = 0;
          port = vmstr2int(&hostport[++hix]);
          break;
      }
   }   

   int err=getaddrinfo(host, 0, &ainfo ,&res);
   if (err!=0) {
       fprintf(stderr, "failed to resolve remote socket address (err=%d)", err);
       return;
   }

   // make a copy of the sockaddr structure 
   memcpy(&remote_sockaddr, res->ai_addr, res->ai_addrlen);
   struct sockaddr_in* tsockaddr_in = (struct sockaddr_in*)&remote_sockaddr;
   tsockaddr_in->sin_port = htons(port);
   remote_sockaddr_len =  res->ai_addrlen;

   // get local socket matching remote socket type and ip protocol 
   fprintf(stderr, "res :%d %d %d\n", res->ai_family, res->ai_socktype, res->ai_protocol);
   socketfd = socket(res->ai_family, SOCK_DGRAM, 0);
   if (socketfd < 0) {
      fprintf(stderr, "failed to open socket address (err=%d)", errno);
      return;
   }

   fprintf(stderr, "initreportclient %s\n", hostport);

   // synch mechanism
   pthread_mutexattr_init( &mutattr);
   pthread_mutexattr_settype( &mutattr, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init ( &mut, &mutattr);


}

void initreportingfile(char *filename) {
   fh = fopen(filename, "w");
   if (fh == 0 /*NULL*/) {
       fprintf(stderr, "failed to open reporting file %s (err=%d)", filename, errno);
       return;       
   }
}


/*
 * Frontend functions for memory services
 */

void *malloc(size_t sz) 
{ 
  void* ptr = 0; 
  char  buffer[BSIZE];
  char* bt = 0;
  size_t jsonsize = 0;

  if (real_malloc == 0) {
     //fprintf (stderr,  "VAPPS: malloc init\n");fflush(stderr);
     real_malloc  = (void * (*)(size_t))dlsym(RTLD_NEXT, "malloc");
  }
 
  if (config_includeBacktrace) {

  }

  ptr = (*real_malloc)(sz); 
  formatjson(MALLOC, sz, ptr, bt, buffer, &jsonsize);
  report(buffer, jsonsize);
  return ptr; 
} 


static void* temp_calloc(size_t n, size_t sz) {
   return 0;
}


void *calloc(size_t n, size_t sz )
{
  void* ptr = 0;
  char  buffer[BSIZE];
  char* bt = 0;
  size_t jsonsize = 0;

  if (real_calloc == 0) {
      //fprintf (stderr,  "VAPPS: calloc init\n");fflush(stderr);
      real_calloc = temp_calloc;
      real_calloc  = (void * (*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
  }

  if (config_includeBacktrace) {

  }

  ptr = (*real_calloc)(n, sz);
  size_t tsz = n * sz;
  formatjson(CALLOC, tsz, ptr, bt, buffer, &jsonsize);
  report(buffer, jsonsize);
  return ptr;
}

void *realloc(void *p, size_t sz)
{
  void* ptr = 0;
  char  buffer[BSIZE];
  char* bt = 0;
  size_t jsonsize = 0;

  if (real_realloc == 0) {
      //fprintf (stderr,  "VAPPS: realloc init\n");fflush(stderr);
      real_realloc = (void * (*)(void *,size_t))dlsym(RTLD_NEXT, "realloc");
  }

  if (config_includeBacktrace) {

  }

  ptr = (*real_realloc)(p, sz);
  formatjson_realloc(sz, p, ptr, bt, buffer, &jsonsize);
  report(buffer, jsonsize);
  return ptr;
} 

void free (void *ptr)
{
  char  buffer[BSIZE];
  char* bt = 0;
  size_t jsonsize = 0;

  if (real_free == 0) {
      //fprintf (stderr,  "VAPPS: free init\n");fflush(stderr);
      real_free = (void   (*)(void *))dlsym(RTLD_NEXT, "free");
  }

  if (ptr == 0) {
     return;
  }


  if (config_includeBacktrace) {

  }

  (*real_free)(ptr);
  formatjson_free(ptr, bt, buffer, &jsonsize);
  report(buffer, jsonsize);
  return;
}

char* formatBacktrace(char* buffer, size_t len) {
    void* b[BTDEPTH];
    buffer[0] = 0;
    int   bsz = backtrace(b, BTDEPTH);
    //fprintf (stderr,  "+m %p %8zu\n", ptr, sz);
    //fflush (stderr);
    backtrace_symbols_fd(b, bsz, fileno(stderr));
    return buffer;
}


/*
 *  Message formatting routine
 */
static const char jsonalloc0[]  = "{\"m\":\"alloc\",\"sz\":";
static const char jsonsz[]      = ",\"szr\":";
static const char jsonaddr[]    = ",\"addr\":";
static const char jsonoldaddr[] = ",\"oldaddr\":";
static const char jsonbt[]      = ",\"bt\":";


static const char jsonalloc[]  = "{\"m\":\"%s\",\"sz\":%zu,\"addr\":%p}";
static const char jsonrealloc[]  = "{\"m\":\"realloc\",\"sz\":%zu,\"oldaddr\":%p,\"addr\":%p}";
static const char jsonfree[]   =  "{\"m\":\"free\",\"addr\":%p,}";

static const char jsonallocbt[]  = "{\"m\":\"%s\",\"sz\":%zu,\"addr\":%p,\"bt\":\"%s\"}";
static const char jsonreallocbt[]  = "{\"m\":\"realloc\",\"sz\":%zu,\"oldaddr\":%p,\"addr\":%p,\"bt\":\"%s\"}";
static const char jsonfreebt[]   =  "{\"m\":\"free\",\"addr\":%p,\"bt\":\"%s\"}";



static void formatjson(int method, size_t size, void* ptr, char* bt, char* buffer, size_t* jsonsize) {
   buffer[0] = 0;
   char tbuffer[128];

//config_includeBacktrace

   if (size == 0) {
       strcpy(tbuffer, "#Memory size is 0");
       return;
   }
   if (ptr == 0) {
       strcpy(tbuffer, "#Memory ptr is 0");
       return;
   }

   if (method == MALLOC) {
       if (config_includeBacktrace == true) {
           vmpsprintf(buffer, jsonallocbt, "alloc", size, ptr, bt);
       }
       else {
          vmpsprintf(buffer, jsonalloc, "alloc", size, ptr);
       }
   }
   else if (method == CALLOC) {
       if (config_includeBacktrace == true) {
           vmpsprintf(buffer, jsonallocbt, "calloc", size, ptr, bt);
       }
       else {
          vmpsprintf(buffer, jsonalloc, "calloc", size, ptr);
       }
   }

   *jsonsize = strlen(buffer);
   return;
}

static void formatjson_realloc(size_t size, void *oldptr, void* ptr, char* bt, char* buffer, size_t* jsonsize) {
   if (config_includeBacktrace == true) {
       vmpsprintf(buffer, jsonreallocbt, size, oldptr, ptr, bt);
   }
   else {
       vmpsprintf(buffer, jsonrealloc, size, oldptr, ptr);
   }
   *jsonsize = strlen(buffer);
   return;
}

static void formatjson_free(void* ptr, char* bt, char* buffer, size_t* jsonsize) {
   if (config_includeBacktrace == true) {
       vmpsprintf(buffer, jsonfreebt, ptr, bt);
   }
   else {
       vmpsprintf(buffer, jsonfree, bt);
   }
   *jsonsize = strlen(buffer);
   return;
}

/*
 *
 */
static void report(char* buffer, size_t jsonsize) {
   
  pthread_mutex_lock ( &mut );
  if (fh != 0) {
     fprintf(fh, "%s\n", buffer);
  }
  if (socketfd > 0) {
     struct sockaddr_in* x = (struct sockaddr_in*)&remote_sockaddr;
     fprintf(stderr, "sockaddr len %d  port %d %lu\n", remote_sockaddr_len, x->sin_port, (unsigned long)x->sin_addr.s_addr);

     if (sendto(socketfd, buffer, jsonsize, 0, (struct sockaddr*)&remote_sockaddr, remote_sockaddr_len) == -1) {
        fprintf(stderr, "udp sendto failed %d", errno);
        close(socketfd);   // dont want to keep pounding on the door
        socketfd = -1;
     }
  }
  if (config_stderr) {
     strcat(buffer, "\n");
     write(2, buffer, strlen(buffer));
  }  
  pthread_mutex_unlock ( &mut );
}

static void error(const char* buffer) {
   write(2, buffer, strlen(buffer)); 
}
