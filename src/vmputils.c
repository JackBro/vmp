#include "vmputils.h" 

char* vmpsprintf(char *buffer, const char* format, ...) {
   size_t len = strlen(format);
   bool   informat = false;
   const char*  formattype = 0;
   char   tbuffer[128];

   va_list vl;
   va_start(vl, format);
   size_t ixs = 0;
   size_t ixf = 0;

   buffer[0] = 0;

   while(ixf<len) {
      if (format[ixf] != '%') {
         buffer[ixs++] = format[ixf++];
         buffer[ixs] = 0;
      }   
      else {
         informat = true;
         formattype = &format[ixf];
         //printf(" format found %s\n", formattype);
     }
     if (informat == true) {
        informat = false;
        if (!strncmp(formattype, "%zu", 3)) {
           size_t i = va_arg(vl, size_t);
           vmpsize2hexstr(i, tbuffer, 128);
           strcat(buffer, tbuffer);
           ixs += strlen(tbuffer);
           ixf += 3;
        }
        else if (!strncmp(formattype, "%s", 2)) {              
           char* str = va_arg(vl, char*);
           //printf("s dormat:[%s]\n", str);  
           if (str != 0) {
              strcat(buffer, str);
              ixs += strlen(str);
           }
           ixf += 2;
        }
        else if (!strncmp(formattype, "%d", 2)) {
           int i = va_arg(vl, int);
           vmpint2str(i, tbuffer, 128);
           //printf("d dormat:[%d] [%s]\n", i, tbuffer);
           strcat(buffer, tbuffer);
           ixs += strlen(tbuffer);
           ixf += 2;
        }
        else if (!strncmp(formattype, "%p", 2)) {
           int i = va_arg(vl, int);
           vmpsize2hexstr(i, tbuffer, 128);
           //printf("p dormat:[%p] [%s]\n", (void*)i, tbuffer);
           strcat(buffer, tbuffer);
           ixs += strlen(tbuffer);
           ixf += 2;
        }
        else {
           // unrecognized format. At minimum do not loop.  
           ++ixf;
        }
     }
   }
   buffer[ixs] = 0; 
   va_end(vl);
   return buffer;
}


static const char int_chars[] = "0123456789";

char* vmpint2str(size_t in, char* intstr, int bsize) {
   if (bsize < (sizeof(size_t)*2)+1) {
        intstr[0] = 0;
        return intstr;
   }
   int s = 0;
   while (in > 10) {
     int digit = in;
     int sub   = 1;
     do {
         digit = digit / 10;
         sub *= 10;
     } while (digit > 9);
     intstr[s++] = int_chars[digit];
     sub *= digit;
     in -= sub;
   };
   intstr[s++] = int_chars[in];
   intstr[s] = 0;

   if (in != 0) {
      int xx = strlen(intstr);
      int ii = 0; 
      for( ; ii<xx; ++ii) {
          if (intstr[ii] != '0') {
             break;
          }
      }
      if (ii == xx) abort();
   }


   return intstr;
}

static const char hex_chars[] = "0123456789ABCDEF";

char* vmpsize2hexstr(size_t in, char* strhex, int bsize) {

   if (bsize < (sizeof(size_t)*2)+1) {
        strhex[0] = 0;
        return strhex;
   }

   unsigned int shift = (sizeof(size_t)-1) * 8;
   unsigned int i=0, s=0, m=sizeof(size_t);

   for( ; i<m; ++i, shift-=8) {
       unsigned char x = in >> shift;
       int x0 = ( x >> 4 ) & 0x0F;
       int x1 =  x & 0x0F;
       if (x0 > 15 || x1 > 15) {
          abort();
       }
       strhex[s++] = hex_chars[ ( x >> 4 ) & 0x0F ];
       strhex[s++] = hex_chars[ x & 0x0F ];
   }
   strhex[s] = 0;

   if (in != 0) {
      int xx = strlen(strhex);
      int ii = 0;
      for(; ii<xx; ++ii) {
          if (strhex[ii] != '0') {
             break;
          }
      }
      if (ii == xx) abort();
   }

   return strhex;
}

char* vmpptr2hexstr(void* in, char* strhex, int bsize) {

   if (bsize < (sizeof(void*)*2)+1) {
        strhex[0] = 0;
        return strhex;
   }

   unsigned int shift = (sizeof(void*)-1) * 8;
   unsigned int i=0, s=0, m=sizeof(void*);

   for( ; i<m; ++i, shift-=8) {
       unsigned char x = (long long)in >> shift;
       int x0 = ( x >> 4 ) & 0x0F;
       int x1 =  x & 0x0F;
       if (x0 > 15 || x1 > 15) {
          abort();
       }
       strhex[s++] = hex_chars[ ( x >> 4 ) & 0x0F ];
       strhex[s++] = hex_chars[ x & 0x0F ];
   }
   strhex[s] = 0;

   if (in != 0) {
      int xx = strlen(strhex);
      int ii = 0;
      for(; ii<xx; ++ii) {
          if (strhex[ii] != '0') {
             break;
          }
      }
      if (ii == xx) abort();
   }

   return strhex;
}

static int asciizero = (int)'0'; 


size_t vmstrlen(char* str, size_t len) {
   size_t ix = 0;
   /* capping the seach here at an arbitary 4k */
   if (len <= 0) len = 4096;
   for (; ix<=len; ++ix) {
      if (str[ix] == 0) {
         return ix;
      }
   }
   return -1;
}


int vmstr2int(char* instr) {
   int slen = vmstrlen(instr, 1024);
   if (slen == -1) {
       return 0;
   }
   int res = 0;
   int pwr10 = 1;
   /* e.g   "123x0" slen = 3; */
   --slen;  /* back to least significent digit */
   for( ; slen>=0; --slen) {
      int i = (char)instr[slen];
      if (i > 57 || i < 48) {
          //fprintf(stderr, "vmstr2int invalid inout %s\n", instr);
          return 0;
      }
      i -= asciizero;
      i *= pwr10;
      res += i;
      pwr10 *= 10;
   }
   return res;
}


