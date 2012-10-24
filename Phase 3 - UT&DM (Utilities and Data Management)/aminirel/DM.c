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

/**** int DM_select( int argc, char* argv[] ) ****

 Usage:
        SELECT [INTO target_relname] (rel.attrX, ...) [WHERE rel.attrY <OP> value]

 Operation:
        * An yparxei to kommati WHERE, anakta tis eggrafes ths sxeshs rel pou ikanopoioun th
          synthikh sto WHERE kommati
        * An den yparxei to WHERE kommati, anakta oles tis eggrafes ths sxeshs rel
        * To mesaio kommati ths target list, to mono ypoxrewtiko, dhlwnei poia attributes ths sxeshs rel
          tha kratithoun gia to apotelesma
        * An yparxei to kommati INTO, to apotelesma anakateythinetai se mia nea sxesh target_relname
        * An den yparxei to kommati INTO, to apotelesma ektypwnetai

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/
int DM_select( int argc, char* argv[] )
{
    int i, j, t, ival, return_val, this_arg, attrcnt, recId, fileDesc, indexFileDesc, conditionAttr, length, create_argc;
    int targetFileDesc, scanDesc, offset, argLength;
    float fval;
    char * relName, * attrName, * newRelName, op, * pch, use_index, newRelation, ** create_argv, * record, * tempRecord;
    char * targetRecord, createdTempFile;
    relDesc * relPtr, * targetRelPtr;
    attrDesc ** attributes, * attrPtr;
    void * value, * temp;

    inline cleanup()
    {
        /* eleytherwnoyme dynamikh mnhmh poy desmeysame */
        if ( attributes != NULL ) {
            for ( i = 0; i < ( attrcnt + 1 ); i++ ) {
                if ( attributes[i] != NULL ) {
                    free( attributes[i] );
                }
            }

            free( attributes );
        }

        if ( record != NULL ) {
            free( record );
        }

        if ( tempRecord != NULL ) {
            free( tempRecord );
        }

        if ( targetRecord != NULL ) {
            free( targetRecord );
        }

        if ( relPtr != NULL ) {
            free( relPtr );
        }

        if ( targetRelPtr != NULL ) {
            free( targetRelPtr );
        }

        /* An exoume anoixto kapoio scan */
        if ( scanDesc >= 0 ) {
            /* einai scan tou eurethriou */
            if ( use_index == TRUE ) {
                if ( AM_CloseIndexScan( scanDesc ) != AME_OK ) {
                    AM_PrintError( "Cannot close index scan" );
                    return_val = AMINIREL_GE;
                }
            }
            /* alliws scan tou arxeiou ths sxeshs */
            else {
                if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                    HF_PrintError( "Cannot close index scan" );
                    return_val = AMINIREL_GE;
                }
            }
        }

        /* kleinoyme to eyrethrio tou condition attribute attrName ths sxeshs relName, an to anoiksame */
        if ( indexFileDesc >= 0 ) {
            if ( AM_CloseIndex( indexFileDesc ) != AME_OK ) {
                AM_PrintError( "Could not close index file" );
                return_val = AMINIREL_GE;
            }
        }

        /* kleinoyme to arxeio ths neas sxeshs, an to anoiksame */
        if ( targetFileDesc >= 0 ) {
            if ( HF_CloseFile( targetFileDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close relation's file" );
                return_val = AMINIREL_GE;
            }
        }
        
        /* kleinoyme to arxeio ths sxeshs relName, an to anoiksame */
        if ( fileDesc >= 0 ) {
            if ( HF_CloseFile( fileDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close relation's file" );
                return_val = AMINIREL_GE;
            }
        }
        
        /* An h sxesh pou dhmioyrghsame htan proswrinh, thn diagrafoume */
        if ( createdTempFile == TRUE ) {
            /* Diagrafoume th sxesh kalwntas thn UT_destroy() */
            strcpy( create_argv[0], "destroy" );
    
            if ( UT_destroy( 2, create_argv ) != AMINIREL_OK ) {
                printf( "Error while destroying temp relation\n" );
                return_val = AMINIREL_GE;
            }
        }
        
        /* afou xrhsimopoihsame to create_argv, to diagrafoume */
        if ( create_argv != NULL ) {
            for ( i = 0; i < create_argc; i++ ) {
                if ( create_argv[i] != NULL ) {
                    free( create_argv[i] );
                }
            }

            free( create_argv );
        }
    }        

    /* initialization */
    return_val = AMINIREL_OK;
    this_arg = 1;
    fileDesc = indexFileDesc = targetFileDesc = AMINIREL_INVALID;
    scanDesc = AMINIREL_INVALID;
    attributes = NULL;
    create_argv = NULL;
    targetRelPtr = NULL;
    relPtr = NULL;
    record = NULL;
    use_index = FALSE;
    createdTempFile = FALSE;

    /*
      Shmasiologikoi elegxoi:
    */
    
    /* Apaitoume toulaxiston 4 arguments: select 1 relname attrname */
    
    if ( argc < 4 ) {
       printf("Error: Cannot Select. Too few arguments provided.\n");
       return AMINIREL_GE;
    }

    /*
       * elegxoume an yparxei to proairetiko kommati "INTO relName".
       * H atoi epistrefei 0 se periptwsh sfalmatos kai de ginetai na exoume 0 san plithos orismatwn pou provallontai
    */
    if ( atoi( argv[this_arg] ) == 0 ) {
        newRelName = argv[this_arg];
        newRelation = TRUE;
        this_arg++;

        /* Mias kai tha dhmioyrghsoyme nea sxesh, prepei na mhn yparxei hdh allh sxesh me to idio onoma! */
        if (( relPtr = getRelation( newRelName, &recId ) ) != NULL ) {
            printf( "Error: Cannot Select. Relation '%s' already exists and cannot be overwritten.\n", newRelName );
            free( relPtr );
            return AMINIREL_GE;
        }
    }
    else {
        newRelName = NULL;
        newRelation = FALSE;
    }

    /* Swzoyme to plithos twn orismatwn pou provallontai */
    if (( attrcnt = atoi( argv[this_arg] ) ) == 0 ) {
        printf( "Error: Cannot select. Could not parse number of attributes.\n" );
        return AMINIREL_GE;
    }

    this_arg++;

    /*
       * tha xeiristoume to poly attrcnt + 1 attributes. to +1 gia to attribute pou yparxei sto condition, an yparxei condition.
       * desmeuoume mnhmh gia ta attributes ayta
    */

    if (( attributes = malloc(( attrcnt + 1 ) * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return AMINIREL_GE;
    }

    /* H thesh ston pinaka opou tha apothikeutei to condition attribute */
    conditionAttr = attrcnt;

    /* initialization toy pinaka gia ta attributes */
    for ( i = 0; i < ( attrcnt + 1 ); i++ ) {
        attributes[i] = NULL;
    }

    /* to onoma ths sxeshs pou xeirizomaste */
    relName = argv[this_arg];

    /* Anoigoume to arxeio ths sxeshs apo to epipedo HF, tha mas xrhsimeysei argotera */
    if (( fileDesc = HF_OpenFile( relName ) ) < 0 ) {
        HF_PrintError( "Could not open relation's file" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Kanoume retrieve tis plhrofories gia th sxesh */
    if (( relPtr = getRelation( relName, &recId ) ) == NULL ) {
        printf( "Error: Cannot select from relation '%s'. Information about relation could not be retrieved.\n", relName );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Kanoyme retrieve tis plhrofories gia kathe ena apo ta attributes pou provallontai */
    for ( i = 0; i < attrcnt; i++, this_arg += 2 ) {
        /* Ta provallomena attributes prepei na einai ths idias sxeshs! */
        if ( strcmp( relName, argv[this_arg] ) != 0 ) {
            printf( "Error: Cannot select. Cannot handle attributes from different relations '%s' and '%s'.\n",
                    relName, argv[this_arg] );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        if (( attributes[i] = getAttribute( argv[this_arg], argv[this_arg + 1], &recId ) ) == NULL ) {
            printf( "Error: Cannot select. Could not retrieve information about attribute '%s' of relation '%s'\n",
                    argv[this_arg + 1], argv[this_arg] );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }
    
    /* Elegxoume an mas exei dwthei 2 fores to idio pedio gia provolh */
    for (i = 0; i < attrcnt; i++) {
     for (j = i + 1; j < attrcnt; j++) {
         if ( strcmp(attributes[i]->attrname, attributes[j]->attrname) == 0 ) {
            printf("Error: Cannot select. Duplicate attribute '%s'.\n", attributes[i]->attrname);
         }
     }
    }

    /*
       elegxoume an yparxei to proairetiko kommati "WHERE condition". An yparxei, ftasame sto telos twn arguments
    */
    if ( this_arg == argc ) {
        /* An den yparxei "where" kommati, xeirizomaste oles tis eggrafes */
        op = GET_ALL;
        value = NULL;
        conditionAttr = 0;
    }
    /* Alliws, an yparxei to "WHERE condition" kommati */
    else {
        /* To attribute sto condition prepei na einai ths idias sxeshs me ta provallomena attributes */
        if ( strcmp( relName, argv[this_arg] ) != 0 ) {
            printf( "Error: Cannot select. Cannot handle attributes from different relations '%s' and '%s'.\n",
                    relName, argv[this_arg] );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* to onoma tou attribute sto condition */
        attrName = argv[this_arg + 1];

        /* kanoyme retrieve tis plhrofories tou attribute pou einai mesa sto condition */
        if (( attributes[conditionAttr] = getAttribute( argv[this_arg], argv[this_arg + 1], &recId ) ) == NULL ) {
            printf( "Error: Cannot select. Could not retrieve information about attribute '%s' of relation '%s'\n",
                    argv[this_arg + 1], argv[this_arg] );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* proxwrame ston operator */
        this_arg += 2;

        if ( strcmp( argv[this_arg], "=" ) == 0 ) {
            op = EQUAL;
        }
        else if ( strcmp( argv[this_arg], "!=" ) == 0 ) {
            op = NOT_EQUAL;
        }
        else if ( strcmp( argv[this_arg], "<" ) == 0 ) {
            op = LESS_THAN;
        }
        else if ( strcmp( argv[this_arg], ">" ) == 0 ) {
            op = GREATER_THAN;
        }
        else if ( strcmp( argv[this_arg], ">=" ) == 0 ) {
            op = GREATER_THAN_OR_EQUAL;
        }
        else if ( strcmp( argv[this_arg], "<=" ) == 0 ) {
            op = LESS_THAN_OR_EQUAL;
        }
        else {
            printf( "Error: Unrecognized symbol '%s' for operator.\n", argv[4] );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* An yparxei eyrethrio, tote to anoigoyme gia na to xrhsimopoihsoyme */
        if ( attributes[conditionAttr]->indexed == TRUE && ( op == EQUAL || op == NOT_EQUAL ) ) {
            if (( indexFileDesc = AM_OpenIndex( relName, attributes[conditionAttr]->indexno ) ) < 0 ) {
                AM_PrintError( "Cannot open index" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            use_index = TRUE;
        }

        /* Telos, mas menei h timh sygkrishs */
        this_arg++;

        /* desmeuoume xwro ston opoio tha kratithei to value */
        if (( value = malloc( attributes[conditionAttr]->attrlength ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* initialize it */
        memset( value, 0, attributes[conditionAttr]->attrlength );

        /* kai kanoyme tis antistoixes energeies */

        switch ( attributes[conditionAttr]->attrtype ) {
            case 'i':
                ival = atoi( argv[this_arg] );

                /* Otan apotygxanei h atoi epistrefei 0. Ara an epestrepse 0 kai o arithmos mas den einai
                   pragmatika to 0, tote error! */
                if ( ival == 0 && strcmp( argv[this_arg], "0" ) != 0 ) {
                    printf( "Error: Cannot Select. Expected integer as value for condition attribute %s\n", attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* antigrafoume thn timh sto value apo to argument mas */
                memcpy( value, &ival, attributes[conditionAttr]->attrlength );

                break;

            case 'f':
                fval = atof( value );

                /* Otan apotygxanei h atof epistrefei 0.0. Ara an epestrepse 0.0 kai o arithmos mas den einai
                   pragmatika to 0.0, tote error! */
                if ( fval == 0.0 && strcmp( argv[this_arg], "0.0" ) != 0 && strcmp( argv[this_arg], "0" ) != 0 ) {
                    printf( "Error: Cannot Select. Expected float as value for condition attribute %s\n", attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* antigrafoume thn timh sto value apo to argument mas */
                memcpy( value, &fval, attributes[conditionAttr]->attrlength );
                break;

            case 'c':
                pch = argv[this_arg];

                /* An exoume string prepei na mas exei dwthei anamesa se " */
                if ( pch[0] != '\"' || pch[strlen( pch ) - 1] != '\"' ) {
                    printf( "Error: Cannot Select. Expected string as value for condition attribute %s\n", attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* prospername to arxiko ' */
                pch++;
                /* kai diagrafoume to teliko ' */
                pch[strlen( pch ) - 1] = '\0';

                strncpy( value, pch, attributes[conditionAttr]->attrlength );
                break;

            default:
                printf( "Error: Cannot Select. Unknown attrtype for attribute '%s'.\n", attrName );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
        } /* switch */
    } /* else */


    /*
      Telos shmasiologikwn elegxwn
    */


    /*
      Dhmioyrgoyme mia sxesh sthn opoia tha mpoyn ta records pou ikanopoioyn th synthikh. An yparxei to
      "INTO new_relation" kommati, tote ayth h sxesh apoktaei to onoma new_relation kai de diagrafetai sto telos ths synarthshs.
      Alliws to onoma ths einai temp_relation
    */

    /*
       * Dhmioyrgoyme ton pinaka me ta arguments poy prepei na perasthei sth synarthsh UT_create()
       * Ta 2 prwta arguments einai to "create" kai to onoma ths sxeshs. Ta ypoloipa arguments einai zeygaria gia kathe
         attribute pou provalletai.
    */

    create_argc = 2 + ( attrcnt * 2 );

    /* desmeuoume mnhmh gia ton pinaka me ta arguments, + 1 gia to NULL sto telos */
    if (( create_argv = malloc(( create_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Initialize it */
    for ( i = 0; i <= create_argc; i++ ) {
        create_argv[i] = NULL;
    }

    /* 
       to megalytero attrlength pou epitrepetai einai 255 xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws: 
    */

    argLength = 255 + 2; /* +2 gia ta ' */

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }
    
    /* desmeuoume mnhmh gia ta arguments */
    for ( i = 0; i < create_argc; i++ ) {
        if (( create_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }

    /* Gemizoume ton pinaka me tis swstes times */
    strcpy( create_argv[0], "\"DMcreate\"" );

    /* An yparxei to "INTO new_relation" kommati tote pairnoume to onoma ths sxeshs apo to antistoixo argument */
    if ( newRelation == TRUE ) {
        length = strlen( newRelName ) + 1;

        /* De theloume n'antigrapsoume parapanw xaraktires apo MAXNAME */
        if ( length > MAXNAME ) {
            length = MAXNAME;
        }

        memcpy( create_argv[1], newRelName, length );
        create_argv[1][length - 1] = '\0';
    }
    /* Alliws h sxesh onomazetai temp_relation */
    else {
        strcpy( create_argv[1], "temp_relation" );
    }

    /* gia ta diafora attributes gemizoume ta arguments */
    for ( i = 2, j = 0; i < create_argc; i += 2, j++ ) {
        strcpy( create_argv[i], attributes[j]->attrname );

        if ( attributes[j]->attrtype == 'c' ) {
            sprintf( create_argv[i+1], "\"c%d\"", attributes[j]->attrlength );
        }
        else {
            sprintf( create_argv[i+1], "\"%c\"", attributes[j]->attrtype );
        }
    }

    /* Dhmioyrgoyme th sxesh sthn opoia tha anakateythinoume to apotelesma ths select */
    if ( UT_create( create_argc, create_argv ) != AMINIREL_OK ) {
        printf( "Error: Cannot Select. Could not create temp relation file\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }
    
    if (newRelation == FALSE) {
       createdTempFile = TRUE;
    }
    
    /* Anoigoume to arxeio ths neas sxeshs apo to epipedo HF gia na eisagoume se auto tis eggrafes */
    if (( targetFileDesc = HF_OpenFile( create_argv[1] ) ) < 0 ) {
        HF_PrintError( "Could not open relation's file" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Kai vriskoume ta dedomena ths neas sxeshs pou tha mas xrhsimeysoyn otan tha kanoume insert */
    if (( targetRelPtr = getRelation( create_argv[1], &recId ) ) == NULL ) {
        printf( "Error: Cannot Select. Information about new relation '%s' could not be retrieved.\n", create_argv[1] );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }


    /* desmeyoyme mnhmh gia to record toy arxeioy ths sxeshs mas */

    if (( record = malloc( relPtr->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( tempRecord = malloc( relPtr->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* kathws kai gia to record ths neas sxeshs */
    if (( targetRecord = malloc( targetRelPtr->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    memset( targetRecord, 0, targetRelPtr->relwidth );

    /*
      Twra, prepei na vroume tis eggrafes pou ikanopoioyn th synthikh kai na tis eisagoume sto arxeio ths neas sxeshs!
    */

    /* An xrhsimopoioyme eyrethrio */
    if ( use_index == TRUE ) {
        /* Ksekiname mia sarwsh sto eyrethrio */
        if (( scanDesc = AM_OpenIndexScan( indexFileDesc, attributes[conditionAttr]->attrtype, attributes[conditionAttr]->attrlength, op, value ) ) < 0 ) {
            AM_PrintError( "Cannot start a scan in index file" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Kathe eggrafh pou vriskoume na ikanopoiei th synthikh */
        while (( recId = AM_FindNextEntry( scanDesc ) ) >= 0 ) {
            /* Anaktoume thn eggrafh */
            if ( HF_GetThisRec( fileDesc, recId, tempRecord, relPtr->relwidth ) != HFE_OK ) {
                HF_PrintError( "Cannot retrieve record from relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /*
               * Dhmioyrgoyme thn eggrafh pou tha topothetithei sth nea sxesh. H eggrafh apoteleitai mono apo ta attributes pou
                 provallontai
               * Ta attributes pou provallontai ta exoume me auksousa seira analoga to offset tous ston pinaka attributes
            */

            for ( i = 0, offset = 0; i < attrcnt; i++ ) {
                /* gia kathe ena apo ta provallomena attributes, valto sthn eggrafh gia th nea sxesh! */
                value = tempRecord + attributes[i]->offset;
                memcpy(( targetRecord + offset ), value, attributes[i]->attrlength );
                offset += attributes[i]->attrlength;
            }

            /* eisagoume thn eggrafh mas sto arxeio ths neas sxeshs */

            if ( HF_InsertRec( targetFileDesc, targetRecord, targetRelPtr->relwidth ) < 0 ) {
                HF_PrintError( "Could not insert record at new relation's file" );
                return_val = AMINIREL_GE;
            }

        } /* while */

        if ( recId != AME_EOF ) {
            AM_PrintError( "Error while finding next entry" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    } /* if */

    /* An de xrhsimopoioyme eyrethrio */
    else {
        /* Ksekiname mia sarwsh sto arxeio ths sxeshs */
        if (( scanDesc = HF_OpenFileScan( fileDesc, relPtr->relwidth , attributes[conditionAttr]->attrtype, attributes[conditionAttr]->attrlength, attributes[conditionAttr]->offset, op, value ) ) < 0 ) {
            HF_PrintError( "Could not start a scan in relation's file" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Kathe eggrafh pou vriskoume na ikanopoiei th synthikh */
        while (( recId = HF_FindNextRec( scanDesc, record ) ) >= 0 ) {
            /*
               * Dhmioyrgoyme thn eggrafh pou tha topothetithei sth nea sxesh. H eggrafh apoteleitai mono apo ta attributes pou
                 provallontai
               * Ta attributes pou provallontai ta exoume me auksousa seira analoga to offset tous ston pinaka attributes
            */

            for ( i = 0, offset = 0; i < attrcnt; i++ ) {
                /* gia kathe ena apo ta provallomena attributes, valto sthn eggrafh gia th nea sxesh! */
                value = record + attributes[i]->offset;
                memcpy(( targetRecord + offset ), value, attributes[i]->attrlength );
                offset += attributes[i]->attrlength;
            }

            /* eisagoume thn eggrafh mas sto arxeio ths neas sxeshs */

            if ( HF_InsertRec( targetFileDesc, targetRecord, targetRelPtr->relwidth ) < 0 ) {
                HF_PrintError( "Could not insert record at new relation's file" );
                return_val = AMINIREL_GE;
            }

        } /* while */

        if ( recId != HFE_EOF ) {
            HF_PrintError( "Error while finding next record." );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    } /* else */

    /* An h sxesh pou dhmioyrghsame htan proswrinh, thn ektypwnoume */
    if ( newRelation == FALSE ) {
        /* ektypwnoume tis eggrafes ths sxeshs mas */

        if ( printRelation( create_argv[1] ) != AMINIREL_OK ) {
            printf( "Error while priting the records of select!\n" );
            return_val = AMINIREL_GE;
        }

        /* Kleinoyme to arxeio ths temp sxeshs */
        if ( HF_CloseFile( targetFileDesc ) != HFE_OK ) {
            HF_PrintError( "Could not close temp relation's file" );
            return_val = AMINIREL_GE;
        }

        targetFileDesc = -1;
    }

    cleanup();
    return return_val;
}
/**** int printRelation( char* relName ) ****

 Operation:
        * Ektypwnei oles tis eggyres eggrafes apo to arxeio mias sxeshs me onoma relName

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/
int printRelation( char * relName )
{
    int return_val, recId, scanDesc, fileDesc, i;
    char * record, * pch;
    relDesc * relPtr;
    attrDesc ** attributes;
    void * value;

    inline cleanup() {
        /* An exoume anoixto scan */
        if ( scanDesc >= 0 ) {
            if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                HF_PrintError( "Cannot close index scan" );
                return_val = AMINIREL_GE;
            }
        }

        /* eleytherwnoyme dynamikh mnhmh poy desmeysame */
        if ( record != NULL ) {
            free( record );
        }

        if ( attributes != NULL ) {
            for ( i = 0; i < relPtr->attrcnt ; i++ ) {
                if ( attributes[i] != NULL ) {
                    free( attributes[i] );
                }
            }

            free( attributes );
        }

        if ( relPtr != NULL ) {
            free( relPtr );
        }

        /* kleinoyme to arxeio ths sxeshs */
        if ( HF_CloseFile( fileDesc ) != HFE_OK ) {
            HF_PrintError( "Could not close relation's file" );
            return_val = AMINIREL_GE;
        }
    }

    /* initialization */
    return_val = AMINIREL_OK;
    relPtr = NULL;
    attributes = NULL;
    record = NULL;
    scanDesc = -1;
    fileDesc = -1;

    /* Anakthsh plhroforiwn gia th sxesh */
    if (( relPtr = getRelation( relName, &recId ) ) == NULL ) {
        printf( "Error: Cannot print records of relation '%s'. Could not retrieve information about the relation.\n", relName );
        return AMINIREL_GE;
    }

    /* Anoigoume to arxeio ths sxeshs apo to epipedo HF, tha mas xrhsimeysei argotera */
    if (( fileDesc = HF_OpenFile( relName ) ) < 0 ) {
        HF_PrintError( "Could not open relation's file" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Kathws kai gia ta attributes ths. Prwta dhmioyrgoyme ton pinaka pou tha kratisei ta attributes */

    if (( attributes = malloc( relPtr->attrcnt * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* initialize toy pinaka */
    for ( i = 0; i < relPtr->attrcnt; i++ ) {
        attributes[i] = NULL;
    }

    /* desmeuoume mnhmh gia to record sto opoio tha topothetoume tis eggrafes ap to attrCat arxeio, parakatw */
    if (( record = malloc( sizeof( attrDesc ) * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Ta attributes mias sxeshs vriskontai sto arxeio attrCat me th seira pou parousiazontai se mia eggrafh ths sxehs.
       Prwta tha vroume dhladh to 1o attribute me offset 0 k.o.k  */
    if (( scanDesc = HF_OpenFileScan( this_db.attrCatDesc, sizeof( attrDesc ), 'c', MAXNAME, 0, EQUAL, relName ) ) < 0 ) {
        HF_PrintError( "Could not start scan in attrCat" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* kanoume retrieve ola ta attributes ths sxeshs */
    for ( i = 0; i < relPtr->attrcnt; i++ ) {
        if ( HF_FindNextRec( scanDesc, record ) < 0 ) {
            HF_PrintError( "Could not find record in attrCat" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* desmeuoume mnhmh gia to attribute */
        if (( attributes[i] = malloc( sizeof( attrDesc ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kai swzoyme ta dedomena toy poy xreiazomaste */

        /* attrname */
        pch = record + ( MAXNAME * sizeof( char ) );
        memcpy( &attributes[i]->attrname,  pch, ( MAXNAME * sizeof( char ) ) );
        /* offset */
        pch += ( MAXNAME * sizeof( char ) );
        memcpy( &attributes[i]->offset,  pch, sizeof( int ) );
        /* attrlength */
        pch += sizeof( int );
        memcpy( &attributes[i]->attrlength,  pch, sizeof( int ) );
        /* attrtype */
        pch += sizeof( int );
        attributes[i]->attrtype = ( *pch );

    }

    if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
        HF_PrintError( "Could not close scan in attrCat" );
        return_val = AMINIREL_GE;
    }

    scanDesc = -1;

    free( record );

    /* desmeyoyme mnhmh gia to record toy arxeioy ths sxeshs */
    if (( record = malloc( relPtr->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Twra ksekiname mia sarwsh sto eurethrio me value = NULL gia na mas epistrafoun ola ta eggyra records */
    if (( scanDesc = HF_OpenFileScan( fileDesc, relPtr->relwidth , attributes[0]->attrtype, attributes[0]->attrlength, attributes[0]->offset, EQUAL, NULL ) ) < 0 ) {
        HF_PrintError( "Could not start a scan in relation's file" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Kathe eggrafh pou vriskoume na ikanopoiei th synthikh */
    while (( recId = HF_FindNextRec( scanDesc, record ) ) >= 0 ) {
        /* gia kathe attribute, ektypwse to onoma tou attribute kai thn timh */
        /*printf("\n\nRecId = %d", recId);*/
        for ( i = 0; i < relPtr->attrcnt; i++ ) {

            printf( "%s=", attributes[i]->attrname );

            /* vriskoume thn timh toy twrinoy attribute mesa sthn eggrafh mas */
            value = record + attributes[i]->offset;

            /* kai thn ektypwnoume analoga ton typo toy attribute */
            switch ( attributes[i]->attrtype ) {
                case 'c':
                    printf( "%s", (( char* )value ) );
                    break;
                case 'i':
                    printf( "%d", *(( int* )value ) );
                    break;
                case 'f':
                    printf( "%g", *(( float* )value ) );
                    break;
            } /* switch */

            if ( i != relPtr->attrcnt - 1 )
                printf( ", " );
        } /* for */

        putchar( '\n' );
    } /* while */

    if ( recId != HFE_EOF ) {
        HF_PrintError( "Error while finding next record." );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    putchar( '\n' );
    cleanup();
    return return_val;
}

/**** int DM_join( int argc, char* argv[] ) ****

 Usage:
        SELECT [INTO target_relname] (rel1.attrX, ...) WHERE rel1.attrY <OP> rel2.attrY

 Operation:
        * Anakta tis eggrafes twn sxesewn rel1, rel2 pou ikanopoioun th
          synthikh sto WHERE kommati
        * To mesaio kommati ths target list, to mono ypoxrewtiko, dhlwnei poia attributes twn 2 sxesewn
          tha kratithoun gia to apotelesma
        * An yparxei to kommati INTO, to apotelesma anakateythinetai se mia nea sxesh target_relname
        * An den yparxei to kommati INTO, to apotelesma ektypwnetai

 Sxolia:
        - Den epitrepetai self-join operation (dhladh, sto condition WHERE prepei rel1 != rel2)
        - Sto kommati tou NJL yparxei antistoixo leptomeres comment
        - Logw ths polyplokothtas ths ylopoihshs, exoun proste8ei merika abstractions, p.x.
            internDesc/externDesc gia na perigrafei to eswteriko/ekswteriko relation, aneksarthta tou
            an auto einai to rel1/rel2 h antistrofa.
        - Leptomeries ylopoihshs sta in-body comments

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei

*/

int DM_join( int argc, char* argv[] )
{
    /*
            Afou mono tou eswterikou arxeiou to index prepei na xrhsimopoihsoume, leme:
                * externDesc tha einai to ekswteriko arxeio
                * internDesc tha einai to eswteriko arxeio
                * targetFileDesc tha einai to arxeio sto opoio tha grafoume (xrhsimopoieitai kai gia testing)
    */
    
    int i, j, t, return_val, this_arg, attrcnt, recId, recId1, recId2, internRecId, externRecId, length, create_argc, offset;
    int internDesc, externDesc, internDescCount, externDescCount, targetFileDesc, internScanId, indexDesc, argLength;
    char * relName1, * relName2, * attrName1, * attrName2, * newRelName, op, use_index, newRelation, * value;
    char * targetRecord, ** create_argv, * externRecord, * internRecord, createdTempFile;
    relDesc * relPtr1, * relPtr2, * internRel, * externRel, * targetRelPtr;
    attrDesc ** attributes, * attrPtr;
    void * temp;
    void * temp_value;

    inline cleanup()
    {
        /* eleutherwnoume dynamikh mnhmh pou desmeusame */  
        if ( attributes != NULL ) {
            for ( i = 0; i < ( attrcnt + 2 ); i++ ) {
                if ( attributes[i] != NULL ) {
                    free( attributes[i] );
                }
            }

            free( attributes );
        }

        if ( internRecord != NULL ) {
            free( internRecord );
        }

        if ( externRecord != NULL ) {
            free( externRecord );
        }

        if ( targetRecord != NULL ) {
            free( targetRecord );
        }

        if ( relPtr1 != NULL ) {
            free( relPtr1 );
        }

        if ( relPtr2 != NULL ) {
            free( relPtr2 );
        }

        if ( targetRelPtr != NULL ) {
            free( targetRelPtr );
        }

        if ( use_index == TRUE && indexDesc >= 0 ) {
            if ( AM_CloseIndex( indexDesc ) != AME_OK ) {
                AM_PrintError( "Could not close index file" );
            }
        }

        /* An exoume anoixto kapoio scan */
        if ( externDesc >= 0 ) {
            if ( HF_CloseFile( externDesc ) != HFE_OK ) {
                HF_PrintError( "Cannot close relation file" );
                return_val = AMINIREL_GE;
            }
        }

        /* kleinoyme to eyrethrio tou condition attribute attrName ths sxeshs relName, an to anoiksame */
        if ( internDesc >= 0 ) {
            if ( HF_CloseFile( internDesc ) != AME_OK ) {
                HF_PrintError( "Could not close relation file" );
                return_val = AMINIREL_GE;
            }
        }

        /* kleinoyme to arxeio ths neas sxeshs, an to anoiksame kai den exei diagrafei */
        if ( targetFileDesc >= 0 ) {
            if ( HF_CloseFile( targetFileDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close relation's file" );
                return_val = AMINIREL_GE;
            }
        }
        
        /* An h sxesh pou dhmioyrghsame htan proswrinh, thn diagrafoume */
        if ( createdTempFile == TRUE ) {
            /* Diagrafoume th sxesh kalwntas thn UT_destroy() */
            strcpy( create_argv[0], "destroy" );
    
            if ( UT_destroy( 2, create_argv ) != AMINIREL_OK ) {
                printf( "Error while destroying temp relation\n" );
                return_val = AMINIREL_GE;
            }
        }
        
        /* afou xrhsimopoihsame to create_argv, to diagrafoume */
        if ( create_argv != NULL ) {
            for ( i = 0; i < create_argc; i++ ) {
                if ( create_argv[i] != NULL ) {
                    free( create_argv[i] );
                }
            }

            free( create_argv );
        }
    }

    /* initialization */
    i = j = t = attrcnt = offset = create_argc =  0;
    return_val = AMINIREL_OK;
    this_arg = 1;
    internDesc = externDesc = targetFileDesc = internScanId = internRecId = externRecId = -1;
    internDescCount = externDescCount = length = -1;
    relName1 = relName2 = attrName1 = attrName2 = newRelName = NULL;
    attributes = NULL;
    attrPtr = NULL;
    create_argv = NULL;
    relPtr1 = relPtr2 = targetRelPtr = internRel = externRel = NULL;
    externRecord = internRecord = targetRecord = NULL;
    use_index = FALSE;
    createdTempFile = FALSE;

    /*
      Shmasiologikoi elegxoi:
    */


    /* Apaitoume toulaxiston 9 arguments. join 1 relname attrname relname1 attrnam1 op relname2 attrname2 */
    if ( argc < 9 ) {
       printf("Error: Cannot Select. Expected at least 9 arguments for join operation\n");
       return AMINIREL_GE;
    }
    
    
    /*
       * elegxoume an yparxei to proairetiko kommati "INTO relName".
       * H atoi epistrefei 0 se periptwsh sfalmatos kai de ginetai na exoume 0 san plithos orismatwn pou provallontai
    */
    if ( atoi( argv[this_arg] ) == 0 ) {
        newRelName = argv[this_arg];
        newRelation = TRUE;
        this_arg++;

        /* Mias kai tha dhmioyrghsoyme nea sxesh, prepei na mhn yparxei hdh allh sxesh me to idio onoma! */
        if (( targetRelPtr = getRelation( newRelName, &recId ) ) != NULL ) {
            printf( "Error: Cannot Select. Relation '%s' already exists and cannot be overwritten.\n", newRelName );
            free( targetRelPtr );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }
    else {
        newRelName = NULL;
        newRelation = FALSE;
    }

    /* Swzoyme to plithos twn orismatwn pou provallontai */
    if (( attrcnt = atoi( argv[this_arg] ) ) == 0 ) {
        printf( "Error: Cannot select. Could not parse number of attributes.\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    this_arg++;

    /*
       * tha xeiristoume to poly attrcnt + 1 attributes. to +1 gia to attribute pou yparxei sto condition, an auto yparxei.
       * desmeuoume mnhmh gia ta attributes ayta.
    */

    if (( attributes = malloc(( attrcnt + 2 ) * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* initialization twn pinakwn gia ta attributes twn 2 sxesewn */
    for ( i = 0; i < ( attrcnt + 2 ); i++ ) {
        attributes[i] = NULL;
    }

    /* Kanoyme retrieve tis plhrofories gia kathe ena apo ta attributes pou provallontai */
    for ( i = 0; i < attrcnt; i++, this_arg += 2 ) {
        /*
            Ta provallomena attributes prepei na anhkoun se kapoia ap'tis 2 sxeseis!
            H prwth sxesh pou synantame tha einai h relName1. An synanthsoume kai allh,
            auth tha einai h relName2. An synanthsoume omws 3h, tote de mporei na ginei join.
        */
        if ( relName1 == NULL ) {
            if (( relName1 = malloc( strlen( argv[this_arg] ) + 1 ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            strcpy( relName1, argv[this_arg] );
        }
        else if (( relName2 == NULL ) && ( strcmp( argv[this_arg], relName1 ) != 0 ) ) {
            if (( relName2 = malloc( strlen( argv[this_arg] ) + 1 ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            strcpy( relName2, argv[this_arg] );
        }
        else if (( strcmp( relName1, argv[this_arg] ) != 0 ) && ( strcmp( relName2, argv[this_arg] ) != 0 ) ) {
            printf( "Error: Cannot join. More than two relations specified.\n" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /*
            to twrino attribute eisagetai ston pinaka me ta attributes analoga me to offset tou. Theloume o pinakas
            na einai taksinomimenos se auksousa seira analoga me to offset twn attributes, epeidh mas dieykolynei
            argotera otan dhmioyrgoyme th sxesh sthn opoia tha eisagoume to apotelesma.
        */

        if (( attributes[i] = getAttribute( argv[this_arg], argv[this_arg + 1], &recId ) ) == NULL ) {
            printf( "Error: Cannot select. Could not retrieve information about attribute '%s' of relation '%s'\n",
                    argv[this_arg + 1], argv[this_arg] );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }

    /* Kanoume retrieve tis plhrofories twn 2 sxesewn */
    if (( relPtr1 = getRelation( relName1, &recId1 ) ) == NULL ) {
        printf( "Error: Cannot select from relation '%s'. Information about relation could not be retrieved.\n", relName1 );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if ( relName2 != NULL ) {
        if (( relPtr2 = getRelation( relName2, &recId2 ) ) == NULL ) {
            printf( "Error: Cannot select from relation '%s'. Information about relation could not be retrieved.\n", relName2 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }

    /*
       * Elegxoume an dwthikan idia onomata se 2 apo ta provallomena, to opoio einai lathos.
       * Einai lathos kathws otan tha dhmioyrghsoyme th nea sxesh mas, tha epistrepsei lathos h UT_create otan dei idio
         onoma se 2 attributes kai mas to thimisan oti prepei na leitoyrgei mia mera prin thn paradwsh :-)
    */
    for ( i = 0; i < attrcnt; i++ ) {
        for ( j = i + 1; j < attrcnt; j++ ) {
            if ( strcmp( attributes[i]->attrname, attributes[j]->attrname ) == 0 ) {
                printf( "Error: More than one attributes in 'select' field named '%s'.\n", attributes[i]->attrname );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }
        }
    }

    /*
       Yparxei sigoura to WHERE kommati, mias kai ekteleitai JOIN operation.
    */

    /* den epitrepetai to self join */
    if ( strcmp( argv[this_arg], argv[this_arg+3] ) == 0 ) {
        printf( "Error: Cannot join. Self-Join prohibited.\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* An sta provallomena attributes yphrxan mono ths mias sxeshs tote orizoume to relName2 kai kanoume retrieve tis info tou */
    if ( relName2 == NULL && strcmp( argv[this_arg], relName1 ) != 0 ) {
        relName2 = argv[this_arg];

        if (( relPtr2 = getRelation( relName2, &recId2 ) ) == NULL ) {
            printf( "Error: Cannot select from relation '%s'. Information about relation could not be retrieved.\n", relName2 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }
    else if ( relName2 == NULL && strcmp( argv[this_arg], relName1 ) == 0 ) {
        relName2 = argv[this_arg+3];

        if (( relPtr2 = getRelation( relName2, &recId2 ) ) == NULL ) {
            printf( "Error: Cannot select from relation '%s'. Information about relation could not be retrieved.\n", relName2 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }

    /*
       * Ta attributes sto condition prepei na einai pragmati anamesa sta provallomena attributes (kai stis idies sxeseis)
       * exoume hdh eleksei gia self join
    */
    if (( strcmp( relName1, argv[this_arg] ) != 0 ) && ( strcmp( relName1, argv[this_arg+3] ) != 0 ) ) {
        printf( "Error: Cannot join. Wrong condition relations\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( strcmp( relName2, argv[this_arg] ) != 0 ) && ( strcmp( relName2, argv[this_arg+3] ) != 0 ) ) {
        printf( "Error: Cannot join. Wrong condition relations\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* ta onomata twn attributes sto condition */
    attrName1 = argv[this_arg + 1]; /* Isoutai me argc-4 */
    attrName2 = argv[this_arg + 4]; /* Isoutai me argc-1 */

    /*
        Kanoyme retrieve tis plhrofories twn attributes pou einai mesa sto condition.
        Sth 8esh "attrcnt" swzoume to 1o ypo sygkrish attribute, enw sthn "attrcnt+1" to 2o.
    */
    if (( attributes[attrcnt] = getAttribute( argv[this_arg], argv[this_arg + 1], &recId ) ) == NULL ) {
        printf( "Error: Cannot join. Could not retrieve information about attribute '%s' of relation '%s'\n",
                argv[this_arg + 1], argv[this_arg] );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( attributes[attrcnt+1] = getAttribute( argv[this_arg+3], argv[this_arg + 4], &recId ) ) == NULL ) {
        printf( "Error: Cannot join. Could not retrieve information about attribute '%s' of relation '%s'\n",
                argv[this_arg + 1], argv[this_arg] );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }
    
    /* Ta 2 pedia sto where kommati prepei na einai toy idioy typoy */
    if ( attributes[attrcnt]->attrtype != attributes[attrcnt+1]->attrtype ) {
       printf("Error: Cannot Select. Attributes in where field have different types.\n");
       cleanup();
       return return_val;
    }

    /* proxwrame ston operator */
    this_arg += 2;

    if ( strcmp( argv[this_arg], "=" ) == 0 ) {
        op = EQUAL;
    }
    else if ( strcmp( argv[this_arg], "!=" ) == 0 ) {
        op = NOT_EQUAL;
    }
    else if ( strcmp( argv[this_arg], "<" ) == 0 ) {
        op = LESS_THAN;
    }
    else if ( strcmp( argv[this_arg], ">" ) == 0 ) {
        op = GREATER_THAN;
    }
    else if ( strcmp( argv[this_arg], ">=" ) == 0 ) {
        op = GREATER_THAN_OR_EQUAL;
    }
    else if ( strcmp( argv[this_arg], "<=" ) == 0 ) {
        op = LESS_THAN_OR_EQUAL;
    }
    else {
        printf( "Error: Unrecognized symbol '%s' for operator.\n", argv[4] );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /*
      Dhmioyrgoyme mia sxesh sthn opoia tha mpoyn ta records pou ikanopoioyn th synthikh. An yparxei to
      "INTO new_relation" kommati, tote ayth h sxesh apoktaei to onoma new_relation kai de diagrafetai sto telos ths synarthshs.
      Alliws to onoma ths einai temp_relation
    */

    /*
       * Dhmioyrgoyme ton pinaka me ta arguments poy prepei na perasthei sth synarthsh UT_create()
       * Ta 2 prwta arguments einai to "create" kai to onoma ths sxeshs. Ta ypoloipa arguments einai zeygaria gia kathe
         attribute pou provalletai.
    */

    create_argc = 2 + ( attrcnt * 2 );

    /* desmeuoume mnhmh gia ton pinaka me ta arguments, + 1 gia to NULL sto telos */
    if (( create_argv = malloc(( create_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Initialize it */
    for ( i = 0; i <= create_argc; i++ ) {
        create_argv[i] = NULL;
    }

    /* 
       to megalytero attrlength pou epitrepetai einai 255 xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws: 
    */

    argLength = 255 + 2; /* +2 gia ta ' */

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }
    
    /* desmeuoume mnhmh gia ta arguments */
    for ( i = 0; i < create_argc; i++ ) {
        if (( create_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }

    /* Gemizoume ton pinaka me tis swstes times */
    strcpy( create_argv[0], "\"DMcreate\"" );

    /* An yparxei to "INTO new_relation" kommati tote pairnoume to onoma ths sxeshs apo to antistoixo argument */
    if ( newRelation == TRUE ) {
        length = strlen( newRelName ) + 1;

        /* De theloume n antigrapsoume perissoterous apo MAXNAME xarakthres */
        if ( length > MAXNAME ) {
            length = MAXNAME;
        }

        memcpy( create_argv[1], newRelName, length );
        create_argv[1][length - 1] = '\0';
    }
    /* Alliws h sxesh onomazetai temp_relation */
    else {
        strcpy( create_argv[1], "temp_relation" );
    }

    /* gia ta diafora attributes gemizoume ta arguments */
    for ( i = 2, j = 0; i < create_argc; i += 2, j++ ) {
        strcpy( create_argv[i], attributes[j]->attrname );

        if ( attributes[j]->attrtype == 'c' ) {
            sprintf( create_argv[i+1], "\"c%d\"", attributes[j]->attrlength );
        }
        else {
            sprintf( create_argv[i+1], "\"%c\"", attributes[j]->attrtype );
        }
    }

    /*
		Proetoimasia gia to Nested Join Loop (NJL)
		
		* An kanena apo ta 2 h kai ta 2 ypo-sygkrish attributes exoun index,
		tote orizontai ws ekswteriko kai eswteriko antistoixa me th seira pou dinontai.
		* Alliws, an dhladh mono ena arxeio exei index, auto xrhsimopoieitai ws eswteriko,
		afou sto ekswteriko tha kanoume fetch oles tis eggrafes!
	*/
	
	{
		int indexedAttr, heapAttr;
		
		/* An einai kai ta 2 TRUE h kai ta 2 FALSE, tote de xrhsimopoioume kanena ws index
         kai ta xrhsimopoioume me th seira pou dinontai */
        if ( attributes[attrcnt]->indexed == attributes[attrcnt+1]->indexed ) {
            indexedAttr = AMINIREL_INVALID;
            heapAttr = AMINIREL_INVALID;
		}
		/* alliws, an to prwto attribute einai indexed, tote ta xrhsimopoioume epishs
         me th seira pou dinontai */
		else if ( attributes[attrcnt]->indexed ) {
            indexedAttr = attrcnt;
            heapAttr = attrcnt+1;
		}
		/* alliws, einai sigoura indexed to 2o attribute, opote to xrhsimopoioume prwto */
		else {
			indexedAttr = attrcnt+1;
			heapAttr = attrcnt;
		}
		
		/*
         1h genikh periptwsh: Ekswteriko to 2o attribute, eswteriko to 1o:
            Auto symbainei an mono to 2o attribute einai indexed. Tote prepei na ginei
            kai mirroring twn operators!
         2h genikh periptwsh: Ekswteriko to 1o attribute, eswteriko to 2o: <-- H pio pi8anh periptwsh
		      Auto symbainei an kanena ap'ta 2 H' kai ta 2 attributes einai indexed, H'
			  an einai indexed to prwto attribute.
			  
        Synolika: 3 endiaferouses periptwseis:
            1) Indexed to 1o attribute
            2) Indexed to 2o attribute
            3) Kanena H' kai ta 2 attributes indexed
		*/
		if ( indexedAttr == attrcnt ) {
            if (( indexDesc = AM_OpenIndex( relName1, attributes[attrcnt]->indexno ) ) < 0 ) {
                AM_PrintError( "Error in DM Join: Cannot open index" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            use_index = TRUE;
            if (( internDesc = HF_OpenFile( relName1 ) ) < 0 ) {
                HF_PrintError( "Could not open relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }


            if (( externDesc = HF_OpenFile( relName2 ) ) < 0 ) {
                HF_PrintError( "Could not open relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* desmeyoyme mnhmh gia ta records twn arxeiwn twn sygkrinomenwn sxesewn */
            if (( externRecord = malloc( relPtr2->relwidth ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            if (( internRecord = malloc( relPtr1->relwidth ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* Abstraction gia to poia relation einai eswterikh kai poia ekswterikh */
            internRel = relPtr1;
            externRel = relPtr2;

            /* Abstraction gia to poio sygkrinomeno attribute einai eswteriko kai poio ekswteriko */
            internDescCount = attrcnt;
            externDescCount = attrcnt + 1;
            
            /* An h prwth sxesh mpei ws eswterikh, tote prepei na ginei mirroring twn operators
               (dld an isxuei 2 <= 5, prepei na rwthsoume 5 >= 2) */
            if ( op == LESS_THAN ) {
                op = GREATER_THAN;
            }
            else if ( op == GREATER_THAN ) {
                op = LESS_THAN;
            }
            else if ( op == GREATER_THAN_OR_EQUAL ) {
                op = LESS_THAN_OR_EQUAL;
            }
            else if ( op == LESS_THAN_OR_EQUAL ) {
                op = GREATER_THAN_OR_EQUAL;
            }
            else {
                /* Exoume operator EQUAL or NOT_EQUAL, oi opoioi de xreiazetai na allaksoun */
            }
		}
		else if ( indexedAttr == attrcnt+1 ) {
           if (( indexDesc = AM_OpenIndex( relName2, attributes[attrcnt+1]->indexno ) ) < 0 ) {
               AM_PrintError( "Error in DM Join: Cannot open index" );
               return_val = AMINIREL_GE;
               cleanup();
               return return_val;
           }

           use_index = TRUE;
           
                       if (( internDesc = HF_OpenFile( relName2 ) ) < 0 ) {
                HF_PrintError( "Could not open relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }


            /* Anoigma ekswterikou arxeiou */
            if (( externDesc = HF_OpenFile( relName1 ) ) < 0 ) {
                HF_PrintError( "Could not open relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* desmeyoyme mnhmh gia ta records twn arxeiwn twn sygkrinomenwn sxesewn */
            if (( externRecord = malloc( relPtr1->relwidth ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            if (( internRecord = malloc( relPtr2->relwidth ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* Abstraction gia to poia relation einai eswterikh kai poia ekswterikh */
            internRel = relPtr2;
            externRel = relPtr1;

            /* Abstraction gia to poio sygkrinomeno attribute einai eswteriko kai poio ekswteriko */
            internDescCount = attrcnt + 1;
            externDescCount = attrcnt;
		}
		else {
            use_index = FALSE;
            
            if (( internDesc = HF_OpenFile( relName2 ) ) < 0 ) {
                HF_PrintError( "Could not open relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }


            /* Anoigma ekswterikou arxeiou */
            if (( externDesc = HF_OpenFile( relName1 ) ) < 0 ) {
                HF_PrintError( "Could not open relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* desmeyoyme mnhmh gia ta records twn arxeiwn twn sygkrinomenwn sxesewn */
            if (( externRecord = malloc( relPtr1->relwidth ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            if (( internRecord = malloc( relPtr2->relwidth ) ) == NULL ) {
                printf( MEM_ERROR );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* Abstraction gia to poia relation einai eswterikh kai poia ekswterikh */
            internRel = relPtr2;
            externRel = relPtr1;

            /* Abstraction gia to poio sygkrinomeno attribute einai eswteriko kai poio ekswteriko */
            internDescCount = attrcnt + 1;
            externDescCount = attrcnt;
        }
    }

    /*
      Telos shmasiologikwn elegxwn
    */

    /* Dhmioyrgoyme th sxesh sthn opoia tha anakateythinoume to apotelesma ths select */
    if ( UT_create( create_argc, create_argv ) != AMINIREL_OK ) {
        printf( "Error: SELECT failed. Could not create temp relation file\n" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }
    
    if (newRelation == FALSE) {
       createdTempFile = TRUE;
    }
    
    /* Anoigoume to arxeio ths neas sxeshs apo to epipedo HF gia na eisagoume se auto tis eggrafes */
    if (( targetFileDesc = HF_OpenFile( create_argv[1] ) ) < 0 ) {
        HF_PrintError( "Could not open relation's file" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Kai vriskoume ta dedomena ths neas sxeshs pou tha mas xrhsimeysoyn otan tha kanoume insert */
    if (( targetRelPtr = getRelation( create_argv[1], &recId ) ) == NULL ) {
        printf( "Error: SELECT failed. Information about new relation '%s' could not be retrieved.\n", create_argv[1] );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* kathws kai gia to record ths neas sxeshs */
    if (( targetRecord = malloc( targetRelPtr->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    memset( targetRecord, 0, targetRelPtr->relwidth );

    /*
      Twra, prepei na vroume tis eggrafes pou ikanopoioyn th synthikh kai na tis eisagoume sto arxeio ths neas sxeshs!
    */

    /* Ksekiname apo thn arxh tou ekswterikou arxeiou */
    if (( externRecId = HF_GetFirstRec( externDesc, externRecord, externRel->relwidth ) ) < 0 ) {
        /* Yparxei h periptwsh to arxeio na mhn exei eggrafes */
        if ( externRecId != HFE_EOF ) {
            HF_PrintError( "Could not retrieve first record of relation" );
            return_val = AMINIREL_GE;
        }

        printf( "Error: Cannot join. A relation file is empty.\n" );
        cleanup();
        return return_val;
    }


    /* An to eswteriko arxeio xrhsimopoiei eurethrio */
    if ( use_index == TRUE ) {
        do {
            /* Briskoume to offset sto opoio yparxei to ypo-sygkrish attribute ths eggrafhs tou ekswterikou arxeiou */
            value = externRecord + attributes[externDescCount]->offset;

            /* Me bash ka8e eggrafh tou ekswterikou arxeiou, kanoume scan se oles tis eggrades tou eswterikou
               (gia thn akribeia, sto antistoixo index) */
            if (( internScanId = AM_OpenIndexScan( indexDesc, attributes[internDescCount]->attrtype, attributes[internDescCount]->attrlength, op, value ) ) < 0 ) {
                AM_PrintError( "Cannot start a scan in index file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* Oso vriskoume antigrafa tou record sto arxeio ths relationDest... */
            while (( internRecId = AM_FindNextEntry( internScanId ) ) >= 0 ) {

                /* Anaktoume thn eggrafh */
                if ( HF_GetThisRec( internDesc, internRecId, internRecord, internRel->relwidth ) != HFE_OK ) {
                    AM_PrintError( "Cannot retrieve record from relation's file" );
                    return_val = AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* kai ftiaxnoume th nea eggrafh */
                for ( i = 0 , offset = 0 ; i < attrcnt ; i++ ) {
                    /* gia ka8ena apo ta prballomena attributes, to bazoume se eggrafh ths neas sxeshs */
                    if ( strcmp( attributes[i]->relname, internRel->relname ) == 0 ) {
                        temp_value = internRecord + attributes[i]->offset;
                    }
                    else {
                        temp_value = externRecord + attributes[i]->offset;
                    }

                    memcpy( targetRecord + offset, temp_value, attributes[i]->attrlength );
                    offset += attributes[i]->attrlength;
                }

                /* eisagoume thn eggrafh mas sto arxeio ths neas sxeshs */
                if ( HF_InsertRec( targetFileDesc, targetRecord, targetRelPtr->relwidth ) < 0 ) {
                    HF_PrintError( "Could not insert record to new relation's file" );
                    return_val = AMINIREL_GE;
                }
            }

            if ( internRecId != AME_EOF ) {
                AM_PrintError( "Error while finding next entry." );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* kleinoyme to scan */
            if ( AM_CloseIndexScan( internScanId ) != AME_OK ) {
                AM_PrintError( "Could not close scan in relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

        }
        while (( externRecId = HF_GetNextRec( externDesc, externRecId, externRecord, externRel->relwidth ) ) >= 0 );

        if ( externRecId != HFE_EOF ) {
            HF_PrintError( "Error while getting next record." );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }
    /* alliws, kai ta 2 arxeia einai arxeia swrou */
    else {
        do {
            /* Briskoume to offset sto opoio yparxei to ypo-sygkrish attribute ths eggrafhs tou ekswterikou arxeiou */
            value = externRecord + attributes[externDescCount]->offset;

            /* Me bash ka8e eggrafh tou ekswterikou arxeiou, kanoume scan se oles tis eggrades tou eswterikou */
            if (( internScanId = HF_OpenFileScan( internDesc, internRel->relwidth, attributes[externDescCount]->attrtype, attributes[internDescCount]->attrlength, attributes[internDescCount]->offset, op, value ) ) < 0 ) {
                HF_PrintError( "Could not start scan in relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* Oso vriskoume antigrafa tou record sto arxeio ths relationDest... */
            while (( internRecId = HF_FindNextRec( internScanId, internRecord ) ) >= 0 ) {
                for ( i = 0 , offset = 0 ; i < attrcnt ; i++ ) {
                    /* gia ka8ena apo ta prballomena attributes, to bazoume se eggrafh ths neas sxeshs */
                    if ( strcmp( attributes[i]->relname, internRel->relname ) == 0 ) {
                        temp_value = internRecord + attributes[i]->offset;
                    }
                    else {
                        temp_value = externRecord + attributes[i]->offset;
                    }

                    memcpy( targetRecord + offset, temp_value, attributes[i]->attrlength );
                    offset += attributes[i]->attrlength;
                }

                /* eisagoume thn eggrafh mas sto arxeio ths neas sxeshs */
                if ( HF_InsertRec( targetFileDesc, targetRecord, targetRelPtr->relwidth ) < 0 ) {
                    HF_PrintError( "Could not insert record to new relation's file" );
                    return_val = AMINIREL_GE;
                }
            }

            if ( internRecId != HFE_EOF ) {
                HF_PrintError( "Error while finding next record." );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* kleinoyme to scan */
            if ( HF_CloseFileScan( internScanId ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }
        }
        while (( externRecId = HF_GetNextRec( externDesc, externRecId, externRecord, externRel->relwidth ) ) >= 0 );

        if ( externRecId != HFE_EOF ) {
            HF_PrintError( "Error while getting next record." );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }

    /* An h sxesh pou dhmioyrghsame htan proswrinh, thn ektypwnoume */
    if ( newRelation == FALSE ) {
        /* ektypwnoume tis eggrafes ths sxeshs mas */

        if ( printRelation( create_argv[1] ) != AMINIREL_OK ) {
            printf( "Error while priting the records of select!\n" );
            return_val = AMINIREL_GE;
        }

        /* Kleinoyme to arxeio ths temp sxeshs */
        if ( HF_CloseFile( targetFileDesc ) != HFE_OK ) {
            HF_PrintError( "Could not close temp relation's file" );
            return_val = AMINIREL_GE;
        }

        targetFileDesc = -1;
    }

    cleanup();
    return return_val;
}

/**** int DM_delete( int argc, char* argv[] ) ****

 Usage:
        DELETE relname [WHERE attr <OP> value]

 Operation:
        * Diagrafei tis eggrafes ths sxeshs poy ikanopoioyn th synthikh, ean den prokypsei kapoio sfalma

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/

int DM_delete( int argc, char* argv[] )
{
    int i, ival, length, recId, return_val, fileDesc, indexFileDesc, op, scanDesc;
    int tempIndexDesc;
    float fval;
    char * relName, * attrName, * record, * tempRecord, * pch, use_index;
    void * value, * realValue;
    relDesc * relPtr;
    attrDesc * attrPtr, ** attributes;

    /*
       to input mas exei mia ek twn 2 morfwn:
       * delete relation
       * delete relation where attr op value
    */
    if ( argc != 2 && argc != 5 ) {
        printf( "Error: Cannot delete. Not enough information\n" );
        return AMINIREL_GE;
    }

    inline cleanup() {
        /* An exoume anoixto kapoio scan */
        if ( scanDesc >= 0 ) {
            /* einai scan tou eurethriou */
            if ( use_index == TRUE ) {
                if ( AM_CloseIndexScan( scanDesc ) != AME_OK ) {
                    AM_PrintError( "Cannot close index scan" );
                    return_val = AMINIREL_GE;
                }
            }
            /* alliws scan tou arxeiou ths sxeshs */
            else {
                if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                    HF_PrintError( "Cannot close index scan" );
                    return_val = AMINIREL_GE;
                }
            }
        }

        /* eleytherwnoyme dynamikh mnhmh poy desmeysame */
        if ( record != NULL ) {
            free( record );
        }

        if ( value != NULL ) {
            free( value );
        }

        if ( attributes != NULL ) {
            for ( i = 0; i < relPtr->attrcnt ; i++ ) {
                if ( attributes[i] != NULL ) {
                    free( attributes[i] );
                }
            }

            free( attributes );
        }

        if ( relPtr != NULL ) {
            free( relPtr );
        }

        if ( tempRecord != NULL ) {
            free( tempRecord );
        }

        /* kleinoyme to arxeio ths sxeshs */
        if ( HF_CloseFile( fileDesc ) != HFE_OK ) {
            HF_PrintError( "Could not close relation's file" );
            return_val = AMINIREL_GE;
        }

        /* An exoume "where" kommati */
        if ( argc == 5 ) {
            /* kleinoyme to arxeio tou eurethriou, an to anoiksame */
            if ( attrPtr != NULL && attrPtr->indexed == TRUE && indexFileDesc >= 0 ) {
                if ( AM_CloseIndex( indexFileDesc ) != AME_OK ) {
                    AM_PrintError( "Cannot close index file" );
                    return_val = AMINIREL_GE;
                }
            }
        }

        if ( attrPtr != NULL ) {
            free( attrPtr );
        }
    }

    /* initialization */
    relName = argv[1];
    return_val = AMINIREL_OK;
    value = NULL;
    record = tempRecord = NULL;
    attributes = NULL;
    attrPtr = NULL;
    relPtr = NULL;
    indexFileDesc = -1;
    scanDesc = -1;
    use_index = FALSE;

    /*
         Shmasiologikoi elegxoi
    */

    /* An yparxei "where" kommati */
    if ( argc == 5 ) {
        attrName = argv[2];

        /* elegxos gia valid operator */
        if ( strcmp( argv[3], "=" ) == 0 ) {
            op = EQUAL;
        }
        else if ( strcmp( argv[3], "!=" ) == 0 ) {
            op = NOT_EQUAL;
        }
        else if ( strcmp( argv[3], "<" ) == 0 ) {
            op = LESS_THAN;
        }
        else if ( strcmp( argv[3], ">" ) == 0 ) {
            op = GREATER_THAN;
        }
        else if ( strcmp( argv[3], ">=" ) == 0 ) {
            op = GREATER_THAN_OR_EQUAL;
        }
        else if ( strcmp( argv[3], "<=" ) == 0 ) {
            op = LESS_THAN_OR_EQUAL;
        }
        else {
            printf( "Error: Unrecognized symbol '%s' for operator.\n", argv[4] );
            return AMINIREL_GE;
        }
    }
    else {
        /* An den yparxei "where" kommati, xeirizomaste oles tis eggrafes */
        op = GET_ALL;
    }

    /* Anoigoume to arxeio ths sxeshs apo to epipedo HF, tha mas xrhsimeysei argotera */
    if (( fileDesc = HF_OpenFile( relName ) ) < 0 ) {
        HF_PrintError( "Could not open relation's file" );
        return AMINIREL_GE;
    }

    /* Kanoume retrieve tis plhrofories gia th sxesh */
    if (( relPtr = getRelation( relName, &recId ) ) == NULL ) {
        printf( "Error: Cannot delete from relation '%s'. Information about relation could not be retrieved.\n", relName );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if ( argc == 5 ) {
        /* An yparxei "where" kommati, kanoyme retrieve tis plhrofories gia to attribute */
        if (( attrPtr = getAttribute( relName, attrName, &recId ) ) == NULL ) {
            printf( "Error: Cannot delete from relation '%s'. Information about attribute '%s' could not be retrieved.\n", relName, attrName );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* An yparxei eyrethrio, tote to anoigoyme gia na to xrhsimopoihsoyme */
        if ( attrPtr->indexed == TRUE && ( op == EQUAL || op == NOT_EQUAL ) ) {
            if (( indexFileDesc = AM_OpenIndex( relName, attrPtr->indexno ) ) < 0 ) {
                AM_PrintError( "Cannot open index" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            use_index = TRUE;
        }
    }
    /* An den yparxei "where" kommati, aplws gemizoume to attrPtr me tyxaies times. To value pou tha perastei stis
       scan synarthseis parakatw tha einai NULL kai de tha elexthoun oi times tou attrPtr etsi ki alliws. */
    else {
        if (( attrPtr = malloc( sizeof( attrDesc ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        attrPtr->attrtype = 'i';
        attrPtr->attrlength = 4;
        attrPtr->offset = 0;
    }

    /* Desmeuoume mnhmh gia ton pinaka pou periexei pointers sta attributes ths sxeshs mas. Tha mas xreiastoun otan tha
       diagrafoume eggrafes apo ta antistoixa eurethria. */
    if (( attributes = malloc( relPtr->attrcnt * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* initialize toy pinaka */
    for ( i = 0; i < relPtr->attrcnt; i++ ) {
        attributes[i] = NULL;
    }

    /* desmeuoume mnhmh gia to record sto opoio tha topothetoume tis eggrafes ap to attrCat arxeio, parakatw */
    if (( record = malloc( sizeof( attrDesc ) * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Ta attributes mias sxeshs vriskontai sto arxeio attrCat me th seira pou parousiazontai se mia eggrafh ths sxehs.
       Prwta tha vroume dhladh to 1o attribute me offset 0 k.o.k  */
    if (( scanDesc = HF_OpenFileScan( this_db.attrCatDesc, sizeof( attrDesc ), 'c', MAXNAME, 0, 1, relName ) ) < 0 ) {
        HF_PrintError( "Could not start scan in attrCat" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* kanoume retrieve ola ta attributes ths sxeshs */
    for ( i = 0; i < relPtr->attrcnt; i++ ) {
        if ( HF_FindNextRec( scanDesc, record ) < 0 ) {
            HF_PrintError( "Could not find record in attrCat" );

            if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in attrCat" );
            }

            scanDesc = -1;
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* desmeuoume mnhmh gia to attribute */
        if (( attributes[i] = malloc( sizeof( attrDesc ) ) ) == NULL ) {
            if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in attrCat" );
            }

            scanDesc = -1;
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kai swzoyme ta dedomena toy poy xreiazomaste */

        /* offset */
        pch = record + 2 * ( MAXNAME * sizeof( char ) );;
        memcpy( &attributes[i]->offset,  pch, sizeof( int ) );
        /* attrlength */
        pch += sizeof( int );
        memcpy( &attributes[i]->attrlength,  pch, sizeof( int ) );
        /* attrtype */
        pch += sizeof( int );
        attributes[i]->attrtype = ( *pch );
        /* indexed pou xrhsimeyei meta gia tis diagrafes apo eyrethria */
        pch += sizeof( char );
        memcpy( &attributes[i]->indexed,  pch, sizeof( int ) );
        /* indexno pou xrhsimeyei meta gia tis diagrafes apo eyrethria */
        pch += sizeof( int );
        memcpy( &attributes[i]->indexno,  pch, sizeof( int ) );
    }

    if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
        HF_PrintError( "Could not close scan in attrCat" );
        return_val = AMINIREL_GE;
    }

    scanDesc = -1;

    /* twra tha xreiastoume records diaforetikou megethous, oso einai h eggrafh twn arxeiwn */
    free( record );
    record = NULL;

    /* Gemizoume to value me th swsth timh symfwna me thn opoia tha ginei o elegxos */

    /* An xeirizomaste oles tis eggrafes, prepei na pername NULL ws value argument */
    if ( op == GET_ALL ) {
        value = NULL;
    }
    /* An exoume "where" kommati tote analoga me ton typo toy attribute anathetoume tin antistoixh timh */
    else if ( argc == 5 ) {
        /* desmeuoume xwro ston opoio tha kratithei to value */
        if (( value = malloc( attrPtr->attrlength ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* initialize it */
        memset( value, 0, attrPtr->attrlength );

        /* kai kanoyme tis antistoixes energeies */

        switch ( attrPtr->attrtype ) {
            case 'i':
                ival = atoi( argv[4] );

                /* Otan apotygxanei h atoi epistrefei 0. Ara an epestrepse 0 kai o arithmos mas den einai
                   pragmatika to 0, tote error! */
                if ( ival == 0 && strcmp( argv[4], "0" ) != 0 ) {
                    printf( "Error: Cannot delete from relation '%s'. Expected integer as value for attribute %s\n",
                            relName, attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* antigrafoume thn timh sto value apo to argument mas */
                memcpy( value, &ival, attrPtr->attrlength );

                break;

            case 'f':
                fval = atof( value );

                /* Otan apotygxanei h atof epistrefei 0.0. Ara an epestrepse 0.0 kai o arithmos mas den einai
                   pragmatika to 0.0, tote error! */
                if ( fval == 0.0 && strcmp( argv[4], "0.0" ) != 0 && strcmp( argv[4], "0" ) != 0 ) {
                    printf( "Error: Cannot delete from relation '%s'. Expected float as value for attribute %s\n",
                            relName, attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* antigrafoume thn timh sto value apo to argument mas */
                memcpy( value, &fval, attrPtr->attrlength );
                break;

            case 'c':
                pch = argv[4];

                /* An exoume string prepei na mas exei dwthei anamesa se " */
                if ( pch[0] != '\"' || pch[strlen( pch ) - 1] != '\"' ) {
                    printf( "Error: Cannot delete from relation \"%s\". Expected string as value for attribute %s\n",
                            relName, attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* prospername to arxiko ' */
                pch++;
                /* kai diagrafoume to teliko ' */
                pch[strlen( pch ) - 1] = '\0';
                /* +1 gia ton terminating NUL character */
                length = strlen( pch ) + 1;

                /* De theloume n antigrapsoume parapanw xaraktires ap to attrlength */
                if ( length > attrPtr->attrlength ) {
                    length = attrPtr->attrlength;
                }

                /* antigrafoume thn timh sto value apo to argument mas xwris ta ' pou exoume bgalei*/
                memcpy( value, pch, length );

                break;

            default:
                printf( "Error: Cannot delete from relation '%s'. Unknown attrtype for attribute '%s'.\n",
                        relName, attrName );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
        } /* switch */
    } /* if */

    /*
       Twra, prepei na diagrapsoume kathe eggrafh ths sxeshs poy ikanopoiei th synthikh
    */


    /*
         Telos shmasiologikwn elegxwn
    */

    /* desmeyoyme mnhmh gia to record toy arxeioy ths sxeshs */
    if (( record = malloc( relPtr->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( tempRecord = malloc( relPtr->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* An xrhsimopoioyme eyrethrio */
    if ( use_index == TRUE ) {

        /* Ksekiname mia sarwsh sto eyrethrio */
        if (( scanDesc = AM_OpenIndexScan( indexFileDesc, attrPtr->attrtype, attrPtr->attrlength, op, value ) ) < 0 ) {
            AM_PrintError( "Cannot start a scan in index file" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Kathe eggrafh pou vriskoume na ikanopoiei th synthikh */
        while (( recId = AM_FindNextEntry( scanDesc ) ) >= 0 ) {
            /* Gia na diagrapsoume mia eggrafh apo kapoio index, theloume ektos apo to recId pou vrhkame kai to value
               gia to antistoixo attribute. Epomenws, prin diagrapsoume to record, to kanoume store sto tempRecord. */
            if ( HF_GetThisRec( fileDesc, recId, tempRecord, relPtr->relwidth ) != HFE_OK ) {
                AM_PrintError( "Cannot retrieve record from relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* Diagrafoume thn eggrafh apo th sxesh*/
            if ( HF_DeleteRec( fileDesc, recId, relPtr->relwidth ) != HFE_OK ) {
                HF_PrintError( "Cannot delete record from relation's file" );
                return_val = AMINIREL_GE;
            }

            /* kathws kai apo tyxon indeces */
            for ( i = 0; i < relPtr->attrcnt; i++ ) {
                if ( attributes[i]->indexed == TRUE ) {

                    /* anoigoume to index */
                    if (( tempIndexDesc = AM_OpenIndex( relName, attributes[i]->indexno ) ) < 0 ) {
                        AM_PrintError( "Could not open index" );
                        return_val = AMINIREL_GE;
                        continue;
                    }

                    /* thetoume ton deikth realValue sto swsto shmeio ths eggrafhs */
                    realValue = tempRecord + attributes[i]->offset;

                    /* diagrafoume thn eggrafh */
                    if ( AM_DeleteEntry( tempIndexDesc, attributes[i]->attrtype, attributes[i]->attrlength, realValue, recId ) != AME_OK ) {
                        AM_PrintError( "Could not delete record from index" );
                        return_val = AMINIREL_GE;
                    }

                    /* kleinoyme to index */
                    if ( AM_CloseIndex( tempIndexDesc ) != AME_OK ) {
                        AM_PrintError( "Could not close index" );
                        return_val = AMINIREL_GE;
                    }
                }
            } /* for */
        } /* while */

        if ( recId != AME_EOF ) {
            AM_PrintError( "Error while finding next entry." );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    } /* if */

    /* An de xrhsimopoioyme eyrethrio */
    else {
        /* Ksekiname mia sarwsh sto arxeio ths sxeshs */
        if (( scanDesc = HF_OpenFileScan( fileDesc, relPtr->relwidth , attrPtr->attrtype, attrPtr->attrlength, attrPtr->offset, op, value ) ) < 0 ) {
            HF_PrintError( "Could not start a scan in relation's file" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Kathe eggrafh pou vriskoume na ikanopoiei th synthikh */
        while (( recId = HF_FindNextRec( scanDesc, record ) ) >= 0 ) {

            /* Th diagrafoume apo th sxesh*/
            if ( HF_DeleteRec( fileDesc, recId, relPtr->relwidth ) != HFE_OK ) {
                HF_PrintError( "Cannot delete record from relation's file" );
                return_val = AMINIREL_GE;
            }

            /* kathws kai apo tyxon indeces */
            for ( i = 0; i < relPtr->attrcnt; i++ ) {
                if ( attributes[i]->indexed == TRUE ) {

                    /* anoigoume to index */
                    if (( tempIndexDesc = AM_OpenIndex( relName, attributes[i]->indexno ) ) < 0 ) {
                        AM_PrintError( "Could not open index" );
                        return_val = AMINIREL_GE;
                        continue;
                    }

                    /* thetoume ton deikth realValue sto swsto shmeio ths eggrafhs */
                    realValue = record + attributes[i]->offset;

                    /* diagrafoume thn eggrafh */
                    if ( AM_DeleteEntry( tempIndexDesc, attributes[i]->attrtype, attributes[i]->attrlength, realValue, recId ) != AME_OK ) {
                        AM_PrintError( "Could not delete record from index" );
                        return_val = AMINIREL_GE;
                    }

                    /* kleinoyme to index */
                    if ( AM_CloseIndex( tempIndexDesc ) != AME_OK ) {
                        AM_PrintError( "Could not close index" );
                        return_val = AMINIREL_GE;
                    }
                }
            } /* for */
        } /* while */

        if ( recId != HFE_EOF ) {
            HF_PrintError( "Error while finding next record." );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    } /* else */

    cleanup();
    return return_val;
}


/**** int DM_subtract( int argc, char* argv[] ) ****

 Usage:
        DELETE relname1 RECORDS OF relname2

 Operation:
        * Diagrafei tis eggrafes ths sxeshs2 pou tis exei h sxesh1 kai ta eyrethria ayths, efoson den parousiastei
          kapoio sfalma

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/
int DM_subtract( int argc, char* argv[] )
{
    int i, fileDescSource, fileDescDest, return_val, recId, delRecId, scanDescDest, scanDescSource, indexDesc, scanDesc;
    char * relNameSource, * relNameDest, * record, * pch, * tempRecord, * value;
    relDesc * relPtrSource, * relPtrDest;
    attrDesc ** attributesSource, ** attributesDest;

    /* Prepei na exoyme 3 arguments. "subtract", relation1, relation2 */
    if ( argc != 3 ) {
        printf( "Error: Cannot subtract. Not enough information\n" );
        return AMINIREL_GE;
    }

    inline cleanup() {
        /* eleytherwnoyme th dynamikh mnhmh poy desmeysame */

        if ( attributesSource != NULL ) {
            for ( i = 0; i < relPtrSource->attrcnt; i++ ) {
                if ( attributesSource[i] != NULL ) {
                    free( attributesSource[i] );
                }
            }

            free( attributesSource );
        }

        if ( attributesDest != NULL ) {
            for ( i = 0; i < relPtrDest->attrcnt; i++ ) {
                if ( attributesDest[i] != NULL ) {
                    free( attributesDest[i] );
                }
            }

            free( attributesDest );
        }

        if ( relPtrSource != NULL ) {
            free( relPtrSource );
        }

        if ( relPtrDest != NULL ) {
            free( relPtrDest );
        }

        if ( tempRecord != NULL ) {
            free( tempRecord );
        }


        /* kleinoyme tyxon scans poy einai anoixta */
        if ( scanDescSource >= 0 ) {
            if ( HF_CloseFileScan( scanDescSource ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in attrCat" );
            }
        }

        if ( scanDescDest >= 0 ) {
            if ( HF_CloseFileScan( scanDescDest ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in attrCat" );
            }
        }

        if ( scanDesc >= 0 ) {
            if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in relations file" );
            }
        }

        /* kleinoyme ta arxeia twn sxeswn poy anoiksame */
        if ( HF_CloseFile( fileDescSource ) != HFE_OK ) {
            HF_PrintError( "Could not close relation file" );
        }

        if ( HF_CloseFile( fileDescDest ) != HFE_OK ) {
            HF_PrintError( "Could not close relation file" );
        }


        if ( record != NULL ) {
            free( record );
        }
    }

    /* initialization */
    relNameDest = argv[1];
    relNameSource = argv[2];
    return_val = AMINIREL_OK;
    relPtrSource = relPtrDest = NULL;
    record = tempRecord = NULL;
    attributesDest = attributesSource = NULL;
    scanDescSource = scanDescDest = scanDesc = indexDesc = -1;

    /*
         Shmasiologikoi elegxoi
    */

    if ( strcmp( relNameDest, relNameSource ) == 0 ) {
        printf( "Error: Different relations expected for DM_subtract() .\n" );
        return AMINIREL_GE;
    }

    /* Anoigoume ta arxeia twn 2 sxesewn, tha mas xreiastoyn parakatw */

    if (( fileDescSource = HF_OpenFile( relNameSource ) ) < 0 ) {
        HF_PrintError( "Could not open relation file" );
        return AMINIREL_GE;
    }

    if (( fileDescDest = HF_OpenFile( relNameDest ) ) < 0 ) {
        HF_PrintError( "Could not open relation file" );

        /* kleinoyme to ena arxeio poy anoiksame */
        if ( HF_CloseFile( fileDescSource ) != HFE_OK ) {
            HF_PrintError( "Could not close relation file" );
        }

        return AMINIREL_GE;
    }

    /* Kanoyme retrieve tis plhrofories gia tis 2 sxeseis */

    if (( relPtrSource = getRelation( relNameSource, &recId ) ) == NULL ) {
        printf( "Error: Cannot subtract from relation '%s'. Information about relation '%s' could not be retrieved.\n",
                relNameDest, relNameSource );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( relPtrDest = getRelation( relNameDest, &recId ) ) == NULL ) {
        printf( "Error: Cannot subtract from relation '%s'. Information about relation '%s' could not be retrieved.\n",
                relNameDest, relNameDest );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Gia na ginei SUBTRACT arkei ta pedia twn 2 sxeswn na einai idioy typoy, mhkoys kai me thn idia seira. Profanws
       prepei na exoyme kai to idio plithos pediwn. De mas apasxoloyn ta onomata twn attributes */

    /* Elegxoume gia idio plithos attributes */
    if ( relPtrDest->attrcnt != relPtrSource->attrcnt ) {
        printf( "Error: Cannot subtract from relation '%s'. Uneven number of attributes amongst relations.\n", relNameDest );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Desmeuoume mnhmh gia toys pinakes pou periexoyn pointers sta attributes twn sxesewn pou xeirizomaste */
    if (( attributesSource = malloc( relPtrSource->attrcnt * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( attributesDest = malloc( relPtrDest->attrcnt * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* initialize twn pinakwn */
    for ( i = 0; i < relPtrDest->attrcnt; i++ ) {
        attributesSource[i] = NULL;
        attributesDest[i] = NULL;
    }

    /* desmeuoume mnhmh gia to record sto opoio tha topothetoume tis eggrafes ap to attrCat arxeio, parakatw */
    if (( record = malloc( sizeof( attrDesc ) * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Ta attributes mias sxeshs vriskontai sto arxeio attrCat me th seira pou parousiazontai se mia eggrafh ths sxehs.
       Prwta tha vroume dhladh to 1o attribute me offset 0 k.o.k  */

    /* Eksetazoume parallhla ta attributes twn 2 sxesewn. Arxika, ksekiname 2 scans. */
    if (( scanDescSource = HF_OpenFileScan( this_db.attrCatDesc, sizeof( attrDesc ), 'c', MAXNAME, 0, EQUAL, relNameSource ) ) < 0 ) {
        HF_PrintError( "Could not start scan in attrCat" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( scanDescDest = HF_OpenFileScan( this_db.attrCatDesc, sizeof( attrDesc ), 'c', MAXNAME, 0, EQUAL, relNameDest ) ) < 0 ) {
        HF_PrintError( "Could not start scan in attrCat" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Gia kathe attribute... */
    for ( i = 0; i < relPtrDest->attrcnt; i++ ) {
        /* Kanoume retrieve to epomeno attribute kathe sxeshs */
        if ( HF_FindNextRec( scanDescSource, record ) < 0 ) {
            HF_PrintError( "Could not find record in attrCat" );
        }

        /* desmeuoume mnhmh gia to attribute */
        if (( attributesSource[i] = malloc( sizeof( attrDesc ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kai swzoyme ta dedomena toy poy xreiazomaste */

        /* offset */
        pch = record + 2 * ( MAXNAME * sizeof( char ) );
        memcpy( &attributesSource[i]->offset,  pch, sizeof( int ) );
        /* attrlength */
        pch += sizeof( int );
        memcpy( &attributesSource[i]->attrlength,  pch, sizeof( int ) );
        /* attrtype */
        pch += sizeof( int );
        attributesSource[i]->attrtype = ( *pch );

        /* paromoiws kai gia to 2o scan */
        if ( HF_FindNextRec( scanDescDest, record ) < 0 ) {
            HF_PrintError( "Could not find record in attrCat" );
        }

        /* desmeuoume mnhmh gia to attribute */
        if (( attributesDest[i] = malloc( sizeof( attrDesc ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kai swzoyme ta dedomena toy poy xreiazomaste */

        /* offset */
        pch = record + 2 * ( MAXNAME * sizeof( char ) );;
        memcpy( &attributesDest[i]->offset,  pch, sizeof( int ) );
        /* attrlength */
        pch += sizeof( int );
        memcpy( &attributesDest[i]->attrlength,  pch, sizeof( int ) );
        /* attrtype */
        pch += sizeof( int );
        attributesDest[i]->attrtype = ( *pch );
        /* indexed pou xrhsimeyei meta gia tis diagrafes apo eyrethria */
        pch += sizeof( char );
        memcpy( &attributesDest[i]->indexed,  pch, sizeof( int ) );
        /* indexno pou xrhsimeyei meta gia tis diagrafes apo eyrethria */
        pch += sizeof( int );
        memcpy( &attributesDest[i]->indexno,  pch, sizeof( int ) );


        /* eksetazoume typo, offset k length anamesa sta 2 attributes! */

        if ( attributesDest[i]->attrtype != attributesSource[i]->attrtype ) {
            printf( "Error: Cannot subtract from relation '%s'. Different types amongst arguments no %d\n", relNameDest, i + 1 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        if ( attributesDest[i]->offset != attributesSource[i]->offset ) {
            printf( "Error: Cannot subtract from relation '%s'. Different offsets amongst arguments no %d\n", relNameDest, i + 1 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        if ( attributesDest[i]->attrlength != attributesSource[i]->attrlength ) {
            printf( "Error: Cannot subtract from relation '%s'. Different lengths amongst arguments no %d\n", relNameDest, i + 1 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    } /* for */

    /*
         Telos shmasiologikwn elegxwn
    */

    /* An ftasame edw, oi 2 sxeseis exoun attributes pou epitrepoun to subtract. To mono pou menei einai gia kathe record ths
       relationSource, na to diagrapsoume apo th relationDest */

    /* twra tha xreiastoume records diaforetikou megethous, oso einai h eggrafh twn arxeiwn */
    free( record );
    record = NULL;

    if (( record = malloc( relPtrSource->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }


    if (( tempRecord = malloc( relPtrSource->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Gia kathe record pou kanoume retrieve apo to arxeio ths sxeshs relSource, to diagrafoume apo th sxesh relDest,
       an yparxei se aythn. */

    /* Ksekiname apo thn arxh tou arxeiou */
    if (( recId = HF_GetFirstRec( fileDescSource, record, relPtrSource->relwidth ) ) < 0 ) {
        /* Yparxei h periptwsh to arxeio na mhn exei eggrafes */
        if ( recId != HFE_EOF ) {
            HF_PrintError( "Could not retrieve first Record from file relCat" );
            return_val = AMINIREL_GE;
        }

        cleanup();
        return return_val;
    }

    do {
        /* anoigoume kathe fora scan gia na doume an yparxei to sygkekrimeno record sth sxesh Dest, kai an yparxei
           na diagrapsoume ola ta instances aytoy. To argument 1 einai gia operator Equal kai to 0 einai to offset. */
        if (( scanDesc = HF_OpenFileScan( fileDescDest, relPtrSource->relwidth, 'c', relPtrSource->relwidth, 0, EQUAL, record ) ) < 0 ) {
            HF_PrintError( "Could not start scan in relation's file" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Oso vriskoume antigrafa tou record sto arxeio ths relationDest... */
        while (( delRecId = HF_FindNextRec( scanDesc, tempRecord ) ) >= 0 ) {
            /* diegrapse ta apo to arxeio... */
            if ( HF_DeleteRec( fileDescDest, delRecId, relPtrDest->relwidth ) != HFE_OK ) {
                HF_PrintError( "Could delete record from relation's file" );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }

            /* kathws kai apo tyxon indeces */
            for ( i = 0; i < relPtrDest->attrcnt; i++ ) {
                if ( attributesDest[i]->indexed == TRUE ) {

                    /* anoigoume to index */
                    if (( indexDesc = AM_OpenIndex( relNameDest, attributesDest[i]->indexno ) ) < 0 ) {
                        AM_PrintError( "Could not open index" );
                        return_val = AMINIREL_GE;
                        continue;
                    }

                    /* thetoume ton deikth value sto swsto shmeio ths eggrafhs */
                    value = tempRecord + attributesDest[i]->offset;

                    /* diagrafoume thn eggrafh */
                    if ( AM_DeleteEntry( indexDesc, attributesDest[i]->attrtype, attributesDest[i]->attrlength, value, delRecId ) != AME_OK ) {
                        AM_PrintError( "Could not delete record from index" );
                        return_val = AMINIREL_GE;
                    }

                    /* kleinoyme to index */
                    if ( AM_CloseIndex( indexDesc ) != AME_OK ) {
                        AM_PrintError( "Could not close index" );
                        return_val = AMINIREL_GE;
                    }
                }
            }
        }

        if ( delRecId != HFE_EOF ) {
            HF_PrintError( "Error while finding next record." );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kleinoyme to scan */
        if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
            HF_PrintError( "Could not close scan in relation's file" );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        scanDesc = -1;
        /* kai proxwrame sto epomeno record ths relationSource */
    }
    while (( recId = HF_GetNextRec( fileDescSource, recId, record, relPtrSource->relwidth ) ) >= 0 );

    if ( recId != HFE_EOF ) {
        HF_PrintError( "Error while getting next record." );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    cleanup();
    return return_val;
}

/**** int DM_insert( int argc, char* argv[] ) ****

 Usage:
        INSERT relname(attr1='val', attr2='va2', ...);

 Operation:
        * Eisagei mia eggrafh se mia sxesh, kathws kai sta antistoixa eyrethria twn gnwrismatwn ths


 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/
int DM_insert( int argc, char* argv[] )
{
    int i, j, ival, length, attrcnt, recId, attrOffset, return_val, fileDesc, relwidth, indexFileDesc;
    float fval;
    char * relName, * record, * pch, * attrName, * value;
    relDesc * relPtr;
    attrDesc ** attributes;

    inline cleanup() {
        /* eleytherwnoyme dynamikh mnhmh poy desmeysame */
        if ( record != NULL ) {
            free( record );
        }

        if ( relPtr != NULL ) {
            free( relPtr );
        }

        if ( attributes != NULL ) {
            for ( i = 0; i < attrcnt; i++ ) {
                if ( attributes[i] != NULL ) {
                    free( attributes[i] );
                }
            }

            free( attributes );
        }

        /* kleinoyme to arxeio ths sxeshs */
        if ( HF_CloseFile( fileDesc ) != HFE_OK ) {
            HF_PrintError( "Could not close relation's file" );
            return_val = AMINIREL_GE;
        }
    }

    /* initialization */
    relName = argv[1];
    record = NULL;
    attributes = NULL;
    relPtr = NULL;
    return_val = AMINIREL_OK;

    /* Apaitoume, ektos apo to onoma ths sxeshs:
       - toulaxiston ena gnwrisma kai thn timh toy
       - oloklhrwmena zeugh <onomatos gnwrismatos, timh>
    */
    if (( argc < 4 ) || ( argc % 2 != 0 ) ) {
        printf( "Error: Cannot insert. Not enough information\n" );
        return AMINIREL_GE;
    }

    /* Anoigoume to arxeio ths sxeshs apo to epipedo HF, tha mas xrhsimeysei argotera */
    if (( fileDesc = HF_OpenFile( relName ) ) < 0 ) {
        HF_PrintError( "Could not open relation's file" );
        return AMINIREL_GE;
    }

    /* Kanoume retrieve tis plhrofories gia th sxesh */
    if (( relPtr = getRelation( relName, &recId ) ) == NULL ) {
        printf( "Error: Cannot insert into relation '%s'. Information about relation could not be retrieved.\n", relName );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    attrcnt = relPtr->attrcnt;
    relwidth = relPtr->relwidth;

    /* O arithmos twn zeugariwnn <attribute, value> pou dwthike isoutai me ton arithmo twn arguments meion ta 2
       prwta arguments: "insert", "relName". Elegxoume an mas dwthike o swstos arithmos apo tetoia zeygaria. */
    if ((( argc - 2 ) / 2 ) != attrcnt ) {
        printf( "Error: Cannot insert into relation '%s'. Expected %d <attribute, value> pairs.\n", relName, attrcnt );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Elegxoume an dwthikan idia onomata se 2 attributes ths relation, to opoio einai lathos */
    for ( i = 2; i < argc; i += 2 ) {
        for ( j = i + 2; j < argc; j += 2 ) {
            if ( strcmp( argv[i], argv[j] ) == 0 ) {
                printf( "Error: More than one attributes named '%s'.\n", argv[i] );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
            }
        }
    }

    /* Desmeuoume mnhmh gia to record pou tha eisagoume sto arxeio ths sxeshs */
    if (( record = malloc( relwidth * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* initialize tou record me mhdenika */
    memset( record, 0, relwidth * sizeof( char ) );

    /* Desmeuoume mnhmh gia ton pinaka pou periexei pointers sta attributes ths sxeshs pou xeirizomaste */
    if (( attributes = malloc( attrcnt * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* initialize toy pinaka */
    for ( i = 0; i < attrcnt; i++ ) {
        attributes[i] = NULL;
    }



    /* Gia kathe zeugari <attribute, value>, symplhrwnoume thn timh tou sto record, an den parousiastei kapoio lathos */

    for ( i = 2, attrOffset = 0; i < argc; i += 2, attrOffset++ ) {
        /* se kathe zeygari to prwto argument einai to onoma tou attribute kai to 2o h timh mesa se ' */
        attrName = argv[i];
        value = argv[i+1];

        /* kanoume retrieve tis plhrofories gia to attribute */
        if (( attributes[attrOffset] = getAttribute( relName, attrName, &recId ) ) == NULL ) {
            printf( "Error: Cannot insert into relation '%s'. Information about attribute '%s' could not be retrieved.\n",
                    relName, attrName );
            return_val =  AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* Odhgoume ton pointer pch sto shmeio ths eggrafhs pou prepei na eisagoume tin timh mas */
        pch = record + attributes[attrOffset]->offset;

        switch ( attributes[attrOffset]->attrtype ) {
            case 'i':
                ival = atoi( value );

                /* Otan apotygxanei h atoi epistrefei 0. Ara an epestrepse 0 kai o arithmos mas den einai
                   pragmatika to 0, tote error! */
                if ( ival == 0 && strcmp( value, "0" ) != 0 ) {
                    printf( "Error: Cannot insert into relation '%s'. Expected integer as value for attribute %s\n",
                            relName, attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                memcpy( pch, &ival, attributes[attrOffset]->attrlength );
                break;

            case 'f':
                fval = atof( value );

                /* Otan apotygxanei h atof epistrefei 0.0. Ara an epestrepse 0.0 kai o arithmos mas den einai
                   pragmatika to 0.0, tote error! */
                if ( fval == 0.0 && strcmp( value, "0.0" ) != 0 && strcmp( value, "0" ) != 0 ) {
                    printf( "Error: Cannot insert into relation '%s'. Expected float as value for attribute %s\n",
                            relName, attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                memcpy( pch, &fval, attributes[attrOffset]->attrlength );
                break;

            case 'c':

                /* An exoume string prepei na mas exei dwthei anamesa se " */
                if ( value[0] != '\"' || value[strlen( value ) - 1] != '\"' ) {
                    printf( "Error: Cannot insert into relation '%s'. Expected string as value for attribute %s\n",
                            relName, attrName );
                    return_val =  AMINIREL_GE;
                    cleanup();
                    return return_val;
                }

                /* prospername to arxiko ' */
                value++;
                /* kai diagrafoume to teliko ' */
                value[strlen( value ) - 1] = '\0';

                strncpy( pch, value, attributes[attrOffset]->attrlength );
                break;

            default:
                printf( "Error: Cannot insert into relation '%s'. Unknown attrtype for attribute '%s'.\n",
                        relName, attrName );
                return_val = AMINIREL_GE;
                cleanup();
                return return_val;
        } /* switch */

        /* H eisagwgh sto eurethriou tou attribute, an ayto yparxei, den mporei na ginei akoma giati de gnwrizoume to
           recId mexri na kanoume to insert sto arxeio ths sxeshs. */
    } /* for */



    /* Twra poy symplhrwsame to record me oles tis times pou prepei na periexei, to eisagoume sto arxeio ths sxeshs mas! */
    if (( recId = HF_InsertRec( fileDesc, record, relwidth ) ) < 0 ) {
        HF_PrintError( "Could not insert record in relation's file" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Twra pou exoume to recId, prepei gia ola ta attributes pou exoun index na eisagoyme se ayta thn timh ths eggrafhs */

    for ( i = 0; i < attrcnt; i++ ) {
        /* Ean exei eyrethrio to attribute */
        if ( attributes[i]->indexed == TRUE ) {

            /* Anoikse to eurethrio */
            if (( indexFileDesc = AM_OpenIndex( relName, attributes[i]->indexno ) ) < 0 ) {
                AM_PrintError( "Could not open index" );
                return_val = AMINIREL_GE;
                continue;
            }

            /* Thetoume ton pointer value sto swsto shmeio ths eggrafhs ap opou tha paroume tin timh mas */
            value = record + attributes[i]->offset;

            /* Eisagoume thn eggrafh sto eurethrio */
            if ( AM_InsertEntry( indexFileDesc, attributes[i]->attrtype, attributes[i]->attrlength, value, recId ) != AME_OK ) {
                AM_PrintError( "Could not insert into index" );
                return_val = AMINIREL_GE;
            }

            /* kleinoyme to eyrethrio */
            if ( AM_CloseIndex( indexFileDesc ) != AME_OK ) {
                AM_PrintError( "Could not close index" );
                return_val = AMINIREL_GE;
            }
        } /* if */
    } /* for */

    cleanup();
    return return_val;
}



/**** int DM_add( int argc, char* argv[] ) ****

 Usage:
        INSERT relname1 RECORDS OF relname2

 Operation:
        * Eisagei tis eggrafes ths sxeshs2 sth sxesh1 kai ta eyrethria ayths, efoson den parousiastei
          kapoio sfalma

 Return Value:
        * AMINIREL_OK, efoson epityxei
        * AMINIREL_GE, efoson apotyxei
*/
int DM_add( int argc, char* argv[] )
{
    int i, j, fileDescSource, fileDescDest, return_val, recId, scanDescDest, scanDescSource, insert_argc, argLength, ival;
    float fval;
    char * relNameSource, * relNameDest, * record, * pch, **insert_argv;
    relDesc * relPtrSource, * relPtrDest;
    attrDesc ** attributesSource, ** attributesDest;

    /* Prepei na exoyme 3 arguments. "add", relation1, relation2 */
    if ( argc != 3 ) {
        printf( "Error: Cannot add. Not enough information\n" );
        return AMINIREL_GE;
    }

    inline cleanup() {
        /* eleytherwnoyme th dynamikh mnhmh poy desmeysame */

        if ( insert_argv != NULL ) {
            for ( i = 0; i < insert_argc; i++ ) {
                if ( insert_argv[i] != NULL ) {
                    free( insert_argv[i] );
                }
            }

            free( insert_argv );
        }

        if ( attributesSource != NULL ) {
            for ( i = 0; i < relPtrSource->attrcnt; i++ ) {
                if ( attributesSource[i] != NULL ) {
                    free( attributesSource[i] );
                }
            }

            free( attributesSource );
        }

        if ( attributesDest != NULL ) {
            for ( i = 0; i < relPtrDest->attrcnt; i++ ) {
                if ( attributesDest[i] != NULL ) {
                    free( attributesDest[i] );
                }
            }

            free( attributesDest );
        }

        if ( relPtrSource != NULL ) {
            free( relPtrSource );
        }

        if ( relPtrDest != NULL ) {
            free( relPtrDest );
        }

        if ( record != NULL ) {
            free( record );
        }

        /* kleinoyme tyxon scans poy einai anoixta */
        if ( scanDescSource >= 0 ) {
            if ( HF_CloseFileScan( scanDescSource ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in attrCat" );
            }
        }

        if ( scanDescDest >= 0 ) {
            if ( HF_CloseFileScan( scanDescDest ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in attrCat" );
            }
        }

        /* kleinoyme ta arxeia twn sxeswn poy anoiksame */
        if ( HF_CloseFile( fileDescSource ) != HFE_OK ) {
            HF_PrintError( "Could not close relation file" );
        }

        if ( HF_CloseFile( fileDescDest ) != HFE_OK ) {
            HF_PrintError( "Could not close relation file" );
        }
    }

    /* initialization */
    relNameDest = argv[1];
    relNameSource = argv[2];
    return_val = AMINIREL_OK;
    relPtrSource = relPtrDest = NULL;
    record = NULL;
    attributesDest = attributesSource = NULL;
    scanDescSource = scanDescDest = -1;
    insert_argv = NULL;

    /* Anoigoume ta arxeia twn 2 sxesewn, tha mas xreiastoyn parakatw */

    if (( fileDescSource = HF_OpenFile( relNameSource ) ) < 0 ) {
        HF_PrintError( "Could not open relation file" );
        return AMINIREL_GE;
    }

    if (( fileDescDest = HF_OpenFile( relNameDest ) ) < 0 ) {
        HF_PrintError( "Could not open relation file" );

        /* kleinoyme to ena arxeio poy anoiksame */
        if ( HF_CloseFile( fileDescSource ) != HFE_OK ) {
            HF_PrintError( "Could not close relation file" );
        }

        return AMINIREL_GE;
    }

    /* Kanoyme retrieve tis plhrofories gia tis 2 sxeseis */

    if (( relPtrSource = getRelation( relNameSource, &recId ) ) == NULL ) {
        printf( "Error: Cannot add into relation '%s'. Information about relation '%s' could not be retrieved.\n",
                relNameDest, relNameSource );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( relPtrDest = getRelation( relNameDest, &recId ) ) == NULL ) {
        printf( "Error: Cannot add into relation '%s'. Information about relation '%s' could not be retrieved.\n",
                relNameDest, relNameDest );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Gia na ginei ADD arkei ta pedia twn 2 sxeswn na einai idioy typoy, mhkoys kai me thn idia seira. Profanws
       prepei na exoyme kai to idio plithos pediwn. De mas apasxoloyn ta onomata twn attributes */

    /* Elegxoume gia idio plithos attributes */
    if ( relPtrDest->attrcnt != relPtrSource->attrcnt ) {
        printf( "Error: Cannot add into relation '%s'. Uneven number of attributes amongst relations.\n", relNameDest );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Desmeuoume mnhmh gia toys pinakes pou periexoyn pointers sta attributes twn sxesewn pou xeirizomaste */
    if (( attributesSource = malloc( relPtrDest->attrcnt * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( attributesDest = malloc( relPtrDest->attrcnt * sizeof( attrDesc * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* initialize twn pinakwn */
    for ( i = 0; i < relPtrDest->attrcnt; i++ ) {
        attributesSource[i] = NULL;
        attributesDest[i] = NULL;
    }

    /* desmeuoume mnhmh gia to record sto opoio tha topothetoume tis eggrafes ap to attrCat arxeio, parakatw */
    if (( record = malloc( sizeof( attrDesc ) * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Ta attributes mias sxeshs vriskontai sto arxeio attrCat me th seira pou parousiazontai se mia eggrafh ths sxehs.
       Prwta tha vroume dhladh to 1o attribute me offset 0 k.o.k  */

    /* Eksetazoume parallhla ta attributes twn 2 sxesewn. Arxika, ksekiname 2 scans. To argument '1'
       einai gia 'EQUAL' */

    if (( scanDescSource = HF_OpenFileScan( this_db.attrCatDesc, sizeof( attrDesc ), 'c', MAXNAME, 0, 1, relNameSource ) ) < 0 ) {
        HF_PrintError( "Could not start scan in attrCat" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    if (( scanDescDest = HF_OpenFileScan( this_db.attrCatDesc, sizeof( attrDesc ), 'c', MAXNAME, 0, 1, relNameDest ) ) < 0 ) {
        HF_PrintError( "Could not start scan in attrCat" );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Gia kathe attribute... */
    for ( i = 0; i < relPtrDest->attrcnt; i++ ) {
        /* Kanoume retrieve to epomeno attribute kathe sxeshs */
        if ( HF_FindNextRec( scanDescSource, record ) < 0 ) {
            HF_PrintError( "Could not find record in attrCat" );
        }

        /* desmeuoume mnhmh gia to attribute */
        if (( attributesSource[i] = malloc( sizeof( attrDesc ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kai swzoyme ta dedomena toy poy xreiazomaste */

        /* offset */
        pch = record + 2 * ( MAXNAME * sizeof( char ) );
        memcpy( &attributesSource[i]->offset,  pch, sizeof( int ) );
        /* attrlength */
        pch += sizeof( int );
        memcpy( &attributesSource[i]->attrlength,  pch, sizeof( int ) );
        /* attrtype */
        pch += sizeof( int );
        attributesSource[i]->attrtype = ( *pch );

        /* paromoiws kai gia to 2o scan */
        if ( HF_FindNextRec( scanDescDest, record ) < 0 ) {
            HF_PrintError( "Could not find record in attrCat" );
        }

        /* desmeuoume mnhmh gia to attribute */
        if (( attributesDest[i] = malloc( sizeof( attrDesc ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        /* kai swzoyme ta dedomena toy poy xreiazomaste */

        /* attrname pou xreiazetai meta otan kanoume insert */
        pch = record + ( MAXNAME * sizeof( char ) );
        memcpy( attributesDest[i]->attrname, pch, MAXNAME );
        /* offset */
        pch += MAXNAME * sizeof( char );
        memcpy( &attributesDest[i]->offset,  pch, sizeof( int ) );
        /* attrlength */
        pch += sizeof( int );
        memcpy( &attributesDest[i]->attrlength,  pch, sizeof( int ) );
        /* attrtype */
        pch += sizeof( int );
        attributesDest[i]->attrtype = ( *pch );

        /* eksetazoume typo, offset k length anamesa sta 2 attributes! */

        if ( attributesDest[i]->attrtype != attributesSource[i]->attrtype ) {
            printf( "Error: Cannot add to relation '%s'. Different types amongst arguments no %d\n", relNameDest, i + 1 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        if ( attributesDest[i]->offset != attributesSource[i]->offset ) {
            printf( "Error: Cannot add to relation '%s'. Different offsets amongst arguments no %d\n", relNameDest, i + 1 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }

        if ( attributesDest[i]->attrlength != attributesSource[i]->attrlength ) {
            printf( "Error: Cannot add to relation '%s'. Different lengths amongst arguments no %d\n", relNameDest, i + 1 );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    } /* for */


    /* An ftasame edw, oi 2 sxeseis exoun attributes pou epitrepoun to add. To mono pou menei einai gia kathe record ths
       relationSource, na to eisagoume sth relationDest */

    /*
       * Tha xrhsimopoihsoyme th synarthsh DM_insert(). Epomenws etoimazoume katallhla ta arguments ths.
       * Ta arguments synolika einai 2 (ta arxika insert + relname) + 1 zeugari <attrname, value>
         gia kathe attribute.
    */
    insert_argc = 2 + ( relPtrDest->attrcnt * 2 );

    /* desmeyoyme mnhmh gia ton pinaka me ta arguments, +1 gia to teleytaio keli poy einai NULL */
    if (( insert_argv = malloc(( insert_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* kanoyme initialize ton pinaka me ta arguments */
    for ( i = 0; i <= insert_argc; i++ ) {
        insert_argv[i] = NULL;
    }

    /* to megalytero attrlength pou epitrepetai einai 255 xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws: */

    argLength = 255 + 2; /* +2 gia ta ' */

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    /* desmeuoume mnhmh gia ta arguments */
    for ( i = 0; i < insert_argc; i++ ) {
        if (( insert_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            return_val = AMINIREL_GE;
            cleanup();
            return return_val;
        }
    }

    /* thetoyme tis gnwstes times sta arguments */
    strcpy( insert_argv[0], "insert" );
    strcpy( insert_argv[1], relNameDest );

    for ( i = 2, j = 0; i < insert_argc; i += 2, j++ ) {
        strcpy( insert_argv[i], attributesDest[j]->attrname );
    }

    /* twra tha xreiastoume record diaforetikou megethous, oso einai h eggrafh twn arxeiwn */
    free( record );

    if (( record = malloc( relPtrSource->relwidth ) ) == NULL ) {
        printf( MEM_ERROR );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    /* Gia kathe record pou kanoume retrieve apo to arxeio ths sxeshs relSource, to eisagoume sth sxesh relDest */

    /* Ksekiname apo thn arxh tou arxeiou */
    if (( recId = HF_GetFirstRec( fileDescSource, record, relPtrSource->relwidth ) ) < 0 ) {
        /* Yparxei h periptwsh to arxeio na mhn exei eggrafes */
        if ( recId != HFE_EOF ) {
            HF_PrintError( "Could not retrieve first Record from file relCat" );
            return_val = AMINIREL_GE;
        }

        cleanup();
        return return_val;
    }

    do {
        /* symplhrwnoyme ta values sta antistoixa arguments */
        for ( i = 2, j = 0; i < insert_argc; i += 2, j++ ) {
            memset( insert_argv[i+1], 0, argLength );

            /* Proxwrame ton pch pointer sto swsto shmeio toy record gia na kanoyme retrieve thn timh */
            pch = record + attributesDest[j]->offset;

            switch ( attributesDest[j]->attrtype ) {
                case 'i' :
                    memcpy( &ival, pch, sizeof( int ) );
                    sprintf( insert_argv[i+1], "%d", ival );
                    break;

                case 'f' :
                    memcpy( &fval, pch, sizeof( float ) );
                    sprintf( insert_argv[i+1], "%f", fval );
                    break;

                case 'c' :
                    /* To arxiko " ths timhs */
                    insert_argv[i+1][0] = '\"';
                    strcpy( &( insert_argv[i+1][1] ), pch );
                    strcat( insert_argv[i+1], "\"" );
                    break;
            } /* switch */
        } /* for */

        /* Telos, eisagoume to record */
        if ( DM_insert( insert_argc, insert_argv ) != AMINIREL_OK ) {
            printf( "Error: Could not insert record in '%s'\n", relNameDest );
        }
    }
    while (( recId = HF_GetNextRec( fileDescSource, recId, record, relPtrSource->relwidth ) ) >= 0 );

    if ( recId != HFE_EOF ) {
        HF_PrintError( "Error while getting next record." );
        return_val = AMINIREL_GE;
        cleanup();
        return return_val;
    }

    cleanup();
    return return_val;
}
