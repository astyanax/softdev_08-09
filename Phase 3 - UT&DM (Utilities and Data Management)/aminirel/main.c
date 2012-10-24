#include <stdio.h>
#include <stdlib.h>

#include "aminirel.h"
#include "parser.h"

/* global metavlhth poy periexei plhrofories gia th database pou diaxeirizomaste twra */
struct db this_db;

int main( int argc, char * argv[] )
{
    if ( argc != 2 ) {
        printf( "Wrong usage\nShould be called as: %s <dbName>\n", argv[0] );
        return EXIT_FAILURE;
    }

    if ( UT_Init( argv[1] ) != AMINIREL_OK ) {
        printf( "Error: Could not initialize UT layer, exiting now..\n" );
        UT_quit();
    }

    printf( "Welcome to aminirel! Please enter your commands:\n" );
    while ( yyparse() < 0 );

    return EXIT_SUCCESS;
}
