/*
* --------------------------
*    vmpmemory:
*    Author: Colm Quinn  
*    quinncolm@optonline.net
* ----------------------------
*
*  Test driver for libvmpmemory 
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




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>i

typedef enum { false, true } bool;

int main(int argc, char* argv[]) {

   printf("Hello world\n");

   void* p0 = malloc(2048);
   void* p1 = calloc(10, 128);

   printf("clib printf P:  %p %p\n", p0, p1);

   p0 = realloc(p0, 4096 * 4);

   free(p1);
   printf("free P: %p\n", p1);

   return 0;
}

