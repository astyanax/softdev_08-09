#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "BF_Lib.h"
#include "BF_BitOperations.h"

/***********
 * STRUCTS *
 ***********/

/* H sygkekrimenh domh krata tis plhrofories pou aforoun ka8e anoigma arxeiou */
struct BF_OpenFile
{
	char * filename;    /* To onoma tou arxeiou */
	char valid;         /* TRUE an xrhsimopoieitai to sygkekrimeno block. An oxi, FALSE. Dhladh: egkyro == xrhsimopoieitai */
	FILE * fp;          /* Deikths sto arxeio me onoma filename */
	short int header;   /* Index sth 8esh tou global pinaka "BF_Headers" pou periexei ta File Headers */
	char fileDescs[4];	/* Bitmap poy periexei indeces gia ton pinaka "BF_InterBuffer",
						   ta opoia antistoixoun se blocks tou sugkekrimenou anoigmatos arxeiou */
	
};

/* H sygkekrimenh domh krata tis plhrofories pou aforoun ka8e block pou fortwnetai sthn endiamesh mnhmh */
struct BF_InterMemBuffer
{
	char buffer[BF_BLOCK_SIZE];	/* H mnhnh mege8ous BF_BLOCK_SIZE sthn opoia fortwnetai to block */
	char valid;                 /* TRUE an yparxei fortwmeno block se auth th 8esh. An oxi, FALSE. Dhladh: egkyro == xrhsimopoieitai */
	char dirty;                 /* TRUE an exoun allaksei ta periexomena tou block. An oxi, FALSE. */
	int32_t blockNum;           /* H 8esh tou block sto arxeio */
	int32_t timestamp;          /* Timestamp pou dhlwnei to xrono teleutaias xrhshs tou block */
	char fileDescs[4];          /* Bitmap poy periexei indeces gia ton pinaka "BF_FileTable",
								   ta opoia antistoixoun se anoigmata arxeiwn pou to exoun kanei pin */
};

/***********
 * GLOBALS *
 ***********/

char BF_errno;  /* Metablhth pou krata ton teleutaio kwdiko la8ous */
int32_t time;   /* Metablhth pou krataei ton "twrino xrono" */

/* Pinakas pou tha krata info gia ka8ena ap'ta MAXOPENFILES anoigmata arxeiwn */
struct BF_OpenFile	  BF_FileTable[MAXOPENFILES];

/* Buffer mege8ous BF_BUFFER_SIZE blocks (default: 20) pou ton onomazoume "Endiamesh mnhmh" */
struct BF_InterMemBuffer BF_InterMemory[BF_BUFFER_SIZE];

/* 
   * Pinakas pou krataei ta header blocks pou antistoixoun se ka8e anoigma arxeiou,
   * To prwto bit xrhsimopoieitai san tis metavlites valid gia to an yparxei fortwmeno header pou xrhsimopoieitai 
     sth sygkekrimenh thesh
 */
char BF_Headers[MAXOPENFILES][BF_BLOCK_SIZE+1];

/* Pinakas pou krata ta mhnymata la8ous */
char BF_Errors[][64] =
{
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
	"Invalid Filename Specified",
	"File already exists",
	"Can't close if pinned blocks still exist",
	"Not enough space in bitmap",
	"An invalid value was passed"
};

/************************
 * Function definitions *
 ************************/

/**** void BF_Init( void ) ****

 Input:
        * None

 Operation:
        * Arxikopoihsh twn global metablhtwn kai domwn

 Output:
        * None

 Return Value:
        * None
*/

void BF_Init( void )
{
	int32_t i;

	/* Me thn enarksh ths leitourgias ths DB mas ksekinaei kai o metrhths xronou. */
	time = 0;
	
	/* Arxikopoihsh twn domwn pou aforoun ta arxeia. */
	for ( i=0 ; i < MAXOPENFILES ; i++ ) {
		BF_FileTable[i].valid = FALSE;
		BF_Headers[i][0] = FALSE;
		memset(&BF_FileTable[i].fileDescs, 0, sizeof(BF_FileTable[i].fileDescs));
	}
	
	/* Arxikopoihsh ths endiameshs mnhmhs. */
	for ( i=0 ; i < BF_BUFFER_SIZE ; i++ ) {
		BF_InterMemory[i].valid = FALSE;
		BF_InterMemory[i].dirty = FALSE;
		BF_InterMemory[i].blockNum = BF_INVALID;
		memset(&BF_InterMemory[i].fileDescs, 0, sizeof(BF_InterMemory[i].fileDescs));
	}
	
}


/**** int BF_CreateFile( char * fileName ) ****

 Input:
        * char * fileName           To onoma tou arxeiou pou tha dhmiourgh8ei

 Operation:
        * Dhmiourgei to arxeio me onoma "filename"
		* Grafei ena keno header se auto

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_CreateFile( char * fileName )
{
	FILE * fp;
	char block[BF_BLOCK_SIZE];
	
	/* Periorismos tou onomatos tou arxeiou. */
	if ( (fileName == NULL) || (strlen(fileName) > FILENAME_MAX-1) ) {
		BF_errno = BFE_INVALIDFILENAME;
		return BFE_INVALIDFILENAME;
	}

	/* An yparxei hdh to arxeio, den epitrepetai na to kanoyme overwrite. */
	if( ( fp = fopen(fileName, "rb") ) != NULL ) {
		if( fclose(fp) == EOF ) {
	        BF_errno = BFE_OS;
		    return BFE_OS;
		}
		
        BF_errno = BFE_FILEEXISTS;
	    return BFE_FILEEXISTS;
	}
	
	/* To arxeio den yparxei, opote mporoume na to dhmiourghsoume. */
	if( ( fp = fopen(fileName, "wb") ) == NULL ) {
        BF_errno = BFE_OS;
	    return BFE_OS;
	}

    /* Grafoume ena keno header sto arxeio. */
    memset(&block, 0, sizeof(block));
    
	if ( fwrite(&block, sizeof(char), sizeof(block), fp) != sizeof(block) ) {
		BF_errno = BFE_OS;
		return BFE_OS;
	}

	if( fclose(fp) == EOF ) {
        BF_errno = BFE_OS;
	    return BFE_OS;
	}

	return BFE_OK;
}


/**** int BF_DestroyFile( char *fileName ) ****

 Input:
        * char * fileName           To onoma tou arxeiou pou tha katastrafei

 Operation:
        * Sbhnei to arxeio me onoma "filename" (efoson auto de xrhsimopoieitai)

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_DestroyFile( char *fileName )
{
	int32_t i;

	
	/* Periorismos tou onomatos tou arxeiou. */
	if ( (fileName == NULL) || (strlen(fileName) > FILENAME_MAX-1) ) {
		BF_errno = BFE_INVALIDFILENAME;
		return BFE_INVALIDFILENAME;
	}

	/* Elegxoume ston pinaka anoixtwn arxeiwn mhpws to arxeio "fileName" xrhsimopoieitai */
	for ( i=0 ; i < MAXOPENFILES ; i++ ) {
		if( BF_FileTable[i].valid == TRUE ) {
		    /* to arxeio poy theloyme na diagrapsoume den prepei na einai hdh anoigmeno. */
		    if ( strcmp(BF_FileTable[i].filename, fileName) == 0 ){
                BF_errno = BFE_FILEOPEN;
				return BFE_FILEOPEN;
			}
		}
	}
	
	if( remove(fileName) != 0 ) {
        BF_errno = BFE_OS;
        return BFE_OS;
	}
   
	return BFE_OK;
}


/**** void BF_PrintError( char * errString ) ****

 Input:
        * char * errString          To symplhrwmatiko mhnyma la8ous pros ektypwsh

 Operation:
        * Ektypwnei to "errString" kai to mhnyma pou antistoixei sth timh tou BF_errno

 Output:
        * As above

 Return Value:
        * None
*/

void BF_PrintError( char * errString )
{
	printf("%s:%s\n",errString,BF_Errors[(BF_errno+1)*-1]);
}


/**** int BF_OpenFile( char * fileName ) ****

 Input:
        * char * fileName           To onoma tou arxeiou pou tha anoixtei

 Operation:
        * Anoigei to arxeio "fileName" kai efoson yparxei slot
        * Elegxei an yparxei hdh anoigma sto idio arxeio sth mnhmh, etsi wste na mhn ksanafortwsei to header
        * An oxi, to kata8etei ston pinaka anoixtwn arxeiwn "FileTable" kai fortwnei to header 
          tou arxeiou ston pinaka "BF_Headers"

 Output:
        * None

 Return Value:
        * Mh arnhtiko ari8mo, o opoios apotelei anagnwristiko tou arxeiou
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_OpenFile( char * fileName )
{
	int32_t i, j, pos;
	char existsInvalid = FALSE, alreadyOpen = FALSE;

    /* Periorismos tou onomatos tou arxeiou. */
	if ( (fileName == NULL) || (strlen(fileName) > FILENAME_MAX-1) ) {
		BF_errno = BFE_INVALIDFILENAME;
		return BFE_INVALIDFILENAME;
	}
	
	for ( i=0 ; i < MAXOPENFILES ; i++ ) {
		if ( BF_FileTable[i].valid == FALSE ) {
            /* Yparxei xwros ston pinaka. */
			existsInvalid = TRUE;
			break;
		}
	}
	
	if ( existsInvalid == FALSE ) {
        /*  Ola ta blocks einai valid, dld xrhsimopoiountai,
			ara de mporei na fortw8ei to arxeio. */
		BF_errno = BFE_FTABFULL;
		return BFE_FTABFULL;
	}
	
	pos = i;

    /* Prospathoyme na anoiksoyme to arxeio. */
	if ( (BF_FileTable[pos].fp = fopen(fileName, "rb+")) == NULL ) {
		BF_errno = BFE_OS;
		return BFE_OS;
	}
	
	if ( (BF_FileTable[pos].filename = malloc(strlen(fileName)+1) ) == NULL ) {
		BF_errno = BFE_NOMEM;
		return BFE_NOMEM;
	}
	
	/*  Elegxoume an exei hdh anoix8ei to arxeio wste na mhn ksanafortwsoume to header,
		opote apla briskoume to index tou yparxontos fortwmenou header. */
	for ( j=0 ; j < MAXOPENFILES ; j++ ) {
		if ( BF_FileTable[j].valid == TRUE ) {
			if ( strcmp(BF_FileTable[j].filename, fileName) == 0 ) {
				alreadyOpen = TRUE;
				break;
			}
		}
	}
	
	/* Etoimazoume thn eggrafh. */
	strcpy(BF_FileTable[pos].filename, fileName);
	BF_FileTable[pos].valid = TRUE;
	
	/* An einai hdh anoixto to arxeio tote orizoyme access sto idio header. */
	if ( alreadyOpen == TRUE ) {
		BF_FileTable[pos].header = BF_FileTable[j].header;
		BF_errno = BFE_FILEOPEN;
		return pos;
	}
	
	/* Alliws, fortwnoyme to header apo to arxeio kai to eisagoume stin prwth kenh thesh toy pinaka "BF_Headers".
	   Eimaste sigoyroi oti an vrhkame xwro ston pinaka me ta open files, tha vroume xwro ston "BF_Headers". */
	else {
        int32_t k;
        
		/* Briskoyme thn prwth kenh 8esh ston "BF_Headers". */
		for (k=0 ; k < MAXOPENFILES ; k++) {
            /* To prwto byte tou header einai flag poy mas leei an xrhsimopoieitai to header (valid). */
			if (BF_Headers[k][0] == FALSE) {
				break;
			}
		}

        /* Fortwnoyme to header apo to arxeio. */
		if ( (fread(&BF_Headers[k][1], sizeof(char), BF_BLOCK_SIZE, BF_FileTable[pos].fp)) != BF_BLOCK_SIZE) {
			BF_errno = BFE_INCOMPLETEREAD;
			return BFE_INCOMPLETEREAD;
		}

		/* Kanoyme set to prwto byte-flag gia na deixnei oti xrhsimopoieitai. */
		BF_Headers[k][0] = TRUE;
		BF_FileTable[pos].header = k;
		
		return pos;
	}
}


/**** int BF_CloseFile( int fileDesc ) ****

 Input:
        * int fileDesc			To anagnwristiko/index tou arxeiou pou tha kleisoume

 Operation:
		* Kleinei to arxeio me kwdiko "fileDesc"
			- efoson de xrhsimopoieitai
			- swzontas tyxon allages pou exoun ginei (px header)
		* Akyrwnei th sygkekrimenh kataxwrish apo ton pinaka "FileTable"

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_CloseFile( int fileDesc )
{
	int32_t i, k;
	char stillOpen;
	
	/* Elegxos gia to an exoume valid fileDesc. */
	if (fileDesc < 0 || fileDesc >= MAXOPENFILES || BF_FileTable[fileDesc].valid == FALSE) {
        BF_errno = BFE_FD;
        return BFE_FD;
	}
	
	/*
		Den mporoyme na to kleisoyme an yparxoyn akoma pinned blocks apo to sygkekrimeno anoigma.
		Epomenws, elegxoume thn endiamesh mnhmh...
	*/
	for (i=0 ; i < BF_BUFFER_SIZE ; i++) {
		if (BF_InterMemory[i].valid == TRUE)
		{
            /* elegxos sto bitmap tou valid block stin endiamesh mnhmh. */
            if (!BF_isBitZero(BF_InterMemory[i].fileDescs[fileDesc/8], fileDesc%8)) {
                /* brhkame valid (pinned) block, opote de mporoyme na kleisoume to arxeio. */
                BF_errno = BFE_CLOSEBLOCKFIXED;
                return BFE_CLOSEBLOCKFIXED;
			}
		}
	}
	
	stillOpen = FALSE;

	/*  Elegxoume an yparxei allo anoigma sto idio filename, an oxi tote prepei na swsoyme
		to header tou arxeiou prin to kleisoyme. */
	for (i=0 ; i < MAXOPENFILES ; i++) {
		if ( i != fileDesc) {
			if (BF_FileTable[i].valid == TRUE && strcmp(BF_FileTable[i].filename, BF_FileTable[fileDesc].filename) == 0) {
				stillOpen = TRUE;
				break;
			}
		}
	}
	
	/* An den yparxei allo anoigma sto idio filename */
	if (stillOpen == FALSE) {
		/* pame stin arxh tou arxeiou */
		if (fseek(BF_FileTable[fileDesc].fp, 0, SEEK_SET) != 0) {
			BF_errno = BFE_OS;
			return BFE_OS;
	    }
	    
		/* gia na swsoyme to header. */
		if (fwrite(&BF_Headers[BF_FileTable[fileDesc].header][1], sizeof(char), BF_BLOCK_SIZE, BF_FileTable[fileDesc].fp) != BF_BLOCK_SIZE) {
			BF_errno = BFE_INCOMPLETEWRITE;
			return BFE_INCOMPLETEWRITE;
		}
		
		/* O xwros toy header sth mnhmh mporei twra na xrhsimopoih8ei ksana. */
		BF_Headers[BF_FileTable[fileDesc].header][0] = FALSE;
	}
	
			
	/* Prepei na swsoyme epishs ola ta dirty unpinned (invalid) blocks toy twrinoy anoigmatos. */
    /* Gia kathe block tou anoigmatos mas */
	for (i=0; i < BF_BUFFER_SIZE ; i++) {
        if (!BF_isBitZero(BF_FileTable[fileDesc].fileDescs[i/8], i%8)) {
           /* elegxoume an einai dirty, an nai to swzoume */
           if (BF_InterMemory[i].valid == TRUE && BF_InterMemory[i].dirty == TRUE) {
              if ((k = BF_SaveDirtyBlock(i)) != BFE_OK) {
				 /* an de mporoume na swsoume ena dirty block, epistrefoume to giati. */
                 BF_errno = k;
                 return k;
              }
           }
        }
    }
	
	/* "Ka8arizoume" thn eggrafh apo to FileTable. */
	free(BF_FileTable[fileDesc].filename);
    BF_FileTable[fileDesc].filename = NULL;
    BF_FileTable[fileDesc].valid = FALSE;
    memset(&BF_FileTable[fileDesc].fileDescs, 0, sizeof(BF_FileTable[fileDesc].fileDescs));

    if (fclose(BF_FileTable[fileDesc].fp) == EOF) {
		BF_errno = BFE_OS;
		return BFE_OS;
	}
	
    return BFE_OK;
}


/**** int BF_AllocBlock( int fileDesc, int * blockNum, char ** blockBuf ) ****

 Input:
        * int fileDesc              O index tou anoixtou arxeiou
		* int * blockNum            O ari8mos tou neou block tou arxeiou
		* char ** blockBuf          Emmesos deikths pros thn endiamesh mnhmh

 Operation:
        * Psaxnei gia eleytherh thesh sto arxeio        
        * An vrei, Psaxnei gia eleu8erh 8esh sthn endiamesh mnhmh.
          - An brei, desmeuei ekei to neo block
          - An DEN brei eleu8erh 8esh, psaxnei stis valid 8eseis  
            - An einai oles pinned, apotygxanei
            - Alliws, antika8ista mia mesw LRU
                - afou prwta thn swsei, ean auto einai aparaithto
        * An Den yparxei diathesimh thesh sto arxeio, apotygxanei
            
        * Me to pou vrei mia thesh sto arxeio kai sth mnhmh, 
          desmeuei sth mnhmh ena keno block kai to grafei sthn prwth diathesimh thesh sto arxeio

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_AllocBlock( int fileDesc, int * blockNum, char ** blockBuf )
{
	int32_t i, k, nblocks, memPos, maxBlocks, filePos;
	char * header_offset;
	char foundPos;
	
	/* Elegxos gia to an exoume valid fileDesc. */
	if (fileDesc < 0 || fileDesc >= MAXOPENFILES || BF_FileTable[fileDesc].valid == FALSE) {
        BF_errno = BFE_FD;
        return BFE_FD;
	}
	
	header_offset = BF_Headers[BF_FileTable[fileDesc].header];
	
	/* Prospername to arxiko flag poy mas deixnei an xrhsimopoieitai to header */
	header_offset++;
	/* posa to poly blocks exei to arxeio mas apothikeyontai sta 4 prwta bytes tou header block. */
	memcpy(&nblocks, header_offset, sizeof(nblocks));

	/* Ta maximum blocks poy mporoyme na exoume sto arxeio einai osa bytes einai to header mas * 8 (plh8os bits)
	   meion omws ta sizeof(int32_t) bytes toy arithmoy poy metraei posa exoume hdh */
	maxBlocks = (BF_BLOCK_SIZE - sizeof(int32_t)) * 8;
	
	/* Prospername ton arxiko sum arithmo poy molis diavasame. */
	header_offset += sizeof(int32_t);
	
	foundPos = FALSE;
	/* Briskoume thn prwth diathesimh thesh gia na valoyme to block mas sto arxeio. */
	for (i=0; i < maxBlocks; i++) {
        if ( BF_isBitZero(header_offset[i/8], i%8) ) {
            foundPos = TRUE;
			break;
		}
	}
	
	/* de vrhkame kenh thesh sto arxeio gia na valoume to neo block mas */
	if (foundPos == FALSE) {
       BF_errno = BFE_NOBITMAP;
       return BFE_NOBITMAP;
    }
    
    /* H thesh sto bitmap kai sto arxeio opoy tha mpei to neo block. */
	filePos = i; 
	
	
	/* psaxnoume mia thesh sth mnhmh poy tha xrhsimopoihsoyme */
	if ( (k = BF_findMemPos (&memPos)) != BFE_OK) {
       return k;
    }
	
	/* Setaroume swsta to neo keno block sthn endiamesh mnhmh. */
	memset(BF_InterMemory[memPos].buffer, 0, sizeof(BF_InterMemory[memPos].buffer));
	memset(BF_InterMemory[memPos].fileDescs, 0, sizeof(BF_InterMemory[memPos].fileDescs));
	time++;

	BF_InterMemory[memPos].timestamp = time;
	BF_InterMemory[memPos].dirty = FALSE;
	BF_InterMemory[memPos].valid = TRUE;
		
	/* Kanoyme set to swsto bit poy antistoixei sto fileDesc poy mas kanei pin. */
	BF_bitSet((int *) &BF_InterMemory[memPos].fileDescs[fileDesc/8], fileDesc%8);
	BF_InterMemory[memPos].blockNum = filePos;
	
	
	/*
		Twra exoume to block sthn endiamesh mnhmh, mas menei na enhmerwsoume to header kai na to valoume st arxeio
	*/
	
	/* 8etoyme ta arguments-pointers stis swstes dieu8unseis mnhmhs. */
	(*blockNum) = filePos;
	(*blockBuf) = (char *)BF_InterMemory[memPos].buffer;
	
	/* Kanoyme set to bit poy antistoixei sth thesh tou block poy molis dhmioyrgisame. */
    BF_bitSet((int *) &BF_FileTable[fileDesc].fileDescs[memPos/8], memPos%8);

	/* Enhmerwnoyme to bitmap tou header oti tha valoume to block stin "filePos"-osth 8esh. */
	BF_bitSet((int *) &header_offset[filePos / 8], filePos % 8);
	
	/* Bazoume ena parapanw block ston counter an ta ypervhkame */
	if (filePos == nblocks) {
	   nblocks++;
	   /* kai enhmerwnoyme to header me ton neo counter. */
   	   header_offset-=sizeof(int32_t);
	   memcpy(header_offset, &nblocks, sizeof(nblocks));
    }
	
	/* Pame sth thesh poy prepei na mpei to block, +1 gia na prosperasoume to arxiko header. */
	if ( fseek(BF_FileTable[fileDesc].fp, (filePos+1) * BF_BLOCK_SIZE, SEEK_SET) != 0 ) {
		BF_errno = BFE_OS;
		return BFE_OS;
	}
	
	/* Telos, grafoume to block sth swsth thesh sto arxeio. */
	if ( fwrite(BF_InterMemory[memPos].buffer, sizeof(char), BF_BLOCK_SIZE, BF_FileTable[fileDesc].fp) != BF_BLOCK_SIZE ) {
		BF_errno = BFE_INCOMPLETEWRITE;
		return BFE_INCOMPLETEWRITE;
	}
	
	return BFE_OK;
}


/**** int BF_UnpinBlock(int fileDesc, int blockNum, int dirty) ****

 Input:
        * int fileDesc          O index tou anoixtou arxeiou
		* int blockNum          O ari8mos tou block tou arxeiou pou tha ksekarfwsoume
		* int dirty             Dhlwnei an to block exei allax8ei sth mnhmh

 Operation:
        * Psaxnei to "blockNum" block tou anoigmatos "fileDesc"
          kai an to brei to kanei unpin, afou prwta to enhmerwsei ws dirty an xreiazetai.

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_UnpinBlock(int fileDesc, int blockNum, int dirty)
{
	int32_t i, maxBlocks, memPos;
	char found_it;
	
	/* elegxos gia to an exoume valid fileDesc */
	if (fileDesc < 0 || fileDesc >= MAXOPENFILES || BF_FileTable[fileDesc].valid == FALSE) {
        BF_errno = BFE_FD;
        return BFE_FD;
	}
	
	/* Ta maximum blocks poy mporoyme na exoume sto arxeio einai osa bytes einai to header mas * 8 meion omws 
       ta sizeof(int32_t) bytes toy arithmoy poy metraei posa exoume so far */
	maxBlocks = (BF_BLOCK_SIZE - sizeof(int32_t) ) * 8;

	/* Elegxos gia to an exoume valid block number. */
    if (blockNum < 0 || blockNum >= maxBlocks) {
        BF_errno = BFE_INVALIDBLOCK;
        return BFE_INVALIDBLOCK;
	}
    
    /* Koitame an mas dw8hke asxeth timh sto dirty. */
    if (dirty != FALSE && dirty != TRUE) {
       BF_errno = BFE_INVALIDVAL;
       return BFE_INVALIDVAL;
    }
    
	/* Psaxnoyme na vroume to swsto block sth endiamesh mnhmh poy antistoixei sto anoigma "fileDesc" kai einai
       sth thesh "blockNum" tou arxeiou tou. */
	found_it = FALSE;
	for (i = 0; i < BF_BUFFER_SIZE; i++) {
        /* Koitame gia swsto blockNumber */
		if (BF_InterMemory[i].valid == TRUE && BF_InterMemory[i].blockNum == blockNum) {
            /* koitame an egine pin apo to anoigma poy theloyme (fileDesc) */                        
			if (!BF_isBitZero(BF_InterMemory[i].fileDescs[fileDesc / 8], fileDesc % 8)) {
				found_it = TRUE;
				/* found it, now unpin it! */
				BF_bitClear((int *)&BF_InterMemory[i].fileDescs[fileDesc / 8], fileDesc % 8);
				break;
			}
		}
	}

    /* De bre8hke... */
	if (found_it == FALSE) {
		BF_errno = BFE_BLOCKNOTINBUF;
		return BFE_BLOCKNOTINBUF;
	}
	
	/* Bre8hke... */
	memPos = i;

	/* An einai dirty, to shmeiwnoyme wste argotera otan ginei h antikatastash na swthei prwta sto arxeio */
	if (dirty == TRUE) {
        BF_InterMemory[memPos].dirty = TRUE;        		
	}
	
	return BFE_OK;
}


/**** int BF_DisposeBlock(int fileDesc, int *blockNum) ****

 Input:
        * int fileDesc              O index tou anoixtou arxeiou
		* int * blockNum            O ari8mos tou block tou arxeiou pou tha anakyklw8ei

 Operation:
        * Elegxoume an to block sto arxeio tou fileDesc me nohth thesh th blockNum yparxei fortwmeno sth mnhmh 
        * An yparxei fortwmeno sth mnhmh, tote den prepei na to xei kaneis pin kai katharizoume to keli 
          auto tis endiameshs mnhmhs
        * To block diagrafetai apo to bitmap tou header tou arxeiou

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_DisposeBlock(int fileDesc, int *blockNum)
{
    int32_t i, j, nblocks, maxBlocks, memPos;
	char found_it;
	char * header_offset;

	/* Elegxos gia to an exoume valid fileDesc */
	if (fileDesc < 0 || fileDesc >= MAXOPENFILES || BF_FileTable[fileDesc].valid == FALSE) {
        BF_errno = BFE_FD;
        return BFE_FD;
	}

	/* 
       * Elegxos gia to an exoume valid block number.
	   * Ta maximum blocks poy mporoyme na exoume sto arxeio einai osa bytes einai to header mas * 8 meion omws 
         ta sizeof(int32_t) bytes toy arithmoy poy metraei posa exoume so far
    */
	maxBlocks = (BF_BLOCK_SIZE - sizeof(int32_t) ) * 8;
    if ((*blockNum) < 0 || (*blockNum) >= maxBlocks) {
        BF_errno = BFE_INVALIDBLOCK;
        return BFE_INVALIDBLOCK;
	}
	
	/* 
       * Elegxoume gia to an yparxei ontws to block sto bitmap tou header tou arxeiou,
         prin epixeirhsoyme na to svisoume.
    */
    header_offset = BF_Headers[BF_FileTable[fileDesc].header];
	/* Prospername arxiko flag */
	header_offset++;
    /* prospername ton counter pou mas leei mexri pou xreiazetai na psaksoume */
    header_offset+=sizeof(int32_t);
    
    /* an den yparxei sto header to block me arithmo blockNum, tote einai hdh free. */
    if (BF_isBitZero(header_offset[(*blockNum) / 8], (*blockNum)%8)) {
       BF_errno = BFE_BLOCKFREE;
       return BFE_BLOCKFREE;
    }
    
    /* Epistrefoume sthn arxh tou counter. */
	header_offset -= sizeof(int32_t);
	
	
	/* 
        Psaxnoyme na vroyme to block sthn endiamesh mnhmh.
    */

	found_it = FALSE;
	for ( i=0 ; i < BF_BUFFER_SIZE ; i++) {
        /* An vroume block me idio blockNumber */
		if (BF_InterMemory[i].valid == TRUE && BF_InterMemory[i].blockNum == (*blockNum)) {
			/*  tote elegxoyme an einai block tou arxeiou mas (fileDesc). */
			if (!BF_isBitZero(BF_FileTable[fileDesc].fileDescs[i / 8], i%8)) {
				found_it = TRUE;
				break;
			}
		}
	}
	
	memPos = i;
	
	if (found_it == TRUE) {
		/* Afou vrhkame to swsto block sthn endiamesh mnhmh, prepei na mhn to exei kaneis pinned. */
		for (j = 0; j < MAXOPENFILES ; j++) {
	        if (!BF_isBitZero(BF_InterMemory[memPos].fileDescs[j / 8], j % 8)) {
				BF_errno = BFE_CLOSEBLOCKFIXED;
				return BFE_CLOSEBLOCKFIXED;
			}
		}

		/* "Ka8arise" to block. */
		BF_InterMemory[memPos].valid = FALSE;
		BF_InterMemory[memPos].timestamp = 0;
		BF_InterMemory[memPos].dirty = FALSE;
		BF_InterMemory[memPos].blockNum = BF_INVALID;
		memset(&BF_InterMemory[memPos].fileDescs, 0, sizeof(BF_InterMemory[memPos].fileDescs));
	}

	/* Diavazoume ton counter */
	memcpy(&nblocks, header_offset, sizeof(int32_t));

	/* kai elegxoume an to block pou diagrafthke htan to teleutaio tou arxeiou, an nai meiwnoyme ton counter */
	if ((*blockNum) == (nblocks - 1)) {
       nblocks--;
       memcpy(header_offset , &nblocks, sizeof(int32_t)); 
    }		
	
    /* prospername ton counter */
	header_offset+=sizeof(int32_t);
	
	/* kai sbhnoyme to block apo to header tou arxeiou. */
	BF_bitClear((int *)&header_offset[(*blockNum)/8], (*blockNum) % 8);
	
	if (found_it == TRUE) {
       /* Frontizoume, twra poy ginetai dispose, na mhn yparxoun bits se bitmaps anoigmatwn
          pou na synexisoun na deixnoyn edw */
       for (i=0; i < MAXOPENFILES; i++) {
	       BF_bitClear((int *) &BF_FileTable[i].fileDescs[memPos/8], memPos%8);
       }
	}
	
	return BFE_OK;
}


/**** int BF_GetFirstBlock( int fileDesc, int *blockNum, char **blockBuf ) ****

 Input:
        * int fileDesc              O index tou anoixtou arxeiou
		* int * blockNum            O ari8mos tou prwtou block tou arxeiou
		* char ** blockBuf          Emmesos deikths pros thn endiamesh mnhmh

 Operation:
        * Mas epistrefei ton ari8mo tou prwtou egkyroy block tou arxeiou "fileDesc", opws kai to idio to block.
        * An psaksei se keno arxeio, apotygxanei k epistrefei BFE_EOF
        * An brei ton arithmo block pou mas endiaferei,
          elegxei mhpws to block brisketai hdh sthn endiamesh mnhmh kai aplws to kanei pin 
          gia to sygkekrimeno anoigma tou arxeiou
        * An den yparxei sthn endiamesh mnhmh, to diabazei apo to arxeio kai to kanei pin gia to 
          anoigma tou arxeiou

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * BFE_EOF, an psaksei keno arxeio
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_GetFirstBlock( int fileDesc, int *blockNum, char **blockBuf )
{
    int32_t i, nblocks, filePos;
    char * header_offset;
    char exists;

	/* Elegxos gia to an exoume valid fileDesc */
	if (fileDesc < 0 || fileDesc >= MAXOPENFILES || BF_FileTable[fileDesc].valid == FALSE) {
        BF_errno = BFE_FD;
        return BFE_FD;
	}
	
	/* Swzoume sto offset th dieu8unsh tou header tou arxeiou me kwdiko fileDesc sth mnhnh */
	header_offset = BF_Headers[BF_FileTable[fileDesc].header];

	/* prospername to arxiko flag poy mas deixnei an xrhsimopoieitai to header. */
	header_offset++;
	
	/* Sta 4 prwta bytes tou header block apo8hkeuetai to posa to poly blocks exei to arxeio mas. */
	memcpy(&nblocks, header_offset, sizeof(nblocks));
	
	/* Prospername twra kai ton arxiko arithmo. */
	header_offset += sizeof(nblocks);
	
	/* H flag exists krataei an exoume brei block sto arxeio. */
	exists = FALSE;
    for( i=0 ; i < nblocks ; i++ ) {
         if (!BF_isBitZero(header_offset[i/8], i%8)) {
            exists = TRUE;
            break;
         }
    }
    
    /* de brhkame kanena block sto arxeio */
    if( exists == FALSE ) {
        BF_errno = BFE_EOF;
        return BFE_EOF;
    }
    
    /* alliws yparxei block sto arxeio, kratame se poia 8esh brisketai. */
    filePos = i;
    
    /* Swzoume to ena zhtoumeno. */
    (*blockNum) = filePos;
    
    /* kai fortwnoyme to filePos-osto block sth mnhmh */
    return BF_GetThisBlock(fileDesc, filePos, blockBuf );
}


/**** int BF_GetNextBlock( int fileDesc, int *blockNum, char **blockBuf ) ****

 Input:
        * int fileDesc              O index tou anoixtou arxeiou
		* int * blockNum            O ari8mos tou block tou arxeiou ap'opou arxizei to psaksimo
		* char ** blockBuf          Emmesos deikths pros thn endiamesh mnhmh

 Operation:
        * Elegxei an yparxei to parexomeno blockNum
        * An to brei, proxwraei mexri na brei to epomeno valid
        * An den brei kapoio, epistrefei EOF
        * An brei, elegxei an yparxei hdh anoixto sth mnhmh, opote aplws to kanei pin 
          gia to sygkekrimeno anoigma tou arxeiou
        * An oxi, to fortwnei apo to arxeio kai to kanei pin gia to anoigma tou arxeiou

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * BFE_EOF, an de brei epomeno block
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_GetNextBlock( int fileDesc, int *blockNum, char **blockBuf )
{
    int32_t i, nblocks, filePos;
    char * header_offset;
    char exists;

	/* Elegxos gia to an exoume valid fileDesc */
	if (fileDesc < 0 || fileDesc >= MAXOPENFILES || BF_FileTable[fileDesc].valid == FALSE) {
        BF_errno = BFE_FD;
        return BFE_FD;
	}
	
	/* Swzoume sto offset th dieu8unsh tou header tou arxeiou me kwdiko fileDesc sth mnhnh */
	header_offset = BF_Headers[BF_FileTable[fileDesc].header];

	/* prospername to arxiko flag poy mas deixnei an xrhsimopoieitai to header. */
	header_offset++;

	/* Sta 4 prwta bytes tou header block apo8hkeuetai to posa to poly blocks exei to arxeio mas. */
	memcpy(&nblocks, header_offset, sizeof(nblocks));
	
	/* Elegxos gia to an exoume valid block number. */
    if ((*blockNum) < 0 || (*blockNum) >= nblocks) {
        BF_errno = BFE_INVALIDBLOCK;
        return BFE_INVALIDBLOCK;
	}
	
	/* Prospername twra kai ton arxiko counter. */
	header_offset += sizeof(nblocks);
	
	/* Elegxoume apo to bitmap an to block me arithmo "*blockNum" yparxei sto arxeio. */
	if (BF_isBitZero(header_offset[(*blockNum)/8], (*blockNum) % 8) ) {
       BF_errno = BFE_INVALIDBLOCK;
       return BFE_INVALIDBLOCK;
    }
            
    exists = FALSE;
    /* Twra psaxnoyme to prwto ap'ta epomena valid blocks sto arxeio. */
    for (i = (*blockNum) + 1; i < nblocks; i++) {
        if (!BF_isBitZero(header_offset[i/8], i % 8) ) {
           exists = TRUE;
           break;
        }
    }
    
    filePos = i;
    
    /* An den yparxei epomeno valid block sto arxeio, epistrefoume EOF.*/
    if (exists == FALSE) {       
       BF_errno = BFE_EOF;
       return BFE_EOF;
    }
    
    /* Enhmerwnoyme to prwto zhtoymeno me ton arithmo toy epomenoy block poy diavasame. */
    (*blockNum) = filePos;
    
    /* kai fortwnoyme to filePos-osto block sth mnhmh */
    return BF_GetThisBlock(fileDesc, filePos, blockBuf );
}


/**** int BF_GetThisBlock( int fileDesc, int blockNum, char **blockBuf ) ****

 Input:
        * int fileDesc              O index tou anoixtou arxeiou
		* int blockNum              O ari8mos tou zhtoumenou block tou arxeiou
		* char ** blockBuf          Emmesos deikths pros thn endiamesh mnhmh

 Operation:
        * Elegxei an yparxei to zhtoumeno blockNum
        * An to brei, elegxei an yparxei hdh anoixto sth mnhmh, opote to aplws to kanei pin 
          gia to sygkekrimeno anoigma tou arxeiou
        * An oxi, to fortwnei apo to arxeio kai to kanei pin gia to sygkekrimeno anoigma arxeiou

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * BFE_INVALIDBLOCK, an de brei to zhtoumeno block
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_GetThisBlock( int fileDesc, int blockNum, char **blockBuf )
{
    int32_t j, k, nblocks, filePos, memPos;
    char * header_offset;
    char found_it;

	/* Elegxos gia to an exoume valid fileDesc */
	if (fileDesc < 0 || fileDesc >= MAXOPENFILES || BF_FileTable[fileDesc].valid == FALSE) {
        BF_errno = BFE_FD;
        return BFE_FD;
	}
	
	/* Swzoume sto offset th dieu8unsh tou header tou arxeiou me kwdiko fileDesc sth mnhnh */
	header_offset = BF_Headers[BF_FileTable[fileDesc].header];

	/* prospername to arxiko flag poy mas deixnei an xrhsimopoieitai to header. */
	header_offset++;

	/* Sta 4 prwta bytes tou header block apo8hkeuetai to posa to poly blocks exei to arxeio mas. */
	memcpy(&nblocks, header_offset, sizeof(nblocks));
	
	/* Elegxos gia to an exoume valid block number. */
    if ( blockNum < 0 || blockNum >= nblocks) {
        BF_errno = BFE_INVALIDBLOCK;
        return BFE_INVALIDBLOCK;
	}

	/* Prospername twra kai ton arxiko counter. */
	header_offset += sizeof(nblocks);

	/* Elegxoume apo to bitmap an to block me arithmo "*blockNum" yparxei sto arxeio. */
	if (BF_isBitZero(header_offset[blockNum/8], blockNum%8) ) {
       BF_errno = BFE_INVALIDBLOCK;
       return BFE_INVALIDBLOCK;
    }
    
    filePos = blockNum;

    /*
        Psaxnoyme na vroyme to block an yparxei hdh sthn endiamesh mnhmh  (gia na mh ksanafortw8ei apo to arxeio)
    */
	found_it = BF_existsInMem(fileDesc, filePos, &memPos);

	/*
       Ara, exoume brei oti to "j"-osto keli tou pinaka anoixtwn arxeiwn anaferetai sto epi8ymhto arxeio, poy einai
       to arxeio apo to opoio theloyme na fortwsoyme to block. Ara mias kai einai hdh fortwmeno to block,
       de to ksanafortwnoyme, aplws to kanoyme pin gia to twrino anoigma (fileDesc)
	*/

	if (found_it == TRUE) {
       /* Swzoume kai to allo zhtoumeno (deixnoume sto zhtoumeno block). */
       (*blockBuf) = (char *)BF_InterMemory[memPos].buffer;
       
       /* Elegxoume an einai hdh karfwmeno apo to anoigma fileDesc */
       if (!BF_isBitZero(BF_InterMemory[memPos].fileDescs[fileDesc / 8], fileDesc%8)) {
          BF_errno =  BFE_BLOCKFIXED;
       }

       /* kanoyme pin to block apo to twrino anoigma (fileDesc) */
       BF_bitSet((int *) &BF_InterMemory[memPos].fileDescs[fileDesc / 8], fileDesc % 8);

	   /* shmeiwnoyme kai sto twrino anoigma oti einai diko tou to memPos block */
       BF_bitSet((int *) &BF_FileTable[fileDesc].fileDescs[memPos / 8], memPos % 8);

	   /* ananewnoyme ton global xrono kai vazoume to swsto timestamp. */
	   time++;
	   BF_InterMemory[memPos].timestamp = time;

	   return BFE_OK;
    }

	/*
        Alliws den yparxei sth mnhmh, opote prepei na to fortwsoume apo to arxeio,
        opws kai se prohgoumenh sunarthsh (BF_AllocBlock)
	*/

	/* psaxnoume mia thesh sth mnhmh poy tha xrhsimopoihsoyme */
	if ( (k = BF_findMemPos (&memPos)) != BFE_OK) {
       return k;
    }

	/* diavazoume to block apo ti thesh sto arxeio pou vrhkame sti thesh mnhmhs pou epishs vrikame ws diathesimh */
    j = BF_readBlock (fileDesc, filePos, memPos);
    if (j != BFE_OK) {
       return j;
    }
	
	/* Swzoume kai to allo zhtoumeno (deixnoume sto zhtoumeno block). */
	(*blockBuf) = BF_InterMemory[memPos].buffer;

    return BFE_OK;
}

/************************
 * Xrhsimes synarthseis *
 ************************/

/**** int BF_SaveDirtyBlock (int memPos) ****

 Input:
        * int memPos            H 8esh sthn mnhmh tou dirty block pou tha grapsoume
        
 Operation:
        * Briskei ena anoigma arxeiou pou na periexei to epi8ymhto block
        * Molis to brei, paei kai enhmerwnei to block sto arxeio me th nea version

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_SaveDirtyBlock (int memPos)
{
        int32_t fileDesc, i;
	    char found_it;

	    /* Elegxos gia to an exoume valid memory position. */
	    if (memPos < 0 || memPos >= BF_BLOCK_SIZE || BF_InterMemory[memPos].valid == FALSE) {
           BF_errno = BFE_INVALIDBLOCK;
           return BFE_INVALIDBLOCK;
	    }

      	/* Briskoyme ena arxeio me fileDesc pou exei to "memPos"-block gia na exoume access ston file pointer */
      	found_it = FALSE;
	    for (i=0; i < MAXOPENFILES; i++) {
            /* an vroume ena anoigma poy na exei san diko tou block to block sth 8esh "memPos" tote eimaste entaksei */
            if (!BF_isBitZero(BF_FileTable[i].fileDescs[memPos / 8], memPos%8)) {
               found_it = TRUE;
               break;
            }
        }

        fileDesc = i;

        /* An de vrikame anoigma arxeiou pou na xei san block tou to block sth 8esh "memPos", tote kati phge strava pio prin */
        if (found_it == FALSE) {
           BF_errno = BFE_OS;
           return BFE_OS;
        }


        /* Pame sth thesh tou block sto arxeio (+1 gia na prosperasoume to arxiko header) */
        if (fseek(BF_FileTable[fileDesc].fp, (BF_InterMemory[memPos].blockNum + 1) * BF_BLOCK_SIZE, SEEK_SET) != 0) {
            BF_errno = BFE_OS;
			return BFE_OS;
		}

		/* kai enhmerwnoyme to arxeio me th nea version tou block. */
    	if ( fwrite(BF_InterMemory[memPos].buffer, sizeof(char), BF_BLOCK_SIZE, BF_FileTable[fileDesc].fp) != BF_BLOCK_SIZE ) {
            BF_errno = BFE_OS;
			return BFE_OS;
		}
		/* No longer dirty. */
		BF_InterMemory[memPos].dirty = FALSE;

		return BFE_OK;
}

/**** int BF_findMemPos (int * memPos) ****

 Input:
        * int * memPos          H dia8esimh 8esh sth mnhmh gia to block

 Operation:
        * Psaxnei sthn endiamesh mnhmh gia na brei invalid block.
        * An ola ta blocks einai valid, efarmozei LRU gia na brei mia valid+unpinned 8esh.
        * An de brei oute valid 8esh, tote epistrefei mhnyma la8ous.
        * An brei, tote swzei th dia8esimh 8esh sth "memPos" kai epistrefei BFE_OK.

 Output:
        * None

 Return Value:
        * BFE_OK, efoson bre8ei dia8esimh 8esh mnhmhs.
        * Enan kwdiko la8ous, efoson apotyxei h LRU.
*/

int BF_findMemPos(int * memPos)
{
    char invalid;
    int j, k;
    
	invalid = FALSE;
	for (j=0 ; j < BF_BUFFER_SIZE ; j++) {
		if (BF_InterMemory[j].valid == FALSE) {
			invalid = TRUE;
			break;
		}
	}		
	
    /* An de vrhkame oyte ena invalid block tote synexizoume to psaksimo sta valid blocks */
	if (invalid == FALSE) {
        if ( (k = BF_LRU_Recycle(memPos)) < 0 ) {            
            return k;
        }
    }
    else {
         /* An vroume ena invalid block, xrhsimopoioyme ayto kai swzoume th thesh tou sto memPos. */
	     (*memPos) = j;
    }

    return BFE_OK;
}

/**** int BF_LRU_Recycle(int * memPos) ****

 Input:
        * int * memPos            H 8esh sthn mnhmh tou Least-Recently Used block,
                                  thn opoia parexei auth h sunarthsh

 Operation:
        * Psaxnei na brei valid block pou na mhn einai pinned
        * An einai ola pinned, epistrefei pws den yparxei xwros sthn endiamesh mnhmh
        * An brei, swzei to index ths 8eshs tou LRU block sto "memPos"

 Output:
        * None

 Return Value:
        * BFE_OK, an oloklhrw8hke epityxws
        * BFE_NOBUF, an de brei unpinned valid block
        * Kapoio Error Code, se periptwsh la8ous
*/

int BF_LRU_Recycle(int * memPos)
{
		int32_t j, k, min_timestamp = 0x7fffffff, min_pos = 0x7fffffff;
		char isPinned;

        /* Elegxoyme ean yparxoyn valid blocks poy den exoun ginei pinned apo kapoio anoixto arxeio */
        for (j=0 ; j < BF_BUFFER_SIZE ; j++) {

			/* elegxoyme se ola ta bitmaps ean exei ginei pinned to "j"-osto block. */
			isPinned = FALSE;
            for (k = 0; k < MAXOPENFILES; k++) {
                if (!BF_isBitZero(BF_InterMemory[j].fileDescs[k / 8], k % 8)) {
					isPinned = TRUE;
					break;
				}
			}

			/* An den einai pinned, elegxoume an einai to Least-Recently Used. */
			if (isPinned == FALSE && BF_InterMemory[j].timestamp < min_timestamp ) {
				min_timestamp = BF_InterMemory[j].timestamp;
				min_pos = j;
			}
		}

		/* Brhkame estw kai ena block poy na mhn einai pinned? */
		if ( min_pos == 0x7fffffff ) {
			BF_errno = BFE_NOBUF;
			return BFE_NOBUF;
		}
		else {
            /* Dialegoume 8esh sthn endiamesh mnhmh xrhsimopoiwntas LRU */
			(*memPos) = min_pos;
			/* prin omws xrhsimopoihsoyme th 8esh, elegxoume an einai dirty outws wste
			   na antikatasta8hsoume to palio */
            if( BF_InterMemory[(*memPos)].dirty == TRUE ) {
               if( (k = BF_SaveDirtyBlock((*memPos))) != BFE_OK) {
                    /* an de mporoume na swsoume ena dirty block, epistrefoume to giati. */
                    BF_errno = k;
                    return k;
               }
            }

            /* Frontizoume twra poy ginetai antikatastash, na mhn yparxoun bits se
			   bitmaps anoigmatwn pou na synexisoun na deixnoyn edw. */
            for( k=0 ; k < MAXOPENFILES ; k++) {
	            BF_bitClear((int *) &BF_FileTable[k].fileDescs[(*memPos)/8], (*memPos)%8);
            }
		}
		
		return BFE_OK;
}

/**** char BF_existsInMem (int fileDesc, int filePos, int * memPos) ****

 Input:
        * int fileDesc              O index tou anoixtou arxeiou
        * int filePos               O index tou block sto arxeio
        * int * memPos              O index sthn endiamesh mnhmh sthn opoia yparxei to block,
                                    (dhladh opou einai hdh fortwmeno to block sthn endiamesh mnhmh)

 Operation:
        * Psaxnei sta kelia ths endiameshs mnhmhs kai elegxei an einai hdh fortwmeno to block
          pou mas endiaferei (to "filePos" sto anoigma "fileDesc").

 Output:
        * None

 Return Value:
        * TRUE, efoson to block yparxei hdh sthn endiamesh mnhmh
        * FALSE, ean oxi.
*/

char BF_existsInMem(int fileDesc, int filePos, int * memPos)
{
    int32_t i, j;

	/* Psaxnoume se ola ta kelia ths endiameshs mnhmhs */
	for (i = 0; i < BF_BUFFER_SIZE; i++) {
        /* an vroume block me idio blockNumber */
		if (BF_InterMemory[i].valid == TRUE && BF_InterMemory[i].blockNum == filePos) {
            /* elegxoume ston pinaka anoixtwn arxeiwn */
            for ( j=0 ; j < MAXOPENFILES ; j++ ) {
                /* an to sygkekrimeno (i) block einai tou arxeiou tou sygkekrimenou (j) anoigmatos */
            	if (!BF_isBitZero(BF_FileTable[j].fileDescs[i / 8], i%8)) {
                   /* kai an milame profanws gia to idio arxeio. */
                   if( strcmp( BF_FileTable[j].filename, BF_FileTable[fileDesc].filename ) == 0 ) {
                        /* Brhkame to keli sth mnhmh opou exei hdh fortw8ei to block */
                       (*memPos) = i;
                       return TRUE;
                   }
    			}
            }
		}
	}

    return FALSE;
}

/**** int BF_readBlock (int fileDesc, int filePos, int memPos) ****

 Input:
        * int fileDesc              O index tou anoixtou arxeiou
        * int filePos               O index tou block sto arxeio
        * int memPos                O index sthn endiamesh mnhmh opou tha fortw8ei to block.

 Operation:
        * Fortwnei to "filePost" block tou anoigmatos arxeiou "fileDesc" sth 8esh "memPos" ths endiameshs mnhmhs

 Output:
        * None

 Return Value:
        * BFE_OK, ean oloklhrw8ei swsta
        * BFE_OS, ean de mporei na metakinh8ei sth swsth 8esh tou arxeiou
        * BFE_INCOMPLETEREAD, ean de mporesei na diabasei to block ap'to arxeio
*/

int BF_readBlock(int fileDesc, int filePos, int memPos)
{
	/* Metakinoyme ton file pointer sth 8esh tou block sto arxeio */
    if( fseek( BF_FileTable[fileDesc].fp , (filePos+1)*BF_BLOCK_SIZE, SEEK_SET) != 0 ) {
        BF_errno = BFE_OS;
        return BFE_OS;
    }
	
	/* kai twra diavazoume to block apo to arxeio. */
    if ( fread( BF_InterMemory[memPos].buffer, sizeof(char), BF_BLOCK_SIZE, BF_FileTable[fileDesc].fp ) != BF_BLOCK_SIZE ) {
         BF_errno = BFE_INCOMPLETEREAD;
         return BFE_INCOMPLETEREAD;
    }
    
	/* Setaroume swsta to neo keno block sthn endiamesh mnhmh */
	memset(BF_InterMemory[memPos].fileDescs, 0, sizeof(BF_InterMemory[memPos].fileDescs));
	/* ananewnoyme ton global xrono kai vazoume to swsto timestamp. */
    time++;
	BF_InterMemory[memPos].timestamp = time;
    BF_InterMemory[memPos].blockNum = filePos;
	BF_InterMemory[memPos].dirty = FALSE;
	BF_InterMemory[memPos].valid = TRUE;

	/* kanoyme set to swsto bit poy antistoixei sto fileDesc poy mas kanei pin */
	BF_bitSet((int *) &BF_InterMemory[memPos].fileDescs[fileDesc / 8], fileDesc % 8);
	
	/* kanoyme set to swsto bit poy antistoixei sth thesh tou block poy molis dhmioyrgisame */	
	BF_bitSet((int *) &BF_FileTable[fileDesc].fileDescs[memPos/8], memPos%8);
	
	return BFE_OK;
}
