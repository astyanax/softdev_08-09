/* dbcreate.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "HF_Lib.h"

#define MAXNAME 32
#define MAXSTRINGLENGTH 64
#define LINE_SIZE 128

#define AMINIREL_GE -1
#define AMINIREL_OK 0

enum {FALSE, TRUE};

typedef struct {
    char relname[MAXNAME];  /* όνομα πίνακα */
    int relwidth;           /* εύρος εγγραφής πίνακα σε bytes */
    int attrcnt;            /* αριθμός πεδίων εγγραφής */
    int indexcnt;           /* αριθμός ευρετηρίων πίνακα */
} relDesc;

typedef struct {
    char relname[MAXNAME];  /* όνομα πίνακα */
    char attrname[MAXNAME]; /* όνομα πεδίου του πίνακα */
    int offset;             /* απόσταση αρχής πεδίου από την αρχή της εγγραφής σε bytes */
    int attrlength;         /* μήκος πεδίου σε bytes */
    char attrtype;          /* τύπος πεδίου ('i', 'f', ή 'c') */
    int indexed;            /* TRUE αν το πεδίο έχει ευρετήριο */
    int indexno;            /* αύξων αριθμός του ευρετηρίου αν indexed=TRUE */
} attrDesc;

typedef struct {
    char viewname[MAXNAME];
    int type;
    char relname1[MAXNAME];
    char attrname1[MAXNAME];
    int op;
    char relname2[MAXNAME];
    char attrname2[MAXNAME];
    char value[MAXSTRINGLENGTH];
    int attrcnt;
} viewDesc;

typedef struct {
    char viewname[MAXNAME];
    char viewattrname[MAXNAME];
    char relname[MAXNAME];
    char relattrname[MAXNAME];
} viewAttrDesc;

int updateRelCat( relDesc * relInfo );
int updateAttrCat( attrDesc * attrInfo );

int main( int argc, char * argv[] )
{
    char command[LINE_SIZE];
    char * dbname;
    relDesc relInfo;
    attrDesc attrInfo;
    int offset;

    HF_Init();

    if ( argc != 2 ) {
        printf( "La8os arguments\n" );
        return AMINIREL_GE;
    }

    dbname = argv[1];

    /* elegxoume an yparxei hdh h database gia na mh ginei katalathos overwrite */
    if ( !chdir( dbname ) ) {
        printf( "Database %s already exists", dbname );
        return AMINIREL_GE;
    }

    sprintf( command, "mkdir %s", dbname );
    system( command );

    if ( chdir( dbname ) ) {
        printf( "Den eftiaksa database giati den eftiaksa enan katalogo, poso gay eimai?" );
        return AMINIREL_GE;
    }

    if ( HF_CreateFile( "relCat" ) != HFE_OK ) {
        HF_PrintError( "Couldn't create relCat" );
        exit( EXIT_FAILURE );
    }

    if ( HF_CreateFile( "attrCat" ) != HFE_OK ) {
        HF_PrintError( "Couldn't create attrCat" );
        exit( EXIT_FAILURE );
    }

    if ( HF_CreateFile( "viewCat" ) != HFE_OK ) {
        HF_PrintError( "Couldn't create viewCat" );
        exit( EXIT_FAILURE );
    }

    if ( HF_CreateFile( "viewAttrCat" ) != HFE_OK ) {
        HF_PrintError( "Couldn't create viewAttrCat" );
        exit( EXIT_FAILURE );
    }

    /* gemizoume to struct relInfo me tis plhrofories tou relCat gia ton katalogo relCat */
    memset( relInfo.relname, 0, sizeof( relInfo.relname ) );
    strcpy( relInfo.relname, "relCat" );
    relInfo.relwidth = sizeof( relDesc );
    relInfo.attrcnt = 4;
    relInfo.indexcnt = 0;


    if ( updateRelCat( &relInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto relCat arxeio\n" );
        return AMINIREL_GE;
    }

    /* gemizoume to struct relInfo me tis plhrofories tou attrCat gia ton katalogo relCat */
    memset( relInfo.relname, 0, sizeof( relInfo.relname ) );
    strcpy( relInfo.relname, "attrCat" );
    relInfo.relwidth = sizeof( attrDesc );
    relInfo.attrcnt = 7;
    relInfo.indexcnt = 0;


    if ( updateRelCat( &relInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto relCat arxeio\n" );
        return AMINIREL_GE;
    }

    /* gemizoume to struct relInfo me tis plhrofories tou viewCat gia ton katalogo relCat */
    memset( relInfo.relname, 0, sizeof( relInfo.relname ) );
    strcpy( relInfo.relname, "viewCat" );
    relInfo.relwidth = sizeof( attrDesc );
    relInfo.attrcnt = 9;
    relInfo.indexcnt = 0;


    if ( updateRelCat( &relInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto relCat arxeio\n" );
        return AMINIREL_GE;
    }

    /* gemizoume to struct relInfo me tis plhrofories tou viewAttrCat gia ton katalogo relCat */
    memset( relInfo.relname, 0, sizeof( relInfo.relname ) );
    strcpy( relInfo.relname, "viewAttrCat" );
    relInfo.relwidth = sizeof( attrDesc );
    relInfo.attrcnt = 4;
    relInfo.indexcnt = 0;


    if ( updateRelCat( &relInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto relCat arxeio\n" );
        return AMINIREL_GE;
    }

    /* Gia kathe attribute twn 2 pinakwn, eisagoume tis plhrofories tou sto attrCat katalogo */

    /* Prwta gia ta attributes tou relCat */
    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "relCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "relname" );
    attrInfo.offset = 0;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset = MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "relCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "relwidth" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( int );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "relCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "attrcnt" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( int );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "relCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "indexcnt" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    /* Twra eisagoume tis plhrofories gia ta attributes tou attrCat */
    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "attrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "relname" );
    attrInfo.offset = 0;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset = MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "attrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "attrname" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "attrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "offset" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( int );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "attrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "attrlength" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( int );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "attrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "attrtype" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( char );
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( char );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "attrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "indexed" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( int );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "attrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "indexno" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    /* Twra eisagoume tis plhrofories gia ta attributes tou viewCat */
    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "viewname" );
    attrInfo.offset = 0;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset = MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "type" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( int );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "relname1" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "attrname1" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "op" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += sizeof( int );

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "relname2" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "attrname2" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "value" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXSTRINGLENGTH;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXSTRINGLENGTH;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "attrcnt" );
    attrInfo.offset = offset;
    attrInfo.attrlength = sizeof( int );
    attrInfo.attrtype = 'i';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    /* Twra eisagoume tis plhrofories gia ta attributes tou viewAttrCat */
    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewAttrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "viewname" );
    attrInfo.offset = 0;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset = MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewAttrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "viewattrname" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewAttrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "relname" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    offset += MAXNAME;

    memset( attrInfo.relname, 0, sizeof( attrInfo.relname ) );
    strcpy( attrInfo.relname, "viewAttrCat" );
    memset( attrInfo.attrname, 0, sizeof( attrInfo.attrname ) );
    strcpy( attrInfo.attrname, "relattrname" );
    attrInfo.offset = offset;
    attrInfo.attrlength = MAXNAME;
    attrInfo.attrtype = 'c';
    attrInfo.indexed = FALSE;
    attrInfo.indexno = 0;

    if ( updateAttrCat( &attrInfo ) != AMINIREL_OK ) {
        printf( "Den mporese na ginei h eisagwgh ths record sto attrCat arxeio\n" );
        return AMINIREL_GE;
    }

    return AMINIREL_OK;
}


int updateRelCat( relDesc * relInfo )
{
    char * record, * pch;
    int fileDesc;

    if ( relInfo == NULL ) {
        return AMINIREL_GE;
    }

    /* dhmioyrgoyme to record san ena megalo string me oles tis plhrofories gia na perastei sto HF epipedo */
    if (( record = malloc( sizeof( relDesc ) ) ) == NULL ) {
        printf( "Memory Allocation Error\n" );
        return AMINIREL_GE;
    }

    memcpy( record, relInfo->relname, sizeof( relInfo->relname ) );
    pch = record + sizeof( relInfo->relname );
    memcpy( pch, &( relInfo->relwidth ), sizeof( relInfo->relwidth ) );
    pch += sizeof( relInfo->relwidth );
    memcpy( pch, &( relInfo->attrcnt ), sizeof( relInfo->attrcnt ) );
    pch += sizeof( relInfo->attrcnt );
    memcpy( pch, &( relInfo->indexcnt ), sizeof( relInfo->indexcnt ) );

    /* anoigoume to arxeio relCat */
    if (( fileDesc =  HF_OpenFile( "relCat" ) ) < 0 ) {
        HF_PrintError( "Couldn't open relCat" );
        free( record );
        return AMINIREL_GE;
    }

    /* eisagoume thn eggrafh sto relCat */
    if ( HF_InsertRec( fileDesc, record, sizeof( relDesc ) ) < 0 ) {
        HF_PrintError( "Couldn't insert in relCat" );
        free( record );
        return AMINIREL_GE;
    }

    if ( HF_CloseFile( fileDesc ) != HFE_OK ) {
        HF_PrintError( "Couldn't close relCat" );
        free( record );
        return AMINIREL_GE;
    }

    return AMINIREL_OK;
}

int updateAttrCat( attrDesc * attrInfo )
{
    char * record, * pch;
    int fileDesc;

    if ( attrInfo == NULL ) {
        return AMINIREL_GE;
    }

    /* dhmioyrgoyme to record san ena megalo string me oles tis plhrofories gia na perastei sto HF epipedo */
    if (( record = malloc( sizeof( attrDesc ) ) ) == NULL ) {
        printf( "Memory Allocation Error\n" );
        return AMINIREL_GE;
    }

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

    /* anoigoume to arxeio attrCat */
    if (( fileDesc =  HF_OpenFile( "attrCat" ) ) < 0 ) {
        HF_PrintError( "Couldn't open attrCat" );
        free( record );
        return AMINIREL_GE;
    }

    /* eisagoume thn eggrafh sto attrCat */
    if ( HF_InsertRec( fileDesc, record, sizeof( attrDesc ) ) < 0 ) {
        HF_PrintError( "Couldn't insert in attrCat" );
        free( record );
        return AMINIREL_GE;
    }

    if ( HF_CloseFile( fileDesc ) != HFE_OK ) {
        HF_PrintError( "Couldn't close attrCat" );
        free( record );
        return AMINIREL_GE;
    }

    return AMINIREL_OK;
}
