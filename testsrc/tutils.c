/*
 * --------------------------
 *    vmpmemory:
 *    Author: Colm Quinn  
 *    quinncolm@optonline.net
 * ----------------------------
 *
 *  Test driver for vmptuils 
 *  (methods to replace C library methods that might invoke memory routines)
 *
 *  Example usage:
 *
 *    export VMPMEMORY_REPORTHOST=127.0.0.1:7777
 *    export VMPMEMORY_STDERR=1
 *    export VMPMEMORY_REPORTFILE=$PWD/vmpdata
 *    LD_PRELOAD=$PWD/libvmpmemory.so.0.1 ./tmemory
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include <vmputils.h>

static const char jsonalloc[]  = "{\"m\":\"%s\",\"sz\":%zu,\"addr\":%p,\"bt\":\"%s\"}";
static const char jsonrealloc[]  = "{\"m\":\"realloc\",\"sz\":%zu,\"oldaddr\":%p,\"addr\":%p,\"bt\":\"%s\"}";
static const char jsonfree[]   =  "{\"m\":\"free\",\"addr\":%p,\"bt\":\"%s\"}";



int main(int argc, char* argv[]) {

   printf("Hello world\n");
   char buffer[256];
   int x = 255 * 3;
   void* p = &x;
   char* bt[10];
   bt[0] = 0;

  // vmpsprintf(buffer, jsonalloc, "calloc", x, p, bt);
  // printf("1 %s\n", buffer);

   size_t sz= 1024;
   p = malloc(sz);
   buffer[0] = 0;
   vmpsprintf(buffer, jsonalloc, "malloc", sz, p, bt);
      printf("2 %s\n", buffer);


   int n = 5;
   p = calloc(sz, n);
   buffer[0] = 0;
   vmpsprintf(buffer, jsonalloc, "calloc", sz*n, p, bt);
   printf("3 %s\n", buffer);


// failing case
   p =   0x1806040;
   sz = 2048;
   buffer[0] = 0;
   vmpsprintf(buffer, jsonalloc, "balloc", sz, p, bt);
  printf("4 %s\n", buffer);

 
   vmpsprintf(buffer, jsonfree, p, bt);
   printf("5 %s\n", buffer);

   char sport[10];
   strcpy(sport, "10238");

   int port = vmstr2int(sport);
   printf("port %d\n", port);


   return 0;
}






