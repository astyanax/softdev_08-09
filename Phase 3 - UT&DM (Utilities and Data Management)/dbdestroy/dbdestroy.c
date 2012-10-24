/* dbdestroy.c */
#include <stdio.h>
#include <stdlib.h>

#define MAXNAME 32
#define LINE_SIZE 128

int main( int argc, char * argv[] )
{
    char command[LINE_SIZE];
    char * dbname;

    if ( argc != 2 ) {
        printf( "La8os arguments\n" );
        return EXIT_FAILURE;
    }

    dbname = argv[1];

    sprintf( command, "rmdir /S /Q %s", dbname );
    system( command );

    return EXIT_SUCCESS;
}
