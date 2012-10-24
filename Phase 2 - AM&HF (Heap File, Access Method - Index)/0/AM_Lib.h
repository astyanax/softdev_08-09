#ifndef _AM_LIB_H_
#define _AM_LIB_H_

#include <stdint.h>
#include "BF_Lib.h"

/* Constants and definitions */
#define MAXOPENFILES 25
#define MAXSCANS 20

#define AM_INVALID -1
#define AM_SPLIT 1
#define AM_BITSIZE 0.125
#define AM_FIRST_RECORD 0
#define AM_FIRST_BLOCK 1
#define AM_MAX_GLOBAL_DEPTH 10

enum {EQUAL = 1, LESS_THAN, GREATER_THAN, LESS_EQUAL_THAN, GREATER_EQUAL_THAN, NOT_EQUAL, GET_ALL};
enum {FALSE, TRUE};

/* Error Codes */
#define AME_OK 0
#define AME_NOMEM -1
#define AME_OS -5
#define AME_INCOMPLETEREAD -6
#define AME_INCOMPLETEWRITE -7
#define AME_INVALIDBLOCK -8
#define AME_EOF -12
#define AME_INVALIDFILENAME -14
#define AME_INVALIDATTRTYPE -19
#define AME_INVALIDATTRLENGTH -20
#define AME_INVALIDINDEXNO -21
#define AME_INDEXEXISTS -22
#define AME_INDEXOPEN -23
#define AME_INVALIDVALUE -24
#define AME_INVALIDOPERATOR -25
#define AME_SCANTABFULL -26
#define AME_INVALIDSCAN -27
#define AME_INVALIDDELETE -28
#define AME_MAXGLOBALDEPTH -29

struct AM_Bucket;

/* AM Interface */
void AM_Init( void );
int AM_CreateIndex( char *fileName, int indexNo, char attrType, int attrLength );
int AM_DestroyIndex( char *fileName, int indexNo );
int AM_OpenIndex ( char * fileName, int indexNo );
int AM_CloseIndex ( int fileDesc );
int AM_InsertEntry( int fileDesc, char attrType, int attrLength, char *value, int recId );
int AM_DeleteEntry( int fileDesc, char attrType, int attrLength, char *value, int recId );
int AM_OpenIndexScan( int fileDesc, char attrType, int attrLength, int op, char *value );
int AM_FindNextEntry( int scanDesc );
int AM_CloseIndexScan( int scanDesc );
void AM_PrintError( char * errString );

/* Other useful functions */
int AM_checkAttributes ( char attrType, int attrLength );
char * AM_convertFileName ( char * fileName, int indexNo );

int AM_insertInBucket ( struct AM_Bucket * bucket, char *value, int recId, int maxRecords, int attrLength );
struct AM_Bucket * AM_bufferToBucket ( char * buffer, int pos, int blockPos );
int AM_deleteFromBucket ( struct AM_Bucket * bucket, char *value, int recId, int maxRecords, int attrLength, char attrType );
int AM_Split( int fileDesc, struct AM_Bucket * splitBucket, char *value, int recId );

int AM_Compare( void * value1, void * value2, char type, int op );
int AM_getFirstBits( int number, int32_t numBits );
int AM_notExists( char val, char * tempTable, int totalBlocks );
int AM_powOf2( int power );

int32_t AM_HashFunction( void *key, char type );
int AM_createHashTable ( int pos, char * pch );
void AM_addBucketToHashTable ( int pos, int * hashTableIndex, char * bucket, int bucketPos );

#endif
