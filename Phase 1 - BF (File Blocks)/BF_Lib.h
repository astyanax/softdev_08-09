#ifndef _BF_LIB_H_
#define _BF_LIB_H_

/* Definitions and constants */
#define BF_BLOCK_SIZE 1024
#define BF_BUFFER_SIZE 20
#define MAXOPENFILES 25
#define BF_INVALID -1
enum {FALSE, TRUE};

/* Error Codes */
#define BFE_OK 0					/* OK */
#define BFE_NOMEM -1				/* Not enough memory */
#define BFE_NOBUF -2				/* Not enough buffer space */
#define BFE_BLOCKFIXED -3			/* Block is already pinned in buffer */
#define BFE_BLOCKNOTINBUF -4		/* Block to be unpinned doesn't exist in buffer */
#define BFE_OS -5					/* General OS-related error */
#define BFE_INCOMPLETEREAD -6		/* Incomplete block read */
#define BFE_INCOMPLETEWRITE -7		/* Incomplete buffer write */
#define BFE_INVALIDBLOCK -8         /* Invalid block id */
#define BFE_FILEOPEN -9             /* File is already open */
#define BFE_FTABFULL -10			/* Open file table is full */
#define BFE_FD -11					/* Invalid file descriptor */
#define BFE_EOF -12					/* End Of File */
#define BFE_BLOCKFREE -13			/* Block is already free */
#define BFE_INVALIDFILENAME -14		/* Invalid Filename */
#define BFE_FILEEXISTS -15			/* File already exists */
#define BFE_CLOSEBLOCKFIXED -16     /* Can't close if pinned blocks still exist */
#define BFE_NOBITMAP -17			/* Not enough space in bitmap */
#define BFE_INVALIDVAL -18			/* An invalid value was passed */

/* Function prototypes */
void BF_Init( void );
int BF_CreateFile( char *fileName );
int BF_DestroyFile( char *fileName );
int BF_OpenFile( char *fileName );
int BF_CloseFile( int fileDesc );
int BF_AllocBlock( int fileDesc, int * blockNum, char ** blockBuf );
int BF_UnpinBlock(int fileDesc, int blockNum, int dirty);
int BF_DisposeBlock(int fileDesc, int *blockNum);
int BF_GetFirstBlock( int fileDesc, int *blockNum, char **blockBuf );
int BF_GetNextBlock( int fileDesc, int *blockNum, char **blockBuf );
int BF_GetThisBlock( int fileDesc, int blockNum, char **blockBuf );
void BF_PrintError( char *errString );

int BF_SaveDirtyBlock (int memPos);
int BF_findMemPos (int * memPos);
int BF_LRU_Recycle(int * memPos);
int BF_readBlock (int fileDesc, int filePos, int memPos);
char BF_existsInMem(int fileDesc, int filePos, int * memPos);

#endif
