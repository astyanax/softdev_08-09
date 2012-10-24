#ifndef _BF_BIT_OPERATIONS_H_
#define _BF_BIT_OPERATIONS_H_

void BF_debug_bitPrint( int num, int nbits );
void BF_bitSet( int * num, int pos );
void BF_bitClear( int * num, int pos );
int  BF_isBitZero( int num, int pos );

#endif
