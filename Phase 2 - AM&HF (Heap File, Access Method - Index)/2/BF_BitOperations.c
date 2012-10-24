#include <stdio.h>
#include "BF_BitOperations.h"

/**** void BFdebug_bitPrint(int num, int nbits) ****

 Input:
        * int num               ari8mos pros meleth
        * int pos               plh8os bits pou tha ektypw8oun

 Operation:
        * Ektypwnei ta "nbits" prwta bits tou ari8mou "num" (for debugging reasons)

 Output:
        * As above

 Return Value:
        * None
*/

void BF_debug_bitPrint( int num, int nbits )
{
    int i;
    /* note to self: otan pame gia position 0 px kanoyme access to lsb, aneksarthtws endianess */
    printf( "[Debug] Number is (LSB->MSB): " );

    for ( i = 0; i < nbits; i++ ) {
        if ( BF_isBitZero( num, i ) ) {
            putchar( '0' );
        }
        else {
            putchar( '1' );
        }
    }

    printf( "\n:%d (10)\n", num );
}


/**** int BF_isBitZero(int num, int pos) ****

 Input:
        * int num               ari8mos pros meleth
        * int pos               8esh tou bit pou tha elegxei

 Operation:
        * Elegxei an to "pos"-osto bit tou "num" einai 0

 Output:
        * None

 Return Value:
        * 1, an to bit isoutai me 0 (mhden)
        * 0, an to bit isoutai me 1 (ena)
*/

int BF_isBitZero( int num, int pos )
{
    return ( !( num & ( 1 << pos ) ) );
}


/**** void BF_bitSet(int * num, int pos) ****

 Input:
        * int * num             ari8mos pros epeksergasia
        * int pos               8esh tou bit pou tha allax8ei

 Operation:
        * 8etei to "pos"-osto bit tou "num" iso me 1

 Output:
        * None

 Return Value:
        * None
*/

void BF_bitSet( int * num, int pos )
{
    ( *num ) |= ( 1 << pos );
}


/**** void BF_bitClear(int * num, int pos) ****

 Input:
        * int * num             ari8mos pros epeksergasia
        * int pos               8esh tou bit pou tha allax8ei

 Operation:
        * 8etei to "pos"-osto bit tou "num" iso me 0

 Output:
        * None

 Return Value:
        * None
*/

void BF_bitClear( int * num, int pos )
{
    ( *num ) &= ~( 1 << pos );
}
