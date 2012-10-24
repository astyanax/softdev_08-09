#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "AM_Lib.h"
#include "BF_Lib.h"
#include "HF_Lib.h"

#include "aminirel.h"

/* H database pou diaxeirizomaste twra */
extern struct db this_db;


/**** int UT_Init( char * dbname ) ****

 Input:
        * char * dbname    To onoma ths database pou tha xrhsimopoihsei o xrhsths

 Operation:
        * Arxikopoiei to UT epipedo

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/

int UT_Init( char * dbname )
{

    /* Arxikopoioume ta katwtera epipeda */
    HF_Init();
    AM_Init();

    /* kai metaferomaste sto fakelo ths database */
    if ( chdir( dbname ) ) {
        printf( "Error: Database %s does not exist", dbname );
        return AMINIREL_GE;
    }

    /* initialize twn fileDescriptors se non valid timh */
    this_db.attrCatDesc = this_db.relCatDesc = this_db.viewAttrCatDesc = this_db.viewCatDesc = AMINIREL_INVALID;

    /* Anoigma twn 4 vasikwn arxeiwn mias database pou tha mas xreiastoun argotera */
    if (( this_db.attrCatDesc =  HF_OpenFile( "attrCat" ) ) < HFE_OK ) {
        HF_PrintError( "Couldn't open attrCat" );
        return AMINIREL_GE;
    }

    if (( this_db.relCatDesc =  HF_OpenFile( "relCat" ) ) < HFE_OK ) {
        HF_PrintError( "Couldn't open relCat" );
        return AMINIREL_GE;
    }

    if (( this_db.viewAttrCatDesc =  HF_OpenFile( "viewAttrCat" ) ) < HFE_OK ) {
        HF_PrintError( "Couldn't open viewAttrCat" );
        return AMINIREL_GE;
    }

    if (( this_db.viewCatDesc =  HF_OpenFile( "viewCat" ) ) < HFE_OK ) {
        HF_PrintError( "Couldn't open viewCat" );
        return AMINIREL_GE;
    }

    return AMINIREL_OK;
}


/**** void printArgs( int argc, char **argv ) ****

 Input:
        * int argc        Plh8os twn arguments
        * char **argv     Pinakas twn arguments

 Operation:
        * Ektypwnei ta parexomena arguments

 Return Value:
        * None
*/

void printArgs( int argc, char **argv )
{
    int i;

    for ( i = 0; i <= argc; ++i )
        printf( "argv[%d]=%s, ", i, argv[i] );

    printf( "\n\n" );
}


/**** int UT_create( int argc, char* argv[] ) ****

 Usage:
        CREATE relname (attr1=format1, ... , attrN=formatN);

 Operation:
        * Dhmiourgei mia nea bash dedomenwn
        * Eisagei tis arxikes eggrafes sta arxeia relCat, attrCat, oi opoies
          perigrafoun th bash


 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/

int UT_create( int argc, char* argv[] )
{
    relDesc relInfo, * relPtr;
    attrDesc attrInfo;
    char * pch, * relName;
    int i, j, offset, num;

    /* Apaitoume, ektos apo to onoma ths sxeshs:
       - toulaxiston ena gnwrisma kai ton typo tou
       - oloklhrwmena zeugh <onomatos gnwrismatos, typou gnwrismatos>
    */
    if (( argc < 4 ) || ( argc % 2 != 0 ) ) {
        printf( "Error: Cannot create relation: Not enough information\n" );
        return AMINIREL_GE;
    }

    relName = argv[1];

    /* max epitrepto mhkos onomatos = MAXNAME */
    if ( strlen( relName ) >= MAXNAME ) {
        printf( "Error: Cannot create relation: Relation name too long\n" );
        return AMINIREL_GE;
    }

    /* mhpws yparxei hdh h relation ? */
    if (( relPtr = getRelation( relName, &i ) ) != NULL ) {
        printf( "Error: Cannot create relation '%s'. Relation already exists.\n", relName );
        free( relPtr );
        return AMINIREL_GE;
    }
    
    /*
      * H relation "temp_relation" einai reserved apo to systhma
      * Epitrepetai na dimioyrgithei mono apo tis dm_select & dm_join
    */
    if (strcmp(relName, "temp_relation") == 0 && strcmp(argv[0], "\"DMcreate\"") != 0) {
       printf("Error: Cannot create 'temp_relation'. It is a relation reserved by the system\n");
       return AMINIREL_GE;
    }
    
    /* Elegxoume an dwthikan idia onomata se 2 attributes ths relation to opoio einai lathos */
    for ( i = 2; i < argc; i += 2 ) {
        for ( j = i + 2; j < argc; j += 2 ) {
            if ( strcmp( argv[i], argv[j] ) == 0 ) {
                printf( "Error: More than one attributes named '%s'.\n", argv[i] );
                return AMINIREL_GE;
            }
        }
    }


    /* Arxikopoihsh ths eggrafhs perigrafhs pleiadas */
    memset( relInfo.relname, 0, MAXNAME );
    strcpy( relInfo.relname, relName );
    /* To relwidth 8a symplhrw8ei meta, otan tha swsoume tis plhrofories twn attributes */
    relInfo.relwidth = 0;
    relInfo.attrcnt = ( argc - 2 ) / 2;
    relInfo.indexcnt = 0;

    /* Arxikopoihsh ths eggrafhs perigrafhs pediwn */
    memset( attrInfo.relname, 0, MAXNAME );
    strcpy( attrInfo.relname, relName );
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    /* Fortwsh twn plhroforiwn twn pediwn */
    for ( i = 2, offset = 0 ; argv[i] != NULL ; i += 2 ) {
        if ( strlen( argv[i] ) >= MAXNAME ) {
            printf( "Error: Cannot create relation: Attribute name too long\n" );
            return AMINIREL_GE;
        }

        memset( attrInfo.attrname, 0, MAXNAME );
        strcpy( attrInfo.attrname, argv[i] );
        attrInfo.offset = offset;

        pch = argv[i+1];

        /* Prospername to arxiko eisagwgiko ' */
        pch++;

        switch ( *pch ) {
            case 'i':
                offset += sizeof( int );
                attrInfo.attrtype = 'i';
                attrInfo.attrlength = sizeof( int );
                break;
            case 'f':
                offset += sizeof( float );
                attrInfo.attrtype = 'f';
                attrInfo.attrlength = sizeof( float );
                break;
            case 'c':
                num = atoi( pch + 1 );

                if ( num <= 0 || num > 255 ) {
                    printf( "Error: Wrong attribute size\n" );
                    return AMINIREL_GE;
                }

                offset += num;
                attrInfo.attrtype = 'c';
                attrInfo.attrlength = num;

                break;
            default:
                printf( "Error: Unrecognised type\n" );
                return AMINIREL_GE;
        }

        /* Pros8etoume thn eggrafh gia to sygkekrimeno gnwrisma */
        if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
            printf( "Error: Couldn't update attrCat\n" );
            return AMINIREL_GE;
        }
    }

    relInfo.relwidth = offset;

    /* Pros8etoume thn eggrafh ths sxeshs sto relCat */
    if ( updateRelCat( &relInfo ) != AMINIREL_OK ) {
        printf( "Error: Couldn't update relCat\n" );
        return AMINIREL_GE;
    }

    /* Dhmioyrgoyme to neo arxeio */
    if ( HF_CreateFile( relName ) != HFE_OK ) {
        HF_PrintError( "Could not create new relation file in UT_create() " );
        return AMINIREL_GE;
    }

    return AMINIREL_OK;
}


/**** int UT_buildindex( int argc, char* argv[] ) ****

 Usage:
        BUILDINDEX relname(attrName);

 Operation:
        * Dhmiourgei ena eurethrio gia ena sygkekrimeno gnwrisma


 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/

int UT_buildindex( int argc, char* argv[] )
{
    relDesc * relInfo;
    attrDesc * attrInfo;
    char * relName, * attrName, * record, * value;
    int relRecId, attrRecId, return_val, recId, indexFileDesc, fileDesc;

    if ( argc != 3 ) {
        printf( "Error: Three arguments expected for function UT_buildindex\n" );
        return AMINIREL_GE;
    }

    inline cleanup() {
        /* apodesmeush dynamikhs mnhmhs */
        if ( relInfo != NULL ) {
            free( relInfo );
        }

        if ( attrInfo != NULL ) {
            free( attrInfo );
        }

        if ( record != NULL ) {
            free( record );
        }

        /* Kleinoyme arxeia pou isws anoiksame */

        /* kleinoyme to arxeio ths sxeshs */
        if ( fileDesc >= 0 ) {
            if ( HF_CloseFile( fileDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close relation's file" );
                return_val = AMINIREL_GE;
            }
        }

        /* kleinoyme to eyrethrio */
        if ( indexFileDesc >= 0 ) {
            if ( AM_CloseIndex( indexFileDesc ) != AME_OK ) {
                AM_PrintError( "Could not close index" );
                return_val = AMINIREL_GE;
            }
        }
    }

    /* initialization */
    relName = argv[1];
    attrName = argv[2];
    relInfo = NULL;
    attrInfo = NULL;
    record = NULL;
    return_val = AMINIREL_OK;
    fileDesc = indexFileDesc = AMINIREL_INVALID;

    /* kanoume retrieve tis plhrofories gia th sxesh me onoma relName */
    if (( relInfo = getRelation( relName, &relRecId ) ) == NULL ) {
        printf( "Error: Could not retrieve information about relation '%s'\n", relName );
        return AMINIREL_GE;
    }

    /* Mias kai mporoume na exoume to poly 1 eurethrio gia kathe attribute, an o arithmos twn eurethriwn
       isoutai me ton arithmo twn attributes tote de ginetai na ftiaksoume allo index */
    if ( relInfo->attrcnt == relInfo->indexcnt ) {
        printf( "Error: Cannot create yet another index for any of %s's attributes, delete one first\n", relName );
        return_val =  AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* afou mporesame na vroume th sxesh relName, twra kanoume retrieve tis plhrofories gia to attribute ths */
    if (( attrInfo = getAttribute( relName, attrName, &attrRecId ) ) == NULL ) {
        printf( "Error: Could not retrieve information about attribute '%s' of relation %s\n", attrName, relName );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Vrhkame to attribute, twra elegxoume an yparxei hdh ena eurethrio gia to attribute! */
    if ( attrInfo->indexed == TRUE ) {
        printf( "Error: There is already an index for attribute '%s' of relation '%s'\n", attrName, relName );
        return_val =  AMINIREL_GE;
        cleanup();
        return return_val;
    }
    else {

        /* Mporoume na ftiaksoume neo eurethrio! :v */
        /* Enhmerwnoume to indexed kai indexno tou attrInfo */
        attrInfo->indexed = TRUE;
        attrInfo->indexno = relInfo->indexcnt;

        /* Enhmerwnoume ton indexcnt tou relInfo */
        ( relInfo->indexcnt )++;

        /* Mias kai to HF epipedo mas epistrefei antigrafa twn records, emeis twra pou exoume tis nees versions twn records
           prepei na diagrapsoume ta original kai na eisagoume pali ta kainourgia! */

        /* Diagrafh ths eggrafhs sto relCat */
        if ( HF_DeleteRec( this_db.relCatDesc, relRecId, sizeof( relDesc ) ) != HFE_OK ) {
            HF_PrintError( "Could not Destroy record in relCat " );
            return_val =  AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Diagrafh ths eggrafhs sto attrCat */
        if ( HF_DeleteRec( this_db.attrCatDesc, attrRecId, sizeof( attrDesc ) ) != HFE_OK ) {
            HF_PrintError( "Could not Destroy record in attrCat " );
            return_val =  AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Eisagwgh twn updated records sto relCat */
        if ( updateRelCat( relInfo ) != AMINIREL_OK ) {
            printf( "ton hpie h update relcat\n" )  ;
            return_val =  AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kai sto attrCat */
        if ( updateAttrCat( attrInfo ) != AMINIREL_OK ) {
            printf( "ton hpie h update attrcat\n" )  ;
            return_val =  AMINIREL_GE;
            cleanup();
            return return_val;
        }


        /* Telos, dhmioyrgoyme to eurethrio */

        if ( AM_CreateIndex( relName, attrInfo->indexno, attrInfo->attrtype, attrInfo->attrlength ) != AME_OK ) {
            AM_PrintError( "Could not Create Index in UT_buildindex() " );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /*
           Twra pou dhmioyrghsame to eyrethrio, prepei oses eggrafes yparxoun sto arxeio ths sxeshs na perastoun sto
           eurethrio!
        */

        /* Desmeuoume xwro gia to xwro pou tha apothikeuetai to kathe record tou arxeiou */
        if (( record = malloc( relInfo->relwidth ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        memset( record, 0, relInfo->relwidth );

        /* Anoigoume to eurethrio */
        if (( indexFileDesc = AM_OpenIndex( relName, attrInfo->indexno ) ) < 0 ) {
            AM_PrintError( "Could not Open Index in UT_buildindex() " );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Anoigoume to arxeio ths sxeshs */

        if (( fileDesc = HF_OpenFile( relName ) ) < 0 ) {
            HF_PrintError( "Could not Open relation's file in UT_buildindex() " );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* ksekiname me thn prwth eggrafh */
        if (( recId = HF_GetFirstRec( fileDesc, record, relInfo->relwidth ) ) < 0 ) {
            /* Yparxei h periptwsh to arxeio na mhn exei eggrafes */
            if ( recId != HFE_EOF ) {
                HF_PrintError( "Could not retrieve first Record from relation's file" );
                return_val = AMINIREL_GE;
            }

            cleanup();
            return return_val;
        }

        /* Eisagoume oles tis eggrafes tou arxeiou ths sxeshs, sto eurethrio */
        do {
            /* Thetoume ton pointer value sto swsto shmeio ths eggrafhs ap opou tha paroume tin timh mas */
            value = record + attrInfo->offset;

            /* Eisagoume thn eggrafh sto eurethrio */
            if ( AM_InsertEntry( indexFileDesc, attrInfo->attrtype, attrInfo->attrlength, value, recId ) != AME_OK ) {
                AM_PrintError( "Could not insert into index" );
                return_val = AMINIREL_GE;
            }
        }
        while (( recId = HF_GetNextRec( fileDesc, recId, record , relInfo->relwidth ) ) >= 0 );

        if ( recId != HFE_EOF ) {
            HF_PrintError( "Error: Cannot build index\n" );
            cleanup();
            return return_val;
        }

        return_val = AMINIREL_OK;
    }

    cleanup();
    return return_val;
}


/**** int UT_destroy( int argc, char* argv[] ) ****

 Usage:
        DESTROY relname;

 Operation:
        * Katastrefei to relation relname

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/

int UT_destroy( int argc, char* argv[] )
{
    int recId, attrcnt, indexcnt, return_val;
    char * relName, * record;
    relDesc * relPtr;

    if ( argc != 2 ) {
        printf( "Error: Two arguments expected for function UT_destroy\n" );
        return AMINIREL_GE;
    }

    return_val = AMINIREL_OK;

    inline cleanup() {
        if ( record != NULL ) {
            free( record );
        }

        if ( attrcnt > 0 ) {
            printf( "Error: De diagraftikan ola ta records twn attributes ths sxeshs '%s'\n", relName );
            return_val = AMINIREL_GE;
        }

        if ( indexcnt > 0 ) {
            printf( "Error: De diagraftikan ola ta indeces twn attributes ths sxeshs '%s'\n", relName );
            return_val = AMINIREL_GE;
        }

        if ( relName != NULL ) {
            if ( HF_DestroyFile( relName ) != HFE_OK ) {
                HF_PrintError( "Could not destroy relation in UT_destroy() " );
                return_val =  AMINIREL_GE;
            }
        }
    }

    /* Arxikopoihseis */
    relName = argv[1];
    record = NULL;

    /* Kanoume retrieve tis plhrofories gia thn relation */
    if (( relPtr = getRelation( relName, &recId ) ) == NULL ) {
        printf( "Error: Cannot destroy relation '%s'. Could not retrieve information about relation.\n", argv[1] );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* an th vrhkame, swzoume posa attributes kai indeces exei, kai th diagrafoume */
    attrcnt = relPtr->attrcnt;
    indexcnt = relPtr->indexcnt;
    free( relPtr );

    /* Diagrafoume thn eggrafh sto relCat */
    if ( HF_DeleteRec( this_db.relCatDesc, recId, sizeof( relDesc ) ) != HFE_OK ) {
        HF_PrintError( "Error in UT Destroy: Could not delete record from relCat\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }


    /* An th vrhkame sto relCat arxeio, anoigoume to attrCat arxeio gia na diagrapsoume ola ta records gia ta
       attrcnt synolika attributes ths */


    /* sto record tha apothikeyetai h eggrafh tou arxeiou attrCat */
    if (( record = malloc( sizeof( attrDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Ksekiname to psaksimo apo thn arxh tou arxeiou */
    if (( recId = HF_GetFirstRec( this_db.attrCatDesc, record, sizeof( attrDesc ) ) ) < 0 ) {
        HF_PrintError( "Could not retrieve first Record from file relCat" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    do {
        /* To onoma ths sxeshs einai sthn arxh tou record */
        if ( !strcmp( record, relName ) ) {
            char * pch;
            int indexed;

            /* to pch twra deixnei sto 'indexed' melos tou struct attrDesc */
            pch = record + 2 * MAXNAME * sizeof( char ) + 2 * sizeof( int ) + sizeof( char );
            memcpy( &indexed, pch, sizeof( int ) );

            /* An exei eurethrio, prepei na diagrafei kai auto */
            if ( indexed == TRUE ) {
                int indexno;

                /* to pch twra deixnei sto indexno melos tou struct attrDesc */
                pch += sizeof( int );
                memcpy( &indexno, pch, sizeof( int ) );

                /* diagrafoume to eurethrio */
                if ( AM_DestroyIndex( relName, indexno ) != AME_OK ) {
                    AM_PrintError( "Could not delete index: " );
                    return_val = AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                indexcnt--;
            }

            /* diagrafoume thn eggrafh tou attribute sto attrCat */
            if ( HF_DeleteRec( this_db.attrCatDesc, recId, sizeof( attrDesc ) ) != HFE_OK ) {
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            attrcnt--;
        }

    }
    while (( recId = HF_GetNextRec( this_db.attrCatDesc, recId, record  , sizeof( attrDesc ) ) ) >= 0 );

    if ( recId != HFE_EOF ) {
        HF_PrintError( "Error in UT Destroy while deleting records from file attrCat\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }


    cleanup();    
    return return_val;
}


/**** void UT_quit( void ) ****

 Usage:
        QUIT;

 Operation:
        * Termatizei to programma

 Return Value:
        * None
*/

void UT_quit( void )
{
    int exit_val = EXIT_SUCCESS;

    if ( HF_CloseFile( this_db.attrCatDesc ) != HFE_OK ) {
        HF_PrintError( "Couldn't close attrCat" );
        exit_val = EXIT_FAILURE;
    }

    if ( HF_CloseFile( this_db.relCatDesc ) != HFE_OK ) {
        HF_PrintError( "Couldn't close relCat" );
        exit_val = EXIT_FAILURE;
    }

    if ( HF_CloseFile( this_db.viewAttrCatDesc ) != HFE_OK ) {
        HF_PrintError( "Couldn't close viewAttrCat" );
        exit_val = EXIT_FAILURE;
    }

    if ( HF_CloseFile( this_db.viewCatDesc ) != HFE_OK ) {
        HF_PrintError( "Couldn't close viewCat" );
        exit_val = EXIT_FAILURE;
    }

    printf( "Bye!\n\n" );
    exit( exit_val );
}

/**** int updateRelCat( relDesc * relInfo ) ****

 Input:
        * relDesc * relInfo     H eggrafh pros apo8hkeush

 Operation:
        * Pros8etei thn eggrafh "relInfo" sto arxeio "relCat"

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/

int updateRelCat( relDesc * relInfo )
{
    char * record, * pch;

    if ( relInfo == NULL ) {
        return AMINIREL_GE;
    }

    /* Dhmiourgoume to record san ena megalo string, pou tha periexei me oles tis aparaithtes plhrofories,
       to opoio 8a perastei sto HF epipedo */

    if (( record = malloc( sizeof( relDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return AMINIREL_GE;
    }

    /* Fortwma twn plhroforiwn sthn eggrafh */
    memcpy( record, relInfo->relname, sizeof( relInfo->relname ) );
    pch = record + sizeof( relInfo->relname );
    memcpy( pch, &( relInfo->relwidth ), sizeof( relInfo->relwidth ) );
    pch += sizeof( relInfo->relwidth );
    memcpy( pch, &( relInfo->attrcnt ), sizeof( relInfo->attrcnt ) );
    pch += sizeof( relInfo->attrcnt );
    memcpy( pch, &( relInfo->indexcnt ), sizeof( relInfo->indexcnt ) );

    /* eisagoume thn eggrafh pou periexei tis plhrofories gia ton eayto toy */
    if ( HF_InsertRec( this_db.relCatDesc, record, sizeof( relDesc ) ) < HFE_OK ) {
        HF_PrintError( "Couldn't insert in relCat" );
        free( record );
        return AMINIREL_GE;
    }

    free( record );
    return AMINIREL_OK;
}


/**** int updateAttrCat( attrDesc * attrInfo ) ****

 Input:
        * attrDesc * attrInfo     H eggrafh pros apo8hkeush

 Operation:
        * Pros8etei thn eggrafh "attrInfo" sto arxeio "attrCat"

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/

int updateAttrCat( attrDesc * attrInfo )
{
    char * record, * pch;

    if ( attrInfo == NULL ) {
        return AMINIREL_GE;
    }

    /* Dhmiourgoume to record san ena megalo string, pou tha periexei me oles tis aparaithtes plhrofories,
       to opoio 8a perastei sto HF epipedo */

    if (( record = malloc( sizeof( attrDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return AMINIREL_GE;
    }


    /* Fortwma twn plhroforiwn sthn eggrafh */
    memcpy( record, attrInfo->relname, sizeof( attrInfo->relname ) );
    pch = record + sizeof( attrInfo->relname );
    memcpy( pch, attrInfo->attrname, sizeof( attrInfo->attrname ) );
    pch += sizeof( attrInfo->attrname );
    memcpy( pch, &( attrInfo->offset ), sizeof( attrInfo->offset ) );
    pch += sizeof( attrInfo->offset );
    memcpy( pch, &( attrInfo->attrlength ), sizeof( attrInfo->attrlength ) );
    pch += sizeof( attrInfo->attrlength );
    memcpy( pch, &( attrInfo->attrtype ), sizeof( attrInfo->attrtype ) );
    pch += sizeof( attrInfo->attrtype );
    memcpy( pch, &( attrInfo->indexed ), sizeof( attrInfo->indexed ) );
    pch += sizeof( attrInfo->indexed );
    memcpy( pch, &( attrInfo->indexno ), sizeof( attrInfo->indexno ) );

    /* eisagoume thn eggrafh pou periexei tis plhrofories gia ton eayto toy */
    if ( HF_InsertRec( this_db.attrCatDesc, record, sizeof( attrDesc ) ) < HFE_OK ) {
        HF_PrintError( "Couldn't insert in attrCat" );
        free( record );
        return AMINIREL_GE;
    }

    free( record );
    return AMINIREL_OK;
}

/**** relDesc getRelation( char * relName ) ****

 Input:
        * char * relName     To onoma ths sxeshs thn opoia anazhtoume
        * int * recId        Edw tha topothetithei to record Id pou antistoixei sthn eggrafh gia th sxesh ayth

 Operation:
        * Epistrefei pointer se struct tpoy relDesc pou periexei oles tis plhrofories gia th sxesh 'relName'
          kai swzei sth metavlhth 'recId' to record Id ths eggrafhs gia th sxesh 'relName' wste meta na mporesoyme
          na th diaxeiristoyme

 Return Value:
        * pointer sto struct typou relDesc, efoson epityxei
        * NULL, efoson apotyxei
*/

relDesc * getRelation( char * relName, int * recId )
{
    relDesc * relInfo;
    char * record, found_it, * pch;
    int temp_recId;

    inline cleanup() {
        if ( record != NULL ) {
            free( record );
        }

        if ( relInfo != NULL ) {
            free( relInfo );
        }
    }

    /* initialization */
    record = NULL;
    relInfo = NULL;
    ( *recId ) = AMINIREL_INVALID;

    /* desmeuoume mnhmh gia to struct pou tha krataei tis plhrofories ths sxeshs */
    if (( relInfo = malloc( sizeof( relDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return NULL;
    }

    /* desmeuoume mnhmh gia to xwro ston opoio tha eisaxthei h eggrafh tou arxeiou relCat */
    if (( record = malloc( sizeof( relDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return NULL;
    }

    /* ksekiname thn anazhthsh ths sxeshs apo thn prwth eggrafh sto arxeio relCat ! */
    if (( temp_recId = HF_GetFirstRec( this_db.relCatDesc, record, sizeof( relDesc ) ) ) < 0 ) {
        HF_PrintError( "Could not retrieve first record from file relCat" );
        cleanup();
        return NULL;
    }

    found_it = FALSE;

    do {
        /* To onoma ths sxeshs einai sthn arxh tou record */
        if ( !strcmp( record, relName ) ) {
            /* An bre8hke, symplhrwnoume tis plhrofories sto relInfo kai to epistrefoume */
            pch = record;

            /* symplhrwnoume to onoma ths sxeshs */
            memset( relInfo->relname, 0, sizeof( relInfo->relname ) );
            strcpy( relInfo->relname, pch );

            /* to relwidth */
            pch += sizeof( relInfo->relname );
            memcpy( &( relInfo->relwidth ), pch, sizeof( int ) );

            /* attrcnt */
            pch += sizeof( int );
            memcpy( &( relInfo->attrcnt ), pch, sizeof( int ) );

            /* indexcnt */
            pch += sizeof( int );
            memcpy( &( relInfo->indexcnt ), pch, sizeof( int ) );
            ( *recId ) = temp_recId;

            free( record );

            return relInfo;
        }
    }
    while (( temp_recId = HF_GetNextRec( this_db.relCatDesc, temp_recId, record, sizeof( relDesc ) ) ) >= 0 );

    if ( temp_recId != HFE_EOF ) {
        HF_PrintError( "Error in getRelation()\n" );
        cleanup();
        return NULL;
    }


    cleanup();
    return NULL;
}


/**** attrDesc * getAttribute( char * relName, char * attrName, int * recId ) ****

 Input:
        * char * relName     To onoma ths sxeshs ths opoias to attribute anazhtoume
        * char * attrName    To onoma tou attribute pou anazhtoume
        * int * recId        Edw tha topothetithei to record Id pou antistoixei sthn eggrafh gia to attribute ayto

 Operation:
        * Epistrefei pointer se struct typoy attrDesc pou periexei oles tis plhrofories gia to attribute 'attrName'
          kai swzei sth metavlhth 'recId' to record id ths eggrafhs gia to attribute 'attrName' wste meta na mporesoyme
          na th diaxeiristoyme

 Return Value:
        * pointer sto struct typou attrDesc, efoson epityxei
        * NULL, efoson apotyxei
*/

attrDesc * getAttribute( char * relName, char * attrName, int * recId )
{
    attrDesc * attrInfo;
    char * record, found_it, * pch;
    int temp_recId;

    inline cleanup() {
        if ( record != NULL ) {
            free( record );
        }

        if ( attrInfo != NULL ) {
            free( attrInfo );
        }
    }

    /* initialization */
    record = NULL;
    attrInfo = NULL;
    ( *recId ) = AMINIREL_INVALID;

    /* desmeuoume mnhmh gia to struct pou tha krataei tis plhrofories toy attribute */
    if (( attrInfo = malloc( sizeof( attrDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return NULL;
    }

    /* desmeuoume mnhmh gia to xwro ston opoio tha eisaxthei h eggrafh tou arxeiou attrCat */
    if (( record = malloc( sizeof( attrDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return NULL;
    }

    /* ksekiname thn anazhthsh ths sxeshs apo thn prwth eggrafh sto arxeio relCat ! */
    if (( temp_recId = HF_GetFirstRec( this_db.attrCatDesc, record, sizeof( attrDesc ) ) ) < 0 ) {
        HF_PrintError( "Could not retrieve first record from file relCat" );
        cleanup();
        return NULL;
    }

    found_it = FALSE;

    do {
        /* To onoma ths sxeshs einai sthn arxh tou record */
        if ( !strcmp( record, relName ) ) {
            pch = record;
            pch += sizeof( attrInfo->relname );

            /* To onoma tou attribute akolouthei amesws meta to onoma ths sxeshs */
            if ( !strcmp( pch, attrName ) ) {
                /* An bre8hke, symplirwnoume tis plhrofories sto attrInfo kai to epistrefoume */

                /* symplhrwnoume to onoma ths sxeshs */
                memset( attrInfo->relname, 0, sizeof( attrInfo->relname ) );
                strcpy( attrInfo->relname, relName );
                /* kai to onoma tou attribute */
                memset( attrInfo->attrname, 0, sizeof( attrInfo->attrname ) );
                strcpy( attrInfo->attrname, attrName );

                /* to offset */
                pch += sizeof( attrInfo->attrname );
                memcpy( &( attrInfo->offset ), pch, sizeof( int ) );

                /* attrlength */
                pch += sizeof( int );
                memcpy( &( attrInfo->attrlength ), pch, sizeof( int ) );

                /* attrtype */
                pch += sizeof( int );
                memcpy( &( attrInfo->attrtype ), pch, sizeof( char ) );

                /* indexed */
                pch += sizeof( char );
                memcpy( &( attrInfo->indexed ), pch, sizeof( int ) );

                /* indexno */
                pch += sizeof( int );
                memcpy( &( attrInfo->indexno ), pch, sizeof( int ) );

                ( *recId ) = temp_recId;

                free( record );

                return attrInfo;
            }
        }
    }
    while (( temp_recId = HF_GetNextRec( this_db.attrCatDesc, temp_recId, record  , sizeof( attrDesc ) ) ) >= 0 );

    if ( temp_recId != HFE_EOF ) {
        HF_PrintError( "Error in getRelation()\n" );
        cleanup();
        return NULL;
    }

    cleanup();
    return NULL;
}
