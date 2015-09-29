/*
 * Helper utilities that not rely on an C library functions
 * vms[xxx]  where xxx is the c library routine.
 *
 * IMPORTANT NOTE - 
 *    these get added to as need be, so do not assume complete support unless noted
 *
 *  quinncolm@optonline.net  
 *
 */

#ifndef VMPUTILS__
#define VMPUTILS__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

typedef enum { false, true } bool;

/* vmp* c-lib free utils */
char* vmpsprintf(char *buffer, const char* format, ...);

size_t vmstrlen(char* str, size_t len);


/*  formatting helpers */
char* vmpint2str(size_t in, char* intstr, int bsize);
char* vmpsize2hexstr(size_t in, char* intstr, int bsize);
int   vmstr2int(char* instr);

#endif

