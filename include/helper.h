#ifndef __HELPER_H__
#define __HELPER_H__

int helper_parse_setup ( char * string, char ***output, int *lenght, ... );

char* fgets_s ( char* s, int n, FILE *iop, char sep );

char **tokenize ( const char *input );


int find_arg_char ( const int argc, char * const argv[], const char * const key, char *val );

int find_arg_uint ( const int argc, char * const argv[], const char * const key, unsigned int *val );


int find_arg_int ( const int argc, char * const argv[], const char * const key, int *val );


int find_arg_str ( const int argc, char * const argv[], const char * const key, char** val );

int find_arg ( const int argc, char * const argv[], const char * const key );

#endif // __HELPER_H__
