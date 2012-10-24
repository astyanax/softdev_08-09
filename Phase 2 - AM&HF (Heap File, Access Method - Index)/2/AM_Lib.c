#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "BF_Lib.h"
#include "AM_Lib.h"
#include "BF_BitOperations.h"

/****************
 *  Structures  *
 ****************/

/* H domh pou perigrafei mia eggrafh typou <value, recordId> */
struct AM_Record
{
    char * val;
    int32_t recId;
};

/* H domh pou perigrafei tous kadous */
struct AM_Bucket
{
    int32_t localDepth;             /* To topiko ba8os tou kadou */
    char * bitmap;                  /* To bitmap pou afora tis eggrafes */
    char * buffer;                  /* Pointer ston kado sthn Endiamesh Mnhmh toy epipedoy BF */
    int32_t bitmapSize;             /* To mege8os tou bitmap (gia oikonomia ypologismwn) */
    int32_t blockNum;               /* Se poio block tou arxeiou antistoixei */
};

/* H domh pou perigrafei ena eurethrio */
struct AM_OpenIndex
{
    int32_t depth;          /* H metablhth pou krataei to oliko ba8os tou eurethriou mas */
    int32_t totalBlocks;    /* H metablhth pou krataei to synoliko plh8os twn kadwn */
    int32_t maxRecords;     /* To megisto plh8os twn eggrafwn pou xwrane sto eurethrio */
    int32_t maxBlocks;      /* To synoliko dynato plh8os twn blocks sto arxeio (opws sto epipedo BF) */
    char attrType;          /* To eidos tou value */
    int32_t attrLength;     /* To megethos tou value se bytes */
    char valid;             /* Ean yfistatai to eurethrio */
    char * fileName;        /* To onoma tou arxeiou tou eurethriou */
    char * hashTable;       /* O pinakas katakermatismou */
};

/* H domh pou perigrafei mia sarwsh */
struct AM_OpenScan
{
    char valid;         /* Ean yfistatai to scan, dld an xrhsimopoieitai to keli (tou pinaka anoixtwn sarwsewn) */
    int op;             /* O tropos me ton opoio tha sygkrinoume times tou eurethriou me thn timh value */
    char * value;       /* H timh symfwna me tin opoia tha ginetai h sygkrish */
    char attrType;      /* O typos tou value */
    int attrLength;     /* To mhkos tou value se bytes */
    int lastRecord;     /* To teleytaio record pou epistrepsame */
    int lastBucketNum;  /* apo to teleutaio bucket pou episkeftikame */
    int fileDesc;       /* index sto Eurethrio arxeio pou kanoume scan */
};

/*************
 *  Globals  *
 *************/

/* Metablhth pou krata ton teleutaio kwdiko sfalmatos */
char AM_errno;

/* O pinakas twn Anoixtwn Eurethriwn */
struct AM_OpenIndex AM_OpenIndeces[MAXOPENFILES];

/* O pinakas twn Anoixtwn Sarwsewn */
struct AM_OpenScan AM_OpenScans[MAXSCANS];

/* O pinakas pou krata ta mhnymata sfalmatwn */
char AM_Errors[][64] = {
    "Not enough memory",
    "Not enough buffer space",
    "Block is already pinned in buffer",
    "Block to be unpinned doesn't exist in buffer",
    "General OS-related error",
    "Incomplete block read",
    "Incomplete buffer write",
    "Invalid block id",
    "File is already open",
    "Open file table is full",
    "Invalid file descriptor",
    "End Of File",
    "Block is already free",
    "Filename is too large",
    "File already exists",
    "Can't close if pinned blocks still exist",
    "Not enough space in bitmap",
    "An invalid value was passed",
    "Invalid Attribute Type specified",
    "Invalid Attribute Length specified",
    "Invalid Index specified",
    "Index already exists",
    "Index is already open",
    "Invalid Value specified",
    "Invalid Operator specified",
    "Open Scan Table is full",
    "Invalid scan specified",
    "Could not delete entry because it doesn't exist",
    "Can't split bucket, hash table is too large."
};


/************************
 * Function Definitions *
 ************************/

/**** void AM_Init( void ) ****

 Input:
        * None

 Operation:
        * Arxikopoiei tous pinakes twn anoiktwn eurethriwn kai anoiktwn sarwsewn,
          ka8ws kai th global metablhth la8wn ("AM_errno"), afou kalesei th sunarthsh
          arxikopoihshs tou BF epipedou.

 Return Value:
        * None
*/

void AM_Init( void )
{
    int i;

    /* Arxikopoihsh tou BF epipedou */
    BF_Init();

    /* Arxikopoihsh ths domhs anoixtwn eurethriwn */
    for ( i = 0 ; i < MAXOPENFILES ; i++ ) {
        AM_OpenIndeces[i].depth = AM_OpenIndeces[i].totalBlocks = AM_INVALID;
        AM_OpenIndeces[i].maxRecords = AM_OpenIndeces[i].maxBlocks = AM_INVALID;
        AM_OpenIndeces[i].attrType = AM_INVALID;
        AM_OpenIndeces[i].valid = FALSE;
        AM_OpenIndeces[i].fileName = NULL;
        AM_OpenIndeces[i].hashTable = NULL;
    }

    /* Arxikopoihsh ths domhs anoixtwn sarwsewn */
    for ( i = 0 ; i < MAXSCANS ; i++ ) {
        AM_OpenScans[i].valid = FALSE;
        AM_OpenScans[i].op = AM_INVALID;
        AM_OpenScans[i].value = NULL;
        AM_OpenScans[i].attrType = FALSE;
        AM_OpenScans[i].attrLength = AM_INVALID;
        AM_OpenScans[i].lastRecord = AM_INVALID;
        AM_OpenScans[i].lastBucketNum = 0;
        AM_OpenScans[i].fileDesc = AM_INVALID;
    }

    /* Arxikopoihsh ths global metablhths la8wn */
    AM_errno = AME_OK;
}


/**** int AM_CreateIndex( char *fileName, int indexNo, char attrType, int attrLength ) ****

 Input:
        * char * fileName               To onoma tou arxeiou eurethriou
        * int indexNo                   O index tou eurethriou
        * char attrType                 O typos dedomenwn ths timhs tou pediou
        * int attrLength                To mhkos se bytes tou pediou

 Operation:
        * Elegxei an to arxeio eurethriou yparxei hdh, an nai tote termatizei me AME_INDEXEXISTS
        * An nai, to dhmiourgei, eisagei to BF header kai meta to AM header,
          arxikopoiwntas to AM header katallhla.

 Return Value:
        * AME_OK, efoson epityxei
        * AME_INDEXEXISTS, efoson yparxei hdh to eurethrio
        * Kapoio allo kwdiko sfalmatos, efoson apotyxei
*/

int AM_CreateIndex( char *fileName, int indexNo, char attrType, int attrLength )
{
    char * indexFileName, * pch;
    int32_t i, depth, totalBlocks, blockPos, maxRecords, maxBlocks, fileDesc;
    FILE * fp;
    
    /* Check for valid input. */
    if (( i = AM_checkAttributes( attrType, attrLength ) ) != AME_OK ) {
        return i;
    }

    if (( indexFileName = AM_convertFileName( fileName, indexNo ) ) == NULL ) {
        return AM_errno;
    }

    /* To arxeio me onoma "indexFileName" prepei na einai monadiko, dhladh na mhn yparxei hdh. */
    if (( fp = fopen( indexFileName, "rb" ) ) != NULL ) {
        if ( fclose( fp ) == EOF ) {
            AM_errno = AME_OS;
            return AME_OS;
        }

        AM_errno = AME_INDEXEXISTS;
        return AME_INDEXEXISTS;
    }

    /* Dhmioyrgoyme to arxeio, prwta apo to epipedo BF */
    if (( i = BF_CreateFile( indexFileName ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    /*
       Kai twra tha eisagoyme to diko mas header (me keno pinaka katakermatismou), pou periexei:

       * int32_t depth        - to oliko vathos tou pinaka katakermatismou
       * int32_t totalBlocks  - posous kadous exoume mexri twra
       * int32_t maxRecords   - posa records, zeygaria <value, kleidi>, xwrane se enan kado
       * int32_t maxBlocks    - posous kadous to poly mporoyme na exoume logw tou bitmap tou epipedou BF
       * char attrType        - o typos tou value
       * int32_t attrLength   - to megethos tou value se bytes

       Shmeiwsh: O logos pou swzoume twra ta maxRecords kai ta maxBlocks einai gia na mh xreiazomaste na ta
                 ypologizoyme kathe fora poy anoigei to eurethrio.
    */

    /* Anoigoume to arxeio apo to BF epipedo gia na eisagoume to diko mas header sto prwto non-header block tou BF epipedou */
    if (( fileDesc = BF_OpenFile( indexFileName ) ) < 0 ) {
        AM_errno = i;
        return i;
    }

    /* desmeuoume to block tou header */
    if (( i = BF_AllocBlock( fileDesc, &blockPos, &pch ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    /* kai to arxikopoioume. */
    memset( pch, 0, BF_BLOCK_SIZE );
    depth = totalBlocks = 0;
 
    /* Swzoume to oliko ba8os kai to synoliko plh8os blocks */
    memcpy( pch, &depth, sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( pch, &totalBlocks, sizeof( int32_t ) );
    pch += sizeof( int32_t );
 
    /* To megisto plh8os eggrafwn pou xwraei enas kados isoutai me to megisto plh8os zeugariwn <value, key>,
       dld einai iso me to mege8os tou block meion to megethos ths metablhths tou topikou ba8ous pros
       to megethos enos zeugariou (attrLength + mege8os kleidiou) syn 1 bit gia th 8esh sto bitmap */
    maxRecords = ( BF_BLOCK_SIZE - sizeof( int32_t ) ) / ( float )( attrLength + sizeof( int32_t ) + AM_BITSIZE );
 
    /* swzoume kai to megisto plh8os eggrafwn */
    memcpy( pch, &maxRecords, sizeof( int32_t ) );
    pch += sizeof( int32_t );
 
    /* Ta maximum blocks poy mporoyme na exoume sto arxeio einai osa bytes einai to header mas * 8 (plh8os bits)
       meion omws ta sizeof(int32_t) bytes toy arithmoy poy metraei posa exoume hdh ("totalBlocks") */
    maxBlocks = ( BF_BLOCK_SIZE - sizeof( int32_t ) ) * 8;
 
    /* Apo8hkeuoume kai to megisto plh8os blocks, ton typo ths timhs kai to mege8os ths se bytes */
    memcpy( pch, &maxBlocks, sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( pch, &attrType, sizeof( char ) );
    pch += sizeof( char );
    memcpy( pch, &attrLength, sizeof( int32_t ) );

    /* Afou de xreiazomaste pleon to block sth mnhmh, to kanoume unpin */
    if (( i = BF_UnpinBlock( fileDesc, blockPos, TRUE ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    if (( i = BF_CloseFile( fileDesc ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    free( indexFileName );
    return AME_OK;
}


/**** int AM_OpenIndex ( char * fileName, int indexNo ) ****

 Input:
        * char * fileName    To onoma tou arxeiou sto opoio antistoixei to eurethrio
        * int indexNo        Aukswn ari8mos eurethriou gia to arxeio

 Operation:
        * Afou ftiaksei to onoma tou eurethriou, fortwnei to BF header kai tis aparaithtes
          plhrofories apo auto
        * Epeita dhmiourgei enan keno pinaka katakermatismou (efoson den yparxei hdh)

 Return Value:
        * fileDesc, dld ton index ston pinaka Anoixtwn Eurethriwn, efoson epityxei
        * Kapoio kwdiko sfalmatos, efoson apotyxei
*/

int AM_OpenIndex( char * fileName, int indexNo )
{
    char * indexFileName, * pch;
    int32_t fileDesc, i, blockPos;
    
    /* Ftiaxnoume to onoma tou eurethriou */
    if (( indexFileName = AM_convertFileName( fileName, indexNo ) ) == NULL ) {
        return AM_errno;
    }

    /* Fortwma tou BF Header */
    if (( fileDesc = BF_OpenFile( indexFileName ) ) < 0 ) {
        AM_errno = fileDesc;
        return fileDesc;
    }

    /* Fortwmoume to onoma tou arxeiou eurethriou */
    if (( AM_OpenIndeces[fileDesc].fileName = malloc( strlen( indexFileName ) + 1 ) ) == NULL ) {
        AM_errno = AME_NOMEM;
        return AME_NOMEM;
    }

    strcpy( AM_OpenIndeces[fileDesc].fileName, indexFileName );

    /* epeita, fortwnoume to header tou AM epipedou, pou einai to prwto block tou BF epipedou */
    if (( i = BF_GetFirstBlock( fileDesc, &blockPos, &pch ) ) != BFE_OK ) {
        AM_errno = i;
        free( indexFileName );
        return i;
    }

    /* kai telos, diavazoume ta dedomena tou header tou AM epipedou kai ta apothikeuoume sth domh anoixtwn eurethriwn */
    memcpy( &AM_OpenIndeces[fileDesc].depth, pch , sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( &AM_OpenIndeces[fileDesc].totalBlocks, pch , sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( &AM_OpenIndeces[fileDesc].maxRecords, pch , sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( &AM_OpenIndeces[fileDesc].maxBlocks, pch , sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( &AM_OpenIndeces[fileDesc].attrType, pch , sizeof( char ) );
    pch += sizeof( char );
    memcpy( &AM_OpenIndeces[fileDesc].attrLength, pch , sizeof( int32_t ) );
    pch += sizeof( int32_t );
    AM_OpenIndeces[fileDesc].valid = TRUE;

    /* An den exoume (teleiws keno) pinaka katakermatismou */
    if ( AM_OpenIndeces[fileDesc].totalBlocks > 0 ) {
        /* dhmioyrgise ton */
        if (( i = AM_createHashTable( fileDesc, pch ) ) != AME_OK ) {
            return i;
        }
    }

    /* Afou de xrhsimopoioume pleon to block, to apodesmeuoume */
    if (( i = BF_UnpinBlock( fileDesc, blockPos, FALSE ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    free( indexFileName );
    return fileDesc;
}


/**** int AM_DestroyIndex( char *fileName, int indexNo ) ****

 Input:
        * char * fileName       To onoma tou arxeiou sto opoio antistoixei to eurethrio
        * int indexNo           Aukswn ari8mos eurethriou gia to arxeio

 Operation:
        * Elegxei ston pinaka anoixtwn eurethriwn ean yparxei to "filename.indexNo" arxeio
          kai to diagrafei, efoson den einai anoixto

 Return Value:
        * AME_OK, efoson oloklhrw8ei epityxws
        * AME_INDEXOPEN, efoson to eurethrio einai anoixto (epomenws de mporei na diagrafei)
        * Kapoio allo kwdiko sfalmatos, efoson apotyxei
*/

int AM_DestroyIndex( char *fileName, int indexNo )
{
    int32_t i;
    char * indexFileName;

    /* Ftiaxnoume to onoma tou eurethriou */
    if (( indexFileName = AM_convertFileName( fileName, indexNo ) ) == NULL ) {
        return AM_errno;
    }

    /* Elegxoume ston pinaka anoixtwn eurethriwn mhpws to arxeio "indexFileName" xrhsimopoieitai */
    for ( i = 0 ; i < MAXOPENFILES ; i++ ) {
        if ( AM_OpenIndeces[i].valid == TRUE ) {
            /* to eurhthrio poy theloyme na diagrapsoume den prepei na einai hdh anoigmeno. */
            if ( AM_Compare( AM_OpenIndeces[i].fileName, indexFileName, AM_OpenIndeces[i].attrType, EQUAL ) == 0 ) {
                AM_errno = AME_INDEXOPEN;
                return AME_INDEXOPEN;
            }
        }
    }

    if (( i = BF_DestroyFile( indexFileName ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    free( indexFileName );
    return AME_OK;
}


/**** int AM_CloseIndex ( int fileDesc ) ****

 Input:
        * int fileDesc      O ari8mos 8eshs tou eurethriou ston pinaka anoixtwn eurethriwn

 Operation:
        * Apo8hkeuei ta dedomena ston disko kai afairei/epanarxikopoiei thn eggrafh apo ton pinaka anoixtwn eurethriwn

 Return Value:
        * AME_OK, efoson oloklhrw8ei epityxws
        * Kapoio kwdiko sfalmatos, efoson apotyxei
*/

int AM_CloseIndex( int fileDesc )
{
    int i, blockPos, total;
    char * pch;
    
    /* Check for valid input input */
    if ( fileDesc < 0 || fileDesc > MAXOPENFILES || AM_OpenIndeces[fileDesc].valid == FALSE ) {
        AM_errno = AME_INVALIDBLOCK;
        return AME_INVALIDBLOCK;
    }

    /* diavazoume to header apo to epipedo BF */
    if (( i = BF_GetFirstBlock( fileDesc, &blockPos, &pch ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    /* ananewnoume to neo header mas sth mnhmh */
    memcpy( pch, &AM_OpenIndeces[fileDesc].depth, sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( pch, &AM_OpenIndeces[fileDesc].totalBlocks, sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( pch, &AM_OpenIndeces[fileDesc].maxRecords, sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( pch, &AM_OpenIndeces[fileDesc].maxBlocks, sizeof( int32_t ) );
    pch += sizeof( int32_t );
    memcpy( pch, &AM_OpenIndeces[fileDesc].attrType, sizeof( char ) );
    pch += sizeof( char );
    memcpy( pch, &AM_OpenIndeces[fileDesc].attrLength, sizeof( int32_t ) );
    pch += sizeof( int32_t );

    /* swzoume kai ton pinaka katakermatismou, an autos yparxei  */
    if ( AM_OpenIndeces[fileDesc].hashTable != NULL ) {
        total = AM_powOf2( AM_OpenIndeces[fileDesc].depth );
        memcpy( pch, AM_OpenIndeces[fileDesc].hashTable, total * sizeof( char ) );
    }

    /* Kanoume unpin to header kai to katagrafoume ws dirty, outws wste na graftei parakatw */
    if (( i = BF_UnpinBlock( fileDesc, blockPos, TRUE ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    /* To epipedo ths BF kleinei to arxeio */
    if (( i = BF_CloseFile( fileDesc ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    /* kai eleytherwnoyme tin antistoixh thesh poy desmeue to eyrethrio sth mnhmh mas */
    if ( AM_OpenIndeces[fileDesc].fileName != NULL ) {
        free( AM_OpenIndeces[fileDesc].fileName );
        AM_OpenIndeces[fileDesc].fileName = NULL;
    }

    if ( AM_OpenIndeces[fileDesc].hashTable != NULL ) {
        free( AM_OpenIndeces[fileDesc].hashTable );
        AM_OpenIndeces[fileDesc].hashTable = NULL;
    }

    /* kai thn epan-arxikopoioume */
    AM_OpenIndeces[fileDesc].depth = AM_OpenIndeces[fileDesc].totalBlocks = AM_INVALID;
    AM_OpenIndeces[fileDesc].maxRecords = AM_OpenIndeces[fileDesc].maxBlocks = AM_INVALID;
    AM_OpenIndeces[fileDesc].attrType = AM_INVALID;
    AM_OpenIndeces[fileDesc].valid = FALSE;
    return AME_OK;
}


/**** int AM_InsertEntry( int fileDesc, char attrType, int attrLength, char *value, int recId ) ****

 Input:
        * int fileDesc      H 8esh ston pinaka twn anoixtwn eurethriwn
        * char attrType     O typos tou pediou
        * int attrLength    To mege8os tou pediou se bytes
        * char * value      Deikths pros th timh-pedio pou eisagetai
        * int recId         To kleidh tou parapanw pediou

 Operation:
        * Eisagei thn eggrafh <*value, recId> sto eurethrio sth 8esh "fileDesc" ston pinaka anoixtwn eurethriwn.
        * Briskei se poio block tha topo8ethsei thn eggrafh:
          * An to eurethrio einai adeio, dhmiourgei to prwto block.
          * An to topiko ba8os tou kadou einai mikrotero tou olikou, xwrizei ton kado se 2 kai anamoirazei tis eggrafes.
          * An to topiko ba8os tou kadou einai iso me to oliko, diplasiazei to eurethrio kai anamoirazei oles tis eggrafes

 Return Value:
        * AME_OK, efoson oloklhrw8ei epityxws
        * Kapoio kwdiko sfalmatos, efoson apotyxei
*/

int AM_InsertEntry( int fileDesc, char attrType, int attrLength, char *value, int recId )
{
    int32_t i, blockPos;
    char * buffer;
    struct AM_Bucket * thisBucket;
    
    /* Check for valid input. */
    if ( fileDesc < 0 || fileDesc > MAXOPENFILES || AM_OpenIndeces[fileDesc].valid == FALSE ) {
        AM_errno = AME_INVALIDBLOCK;
        return AME_INVALIDBLOCK;
    }

    if (( i = AM_checkAttributes( attrType, attrLength ) ) != AME_OK ) {
        return i;
    }

    if ( value == NULL ) {
        AM_errno = AME_INVALIDVALUE;
        return AME_INVALIDVALUE;
    }

    /*
         Efoson exoume teleiws adeio pinaka katakermatismou, dhmiourgoume enan, opws kai to prwto keli tou
    */
    if ( AM_OpenIndeces[fileDesc].depth == 0 && AM_OpenIndeces[fileDesc].totalBlocks == 0 ) {
        /* dhmioyrgoyme enan pinaka katakermatismou me ena keli. */
        AM_OpenIndeces[fileDesc].totalBlocks++;
        AM_OpenIndeces[fileDesc].hashTable = malloc( 1 * sizeof( char ) );

        if ( AM_OpenIndeces[fileDesc].hashTable == NULL ) {
            AM_errno = AME_NOMEM;
            return AME_NOMEM;
        }

        /* To epipedo BF dhmioyrgei to neo block sto arxeio */
        if (( i = BF_AllocBlock( fileDesc, &blockPos, &buffer ) ) != BFE_OK ) {
            AM_errno = i;
            return i;
        }

        /* thetoume to topiko tou vathos iso me 0. */
        i = 0;
        memcpy( buffer, &i, sizeof( int32_t ) );
        
        /* To prwto keli tou pinaka katakermatismou tha deiksei sto neo block. */
        AM_OpenIndeces[fileDesc].hashTable[0] = blockPos;
    }
    /*
         Alliws, efoson exoume mono ena kado ston opoio phgainoun oi eggrafes, dokimazoume na baloume ekei thn eggrafh
    */
    else if ( AM_OpenIndeces[fileDesc].depth == 0 && AM_OpenIndeces[fileDesc].totalBlocks == 1 ) {
        blockPos = AM_OpenIndeces[fileDesc].hashTable[0];

        /* de xreiazetai na perasoume to value apo hash function, oi eggrafes tha pane s'auton ton kado */
        if (( i = BF_GetThisBlock( fileDesc, blockPos, &buffer ) ) != BFE_OK ) {
            AM_errno = i;
            return i;
        }
    }
    /*
         Se opoiadhpote allh periptwsh, prepei na perasoume to *value apo hash-function. Apo to hash tha paroume
         plh8os bits iso me to "depth" tou pinaka katakermatismou, kai analoga tha kataxwrisoume to record mas
    */
    else {
        int32_t hash, hashTableIndex;

        hash = AM_HashFunction( value, attrType );
        hashTableIndex = AM_getFirstBits( hash, AM_OpenIndeces[fileDesc].depth );

        blockPos = AM_OpenIndeces[fileDesc].hashTable[hashTableIndex];

        if (( i = BF_GetThisBlock( fileDesc, blockPos, &buffer ) ) != BFE_OK ) {
            AM_errno = i;
            return i;
        }
    }

    /*
        Twra poy exoume fortwsei ton kado ston opoio tha steiloyme thn eggrafh mas,
        ksekiname th diadikasia ths eisagwghs
    */

    /* Dhmioyrgoyme ena proswrino kado katakermatismou sto epipedo AM gia na mas dieykolynei thn diaxeirhsh toy. */
    if (( thisBucket = AM_bufferToBucket( buffer, fileDesc, blockPos ) ) == NULL ) {
        return AM_errno;
    }

    /* Eisagoume to zeugari <timh,kleidi> ston kado thisBucket. */
    if (( i = AM_insertInBucket( thisBucket, value, recId, AM_OpenIndeces[fileDesc].maxRecords, attrLength ) ) == AME_OK ) {
        /* De xreiazomaste pia to block karfwmeno. */
        if (( i = BF_UnpinBlock( fileDesc, blockPos, TRUE ) ) != BFE_OK ) {
            AM_errno = i;
            free( thisBucket->bitmap );
            free( thisBucket );
            return i;
        }
    }
    /* An o kados mas de xwraei allh eggrafh, tote kaloyme thn AM_split pou diaxeirizetai aythn thn eidikh periptwsh */
    else if ( i == AM_SPLIT ) {
        if (( i = AM_Split( fileDesc, thisBucket, value, recId ) ) != AME_OK ) {
            free( thisBucket->bitmap );
            free( thisBucket );
            return i;
        }
    }

    free( thisBucket->bitmap );
    free( thisBucket );
    return AME_OK;
}


/**** int AM_DeleteEntry( int fileDesc, char attrType, int attrLength, char *value, int recId ) ****

 Input:
        * int fileDesc      H 8esh ston pinaka twn anoixtwn eurethriwn
        * char attrType     O typos tou pediou
        * int attrLength    To mege8os tou pediou se bytes
        * char * value      Deikths pros th timh-pedio pou diagrafetai
        * int recId         To kleidh tou parapanw pediou

 Operation:
        *  Diagrafei thn eggrafh <*value, recId> apo to anoixto eurethrio sth 8esh "fileDesc" tou pinaka
           anoixtwn eurethriwn.

 Return Value:
        * AME_OK, efoson oloklhrw8ei epityxws
        * Kapoio kwdiko sfalmatos, efoson apotyxei
*/

int AM_DeleteEntry( int fileDesc, char attrType, int attrLength, char *value, int recId )
{
    int32_t i, blockPos, hash, hashTableIndex;
    char * buffer;
    struct AM_Bucket * thisBucket;
    
    /* Check for valid input. */
    if ( fileDesc < 0 || fileDesc > MAXOPENFILES || AM_OpenIndeces[fileDesc].valid == FALSE ) {
        AM_errno = AME_INVALIDBLOCK;
        return AME_INVALIDBLOCK;
    }

    if (( i = AM_checkAttributes( attrType, attrLength ) ) != AME_OK ) {
        return i;
    }

    if ( value == NULL ) {
        AM_errno = AME_INVALIDVALUE;
        return AME_INVALIDVALUE;
    }

    /* Briskoume se poio bucket einai h eggrafh pou 8eloume na sbhsoume. */
    hash = AM_HashFunction( value, attrType );
    hashTableIndex = AM_getFirstBits( hash, AM_OpenIndeces[fileDesc].depth );

    blockPos = AM_OpenIndeces[fileDesc].hashTable[hashTableIndex];

    /* fortwnoume to sygkekrimeno bucket */
    if (( i = BF_GetThisBlock( fileDesc, blockPos, &buffer ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    /* kai dhmioyrgoyme ena proswrino kado katakermatismou sto epipedo AM gia na mas dieykolynei thn diaxeirhsh toy. */
    if (( thisBucket = AM_bufferToBucket( buffer, fileDesc, blockPos ) ) == NULL ) {
        return AM_errno;
    }

    /* Diagrafoume to zeugari <timh,kleidi> ap'ton kado */
    if (( i = AM_deleteFromBucket( thisBucket, value, recId, AM_OpenIndeces[fileDesc].maxRecords, attrLength, attrType ) ) == AME_OK ) {
        /* de xreiazomaste pia to block karfwmeno. */
        if (( i = BF_UnpinBlock( fileDesc, blockPos, TRUE ) ) != BFE_OK ) {
            AM_errno = i;
            return i;
        }

        free( thisBucket->bitmap );
        free( thisBucket );
    }

    return AME_OK;
}


/**** int AM_OpenIndexScan(int fileDesc, char attrType, int attrLength, int op, char *value ) ****

 Input:
        * int fileDesc      H 8esh tou antistoixou eurethriou ston pinaka anoixtwn eurethriwn
        * char attrType     O typos tou pediou
        * int attrLength    To mege8os tou pediou se bytes
        * int op            O telesths sygkrishs
        * char * value      Deikths sth timh pediou pros sygkrish

 Operation:
        * Dhmiourgei mia kataxwrhsh ston pinaka anoixtwn sarwsewn me plhrofories pou aforoun th sarwsh.

 Return Value:
        * Mh arnhtiko ari8mo "scanDesc", an epityxei, pou antistoxei sth 8esh ston pinaka anoixtwn sarwsewn
          sthn opoia swsame th sygkekrimenh sarwsh
        * AME_SCANTABFULL, efoson o pinakas anoixtwn sarwsewn einai gematos
        * Kapoio allo kwdiko la8ous, efoson apotyxei
*/

int AM_OpenIndexScan( int fileDesc, char attrType, int attrLength, int op, char *value )
{
    int32_t i, scanDesc;
    char found_pos;
    
    /*
         Chech for valid input:
         - An o operator einai diaforetikos twn EQUAL kai NOT_EQUAL, epistrefetai la8os
         - Sthn periptwsh pou to value einai NULL, epistrefoume oles tis times tou eurethriou,
           aneksarthta apo to eidos tou operator
         - Sth periptwsh pou de dinetai timh pros elegxo, exoume eidiko operator "GET_ALL" o opoios
           ypodhlwnei pws prepei na epistrefoume oles tis eggrafes aneksarthtou timhs.
    */
    if ( value == NULL ) {
        op = GET_ALL;
    }
    else if ( op != EQUAL && op != NOT_EQUAL ) {
        return AME_INVALIDOPERATOR;
    }

    if ( fileDesc < 0 || fileDesc > MAXOPENFILES || AM_OpenIndeces[fileDesc].valid == FALSE ) {
        AM_errno = AME_INVALIDBLOCK;
        return AME_INVALIDBLOCK;
    }

    if (( i = AM_checkAttributes( attrType, attrLength ) ) != AME_OK ) {
        return i;
    }

    /* Prospathoume na vroume kenh thesh ston pinaka twn anoixtwn sarwsewn */
    found_pos = FALSE;

    for ( i = 0; i < MAXSCANS; i++ ) {
        if ( AM_OpenScans[i].valid == FALSE ) {
            found_pos = TRUE;
            scanDesc = i;
            break;
        }
    }

    /* ean de brhkame kenh 8esh, tote o pinakas einai gematos, ara apotygxanoume. */
    if ( found_pos == FALSE ) {
        AM_errno = AME_SCANTABFULL;
        return AME_SCANTABFULL;
    }

    /* alliws, eisagoume tis plhrofories pou xreiazetai h anoixth sarwsh. */
    AM_OpenScans[scanDesc].valid = TRUE;
    AM_OpenScans[scanDesc].op = op;

    if ( value != NULL ) {
        if (( AM_OpenScans[scanDesc].value = malloc( attrLength ) ) == NULL ) {
            AM_errno = AME_NOMEM;
            return AME_NOMEM;
        }

        memcpy( AM_OpenScans[scanDesc].value, value, attrLength );
    }

    AM_OpenScans[scanDesc].attrType = attrType;
    AM_OpenScans[scanDesc].attrLength = attrLength;
    AM_OpenScans[scanDesc].lastRecord = AM_FIRST_RECORD - 1;
    AM_OpenScans[scanDesc].lastBucketNum = AM_FIRST_BLOCK - 1;
    AM_OpenScans[scanDesc].fileDesc = fileDesc;

    return scanDesc;
}

/**** AM_CloseIndexScan( int scanDesc ) ****

 Input:
        * int scanDesc      H 8esh ston pinaka twn anoixtwn sarwsewn

 Operation:
        * Kleinei thn "scanDesc" 8esh ston pinaka anoixtwn sarwsewn, epanarxikopoiwntas thn.

 Return Value:
        * AME_OK, efoson epityxei
        * AME_INVALIDSCAN, efoson dw8ei la8os ari8mos "scanDesc"
*/

int AM_CloseIndexScan( int scanDesc )
{
    /* Check for valid input. */
    if ( scanDesc < 0 || scanDesc > MAXSCANS || AM_OpenIndeces[scanDesc].valid == FALSE ) {
        AM_errno = AME_INVALIDSCAN;
        return AME_INVALIDSCAN;
    }

    AM_OpenScans[scanDesc].valid = FALSE;
    AM_OpenScans[scanDesc].op = AM_INVALID;

    if ( AM_OpenScans[scanDesc].value != NULL ) {
        free( AM_OpenScans[scanDesc].value );
    }

    AM_OpenScans[scanDesc].value = NULL;
    AM_OpenScans[scanDesc].attrType = FALSE;
    AM_OpenScans[scanDesc].attrLength = AM_INVALID;
    AM_OpenScans[scanDesc].lastRecord = AM_INVALID;
    AM_OpenScans[scanDesc].lastBucketNum = 0;
    AM_OpenScans[scanDesc].fileDesc = AM_INVALID;
    return AME_OK;
}

/**** int AM_FindNextEntry( int scanDesc ) ****

 Input:
        * int scanDesc      H 8esh ston pinaka twn anoixtwn sarwsewn

 Operation:
        * Ektelei sarwsh tou eurethriou pou ths antistoixei me bash tis plhrofories ths sarwshs.
        * Efoson brei timh pou ikanopoiei tis proypo8eseis, thn epistrefei.

 Return Value:
        * Mh arnhtiko ari8mo pou antistoixei se recId, efoson petyxei
        * AME_EOF, efoson de bre8ei epomenh eggrafh pou afora th sarwsh "scanDesc"
        * Kapoio kwdiko sfalmatos, efoson apotyxei
*/

int AM_FindNextEntry( int scanDesc )
{
    char * buffer, * bitmap;
    int fileDesc, recId, i, bitmapSize;
    fileDesc = AM_OpenScans[scanDesc].fileDesc;

    /* Checking for valid input. */
    if ( scanDesc < 0 || scanDesc > MAXSCANS || AM_OpenScans[scanDesc].valid == FALSE ) {
        AM_errno = AME_INVALIDSCAN;
        return AME_INVALIDSCAN;
    }

    /*
        Exoume 2 megales periptwseis:
        - H prwth einai pou h anazhthsh exei operator NOT_EQUAL kai GET_ALL.
          Se autes anagkazomaste na psaksoume olous tous kadous.
        - H deuterh einai pou h anazhthsh exei operator EQUAL.
          Ekei kseroume se poio kado akribws mporei na yparxei h epi8ymhth eggrafh.
        - Allou eidous operators exoun aporrif8ei kata th dhmiourgia ths sarwshs.
    */
    
    if ( AM_OpenScans[scanDesc].op == NOT_EQUAL ||  AM_OpenScans[scanDesc].op == GET_ALL ) {
        /* Ean den exei arxisei to scanning akoma */
        if ( AM_OpenScans[scanDesc].lastRecord == AM_INVALID ) {
            /* pame sth prwth eggrafh tou prwtou block. */
            AM_OpenScans[scanDesc].lastRecord = AM_FIRST_RECORD;
            AM_OpenScans[scanDesc].lastBucketNum = AM_FIRST_BLOCK;
        }
        /* Ean eimaste sth teleutaia eggrafh tou kadou */
        else if ( AM_OpenScans[scanDesc].lastRecord == AM_OpenIndeces[fileDesc].maxRecords ) {
            /* ean eimaste epishs ston teleutaio kado, tote den yparxoun alles eggrafes */
            if ( AM_OpenScans[scanDesc].lastBucketNum == AM_OpenIndeces[fileDesc].totalBlocks ) {
                AM_errno = AME_EOF;
                return AME_EOF;
            }
            /* alliws, proxwrame ston epomeno kado. */
            else {
                AM_OpenScans[scanDesc].lastRecord = AM_FIRST_RECORD;
                AM_OpenScans[scanDesc].lastBucketNum++;
            }
        }
        /* alliws, pame sthn epomenh eggrafh. */
        else {
            AM_OpenScans[scanDesc].lastRecord++;
        }

        /* Fortwnoume to bucket me to opoio arxizei to scanning */
        if (( i = BF_GetThisBlock( fileDesc, AM_OpenScans[scanDesc].lastBucketNum, &buffer ) ) != BFE_OK ) {
            AM_errno = i;
            return i;
        }

        /* To mege8os tou bitmap se bytes einai h orofh tou plh8ous twn eggrafwn dia 8 bits ana byte */
        bitmapSize = AM_OpenIndeces[fileDesc].maxRecords / 8;
        if (( float ) AM_OpenIndeces[fileDesc].maxRecords / ( float ) 8  > bitmapSize ) {
            bitmapSize++;
        }

        /* 8eloume na broume th 8esh tou bitmap sto block */
        bitmap = buffer;
        /* to bitmap vrisketai sto telos tou bucket */
        bitmap += BF_BLOCK_SIZE - bitmapSize;
        
        /* Prospername to topiko vathos. */
        buffer += sizeof( int32_t );
        /* kai pame sth swsth thesh */
        buffer += AM_OpenScans[scanDesc].lastRecord * ( AM_OpenScans[scanDesc].attrLength + sizeof( int32_t ) );

        /* Arxizoume twra to psaksimo ths epomenhs eggrafhs pou mas endiaferei */
        while ( 1 ) {
            /* An h twrinh thesh einai energh sto bitmap */
            if ( !BF_isBitZero( bitmap[AM_OpenScans[scanDesc].lastRecord / 8], AM_OpenScans[scanDesc].lastRecord % 8 ) ) {

                /* psaxnoume me bash ton "op" */
                if ( AM_Compare( buffer, AM_OpenScans[scanDesc].value, AM_OpenScans[scanDesc].attrType, AM_OpenScans[scanDesc].op ) == TRUE ) {

                    /* prospername to "value" kai pame sth dieu8unsh tou recId, to opoio kai epistrefoume. */
                    buffer += AM_OpenScans[scanDesc].attrLength;
                    memcpy( &recId, buffer, sizeof( int32_t ) );

                    /* kai kanoume unpin to twrino bucket sto epipedo BF */
                    if (( i = BF_UnpinBlock( fileDesc, AM_OpenScans[scanDesc].lastBucketNum, FALSE ) ) != BFE_OK ) {
                        AM_errno = i;
                        return i;
                    }

                    return recId;
                }
            }

            /*
               Ean de vrikame timh poy na mas kanei, proxwrame analoga sthn epomenh eggrafh H ston epomeno bucket
            */

            /* Ean eimaste sth teleutaia eggrafh tou kadou */
            if ( AM_OpenScans[scanDesc].lastRecord == AM_OpenIndeces[fileDesc].maxRecords ) {
                /* ean eimaste epishs ston teleutaio kado, tote den yparxoun alles eggrafes */
                if ( AM_OpenScans[scanDesc].lastBucketNum == AM_OpenIndeces[fileDesc].totalBlocks ) {
                    /* kanoume unpin to twrino bucket sto epipedo BF */
                    if (( i = BF_UnpinBlock( fileDesc, AM_OpenScans[scanDesc].lastBucketNum, FALSE ) ) != BFE_OK ) {
                        AM_errno = i;
                        return i;
                    }

                    AM_errno = AME_EOF;
                    return AME_EOF;
                }
                /* alliws, proxwrame ston epomeno kado */
                else {
                    /* kanoume unpin to twrino bucket sto epipedo BF */
                    if (( i = BF_UnpinBlock( fileDesc, AM_OpenScans[scanDesc].lastBucketNum, FALSE ) ) != BFE_OK ) {
                        AM_errno = i;
                        return i;
                    }

                    AM_OpenScans[scanDesc].lastRecord = AM_FIRST_RECORD;
                    AM_OpenScans[scanDesc].lastBucketNum++;

                    /* Fortwnoume to epomeno bucket */
                    if (( i = BF_GetThisBlock( fileDesc, AM_OpenScans[scanDesc].lastBucketNum, &buffer ) ) != BFE_OK ) {
                        AM_errno = i;
                        return i;
                    }

                    /* To bitmap vrisketai sto telos tou bucket */
                    bitmap = buffer;
                    bitmap += BF_BLOCK_SIZE - bitmapSize;

                    /* prospername kai to topiko vathos. */
                    buffer += sizeof( int32_t );
                }
            }
            /* alliws, pame sthn epomenh eggrafh. */
            else {
                AM_OpenScans[scanDesc].lastRecord++;
                buffer += AM_OpenScans[scanDesc].attrLength + sizeof( int32_t );
            }
        }
    }
    else if ( AM_OpenScans[scanDesc].op == EQUAL ) {
        int32_t hash, hashTableIndex, blockPos;

        /* Edw gnwrizoume se poio kado prepei na kanoume thn anazhthsh (me bash to hash) */
        hash = AM_HashFunction( AM_OpenScans[scanDesc].value, AM_OpenScans[scanDesc].attrType );
        hashTableIndex = AM_getFirstBits( hash, AM_OpenIndeces[fileDesc].depth );

        blockPos = AM_OpenIndeces[fileDesc].hashTable[hashTableIndex];

        if (( i = BF_GetThisBlock( fileDesc, blockPos, &buffer ) ) != BFE_OK ) {
            AM_errno = i;
            return i;
        }

        /* Ean den exei arxisei to scanning akoma */
        if ( AM_OpenScans[scanDesc].lastRecord == AM_INVALID ) {
            /* pame sth prwth eggrafh tou block pou mas endiaferei. */
            AM_OpenScans[scanDesc].lastRecord = AM_FIRST_RECORD;
            AM_OpenScans[scanDesc].lastBucketNum = i;
        }
        /* Ean eimaste sth teleutaia eggrafh tou kadou */
        else if ( AM_OpenScans[scanDesc].lastRecord == AM_OpenIndeces[fileDesc].maxRecords ) {
            /* den yparxei h epi8ymhth timh ston kado, ara den yparxei genika */
            AM_errno = AME_EOF;
            return AME_EOF;
        }
        /* Alliws, pame sthn epomenh eggrafh. */
        else {
            AM_OpenScans[scanDesc].lastRecord++;
        }

        bitmapSize = AM_OpenIndeces[fileDesc].maxRecords / 8;
        if (( float ) AM_OpenIndeces[fileDesc].maxRecords / ( float ) 8  > bitmapSize ) {
            bitmapSize++;
        }

        /* To bitmap vrisketai sto telos tou bucket */
        bitmap = buffer;
        bitmap += BF_BLOCK_SIZE - bitmapSize;

        /* Prospername to topiko vathos. */
        buffer += sizeof( int32_t );
        /* kai pame sth swsth thesh */
        buffer += AM_OpenScans[scanDesc].lastRecord * ( AM_OpenScans[scanDesc].attrLength + sizeof( int32_t ) );

        /* Arxizoume twra to psaksimo ths epomenhs eggrafhs pou mas endiaferei */
        while ( 1 ) {
            /* An h twrinh thesh einai energh sto bitmap */
            if ( !BF_isBitZero( bitmap[AM_OpenScans[scanDesc].lastRecord / 8], AM_OpenScans[scanDesc].lastRecord % 8 ) ) {
                /* psaxnoume me bash ton "op" */
                if ( AM_Compare( buffer, AM_OpenScans[scanDesc].value, AM_OpenScans[scanDesc].attrType, AM_OpenScans[scanDesc].op ) == TRUE ) {
                    /* prospername to "value" kai pame sth dieu8unsh tou recId, to opoio kai epistrefoume. */
                    buffer += AM_OpenScans[scanDesc].attrLength;
                    memcpy( &recId, buffer, sizeof( int32_t ) );

                    /* kanoume unpin to twrino bucket sto epipedo BF */
                    if (( i = BF_UnpinBlock( fileDesc, blockPos, FALSE ) ) != BFE_OK ) {
                        AM_errno = i;
                        return i;
                    }

                    return recId;
                }
            }

            /*
               Ean de vrikame timh poy na mas kanei, proxwrame analoga sthn epomenh eggrafh H ston epomeno bucket
            */

            /* Ean eimaste sth teleutaia eggrafh tou kadou */
            if ( AM_OpenScans[scanDesc].lastRecord == AM_OpenIndeces[fileDesc].maxRecords ) {
                /* kanoume unpin to twrino bucket sto epipedo BF */
                if (( i = BF_UnpinBlock( fileDesc, blockPos, FALSE ) ) != BFE_OK ) {
                    AM_errno = i;
                    return i;
                }

                AM_errno = AME_EOF;
                return AME_EOF;
            }
            /* alliws, pame sthn epomenh eggrafh. */
            else {
                AM_OpenScans[scanDesc].lastRecord++;
                buffer += AM_OpenScans[scanDesc].attrLength + sizeof( int32_t );
            }
        }
    }
    else {
        /* La8os operator, auth h periptwsh apokleietai omws logw ths ylopoihshs ths OpenIndexScan(),
           prosti8etai omws gia logous plhrothtas. */
        return 0;
    }
}


/**** void AM_PrintError( char * errString ) ****

 Input:
        * char * errString          To symplhrwmatiko mhnyma sfalmatos pros ektypwsh

 Operation:
        * Ektypwnei to "errString" kai to mhnyma pou antistoixei sth timh tou AM_errno

 Return Value:
        * None
*/

void AM_PrintError( char * errString )
{
    printf( "%s:%s\n", errString, AM_Errors[( AM_errno+1 ) *-1] );
}

/************************
 * Xrhsimes synarthseis *
 ************************/

/**** int AM_checkAttributes ( char attrType, int attrLength ) ****

 Input:
        * char attrType
        * int attrLength

 Operation:
        * Elegxei an to attrType kai to attrLength einai anamesa sta epi8ymhta oria

 Return Value:
        * AME_OK, efoson exoum swstes times
        * AME_INVALIDATTRTYPE/AME_INVALIDATTRLENGTH, sto antistoixo la8os
*/

int AM_checkAttributes( char attrType, int attrLength )
{
    if ( attrType != 'c' && attrType != 'f' && attrType != 'i' ) {
        AM_errno = AME_INVALIDATTRTYPE;
        return AME_INVALIDATTRTYPE;
    }

    if (( attrType == 'f' || attrType == 'i' ) && attrLength != 4 ) {
        AM_errno = AME_INVALIDATTRLENGTH;
        return AME_INVALIDATTRLENGTH;
    }
    else if ( attrType == 'c' && (( attrLength < 1 ) || ( attrLength > 255 ) ) ) {
        AM_errno = AME_INVALIDATTRLENGTH;
        return AME_INVALIDATTRLENGTH;
    }

    return AME_OK;
}


/**** char * AM_convertFileName ( char * fileName, int indexNo ) ****

 Input:
        * char * fileName
        * int indexNo

 Operation:
        * Xrhsimopoiei ta "fileName" kai "indexNo" gia na ftiaksei to filename "fileName.indexNo" tou eurethriou,
          efoson ikanopoiei orismenes proypo8eseis. Desmeuei kai ton aparaithto xwro sth mnhmh.

 Return Value:
        * Th 8esh mnhmhs sthn opoia einai apo8hkeumeno to onoma, efoson epityxei
        * Kapoio kwdiko sfalmatos, efoson apotyxei
*/

char * AM_convertFileName( char * fileName, int indexNo )
{
    int32_t fileNameSize;
    char * indexFileName;

    /* Check for valid input. */
    if ( fileName == NULL ) {
        AM_errno = AME_INVALIDFILENAME;
        return NULL;
    }

    if ( indexNo < 0 ) {
        AM_errno = AME_INVALIDINDEXNO;
        return NULL;
    }

    /* To megethos tou string isoutai me:
       To mege8os tou onomatos tou HeapFile + 1 gia to '.' kai + 11 gia ta megista psifia enos int + 1 gia to \0 */
    fileNameSize = strlen( fileName ) + 13;

    if ( fileNameSize > FILENAME_MAX ) {
        AM_errno = AME_INVALIDFILENAME;
        return NULL;
    }

    indexFileName = malloc( fileNameSize * sizeof( char ) );

    if ( indexFileName == NULL ) {
        AM_errno = AME_NOMEM;
        return NULL;
    }

    sprintf( indexFileName, "%s.%d", fileName, indexNo );
    return indexFileName;
}


/**** void AM_addBucketToHashTable ( int fileDesc, int * hashTableIndex, char * buffer, int bucketPos ) ****

 Input:
         * int fileDesc
         * int * hashTableIndex
         * char * buffer
         * int bucketPos

 Operation:
        * Pros8etei enan kado sto hash table sth 8esh "hashTableIndex"

 Return Value:
        * None
*/

void AM_addBucketToHashTable( int fileDesc, int * hashTableIndex, char * buffer, int bucketPos )
{
    int k, total, localDepth;
    
    /* To local depth brisketai sta sizeof(int32_t) prwta bytes tou kadou */
    memcpy( &localDepth, buffer, sizeof( int32_t ) );

    /*
       Analoga me to topiko ba8os tou bucket, bazoume tous antistoixous pointers na deixnoun ston neo kado.
       Autoi einai synolika 2^k, opoy k = "Oliko vathos - topiko vathos"
    */
    k = AM_OpenIndeces[fileDesc].depth - localDepth;
    total = AM_powOf2( k );

    /* Anathetoume tous 2^k pointers ston idio kado. */
    while ( total > 0 ) {
        AM_OpenIndeces[fileDesc].hashTable[*hashTableIndex] = bucketPos;
        (*hashTableIndex)++;
        total--;
    }
}


/**** int AM_createHashTable ( int fileDesc, char * pch ) ****

 Input:
        * int fileDesc
        * char * pch

 Operation:
        * Dhmiourgei ton pinaka katakermatismou pou antistoixei sthn anoixth sarwsh sth 8esh
          "fileDesc" tou pinaka anoixtwn eurethriwn.

 Return Value:
        * AME_OK, efoson epityxei
        * Kapoio kwdiko la8ous, efoson apotyxei
*/

int AM_createHashTable( int fileDesc, char * pch )
{
    int32_t hashTableIndex, i, j, hashTableSize;
    char * tempTable, temp;
    
    /* Desmeuoume xwro gia na fortwsoume to hash table, synolika apaitei 2^"Oliko_vathos" pointers */
    hashTableSize = AM_powOf2( AM_OpenIndeces[fileDesc].depth );

    if (( AM_OpenIndeces[fileDesc].hashTable = malloc( hashTableSize * sizeof( char ) ) ) == NULL ) {
        AM_errno = AME_NOMEM;
        return AME_NOMEM;
    }

    /* Xreiazomaste kai enan pinaka ston opoio tha apo8hkeusoume ta blocks tou eurethriou */
    if (( tempTable = malloc( AM_OpenIndeces[fileDesc].totalBlocks * sizeof( char ) ) ) == NULL ) {
        AM_errno = AME_NOMEM;
        return AME_NOMEM;
    }

    /*
        Gemizoume ton tempTable me oles tis *monadikes* theseis twn kadwn sto arxeio.
        Dhladh, gia kathe kado, tha yparxei mono mia fora h blockPosition tou ston tempTable,
        se antithesh me ton hashTable pou mporei na exei polles fores thn idia blockPosition
        (epeidh polles theseis deixnoun ston idio kado).
        
        An to hashTable einai {1, 3, 2, 2} kai exoume split tou block me arithmo 1, tote to 
        tempTable tha ginei {1, 4, 3, 2}. Dhladh tha periexei tous arithmous twn blocks me th 
        seira me thn opoia tha prepei na ksanaginei to moirasma twn pointers. Oi prwtoi pointers 
        tou pinaka tha pane sto 1o analoga me to local depth, meta sto 4o tou k.o.k.        
    */
    
    /* To AM header exei hdh ftiaxtei, opote ksekiname apo to block #1 */
    i = j = 1;
    memset( tempTable, -1, AM_OpenIndeces[fileDesc].totalBlocks * sizeof( char ) );

    /* O pointer pch deixnei sth 8esh sto block-header, sthn opoia yparxei/ksekinaei to hashTable. */
    memcpy( &tempTable[0], pch, sizeof( char ) );
    pch += sizeof( char );

    /* Gia ola ta blocks tou arxeiou, koitame se poio block deixnei h sygkekrimenh 8esh, kai metaferoume katallhla */
    while ( j < AM_OpenIndeces[fileDesc].totalBlocks ) {
          memcpy( &temp, pch, sizeof( char ) );

          if ( tempTable[j-1] != temp ) {
             tempTable[j] = temp;
             j++;
          }

          pch += sizeof( char );
          i++;
    }

    /*
       Fortwnoyme olous tous kadous tou eurethriou sthn endiamesh mnhmh
    */
    hashTableIndex = 0;

    for ( j = 0; j < AM_OpenIndeces[fileDesc].totalBlocks; j++ ) {
        if (( i = BF_GetThisBlock( fileDesc, tempTable[j], &pch ) ) == BFE_OK ) {
            AM_addBucketToHashTable( fileDesc, &hashTableIndex, pch, tempTable[j] );
        }
        else {
            AM_errno = i;
            free( tempTable );
            return i;
        }

        if ( ( i = BF_UnpinBlock( fileDesc, tempTable[j], FALSE ) ) != BFE_OK ) {
            AM_errno = i;
            free( tempTable );
            return i;
        }
    }

    free( tempTable );
    return AME_OK;
}

/**** struct AM_Bucket * AM_bufferToBucket ( char * buffer, int fileDesc, int blockPos ) ****

 Input:
        * char * buffer
        * int fileDesc
        * int blockPos

 Operation:
        * Metatrepei enan char buffer pinaka se struct typoy AM_Bucket

 Return Value:
        * Epistrefei to apo8hkeumeno sth mnhmh bucket, an epityxei
        * NULL, an apotyxei
*/

struct AM_Bucket * AM_bufferToBucket( char * buffer, int fileDesc, int blockPos ) {
    struct AM_Bucket * bucket;
    int32_t bitmapSize;

    if (( bucket = malloc( sizeof( struct AM_Bucket ) ) ) == NULL ) {
        AM_errno = AME_NOMEM;
        return NULL;
    }

    bucket->buffer = buffer;

    /* To local depth einai ta sizeof(int32_t) prwta bytes tou kadou */
    memcpy( &bucket->localDepth, buffer, sizeof( int32_t ) );

    /* To mege8os tou bitmap se bytes einai h orofh tou plh8ous twn eggrafwn dia 8 bits ana byte */
    bitmapSize = AM_OpenIndeces[fileDesc].maxRecords / 8;
    if (( float ) AM_OpenIndeces[fileDesc].maxRecords / ( float ) 8  > bitmapSize ) {
        bitmapSize++;
    }

    /* Arxikopoioume to bucker->bitmap me bash to bitmap tou buffer
       (to bitmap brisketai sto telos tou block */
    buffer += BF_BLOCK_SIZE - bitmapSize;
    if (( bucket->bitmap = malloc( bitmapSize * sizeof( char ) ) ) == NULL ) {
        AM_errno = AME_NOMEM;
        return NULL;
    }
    memcpy( bucket->bitmap, buffer, bitmapSize );

    bucket->bitmapSize = bitmapSize;
    bucket->blockNum = blockPos;

    return bucket;
}


/**** int AM_insertInBucket ( struct AM_Bucket * bucket, char *value, int recId, int maxRecords, int attrLength ) ****

 Input:
        * struct AM_Bucket * bucket
        * char *value
        * int recId
        * int maxRecords
        * int attrLength

 Operation:
        * Ektelei thn eisagwgh ths eggrafhs <*value, recId> ston kado "*bucket", efoson yparxei xwros. An den yparxei,
          tote epistrefei AM_SPLIT kai eidopoiei thn kalousa sunarthsh pws apaiteitai spasimo tou bucket.

 Return Value:
        * AME_OK, efoson oloklhrw8hke h eisagwgh ths eggrafhs ston kado
        * AM_SPLIT, efoson en yparxei xwros ston kado kai prepei na xwristei
*/

int AM_insertInBucket( struct AM_Bucket * bucket, char *value, int recId, int maxRecords, int attrLength )
{
    int i;
    char * pch;

    /* Psaxnoume na vroume kenh thesh sto bitmap */
    for ( i = 0; i < maxRecords; i++ ) {
        /* an vroume mia kenh thesh */
        if ( BF_isBitZero( bucket->bitmap[i/8], i % 8 ) ) {

            /* thetoume to antistoixo bit se 1 */
            BF_bitSet(( int * ) &bucket->bitmap[i/8], i % 8 );
            /* kai pame na grapsoume thn eggrafh mas mesa sto kado */
            pch = bucket->buffer;

            /* Prospername to topiko vathos */
            pch += sizeof( int32_t );
            /* prospername eggrafes gia na pame sth swsth thesh */
            pch += i * ( attrLength + sizeof( int32_t ) );
            /* grafoume prwta thn timh value kai meta to kleidi recId */
            memcpy( pch, value, attrLength );
            pch += attrLength;
            memcpy( pch, &recId, sizeof( int32_t ) );

            /* kai telos mas menei na ksanagrapsoume to neo bitmap ston buffer tou epipedou BF */
            pch = bucket->buffer;
            pch += BF_BLOCK_SIZE - bucket->bitmapSize;
            memcpy( pch, bucket->bitmap, bucket->bitmapSize );
            return AME_OK;
        }
    }

    /* An den yparxei xwros sto kado, prepei na diaspastei se 2 */
    return AM_SPLIT;
}


/**** int AM_deleteFromBucket ( struct AM_Bucket * bucket, char *value, int recId, int maxRecords, int attrLength, char attrType ) ****

 Input:
        * struct AM_Bucket * bucket
        * char *value
        * int recId
        * int maxRecords
        * int attrLength
        * char attrType

 Operation:
        * Diagrafei thn eggrafh <*value, recId> apo ton kado "*bucket"

 Return Value:
        * AME_OK, efoson oloklhrw8ei h diagrafh
        * AME_INVALIDDELETE, efoson de bre8ei h epi8ymhth eggrafh
*/

int AM_deleteFromBucket( struct AM_Bucket * bucket, char *value, int recId, int maxRecords, int attrLength, char attrType )
{
    int i;
    char * pch;
    int reallyExists = FALSE;

    /* Psaxnoume na vroume energes 8eseis sto bitmap */
    for ( i = 0; i < maxRecords; i++ ) {
        /* an vroume xrhsimopoioumenh thesh */
        if ( !BF_isBitZero( bucket->bitmap[i/8], i % 8 ) ) {

            /* Pame na elegksoume thn antistoixh eggrafh ston kado */
            pch = bucket->buffer;
            /* prospername to topiko vathos */
            pch += sizeof( int32_t );
            /* prospername eggrafes gia na pame sth swsth thesh */
            pch += i * ( attrLength + sizeof( int32_t ) );

            /* elegxoume an vrhkame thn eggrafh <*value, recId> */
            if ( AM_Compare( pch, value, attrType, EQUAL ) == TRUE ) {
                pch += attrLength;

                if ( AM_Compare( pch, &recId, 'i', EQUAL ) == TRUE ) {
                    /* Tote brhkame pragmati thn eggrafh <*value, recId>, opote th sbhnoume */
                    pch -= attrLength;
                    memset( pch, 0, attrLength + sizeof( int32_t ) );

                    /* kai mhdenizoume thn antistoixh 8esh sto bitmap */
                    BF_bitClear(( int * ) &bucket->bitmap[i/8], i % 8 );

                    /* twra mas menei na ksanagrapsoume to neo bitmap ston buffer tou epipedou BF */
                    pch = bucket->buffer;
                    pch += BF_BLOCK_SIZE - bucket->bitmapSize;
                    memcpy( pch, bucket->bitmap, bucket->bitmapSize );
                    reallyExists = TRUE;
                    break;
                }
                else {
                    pch -= attrLength;
                }
            }
            else {
                continue;
            }
        }
    }

    if ( reallyExists == TRUE ) {
        /* Sbhsthke epityxws zeugari */
        return AME_OK;
    }
    else {
        AM_errno = AME_INVALIDDELETE;
        return AME_INVALIDDELETE;
    }
}


/**** int AM_Split( int fileDesc, struct AM_Bucket * splitBucket, char *value, int recId ) ****

 Input:
        * int fileDesc
        * struct AM_Bucket * splitBucket
        * char *value
        * int recId

 Operation:
        * Auth h sunarthsh exei to dyskolo ergo tou na "spasei" enan kado se dyo".
        * An apaiteitai diplasiasmos tou pinaka, ton ektelei kai auton.
        * Einai epishs ypeu8unh gia thn anakatanomh twn eggrafwn stous kadous.

 Return Value:
        * AME_OK, efoson oloklhrw8ei epityxws
        * Kapoio kwdiko sfalmatos, efoson apotyxei
*/

int AM_Split( int fileDesc, struct AM_Bucket * splitBucket, char *value, int recId )
{
    int32_t i, j, blockPos, localDepth, k, total, hashTableIndex;
    char * tempTable;
    struct AM_Record * records;
    char * buffer, * pch;

    /*
         * Elegxoyme to topiko bathos tou kadou pou theloume na diaspastei:
            - An isoutai me MAX_GLOBAL_DEPTH, tote den epitrepoume na kanoume splitting
            - An isoutai me to oliko vathos, tote prepei na auksh8ei to oliko ba8os kai na 2plasiastei o hash table.
    */
    if ( AM_OpenIndeces[fileDesc].depth == splitBucket->localDepth ) {
        int32_t newSize;

        /* Yparxei ena orio sto oliko ba8os pou prepei na ikanopoieitai */
        if ( AM_OpenIndeces[fileDesc].depth == AM_MAX_GLOBAL_DEPTH ) {
             AM_errno = AME_MAXGLOBALDEPTH;
             return AME_MAXGLOBALDEPTH;
        }
        
        /* Auksanoume to oliko vathos + 1 */
        AM_OpenIndeces[fileDesc].depth++;
        /* kai diplasiazoume ton pinaka katakermatismou */
        newSize = AM_powOf2( AM_OpenIndeces[fileDesc].depth );

        AM_OpenIndeces[fileDesc].hashTable = realloc( AM_OpenIndeces[fileDesc].hashTable, newSize * sizeof( char ) );

        if ( AM_OpenIndeces[fileDesc].hashTable == NULL ) {
            AM_errno = AME_NOMEM;
            return AME_NOMEM;
        }
    }

    /* Auksanoume ta totalBlocks logo tou neou kadou pou tha dimiourgithei */
    AM_OpenIndeces[fileDesc].totalBlocks++;

    /* dhmioyrgoyme to neo kado sto epipedo BF, afou kanoume unpin to splitBucket se periptwsh pou exoume xwro mono gia
       ena kado sth mnhmh */
    if (( i = BF_UnpinBlock( fileDesc, splitBucket->blockNum, TRUE ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    if (( i = BF_AllocBlock( fileDesc, &blockPos, &buffer ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    if (( i = BF_UnpinBlock( fileDesc, blockPos, FALSE ) ) != BFE_OK ) {
        AM_errno = i;
        return i;
    }

    /*
       Dhmioyrgoyme enan pinaka poy periexei tis theseis olwn twn kadwn sto arxeio.
       Autos o pinakas tha xrhsimopoithei parakatw pou tha prepei na ksanamoirasoume tous pointers
       tou pinaka katakermatismou se olous tous kadous tou arxeiou.
    */

    if (( tempTable = malloc( AM_OpenIndeces[fileDesc].totalBlocks * sizeof( char ) ) ) == NULL ) {
        AM_errno = AME_NOMEM;
        return AME_NOMEM;
    }

    /*
       Gemizoume ton tempTable me oles tis *monadikes* theseis twn kadwn sto arxeio.
       Dhladh gia kathe kado tha yparxei mono mia fora h blockPosition tou ston tempTable,
       se antithesh me ton hashTable pou mporei na exei polles fores thn idia blockPosition
       epeidh polles theseis deixnoun ston idio kado.
       
       An to hashTable einai {1, 3, 2, 2} kai exoume split tou block me arithmo 1, tote to 
       tempTable tha ginei {1, 4, 3, 2}. Dhladh tha periexei tous arithmous twn blocks me th 
       seira me thn opoia tha prepei na ksanaginei to moirasma twn pointers. Oi prwtoi pointers 
       tou pinaka tha pane sto 1o analoga me to local depth, meta sto 4o tou k.o.k.
    */

    /* Gemisma tou tempTable */
    i = j = 0;
    memset( tempTable, -1, AM_OpenIndeces[fileDesc].totalBlocks * sizeof( char ) );

    /*
      Ekteleitai epanalhpsh gia olous tous kadous, stis opoies gemizei katallhla o tempTable me bash ton hash table:
      - An den yparxei, tote proxwrame ston epomeno kado
      - An yparxei, tote:
           - An autos einai o kados pou molis "espase", autos sth 8esh j tha isoutai me ton arxiko kado
           (prin to split) kai o epomenos tou me ton neo (ton "blockPos", pou dhmiourgh8hke meta to split)
           - An oxi, tote apla antigrafetai h 8esh tou antistoixou kadou
      Me auton ton tropo, diplasiazontai oi 8eshs tou table pou deixnoun sto idio block, kai frontizoume tautoxrona
      gia ton kado pou xwristhke se 2.
    */
    while ( j < AM_OpenIndeces[fileDesc].totalBlocks ) {
        if ( AM_notExists( AM_OpenIndeces[fileDesc].hashTable[i], tempTable, AM_OpenIndeces[fileDesc].totalBlocks ) )  {
            if ( AM_OpenIndeces[fileDesc].hashTable[i] == splitBucket->blockNum ) {
                tempTable[j] = AM_OpenIndeces[fileDesc].hashTable[i];
                j++;
                tempTable[j] = blockPos;
                j++;
            }
            else {
                tempTable[j] = AM_OpenIndeces[fileDesc].hashTable[i];
                j++;
            }
        }

        i++;
    }

    /*
        8eloume na aukshsoume to local depth tou splitBucket kata ena
    */
    if (( i = BF_GetThisBlock( fileDesc, splitBucket->blockNum, &buffer ) ) != BFE_OK ) {
        AM_errno = i;
        free( tempTable );
        return i;
    }

    /* ta prwta sizeof(int32_t) bytes einai to local depth */
    memcpy( &localDepth, buffer, sizeof( int32_t ) );
    localDepth++;
    memcpy( buffer, &localDepth, sizeof( int32_t ) );

    if (( i = BF_UnpinBlock( fileDesc, splitBucket->blockNum, TRUE ) ) != BFE_OK ) {
        AM_errno = i;
        free( tempTable );
        return i;
    }

    /* To idio gia ton neo bucket */
    if (( i = BF_GetThisBlock( fileDesc, blockPos, &buffer ) ) != BFE_OK ) {
        AM_errno = i;
        free( tempTable );
        return i;
    }

    memcpy( buffer, &localDepth, sizeof( int32_t ) );

    if (( i = BF_UnpinBlock( fileDesc, blockPos, TRUE ) ) != BFE_OK ) {
        AM_errno = i;
        free( tempTable );
        return i;
    }

    /* Ksanamoirazoume tous pointers tou pinaka katakermatismou stous kadous */
    /* Gia kathe arxeio ston tempTable, to fortwnoume gia na doume to local depth tou.
       Meta orizoume 2^k pointers apo ton pinaka katakermatismou na deixnoun se auto!
       opoy k = Oliko vathos - topiko vathos */
    i = 0;
    hashTableIndex = 0;

    while ( i < AM_OpenIndeces[fileDesc].totalBlocks ) {
        /* diavazoume to local depth */
        if (( k = BF_GetThisBlock( fileDesc, tempTable[i], &buffer ) ) != BFE_OK ) {
            AM_errno = k;
            free( tempTable );
            return k;
        }

        memcpy( &localDepth, buffer, sizeof( int32_t ) );
        k = AM_OpenIndeces[fileDesc].depth - localDepth;
        total = AM_powOf2( k );
        /* kai orizoume tous 2^k pointers */
        j = 0;

        while ( j < total ) {
            AM_OpenIndeces[fileDesc].hashTable[hashTableIndex] = tempTable[i];
            j++;
            hashTableIndex++;
        }

        if (( k = BF_UnpinBlock( fileDesc, tempTable[i], FALSE ) ) != BFE_OK ) {
            AM_errno = k;
            free( tempTable );
            return k;
        }

        i++;
    }

    /* Prin ksanaeisagoume ola ta records tou splitBucket, kathws kai to kainoyrgio, ta eisagoume s enan proswrino pinaka */

    if (( records = malloc(( AM_OpenIndeces[fileDesc].maxRecords + 1 ) * sizeof( struct AM_Record ) ) ) == NULL ) {
        AM_errno = AME_NOMEM;
        free( tempTable );
        return AME_NOMEM;
    }

    /* fortwnoume to splitBucket apo to epipedo BF */
    if (( i = BF_GetThisBlock( fileDesc, splitBucket->blockNum, &buffer ) ) != BFE_OK ) {
        AM_errno = i;
        free( tempTable );
        free( records );
        return i;
    }

    /* Gia kathe energo record tou splitBucket, to eisagoume sto pinaka */
    blockPos = splitBucket->blockNum;
    free( splitBucket->bitmap );
    free( splitBucket );
    
    if ( (splitBucket = AM_bufferToBucket( buffer, fileDesc, blockPos )) == NULL) {
       return AM_errno;
    }
    j = 0;

    for ( i = 0 ; i < AM_OpenIndeces[fileDesc].maxRecords; i++ ) {
        if ( !BF_isBitZero( splitBucket->bitmap[i/8], i % 8 ) ) {
            pch = buffer;
            /* prospername to topiko vathos */
            pch += sizeof( int32_t );
            /* prospername oles tis eggrafes gia na pame sth swsth thesh */
            pch += i * ( AM_OpenIndeces[fileDesc].attrLength + sizeof( int32_t ) );

            /* antigrafoume to record ston pinaka twn records */
            if (( records[j].val = malloc( AM_OpenIndeces[fileDesc].attrLength * sizeof( char ) ) ) == NULL ) {
                AM_errno = AME_NOMEM;

                for ( k = 0; k < j; k++ ) {
                    free( records[k].val );
                }

                free( tempTable );
                free( records );
                return AME_NOMEM;
            }

            memcpy( records[j].val, pch, AM_OpenIndeces[fileDesc].attrLength );
            pch += AM_OpenIndeces[fileDesc].attrLength;
            memcpy( &records[j].recId, pch, sizeof( int32_t ) );
            j++;
        }
    }

    /* gia to neo record pou den xwrese prin */
    records[j].val = value;
    records[j].recId = recId;
    j++;
    /* mhdenizoume teleiws to splitBucket, ektos apo to arxiko localDepth */
    memset( buffer + sizeof( int32_t ), 0, BF_BLOCK_SIZE - sizeof( int32_t ) );

    /* kanoyme unpin to splitBucket sto epipedo BF */
    if (( i = BF_UnpinBlock( fileDesc, splitBucket->blockNum, TRUE ) ) != BFE_OK ) {
        AM_errno = i;
        AM_errno = AME_NOMEM;

        for ( k = 0; k < AM_OpenIndeces[fileDesc].maxRecords; k++ ) {
            free( records[k].val );
        }

        free( tempTable );
        free( records );
        return i;
    }

    /* ksanaeisagoume ola ta stoixeia tou pinaka kathws kai to neo record */

    for ( i = 0; i < j; i++ ) {
        if (( k = AM_InsertEntry( fileDesc, AM_OpenIndeces[fileDesc].attrType, AM_OpenIndeces[fileDesc].attrLength, records[i].val, records[i].recId ) ) != AME_OK ) {
            for ( k = 0; k < AM_OpenIndeces[fileDesc].maxRecords; k++ ) {
                free( records[k].val );
            }

            free( tempTable );
            free( records );
            return k;
        }
    }

    /* eleytherwnoume osh dynamikh mnhmh desmeysame */
    for ( i = 0 ; i <= AM_OpenIndeces[fileDesc].maxRecords; i++ ) {
        free( records[i].val );
    }

    free( tempTable );
    free( records );
    return AME_OK;
}


/**** int32_t AM_HashFunction( int key ) ****

 Input:
        * int key       To kleidi pou pername sth hash function

 Operation:
        * A hash function does what a hash function  is supposed to do!

 Return Value:
        * To hash pou antistoixei sto key
*/

int32_t AM_HashFunction( void *key, char type )
{
    int32_t hash;

    if ( type == 'i' || type == 'f' ) {
        /* Credits to Robert Jenkins */
        /* Debug: LEEEEEEEROOOOOOOOOOOOOOOOOOYYY, JJJJEEEEEEENKIIIIIIIIIIINSSSSSS */
        hash = * ( int* ) key;
        hash = ( hash + 0x7ed55d16 ) + ( hash << 12 );
        hash = ( hash ^ 0xc761c23c ) ^( hash >> 19 );
        hash = ( hash + 0x165667b1 ) + ( hash << 5 );
        hash = ( hash + 0xd3a2646c ) ^( hash << 9 );
        hash = ( hash + 0xfd7046c5 ) + ( hash << 3 );
        hash = ( hash ^ 0xb55a4f09 ) ^( hash >> 16 );
    }
    else if ( type == 'c' ) {
        /* Credits to Daniel J. Bernstein */
        hash = 5381;

        for ( ; *( char* ) key != '\0' ; key = ( char* ) key + sizeof( char ) ) {
            hash = (( hash << 5 ) + hash ) + * ( char* ) key;
        }
    }

    return hash;
}


/**** int AM_getFirstBits( int number, int32_t numBits ) ****

 Input:
        * int number
        * int32_t numBits

 Operation:
        * Epistrefei ta "numBits" prwta bits tou ari8mou "number"

 Return Value:
        * Ta "numBits" prwta bits tou ari8mou "number"
*/

int AM_getFirstBits( int number, int32_t numBits )
{
    int32_t temp = 0, i;
    int bnumber[32];
    int sum = 0;

    if ( numBits == 1 ) {
        sum = !BF_isBitZero( number, i );
        return sum;
    }

    /* Fortwsh twn bits */
    for ( i = 0; i < numBits; i++ ) {
        if ( BF_isBitZero( number, i ) ) {
            bnumber[i] = 0;
        }
        else {
            bnumber[i] = 1;
        }
    }

    /* Reverse num */
    for ( i = 0; i < numBits / 2; i++ ) {
        temp = bnumber[i];
        bnumber[i] = bnumber[numBits - i - 1];
        bnumber[numBits - i - 1] = temp;
    }
    
    /* Metatroph se dekadiko */
    for ( i = 0; i < numBits; i++ ) {
        if ( bnumber[i] ) {
            sum += AM_powOf2( i );
        }
    }

    return sum;
    
    /*
         Pi8anh allh ylopoihsh (mh dokimasmenh):
           return ( number >> (32-numBits) );
    */
}


/**** int AM_notExists( int val, int * tempTable, int totalBlocks ) ****

 Input:
        * int val
        * int * tempTable
        * int totalBlocks

 Operation:
        * Elegxei gia ta "totalBlocks" kelia tou pinaka "tempTable" an kapoio
          exei th timh "val".

 Return Value:
        * FALSE, an yparxei h timh val ston pinaka tempTable
        * TRUE, an DEN yparxei
*/

int AM_notExists( char val, char * tempTable, int totalBlocks )
{
    int i;

    for ( i = 0; i < totalBlocks; i++ ) {
        if ( tempTable[i] == val ) {
            return FALSE;
        }
    }

    return TRUE;
}


/**** int AM_Compare(void * value1, void * value2, char type, int op) ****

 Input:
        * void * value1     H dieu8unsh ths prwths timhs pros sygkrish
        * void * value2     H dieu8unsh ths deuterhs timhs pros sygkrish
        * char type         O typos twn timwn
        * int op            O telesths sygkrishs

 Operation:
        * Sygkrinei tis times "value1" kai "value2" symfwna me ton typo tous "type"
          kai ton telesth sygkrishs "op".

 Return Value:
        * TRUE, efoson isxuei h zhtoumenh sygkrish
        * FALSE, efoson oxi
*/

int AM_Compare( void * value1, void * value2, char type, int op )
{
    int result;

    /* Otan exei dwsei NULL sto value, tote prepei na epistrepsoume oles tis eggrafes */
    if ( op == GET_ALL ) {
        return TRUE;
    }

    switch ( type ) {
        case 'c':
            result = strcmp( value1, value2 );
            break;
        case 'i':
        case 'f':
            /* De mas apasxoloun zhthmata Endianess */
            /* 8ewroume (apo ekfwnhsh) pws sizeof(int)=sizeof(float)=4 */
            result = memcmp( value1, value2, sizeof( int ) );
            break;
    }

    /*
        Elegxoume th timh tou result me bash to operation pou ekteloume.
        An den isxuei to antistoixo operation, tote epistrefoume FALSE.
        Operations ektos ths isothtas/anisothtas ypodeix8hke na mhn yposthrizontai,
        kai "aporriptontai" kata to anoigma ths sarwshs, ginetai omws kai edw elegxos
        gia logous plhrothtas.
    */
    if ( op == EQUAL && result == 0 )
        return TRUE;
    else if ( op == NOT_EQUAL && result != 0 )
        return TRUE;
    else
        return FALSE;
}


/**** AM_powOf2( int power ) ****

 Input:
        * int power         O ek8eths ths dynamhs tou 2 pou tha ypologisoume

 Operation:
        * Ypologizei to 2^power

 Return Value:
        * AME_INVALIDVALUE, efoson dw8ei mh apodektos ek8eths
        * Th timh ths epi8ymhths dynamhs tou 2, efoson epityxei
*/

int AM_powOf2( int power )
{
    /* Apodektes times gia signed int: 0-30 */
    if ( power < 0 || power >= 31 ) {
        AM_errno = AME_INVALIDVALUE;
        return AME_INVALIDVALUE;
    }

    return ( 1 << power );
}
