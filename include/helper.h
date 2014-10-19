#ifndef __HELPER_H__
#define __HELPER_H__

int helper_parse_setup ( char * string, char ***output, int *lenght, ... );

char* fgets_s ( char* s, int n, FILE *iop, char sep );

#endif // __HELPER_H__
