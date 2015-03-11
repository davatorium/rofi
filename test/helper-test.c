#include <assert.h>
#include <glib.h>
#include <stdio.h>
#include <helper.h>
#include <string.h>

static int test = 0;

#define TASSERT(a) {\
    assert ( a );\
    printf("Test %i passed (%s)\n", ++test, #a);\
}
int main ( int argc, char ** argv )
{
    cmd_set_arguments(argc, argv);
	char **list = NULL;
	int llength = 0;
	char * test_str = "{host} {terminal} -e bash -c \"{ssh-client} {host}; echo '{terminal} {host}'\"";
	helper_parse_setup( test_str, &list, &llength, "{host}", "chuck",
			"{terminal}", "x-terminal-emulator", NULL);

	TASSERT ( llength == 6 );
	TASSERT ( strcmp(list[0], "chuck")  == 0 );
	TASSERT ( strcmp(list[1], "x-terminal-emulator")  == 0 );
	TASSERT ( strcmp(list[2], "-e")  == 0 );
	TASSERT ( strcmp(list[3], "bash")  == 0 );
	TASSERT ( strcmp(list[4], "-c")  == 0 );
	TASSERT ( strcmp(list[5], "ssh chuck; echo 'x-terminal-emulator chuck'")  == 0 );
	
	g_strfreev(list);
}
