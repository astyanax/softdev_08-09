#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DM_Lib.h"
#include "UT_Lib.h"
#include "HF_Lib.h"
#include "VW_Lib.h"
#include "defn.h"

void printArgs( int argc, char **argv )
{
    int i;
    printf( "-=-PrintArgs-=-\nargc = %d\n", argc );

    for ( i = 0; i < argc; ++i )
        printf( "argv[%d]=%s\n", i, argv[i] );

    printf( "argv[%d]=(null)\n-=-=-=-=-=-=-=-\n\n", i );
    fflush( stdout );
}

/*

int VW_CreateSelect(int argc, char* argv[])

usage:
    CREATE MVIEW viewName AS SELECT (rel1.attr1, ...) [WHERE rel.attr op value];

parser:
    argv[0]="createΜView"
    argv[1]=όνομα-όψης
    argv[2]=πλήθος-προβαλλόμενων-γνωρισμάτων
    argv[3]=όνομα-προβαλλόμενης-σχέσης-1
    argv[4]=όνομα-προβαλλόμενου-πεδίου-1
    ...
    argv[argc-6]=όνομα-προβαλλόμενης-σχέσης-Ν
    argv[argc-5]=όνομα-προβαλλόμενου-πεδίου-Ν
    argv[argc-4]=όνομα-σχέσης
    argv[argc-3]=όνομα-πεδίου
    argv[argc-2]=τελεστής
    argv[argc-1]=τιμή
    argv[argc]=NULL
    
operation:
        Dhmiourgei thn opsh "viewName", xrhsimopoiontas ta pedia relX.attrX, twn eggrafwn
        pou ikanopoioun th syn8hkh (efoson yparxei, alliws pros8etei oles tis eggrafes).

return:
       * VIEW_OK, se periptwsh pou epityxei
       * VIEW_GE, se opoiadhpote allh periptwsh
*/

int VW_createSelect( int argc, char *argv[] )
{
    int i, j, ival, this_arg, attrcnt, conditionAttr;
    int argLength, dmSelect_argc, index, * attrs;
    float fval;
    char * relName, * attrName, op, * pch;
    char * viewName, ** dmSelect_argv, where_part, found_it;

    relDesc relInfo;
    viewDesc viewInfo;
    viewAttrDesc viewAttrInfo;
    attrDesc * attributes;

    inline void cleanup() {
        /* eleytherwnoyme dynamikh mnhmh poy desmeysame */
        if ( attributes != NULL ) {
            free( attributes );
        }

        if ( attrs != NULL ) {
            free( attrs );
        }

        if ( dmSelect_argv != NULL ) {
            for ( i = 0; i < dmSelect_argc; i++ ) {
                if ( dmSelect_argv[i] != NULL ) {
                    free( dmSelect_argv[i] );
                }
            }

            free( dmSelect_argv );
        }
    }

    /* Initialization */
    this_arg = 1;
    attributes = NULL;
    dmSelect_argv = NULL;;
    attrs = NULL;

    /*
      Shmasiologikoi elegxoi:
    */

    /* Apaitoume toulaxiston 4 arguments: select 1 relname attrname */
    if ( argc < 5 ) {
        printf( "Error: Cannot select. Too few arguments provided.\n" );
        cleanup();
        return VIEW_GE;
    }

    /* O deikths sta arguments ksekinaei apo to prwto mh trivial argument, to onoma ths neas opshs */
    viewName = argv[this_arg];

    /* elegxoume to megethous tou onomatos ths opshs */
    if ( strlen( viewName ) >= MAXNAME ) {
        printf( "Error: Cannot select: View name '%s' is too long\n", viewName );
        cleanup();
        return VIEW_GE;
    }

    /* Elegxoume pws to onoma ths opshs den einai to TEMP (reserved by the system) */
    if ( strcmp( viewName, "TEMP" ) == 0 ) {
        printf( "Error: Cannot select: View name 'TEMP' is reserved by the system\n" );
        cleanup();
        return VIEW_GE;
    }

    /* Elegxoume an yparxei hdh h sxesh 'viewName' */
    if ( UT_relInfo( viewName, &relInfo, &attributes ) == 0 ) {
        printf( "Error: Cannot select: Relation '%s' already exists\n", viewName );
        cleanup();
        return VIEW_GE;
    }

    /* Proxwrame sto epomeno argument ths sunarthshs, pou periexei to plh8os twn proballomenwn orismatwn */
    this_arg++;

    /* Swzoyme to plithos twn orismatwn pou provallontai */
    if (( attrcnt = atoi( argv[this_arg] ) ) == 0 ) {
        printf( "Error: Cannot select. Could not parse number of attributes.\n" );
        cleanup();
        return VIEW_GE;
    }

    /* tha kratame se enan pinaka poia apo ta attributes ths sxeshs einai provallomena */
    if (( attrs = malloc( relInfo.attrcnt * sizeof( int ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return VIEW_GE;
    }

    /* initialization toy pinaka */
    for ( i = 0; i < relInfo.attrcnt; i++ ) {
        attrs[i] = 0;
    }

    /* Opote, twra pame sthn lista proballwmenwn pediwn */
    this_arg++;

    /* to onoma ths sxeshs pou xeirizomaste */
    relName = argv[this_arg];

    /* Kanoume retrieve tis plhrofories gia th sxesh */
    if ( UT_relInfo( relName, &relInfo, &attributes ) < 0 ) {
        printf( "Error: Cannot select: Relation '%s' does not exist\n", relName );
        cleanup();
        return VIEW_GE;
    }

    /* Kanoyme retrieve tis plhrofories gia kathe ena apo ta attributes pou provallontai */
    for ( i = 0; i < attrcnt; i++, this_arg += 2 ) {
        /* Ta provallomena attributes prepei na einai ths idias sxeshs! */
        if ( strcmp( relName, argv[this_arg] ) != 0 ) {
            printf( "Error: Cannot select. Cannot handle attributes from different relations '%s' and '%s'.\n",
                    relName, argv[this_arg] );
            cleanup();
            return VIEW_GE;
        }

        found_it = FALSE;

        /* elegxoume an to attribute ontws yparxei sth sxesh 'relName' */
        for ( j = 0; j < relInfo.attrcnt; j++ ) {
            if ( strcmp( attributes[j].attrname, argv[this_arg + 1] ) == 0 ) {
                found_it = TRUE;
                attrs[j] = 1;
                break;
            }
        }

        if ( found_it == FALSE ) {
            printf( "Error: Cannot select. Unknown attribute '%s'\n", argv[this_arg + 1] );
            cleanup();
            return VIEW_GE;
        }
    }

    /*
       elegxoume an yparxei to proairetiko kommati "WHERE condition". An yparxei, ftasame sto telos twn arguments
    */
    if ( this_arg == argc ) {
        /* An den yparxei "where" kommati, xeirizomaste oles tis eggrafes */
        op = 0;
        conditionAttr = 0;
        where_part = FALSE;
    }
    /* Alliws, an yparxei to "WHERE condition" kommati */
    else {
        /* To attribute sto condition prepei na einai ths idias sxeshs me ta provallomena attributes */
        if ( strcmp( relName, argv[this_arg] ) != 0 ) {
            printf( "Error: Cannot select. Cannot handle attributes from different relations '%s' and '%s'.\n",
                    relName, argv[this_arg] );
            cleanup();
            return VIEW_GE;
        }

        where_part = TRUE;

        /* to onoma tou attribute sto condition */
        attrName = argv[this_arg + 1];

        found_it = FALSE;

        /* elegxoyme an to attribute tou condition einai eggyro attribute ths sxeshs 'relName' */
        for ( j = 0; j < relInfo.attrcnt; j++ ) {
            if ( strcmp( attributes[j].attrname, attrName ) == 0 ) {
                found_it = TRUE;
                conditionAttr = j;
                break;
            }
        }

        if ( found_it == FALSE ) {
            printf( "Error: Cannot select. Unknown attribute '%s'\n", attrName );
            cleanup();
            return VIEW_GE;
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
            printf( "Error: Cannot select. Unrecognized symbol '%s' for operator.\n", argv[this_arg] );
            cleanup();
            return VIEW_GE;
        }

        /* Telos, mas menei h timh sygkrishs */
        this_arg++;

        /* kai kanoyme tis antistoixes energeies */

        switch ( attributes[conditionAttr].attrtype ) {
            case 'i':
                ival = atoi( argv[this_arg] );

                /* Otan apotygxanei h atoi epistrefei 0. Ara an epestrepse 0 kai o arithmos mas den einai
                   pragmatika to 0, tote error! */
                if ( ival == 0 && strcmp( argv[this_arg], "0" ) != 0 ) {
                    printf( "Error: Cannot select. Expected integer as value for condition attribute %s\n", attrName );
                    cleanup();
                    return VIEW_GE;
                }

                break;

            case 'f':
                fval = atof( argv[this_arg] );

                /* Otan apotygxanei h atof epistrefei 0.0. Ara an epestrepse 0.0 kai o arithmos mas den einai
                   pragmatika to 0.0, tote error! */
                if ( fval == 0.0 && strcmp( argv[this_arg], "0.0" ) != 0 && strcmp( argv[this_arg], "0" ) != 0 ) {
                    printf( "Error: Cannot select. Expected float as value for condition attribute %s\n", attrName );
                    cleanup();
                    return VIEW_GE;
                }

                break;

            case 'c':
                pch = argv[this_arg];

                if ( strlen( pch ) >  MAXSTRINGLENGTH ) {
                    printf( "Error: Cannot select. Value string too long\n" );
                    cleanup();
                    return VIEW_GE;
                }

                break;

            default:
                printf( "Error: Cannot select. Unknown attrtype '%c' for attribute '%s'.\n",
                        attributes[conditionAttr].attrtype, attrName );
                cleanup();
                return VIEW_GE;
        } /* switch */
    } /* else */


    /*
      Telos shmasiologikwn elegxwn
    */

    /*
        * Dhmioyrgoyme th lista me ta arguments pou prepei na perastoun sthn DM_select()
        * O arithmos twn arguments mas isoutai me =
            "select" + viewName + arithmos_provallomen_pediwn (attrcnt) + attrcnt * zeygaria (relName, attrName) provallomenwn pediwn +
            + ["relNname" + "cond_attrName" + "op" + "value"]
    */

    if ( where_part == TRUE ) {
        dmSelect_argc = 3 + attrcnt * 2 + 4;
    }
    else {
        dmSelect_argc = 3 + attrcnt * 2;
    }


    /* dhmioyrgoyme th lista apo ta arguments, +1 gia to NULL argument sto telos */
    if (( dmSelect_argv = malloc(( dmSelect_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return VIEW_GE;
    }


    /* Arxikopoioume ton pinaka mas */
    for ( i = 0; i <= dmSelect_argc; i++ ) {
        dmSelect_argv[i] = NULL;
    }

    /*
       to megalytero attrlength pou epitrepetai einai MAXSTRINGLENGTH xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws:
    */

    argLength = MAXSTRINGLENGTH;

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    for ( i = 0; i < dmSelect_argc; i++ ) {
        if (( dmSelect_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    /* Thetoume ta orismata */
    strcpy( dmSelect_argv[0], "select" );

    /* Pera apo to prwto, ola ta alla einai idia me ayta ths twrinhs synarthshs, epomenws ta antigrafoume me th seira pou dwthikan. */
    for ( index = 1; index < argc; index++ ) {
        strcpy( dmSelect_argv[index], argv[index] );
    }

    /* To teleutaio argument ypodhlwnei to telos ths entolhs */
    dmSelect_argv[index] = NULL;

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn DM_select:\n");
    printArgs(dmSelect_argc, dmSelect_argv);
    #endif

    /* klhsh ths select into ... */
    if ( DM_select( dmSelect_argc, dmSelect_argv ) != DME_OK ) {
        printf( "Error: Cannot select. Error encountered at DM_select()\n" );
        cleanup();
        return VIEW_GE;
    }


    /* Thetoume tis swstes times sto viewInfo to opoio tha perastei sto updateViewCat gia enhmerwsh ths sxeshs viewCat */

    memset( &viewInfo, 0, sizeof( viewDesc ) );
    viewInfo.attrcnt = attrcnt;
    strcpy( viewInfo.viewname, viewName );
    viewInfo.op = op;

    /* Yparxei condition (Conditional select) */
    if ( where_part == TRUE ) {
        viewInfo.type = COND_SELECT;
        strcpy( viewInfo.value, argv[argc - 1] );
        strcpy( viewInfo.relname1, relName );
        strcpy( viewInfo.attrname1, argv[argc-3] );
        viewInfo.relname2[0] = '\0';
        viewInfo.attrname2[0] = '\0';
    }
    /* Den yparxei :O */
    else {
        viewInfo.type = NONCOND_SELECT;
        viewInfo.value[0] = '\0';
        strcpy( viewInfo.relname1, relName );
        viewInfo.relname2[0] = '\0';
        viewInfo.attrname1[0] = '\0';
        viewInfo.attrname2[0] = '\0';
    }

    /* update tou viewCat */
    if ( updateViewCat( &viewInfo , attributes[conditionAttr].attrtype ) == VIEW_GE ) {
        printf( "Error: Cannot select. Cannot update viewCat\n" );
        cleanup();
        return VIEW_GE;
    }

    /*
        Symplhrwnoume tis times pou tha krathsoume gia ta attributes ths opshs gia na enmherwsoume th viewAttrCat sxesh
    */

    /* gia ola ta provallomena attributes pou ginontai kai attributes ths opshs */
    for ( i = 0; i < relInfo.attrcnt; i++ ) {

        /* Elegxoume wste to twrino attribute na einai provallomeno attribute */
        if ( attrs[i] == 0 ) {
            continue;
        }

        /* symplhrwnoume tis times tous */
        memset( &viewAttrInfo, 0, sizeof( viewAttrDesc ) );
        strcpy( viewAttrInfo.viewname, viewName );
        strcpy( viewAttrInfo.viewattrname, attributes[i].attrname );
        strcpy( viewAttrInfo.relname, attributes[i].relname );
        strcpy( viewAttrInfo.relattrname, attributes[i].attrname );

        /* update tou viewAttrCat */
        if ( updateViewAttrCat( &viewAttrInfo ) == VIEW_GE ) {
            printf( "Error: Cannot select. Cannot update viewAttrCat\n" );
            cleanup();
            return VIEW_GE;
        }
    }

    cleanup();
    return VIEW_OK;
}

/**** int VW_createJoin(int argc, char* argv[]) ****

usage:
    CREATE MVIEW viewName AS SELECT (rel-1.attr-1, ...) WHERE rel-1.attr-1 op rel-2.attr-2;

parser:
    argv[0]="createΜView"
    argv[1]=όνομα-όψης
    argv[2]=πλήθος-προβαλλόμενων-γνωρισμάτων
    argv[3]=όνομα-προβαλλόμενης-σχέσης-1
    argv[4]=όνομα-προβαλλόμενου-πεδίου-1
    ...
    argv[argc-7]=όνομα-προβαλλόμενης-σχέσης-Ν
    argv[argc-6]=όνομα-προβαλλόμενου-πεδίου-Ν
    argv[argc-5]=όνομα-σχέσης-1
    argv[argc-4]=όνομα-πεδίου-1
    argv[argc-3]=τελεστής
    argv[argc-2]=όνομα-σχέσης-2
    argv[argc-1]=όνομα-πεδίου-2
    argv[argc]=NULL

operation:
        Dhmiourgei thn opsh "viewName", xrhsimopoiontas ta pedia relX.attrX, twn eggrafwn
        pou ikanopoioun th syn8hkh.

return:
       * VIEW_OK, se periptwsh pou epityxei
       * VIEW_GE, se opoiadhpote allh periptwsh
*/

int VW_createJoin( int argc, char *argv[] )
{
    int i, j, attrcnt, this_arg, dmJoin_argc, argLength, op;
    char * viewName, * relName1, * relName2, found_it, ** dmJoin_argv;
    relDesc relInfo1, relInfo2, viewRelInfo;
    attrDesc * attributes1, * attributes2, * viewRelAttrs;
    viewDesc viewInfo;
    viewAttrDesc * viewAttrs, viewAttrInfo;

    inline void cleanup() {
        /* Apeleytherwsh dynamikhs mnhmhs poy desmeysame */
        if ( attributes1 != NULL ) {
            free( attributes1 );
        }

        if ( attributes2 != NULL ) {
            free( attributes2 );
        }

        if ( viewAttrs != NULL ) {
            free( viewAttrs );
        }

        if ( viewRelAttrs != NULL ) {
            free( viewRelAttrs );
        }

        /* DEBUG: se comments logw (pi8anws palioterou, den elegx8hke prosfata) bug ths biblio8hkhs */
        /*
        if ( dmJoin_argv != NULL ) {
           for ( i = 0; i < dmJoin_argc; i++ ) {
               if ( dmJoin_argv[i] != NULL ) {
                  free(dmJoin_argv[i]);
               }
           }

           free(dmJoin_argv);
        }
        */
    }

    /* initialization */
    viewName = argv[1];
    attributes1 = NULL;
    attributes2 = NULL;
    viewAttrs = NULL;
    viewRelAttrs = NULL;
    dmJoin_argv = NULL;


    /*
      Shmasiologikoi elegxoi
    */


    /* Apaitoume toulaxiston 10 arguments. createΜView viewname 1 relname attrname relname1 attrnam1 op relname2 attrname2 */
    if ( argc < 10 ) {
        printf( "Error: Cannot join. Expected at least 10 arguments for join mview operation\n" );
        return VIEW_GE;
    }

    /* elegxoume to megethous tou onomatos ths opshs */
    if ( strlen( viewName ) >= MAXNAME ) {
        printf( "Error: Cannot join: View name '%s' is too long\n", viewName );
        return VIEW_GE;
    }

    /* Elegxoume pws to onoma ths opshs den einai to TEMP (reserved by the system) */
    if ( strcmp( viewName, "TEMP" ) == 0 ) {
        printf( "Error: Cannot join: View name 'TEMP' is reserved by the system\n" );
        return VIEW_GE;
    }

    /* Elegxoume an yparxei hdh h sxesh 'viewName' */
    if ( UT_relInfo( viewName, &relInfo1, &attributes1 ) == 0 ) {
        printf( "Error: Cannot join: Relation '%s' already exists\n", viewName );
        cleanup();
        return VIEW_GE;
    }

    /* Swzoyme to plithos twn orismatwn pou provallontai */
    if (( attrcnt = atoi( argv[2] ) ) == 0 ) {
        printf( "Error: Cannot join. Could not parse number of attributes.\n" );
        cleanup();
        return VIEW_GE;
    }

    relName1 = NULL;
    relName2 = NULL;

    /*
       * elegxoume oti kathena apo ta provallomena attributes proerxetai apo mia ek twn 2 sxesewn
       * apothikeuoume sto relName1 kai sto relName2 ta onomata twn sxesewn pou vriskoume sto twrino kommati me
         ta provallomena attributes, wste argotera na ta eleksoume me ta onomata twn sxesewn sto "where condition" komamti
    */
    for ( i = 0, this_arg = 3; i < attrcnt; i++, this_arg += 2 ) {
        if ( relName1 == NULL ) {
            relName1 = argv[this_arg];
        }
        else if (( relName2 == NULL ) && ( strcmp( argv[this_arg], relName1 ) != 0 ) ) {
            relName2 = argv[this_arg];
        }
        /* an to twrino relName den anhkei oute sto relName1, oute sto relName2 */
        else if (( strcmp( relName1, argv[this_arg] ) != 0 ) && ( strcmp( relName2, argv[this_arg] ) != 0 ) ) {
            printf( "Error: Cannot join. More than two relations specified.\n" );
            cleanup();
            return VIEW_GE;
        }
    }

    /* den epitrepetai to self join */
    if ( strcmp( argv[argc - 5], argv[argc - 2] ) == 0 ) {
        printf( "Error: Cannot join. Self-Join prohibited.\n" );
        cleanup();
        return VIEW_GE;
    }

    /* An de vrhkame apo ta provallomena attributes to relName2 (dhladh provallame mono ta attributes ths mias sxeshs) */
    if ( relName2 == NULL ) {
        /* to vriskoume apo ta relNames sto "where condition" kommati */
        if ( strcmp( argv[argc - 2], relName1 ) == 0 ) {
            relName2 = argv[argc - 5];
        }
        else {
            relName2 = argv[argc - 2];
        }
    }

    /* Elegxoume wste oi sxeseis sto condition na einai oi idies sxeseis me aytes twn provallomenwn attributes */
    if ( strcmp( argv[argc - 2], relName1 ) != 0 && strcmp( argv[argc - 5], relName1 ) ) {
        printf( "Error: Cannot join. More than 2 relations specified\n" );
        cleanup();
        return VIEW_GE;
    }

    if ( strcmp( argv[argc - 2], relName2 ) != 0 && strcmp( argv[argc - 5], relName2 ) ) {
        printf( "Error: Cannot join. More than 2 relations specified\n" );
        cleanup();
        return VIEW_GE;
    }

    /* Anaktoume tis plhrofories twn 2 relations */

    if ( UT_relInfo( relName1, &relInfo1, &attributes1 ) != 0 ) {
        printf( "Error: Cannot join: Could not retrieve information about relation '%s'\n", relName1 );
        cleanup();
        return VIEW_GE;
    }

    if ( UT_relInfo( relName2, &relInfo2, &attributes2 ) != 0 ) {
        printf( "Error: Cannot join: Could not retrieve information about relation '%s'\n", relName2 );
        cleanup();
        return VIEW_GE;
    }

    /*
       * Prepei toulaxiston mia apo tis 2 sxeseis na einai prwtarxikh, dhladh na yparxei san sxesh alla na mhn einai opsh
       * Eksetazoume an h sxesh 'relName1' einai prwtarxikh
    */
    if ( getViewRelation( relName1, &viewInfo, &viewAttrs ) == VIEW_OK ) {
        /*
           * An h sxesh 'relName1' den einai prwtarxikh, tote prepei aparaithta h sxesh 'relName2' na einai
           * Apeleytherwnoume dynamikh mnhmh poy desmeysame gia ta attributes ths opshs
        */
        if ( viewAttrs != NULL ) {
            free( viewAttrs );
        }

        viewAttrs = NULL;

        if ( getViewRelation( relName2, &viewInfo, &viewAttrs ) == VIEW_OK ) {
            printf( "Error: Cannot join. Neither of relations '%s' & '%s' is a primary relation\n", relName1, relName2 );
            cleanup();
            return VIEW_GE;
        }
    }

    /*
       Twra pou eimaste sigouroi gia ta onomata twn 2 sxesewn kai exoume tis plhrofories aytwn, elegxoume an ta
       provallomena attributes einai eggyra
    */
    for ( i = 0, this_arg = 3; i < attrcnt; i++, this_arg += 2 ) {
        found_it = FALSE;

        /* anazhthsh sta attributes ths sxeshs 'relName1' */
        if ( strcmp( argv[this_arg], relName1 ) == 0 ) {
            /* elegxoume an to attribute ontws yparxei sth sxesh 'relName1' */
            for ( j = 0; j < relInfo1.attrcnt; j++ ) {
                if ( strcmp( attributes1[j].attrname, argv[this_arg + 1] ) == 0 ) {
                    found_it = TRUE;
                    break;
                }
            }
        }
        /* anazhthsh sta attributes ths sxeshs 'relName2' */
        else {
            /* elegxoume an to attribute ontws yparxei sth sxesh 'relName2' */
            for ( j = 0; j < relInfo2.attrcnt; j++ ) {
                if ( strcmp( attributes2[j].attrname, argv[this_arg + 1] ) == 0 ) {
                    found_it = TRUE;
                    break;
                }
            }

        }

        /* vrhkame to provallomeno attribute sta attributes ths sxeshs mas? */
        if ( found_it == FALSE ) {
            printf( "Error: Cannot join. Could not retrieve information about attribute '%s' of relation '%s'\n",
                    argv[this_arg + 1], argv[this_arg] );
            cleanup();
            return VIEW_GE;
        }
    } /* for */

    /*
       * Elegxoume ean ta attributes sto "where condition" kommati einai eggyra attributes
       * Sthn arxh gia to prwto attribute sto "where condition" kommati, argc - 5
    */

    this_arg = argc - 5;
    found_it = FALSE;

    /* anazhthsh sta attributes ths sxeshs 'relName1' */
    if ( strcmp( argv[this_arg], relName1 ) == 0 ) {
        /* elegxoume an to attribute ontws yparxei sth sxesh 'relName1' */
        for ( j = 0; j < relInfo1.attrcnt; j++ ) {
            if ( strcmp( attributes1[j].attrname, argv[this_arg + 1] ) == 0 ) {
                found_it = TRUE;
                break;
            }
        }
    }
    /* anazhthsh sta attributes ths sxeshs 'relName2' */
    else {
        /* elegxoume an to attribute ontws yparxei sth sxesh 'relName2' */
        for ( j = 0; j < relInfo2.attrcnt; j++ ) {
            if ( strcmp( attributes2[j].attrname, argv[this_arg + 1] ) == 0 ) {
                found_it = TRUE;
                break;
            }
        }

    }

    /* vrhkame to provallomeno attribute sta attributes ths sxeshs mas? */
    if ( found_it == FALSE ) {
        printf( "Error: Cannot join. Could not retrieve information about attribute '%s' of relation '%s'\n",
                argv[this_arg + 1], argv[this_arg] );
        cleanup();
        return VIEW_GE;
    }

    /* Twra gia to deutero attribute sto "where condition" kommati, argc - 5 */
    this_arg = argc - 2;
    found_it = FALSE;

    /* anazhthsh sta attributes ths sxeshs 'relName1' */
    if ( strcmp( argv[this_arg], relName1 ) == 0 ) {
        /* elegxoume an to attribute ontws yparxei sth sxesh 'relName1' */
        for ( j = 0; j < relInfo1.attrcnt; j++ ) {
            if ( strcmp( attributes1[j].attrname, argv[this_arg + 1] ) == 0 ) {
                found_it = TRUE;
                break;
            }
        }
    }
    /* anazhthsh sta attributes ths sxeshs 'relName2' */
    else {
        /* elegxoume an to attribute ontws yparxei sth sxesh 'relName2' */
        for ( j = 0; j < relInfo2.attrcnt; j++ ) {
            if ( strcmp( attributes2[j].attrname, argv[this_arg + 1] ) == 0 ) {
                found_it = TRUE;
                break;
            }
        }

    }

    /* vrhkame to provallomeno attribute sta attributes ths sxeshs mas? */
    if ( found_it == FALSE ) {
        printf( "Error: Cannot join. Could not retrieve information about attribute '%s' of relation '%s'\n",
                argv[this_arg + 1], argv[this_arg] );
        cleanup();
        return VIEW_GE;
    }

    /* Telos, elegxoume ton operator */
    this_arg = argc - 3;

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
        printf( "Error: Cannot join. Unrecognized symbol '%s' for operator.\n", argv[this_arg] );
        cleanup();
        return VIEW_GE;
    }

    /*
      Telos shmasiologikwn elegxwn
    */

    /* Twra tha dhmioyrghsoume th lista me ta arguments pou prepei na perastoun sthn DM_join */
    /*
        * Dhmioyrgoyme th lista me ta arguments pou prepei na perastoun sthn DM_join()
        * O arithmos twn arguments mas isoutai me =
            "join" + viewName + arithmos_provallomen_pediwn (attrcnt) + attrcnt * zeygaria (relName, attrName) provallomenwn pediwn +
            + "relName1" + "cond_attrName1" + "op" + "relName2" + "cond_attrName2"
    */

    dmJoin_argc = 3 + ( attrcnt * 2 ) + 5;

    /* dhmioyrgoyme th lista apo ta arguments, +1 gia to NULL argument sto telos */
    if (( dmJoin_argv = malloc(( dmJoin_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return VIEW_GE;
    }


    /* Arxikopoioume ton pinaka mas */
    for ( i = 0; i <= dmJoin_argc; i++ ) {
        dmJoin_argv[i] = NULL;
    }

    /*
       to megalytero attrlength pou epitrepetai einai MAXSTRINGLENGTH xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws:
    */

    argLength = MAXSTRINGLENGTH;

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    /* desmeysh mnhmhs gia ta strings */
    for ( i = 0; i < dmJoin_argc; i++ ) {
        if (( dmJoin_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    /* Thetoume ta orismata */
    strcpy( dmJoin_argv[0], "join" );

    /* Pera apo to prwto, ola ta alla einai idia me ayta ths twrinhs synarthshs, epomenws ta antigrafoume me th seira pou dwthikan. */
    for ( i = 1; i < argc; i++ ) {
        strcpy( dmJoin_argv[i], argv[i] );
    }

    /* To teleutaio argument ypodhlwnei to telos ths entolhs */
    dmJoin_argv[i] = NULL;

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn DM_join\n");
    printArgs(dmJoin_argc, dmJoin_argv);
    #endif

    if ( DM_join( dmJoin_argc, dmJoin_argv ) != DME_OK ) {
        printf("\n* Error encountered at DM_join()\n");
        cleanup();
        return VIEW_GE;
    }

    /* Thetoume tis swstes times sto viewInfo to opoio tha perastei sto updateViewCat gia enhmerwsh ths sxeshs viewCat */
    memset( &viewInfo, 0, sizeof( viewDesc ) );
    strcpy( viewInfo.viewname, viewName );
    viewInfo.type = JOIN_SELECT;
    strcpy( viewInfo.relname1, argv[argc-5] );
    strcpy( viewInfo.attrname1, argv[argc-4] );
    viewInfo.op = op;
    strcpy( viewInfo.relname2, argv[argc-2] );
    strcpy( viewInfo.attrname2, argv[argc-1] );
    viewInfo.attrcnt = attrcnt;
    viewInfo.value[0] = '\0';

    /*
       * update tou viewCat
       * To 2o argument ypodhlwnei ti typoy einai to "value" to opoio omws value sthn periptwsh tou join den yfistatai, opote
         pername mia tyxaia timh poy agnoeitai etsi ki alliws
    */
    if ( updateViewCat( &viewInfo , 'a' ) == VIEW_GE ) {
        printf("\n* Error: Cannot join. Cannot update viewCat\n");
        cleanup();
        return VIEW_GE;
    }

    /*
        * Symplhrwnoume tis times pou tha krathsoume gia ta attributes ths opshs gia na enmherwsoume th viewAttrCat sxesh
        * se prwth fash, anaktoume plhrofories gia thn opsh pou molis dhmioyrgithike apo ta relCat/attrCat
    */

    /* Anaktoume tis plhrofories ths opshs */
    if ( UT_relInfo( viewName, &viewRelInfo, &viewRelAttrs ) != 0 ) {
        printf("Error: Cannot join. Error while updating viewAttrCat \n" );
        cleanup();
        return VIEW_GE;
    }

    /* gia ola ta provallomena attributes pou ginontai kai attributes ths opshs */
    for ( i = 0, this_arg = 3; i < viewRelInfo.attrcnt; i++, this_arg += 2 ) {


        /* symplhrwnoume tis times tous */
        memset( &viewAttrInfo, 0, sizeof( viewAttrDesc ) );
        strcpy( viewAttrInfo.viewname, viewName );
        strcpy( viewAttrInfo.viewattrname, viewRelAttrs[i].attrname );
        strcpy( viewAttrInfo.relname, argv[this_arg] );
        strcpy( viewAttrInfo.relattrname, argv[this_arg + 1] );

        /* update tou viewAttrCat */
        if ( updateViewAttrCat( &viewAttrInfo ) == VIEW_GE ) {
            printf("Error: Cannot join. Cannot update viewAttrCat\n" );
            cleanup();
            return VIEW_GE;
        }
    }

    cleanup();
    return VIEW_OK;
}

/**** int VW_delete(int argc, char *argv[]) ****

usage:
    DELETE relName [WHERE relName.attrName op val];

parser:
    argv[0]="delete"
    argv[1]=όνομα-σχέσης
    argv[argc-3]=όνομα-πεδίου
    argv[argc-2]=τελεστής
    argv[argc-1]=τιμή
    argv[argc]=NULL

operation:
    Diagrafetai th sygkekrimenh prwtarxikh sxesh kai enhmerwnei oles tis opseis
    pou ephreazontai apo thn diagrafh.

return:
    * VIEW_OK, se periptwsh pou epityxei
    * VIEW_GE, se opoiadhpote allh periptwsh
*/

int VW_delete( int argc, char *argv[] )
{
    int i, j, nviews, attr, offset, op, conditionAttr, where_part,  found_it, ival;
    int return_val, dmSelect_argc, utDestroy_argc, argLength;
    float fval;
    char relName[MAXNAME], * attrName, * utDestroy_argv[3], ** dmSelect_argv, ** views;
    relDesc fromRelDesc;
    attrDesc * fromRelAttrs;
    viewDesc view;
    viewAttrDesc * viewAttrs;

    inline void cleanup() {
        /* eleytherwnoyme dynamikh mnhmh poy desmeysame */
        for ( i = 0; i < utDestroy_argc; i++ ) {
            if ( utDestroy_argv[i] != NULL ) {
                free( utDestroy_argv[i] );
            }
        }

        if ( dmSelect_argv != NULL ) {
            for ( i = 0; i < dmSelect_argc; i++ ) {
                if ( dmSelect_argv[i] != NULL ) {
                    free( dmSelect_argv[i] );
                }
            }

            free( dmSelect_argv );
        }
    }

    /* initialization */
    fromRelAttrs = NULL;
    viewAttrs = NULL;
    views = NULL;
    where_part = FALSE;
    utDestroy_argc = 2;
    dmSelect_argv = NULL;

    /* initialization tou pinaka */
    for ( i = 0; i <= utDestroy_argc; i++ ) {
        utDestroy_argv[i] = NULL;
    }

    /*
      Shmasiologikoi elegxoi
    */
    
    #ifdef DEBUG
    printf("\n* Molis mphkame sthn delete. Arguments:\n");
    printArgs( argc, argv );
    #endif

    /* Apaitoume 2 h 5 akrivws arguments */
    if (( argc != 2 ) && ( argc != 5 ) ) {
        printf("Error: Cannot delete. Expected 2 or 5 arguments.\n");
        return VIEW_GE;
    }

    strcpy( relName, argv[1] );

    /* Eksetazoume an yparxei h sxesh kai einai prwtarxikh */
    if ( UT_relInfo( relName, &fromRelDesc, &fromRelAttrs ) < 0 ) {
        printf("Error: Cannot delete. Could not retrieve information about relation '%s'\n", relName);
        cleanup();
        return VIEW_GE;
    }

    /* H sxesh mas prepei na einai prwtarxikh, dhladh na yparxei san sxesh alla na mhn einai opsh */
    if ( getViewRelation( relName, &view, &viewAttrs ) == VIEW_OK ) {
        printf("Error: Cannot delete. Relation '%s' is not a primary relation.\n", relName);
        cleanup();
        return VIEW_GE;
    }

    /*
       elegxoume an yparxei to proairetiko kommati "WHERE condition". An yparxei, ftasame sto telos twn arguments
    */
    if ( argc == 2 ) {
        /* An den yparxei "where" kommati, xeirizomaste oles tis eggrafes */
        op = 0;
        conditionAttr = 0;
        where_part = FALSE;
    }
    /* Alliws, an yparxei to "WHERE condition" kommati */
    else {
        /* To attribute sto condition prepei na einai ths idias sxeshs me ta provallomena attributes */
        attrName = argv[argc-3];
        where_part = TRUE;
        found_it = FALSE;

        /* elegxoyme an to attribute tou condition einai eggyro attribute ths sxeshs 'relName' */
        for ( j = 0; j < fromRelDesc.attrcnt; j++ ) {
            if ( strcmp( fromRelAttrs[j].attrname, attrName ) == 0 ) {
                found_it = TRUE;
                conditionAttr = j;
                break;
            }
        }

        if ( found_it == FALSE ) {
            printf( "Error: Cannot delete. Unknown attribute '%s'\n", attrName );
            cleanup();
            return VIEW_GE;
        }

        if ( strcmp( argv[argc-2], "=" ) == 0 ) {
            op = EQUAL;
        }
        else if ( strcmp( argv[argc-2], "!=" ) == 0 ) {
            op = NOT_EQUAL;
        }
        else if ( strcmp( argv[argc-2], "<" ) == 0 ) {
            op = LESS_THAN;
        }
        else if ( strcmp( argv[argc-2], ">" ) == 0 ) {
            op = GREATER_THAN;
        }
        else if ( strcmp( argv[argc-2], ">=" ) == 0 ) {
            op = GREATER_THAN_OR_EQUAL;
        }
        else if ( strcmp( argv[argc-2], "<=" ) == 0 ) {
            op = LESS_THAN_OR_EQUAL;
        }
        else {
            printf( "Error: Cannot delete. Unrecognized symbol '%s' for operator.\n", argv[argc-2] );
            return_val = VIEW_GE;
            cleanup();
            return return_val;
        }

        /* kai kanoyme tis antistoixes energeies */
        switch ( fromRelAttrs[conditionAttr].attrtype ) {
            case 'i':
                ival = atoi( argv[argc-1] );

                /* Otan apotygxanei h atoi epistrefei 0. Ara an epestrepse 0 kai o arithmos mas den einai
                   pragmatika to 0, tote error! */
                if ( ival == 0 && strcmp( argv[argc-1], "0" ) != 0 ) {
                    printf( "Error: Cannot delete. Expected integer as value for condition attribute %s\n", attrName );
                    return_val =  VIEW_GE;
                    cleanup();
                    return return_val;
                }

                break;

            case 'f':
                fval = atof( argv[argc-1] );

                /* Otan apotygxanei h atof epistrefei 0.0. Ara an epestrepse 0.0 kai o arithmos mas den einai
                   pragmatika to 0.0, tote error! */
                if ( fval == 0.0 && strcmp( argv[argc-1], "0.0" ) != 0 && strcmp( argv[argc-1], "0" ) != 0 ) {
                    printf( "Error: Cannot delete. Expected float as value for condition attribute %s\n", attrName );
                    return_val =  VIEW_GE;
                    cleanup();
                    return return_val;
                }

                break;

            case 'c':
                if ( strlen( argv[argc-1] ) >  MAXSTRINGLENGTH ) {
                    printf( "Error: Cannot delete. Value string too long\n" );
                    cleanup();
                    return VIEW_GE;
                }

                break;

            default:
                printf( "Error: Cannot delete. Unknown attrtype '%c' for attribute '%s'.\n",
                        fromRelAttrs[conditionAttr].attrtype, attrName );
                return_val = VIEW_GE;
                cleanup();
                return return_val;
        } /* switch */
    } /* else */

    /*
      Telos Shmasiologikwn elegxwn
    */

    /*
          * 1. Δημιούργησε ένα νεο πίνακα TEMP και βάλε σε αυτόν όλες τις εγγραφές της σχέσης relName που ικανοποιούν
               την deletecondition.
          * Ayto tha to kanoyme me mia SELECT INTO eperwthsh
          * O arithmos twn arguments mas isoutai me =
            "select" + viewName + arithmos_provallomen_pediwn (attrcnt) + attrcnt * zeygaria (relName, attrName) provallomenwn pediwn +
            + ["relNname" + "cond_attrName" + "op" + "value"]
    */

    if ( where_part == TRUE ) {
        dmSelect_argc = 3 + fromRelDesc.attrcnt * 2 + 4;
    }
    else {
        dmSelect_argc = 3 + fromRelDesc.attrcnt * 2;
    }

    /* Dhmioyrgoyme ton pinaka ston opoio tha perasoume ta arguments gia thn DM_Select, +1 gia to NULL */
    if (( dmSelect_argv = malloc(( dmSelect_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return VIEW_GE;
    }

    /* initialization tou pinaka */
    for ( i = 0; i <= dmSelect_argc; i++ ) {
        dmSelect_argv[i] = NULL;
    }

    /*
       to megalytero attrlength pou epitrepetai einai MAXSTRINGLENGTH xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws:
    */

    argLength = MAXSTRINGLENGTH;

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    /* desmeuoume mnhmh gia ta strings ths DM_select */
    for ( i = 0; i < dmSelect_argc; i++ ) {
        if (( dmSelect_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    /* Twra, epitelous tha symplhrwsoume tis times tou pinaka dmSelect_argv pou tha perastei meta sthn DM_select() function */
    strcpy( dmSelect_argv[0], "select" );
    strcpy( dmSelect_argv[1], "TEMP" );
    sprintf( dmSelect_argv[2], "%d", fromRelDesc.attrcnt );

    /* Symplhrwnoume tis times twn arguments gia ola ta attributes */
    for ( attr = 0, offset = 3; attr < fromRelDesc.attrcnt; attr++, offset += 2 ) {
        /* to zeugari apoteleitai apo to onoma tou attribute */
        strcpy( dmSelect_argv[offset], fromRelAttrs[attr].relname );
        strcpy( dmSelect_argv[offset+1], fromRelAttrs[attr].attrname );
    }

    /* An eixame "where condition" kommati, tote to symplhrwnoume kai gia thn select eperwthsh */
    if ( where_part == TRUE ) {
        strcpy( dmSelect_argv[offset++], relName );
        strcpy( dmSelect_argv[offset++], argv[argc-3] );
        strcpy( dmSelect_argv[offset++], argv[argc-2] );
        strcpy( dmSelect_argv[offset++], argv[argc-1] );
    }

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn DM_join\n");
    printArgs( dmSelect_argc, dmSelect_argv );
    #endif

    if ( DM_select( dmSelect_argc, dmSelect_argv ) != DME_OK ) {
        printf( "Error: Cannot delete. Could not create relation 'TEMP'\n" );
        cleanup();
        return VIEW_GE;
    }


    /*2. Για κάθε όψη V πάνω στη σχέση relName κάνε ενημέρωση ως εξής: */
    if (( views = getViewsOfRelation( relName, &nviews ) ) != NULL && nviews != 0 ) {
        #ifdef DEBUG
        printf( "\n* Oi opseis panw sth sxesh '%s' :\n", relName );
        printArgs( nviews, views );
        #endif

        for ( i = 0; i < nviews; i++ ) {
            /* 2.1 recursive_update(V,relName,temp, 1); */
            if ( recursive_update( views[i], relName, "TEMP", 1, 0 ) != VIEW_OK ) {
                printf( "Error: Cannot delete. Error while using recursive update\n" );
            }
        }
    }

    /* 3. Κατέστρεψε τον πίνακα TEMP */

    /*
       * Dhmioyrgoyme ta arguments pou tha perastoun sthn UT_destroy.
       * "destroy" + "TEMP" + NULL
    */

    for ( i = 0; i < utDestroy_argc; i++ ) {
        if (( utDestroy_argv[i] = malloc( MAXNAME * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    strcpy( utDestroy_argv[0], "destroy" );
    strcpy( utDestroy_argv[1], "TEMP" );
    utDestroy_argv[2] = NULL;

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn UT_destroy (gia th dhmiourgia TEMP pinaka)\n");
    printArgs( utDestroy_argc, utDestroy_argv );
    #endif

    /* Katastrefoume thn 'TEMP' sxesh */

    if ( UT_destroy( utDestroy_argc, utDestroy_argv ) != OK ) {
        printf( "Error: Cannot delete. Could not destroy relation 'TEMP'\n" );
        cleanup();
        return VIEW_GE;
    }

    /* 4. Διέγραψε από την σχέση relName όλες τις εγγραψές που ικανοποιούν την deletecondition */
    /* Enhmerw8hkan swsta oi opseis, opote einai kairos na ektelestei kai h diagrafh  */
    cleanup();

    #ifdef DEBUG
    printf( "\n* Destroy just completed! Arguments passed to DM_delete:\n" );
    printArgs( argc, argv );
    #endif
    
    return DM_delete( argc, argv );
}


/**** int VW_insert( int argc, char* argv[] ) ****

 Usage:
        INSERT relname(attr1='val', attr2='va2', ...);

 Operation:
       H VW_Insert, afou elegksei pws mporei pragmati na ektelestei h DM_insert (afou dld ektelesei
          olous tous aparaithtous shmasiologikous elegxous), psaxnei oles tis opseis, kai ananewnei
          autes pou ephreazontai apo thn eisagwgh.

 Return Value:
        * VIEW_OK, efoson epityxei
        * VIEW_GE, efoson apotyxoun oi shmasiologikoi elegxoi (sto VW epipedo dld)
        * Kapoion error code (p.x. typou DME_<tade>), efoson apotyxei kapoio katwtero epipedo.
*/
int VW_insert( int argc, char* argv[] )
{
    int i, j, attrcnt, attr, offset, return_val, relwidth, utDestroy_argc, utCreate_argc, argLength, nviews;
    char relName[MAXNAME], * utDestroy_argv[3], ** utCreate_argv, ** views;
    relDesc fromRelDesc;
    attrDesc * fromRelAttrs ;
    viewDesc view;
    viewAttrDesc * viewAttrs;

    inline void cleanup() {
        /* eleytherwnoyme dynamikh mnhmh poy desmeysame */
        for ( i = 0; i < utDestroy_argc; i++ ) {
            if ( utDestroy_argv[i] != NULL ) {
                free( utDestroy_argv[i] );
            }
        }

        if ( utCreate_argv != NULL ) {
            for ( i = 0; i < utCreate_argc; i++ ) {
                if ( utCreate_argv[i] != NULL ) {
                    free( utCreate_argv[i] );
                }
            }

            free( utCreate_argv );
        }

        if ( views != NULL ) {
            for ( i = 0; i < nviews; i++ ) {
                if ( views[i] != NULL ) {
                    free( views[i] );
                }
            }

            free( views );
        }

        if ( fromRelAttrs != NULL ) {
            free( fromRelAttrs );
        }

        if ( viewAttrs != NULL ) {
            free( viewAttrs );
        }
    }

    /* initialization */
    strncpy( relName , argv[1], MAXNAME );
    relName[sizeof( relName ) - 1] = '\0';
    utDestroy_argc = 2;
    return_val = VIEW_OK;
    utCreate_argv = NULL;
    views = NULL;
    fromRelAttrs = NULL;
    viewAttrs = NULL;

    for ( i = 0; i <= utDestroy_argc; i++ ) {
        utDestroy_argv[i] = NULL;
    }

    /*
        Shmasiologikoi elegxoi
    */

    /* Apaitoume, ektos apo to onoma ths sxeshs:
       - toulaxiston ena gnwrisma kai thn timh toy
       - oloklhrwmena zeugh <onomatos gnwrismatos, timh>
    */
    if (( argc < 4 ) || ( argc % 2 != 0 ) ) {
        printf( "Error: Cannot insert. Not enough information\n" );
        return VIEW_GE;
    }


    /* Eksetazoume an yparxei h sxesh */
    if ( UT_relInfo( relName, &fromRelDesc, &fromRelAttrs ) < 0 ) {
        printf( "Error: Cannot insert. Could not retrieve information about relation '%s'\n", relName );
        return VIEW_GE;
    }

    /* H sxesh mas prepei na einai prwtarxikh, dhladh na yparxei san sxesh alla na mhn einai opsh */
    if ( getViewRelation( relName, &view, &viewAttrs ) == VIEW_OK ) {
        printf( "Error: Cannot insert. Relation '%s' is not a primary relation.\n", relName );
        cleanup();
        return VIEW_GE;
    }

    attrcnt = fromRelDesc.attrcnt;
    relwidth = fromRelDesc.relwidth;

    /* O arithmos twn zeugariwnn <attribute, value> pou dwthike isoutai me ton arithmo twn arguments meion ta 2
       prwta arguments: "insert", "relName". Elegxoume an mas dwthike o swstos arithmos apo tetoia zeygaria. */
    if ((( argc - 2 ) / 2 ) != attrcnt ) {
        printf( "Error: Cannot insert into relation '%s'. Expected %d <attribute, value> pairs.\n", relName, attrcnt );
        return_val = VIEW_GE;
        cleanup();
        return return_val;
    }

    /* Elegxoume an dwthikan idia onomata se 2 attributes ths relation, to opoio einai lathos */
    for ( i = 2; i < argc; i += 2 ) {
        for ( j = i + 2; j < argc; j += 2 ) {
            if ( strcmp( argv[i], argv[j] ) == 0 ) {
                printf( "Error: Cannot insert. More than one attributes named '%s'.\n", argv[i] );
                return_val = VIEW_GE;
                cleanup();
                return return_val;
            }
        }
    }

    /*
        Telos shmasiologikwn elegxwn
    */


    /* 1. Δημιούργησε ένα νεο πίνακα TEMP και βάλε σε αυτόν την εγγραφή strcpy */

    /*
       * Dhmioyrgoyme th sxesh TEMP
       * O arithmos twn arguments ths UT_create isoutai me: "create" + "TEMP" + 'attrcnt' zeugaria ths morfhs <attrname, attrtype>
    */

    utCreate_argc = 2 + fromRelDesc.attrcnt * 2;

    /* Dhmioyrgoyme ton pinaka ston opoio tha perasoume ta arguments gia thn UT_create, +1 gia to NULL */
    if (( utCreate_argv = malloc(( utCreate_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return VIEW_GE;
    }

    /* initialization tou pinaka */
    for ( i = 0; i <= utCreate_argc; i++ ) {
        utCreate_argv[i] = NULL;
    }

    /*
       to megalytero attrlength pou epitrepetai einai MAXSTRINGLENGTH xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws:
    */

    argLength = MAXSTRINGLENGTH;

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    /* desmeuoume mnhmh gia ta strings tou utCreate_argv */
    for ( i = 0; i < utCreate_argc; i++ ) {
        if (( utCreate_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    /* Twra, epitelous tha symplhrwsoume tis times tou pinaka utCreate_argv pou tha perastei meta sthn UT_create() function */
    strcpy( utCreate_argv[0], "create" );
    strcpy( utCreate_argv[1], "TEMP" );

    /* Symplhrwnoume tis times twn arguments gia ola ta attributes */
    for ( attr = 0, offset = 2; attr < fromRelDesc.attrcnt; attr++, offset += 2 ) {
        /* to zeugari apoteleitai apo to onoma tou attribute */
        strcpy( utCreate_argv[offset], fromRelAttrs[attr].attrname );

        /* kai ton typo tou */
        switch ( fromRelAttrs[attr].attrtype ) {
            case 'c':
                sprintf( utCreate_argv[offset + 1], "'c%d'", fromRelAttrs[attr].attrlength );
                break;
            case 'i':
                strcpy( utCreate_argv[offset + 1], "'i'" );
                break;
            case 'f':
                strcpy( utCreate_argv[offset + 1], "'f'" );
                break;
            default:
                printf( "Error: Cannot insert. Unknown type for attribute of relation '%s'\n", relName );
                cleanup();
                return VIEW_GE;
        }
    }

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn UT_create (gia th dhmiourgia tou TEMP)\n");
    printArgs( utCreate_argc, utCreate_argv );
    #endif

    if ( UT_create( utCreate_argc, utCreate_argv ) != OK ) {
        printf( "Error: Cannot insert. Could not create relation 'TEMP'\n" );
        cleanup();
        return VIEW_GE;
    }

    /* eisagoume sth sxesh TEMP thn eggrafh mas */
    strcpy( argv[1], "TEMP" );

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn DM_insert (gia th dhmiourgia tou TEMP)\n");
    printArgs( argc, argv );
    #endif

    if ( DM_insert( argc, argv ) != DME_OK ) {
        printf( "Error: Cannot insert. Could not insert into relation 'TEMP'\n" );
        cleanup();
        return VIEW_GE;
    }

    /*2. Για κάθε όψη V πάνω στη σχέση relName κάνε ενημέρωση ως εξής: */
    if (( views = getViewsOfRelation( relName, &nviews ) ) != NULL && nviews != 0 ) {
        #ifdef DEBUG
        printf("\n* Oi opseis panw sth sxesh '%s' :\n", relName);
        printArgs( nviews, views );
        #endif

        for ( i = 0; i < nviews; i++ ) {
            /* 2.1 recursive_update(V,relName,temp, 0); */
            if ( recursive_update( views[i], relName, "TEMP", 0, 0 ) != VIEW_OK ) {
                printf( "Error: Cannot insert. Error while using recursive update\n" );
            }
        }
    }



    /* 3. Κατέστρεψε τον πίνακα TEMP */

    /*
       * Dhmioyrgoyme ta arguments pou tha perastoun sthn UT_destroy.
       * "destroy" + "TEMP" + NULL
    */

    for ( i = 0; i < utDestroy_argc; i++ ) {
        if (( utDestroy_argv[i] = malloc( MAXNAME * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    strcpy( utDestroy_argv[0], "destroy" );
    strcpy( utDestroy_argv[1], "TEMP" );
    utDestroy_argv[2] = NULL;

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn UT_destroy (gia ton pinaka TEMP):\n");
    printArgs( utDestroy_argc, utDestroy_argv );
    #endif

    /* Katastrefoume thn 'TEMP' sxesh */
    if ( UT_destroy( utDestroy_argc, utDestroy_argv ) != OK ) {
        printf( "Error: Cannot insert. Could not destroy relation 'TEMP'\n" );
        cleanup();
        return VIEW_GE;
    }


    /* 4. Εισήγαγε την record στην σχέση relName */
    /* Enhmerw8hkan swsta oi opseis, opote einai kairos na ektelestei kai h eisagwgh */
    cleanup();
    strcpy( argv[1], relName );

    #ifdef DEBUG
    printf("\n* Arguments pou tha perastoun sthn DM_insert (gia thn '%s')\n", relName);
    printArgs( argc, argv );
    #endif

    return DM_insert( argc, argv );
}


/**** int recursive_update(char * viewName, char * relName, char * tempName, int flag, int tempIndex) ****

 Input:
        * char * viewName       To onoma ths opshs
        * char * relName        To onoma ths sxeshs
        * char * tempName       To onoma ths proswrinhs sxeshs pou xrhsimopoioyme
        * int flag              An kanoume insert H delete
        * int tempIndex         Xrhsimopoieitai sthn paragwgh monadikwn onomatwn gia tis 'temp' sxeseis

 Operation:
        * Anadromikh ananewsh olwn twn opsewn mias sxeshs symfwna me to argument flag kai th sxesh 'tempName'

 Return Value:
        * VIEW_OK, efoson epityxei
        * VIEW_GE, efoson apotyxei
*/
int recursive_update( char * viewName, char * relName, char * tempName, int flag, int tempIndex )
{
    int return_val, argc, i, argLength, index, attr, utDestroy_argc, dmFunc_argc, nviews;
    char ** argv, newTempName[MAXNAME], * utDestroy_argv[3], ** views, * dmFunc_argv[4];
    viewDesc viewInfo;
    viewAttrDesc * viewAttrs;

    inline void cleanup() {
        /* Apeleytherwsh ths dynamikhs mnhmhs poy desmeysame */
        if ( argv != NULL ) {
            for ( i = 0; i < argc; i++ ) {
                if ( argv[i] != NULL ) {
                    free( argv[i] );
                }
            }

            free( argv );
        }

        for ( i = 0; i < utDestroy_argc; i++ ) {
            if ( utDestroy_argv[i] != NULL ) {
                free( utDestroy_argv[i] );
            }
        }

        for ( i = 0; i < dmFunc_argc; i++ ) {
            if ( dmFunc_argv[i] != NULL ) {
                free( dmFunc_argv[i] );
            }
        }

        if ( views != NULL ) {
            for ( i = 0; i < nviews; i++ ) {
                if ( views[i] != NULL ) {
                    free( views[i] );
                }
            }

            free( views );
        }

        if ( viewAttrs != NULL ) {
            free( viewAttrs );
        }
    }


    /* initialization */
    return_val = VIEW_OK;
    argv = NULL;
    views = NULL;
    viewAttrs = NULL;
    utDestroy_argc = 2;
    dmFunc_argc = 3;

    for ( i = 0; i <= utDestroy_argc; i++ ) {
        utDestroy_argv[i] = NULL;
    }

    for ( i = 0; i <= dmFunc_argc; i++ ) {
        dmFunc_argv[i] = NULL;
    }

    #ifdef DEBUG
    printf("\n* Klhsh ths recursive update (nview='%s', rel='%s', temp='%s', flag=%d, tempIndex=%d\n",
            viewName, relName, tempName, flag, tempIndex );
    #endif

    /*
    1. Τρέξε την επερώτηση που ορίζει την V αντικαθιστώντας την relName με temp και
    εισάγωντας τα αποτελέσματα σε μια νεα προσωρινή σχέση Τ (θα πρέπει να βρείτε ένα
    temp όνομα που δεν χρησιμοποιείται από άλλη σχέση και να το δώσετε στην Τ)
       Για παράδειγμα, αν η V ορίζεται ως:
        SELECT (relname.attr1, relname2.attr2) where relname.attr2 > relname2.attr2
       Τότε θα πρέπει να μετατραπεί στην επερώτηση:
        SELECT ΙΝΤΟ Τ (temp.attr1, relname2.attr2) where temp.attr2 > relname2.attr2
    */

    /* To neo temp onoma orizetai ws syndiasmos tou TEMP mazi me enan aykswn arithmo ws ekshs: */
    sprintf( newTempName, "TEMP%d", tempIndex );

    /* Anaktame tis plhrofories ths opshs 'viewName' apo ta viewCat/viewAttrCat */
    if ( getViewRelation( viewName, &viewInfo, &viewAttrs ) != VIEW_OK ) {
        printf( "Error in recursive update: Could not retrieve information about view '%s'\n", viewName );
    }

    /* Prwta tha vroume ton arithmo twn arguments gia thn eperwthsh mas o opoios orizetai analoga ton typo ths opshs */

    switch ( viewInfo.type ) {
        case NONCOND_SELECT:
            /*
              O arithmos twn arguments gia th select xwris condition isoytai me:
              "select" + 'newTempName' + attrcnt + attrcnt zeugaria ths morfhs <relName, attrName>
            */
            argc = 3 + viewInfo.attrcnt * 2;
            break;
        case COND_SELECT:
            /*
              O arithmos twn arguments gia th select me condition isoytai me:
              "select" + 'newTempName' + attrcnt + attrcnt zeugaria ths morfhs <relName, attrName> + 'conditionRelName'
              + 'conditionAttrName' + 'op' + 'value'
            */
            argc = 3 + viewInfo.attrcnt * 2 + 4;
            break;
        case JOIN_SELECT:
            /*
              O arithmos twn arguments gia th join isoytai me:
              "join" + 'newTempName' + attrcnt + attrcnt zeugaria ths morfhs <relName, attrName> +
              + 'conditionRelName1' + 'conditionAttrName1' + 'op' + 'conditionRelName2' + 'conditionAttrName2'
            */
            argc = 3 + viewInfo.attrcnt * 2 + 5;
            break;
        default:
            printf( "Error in recursive update: Unknown view type %d\n", viewInfo.type );
            return VIEW_GE;
    }

    /* Desmeuoume xwro sth mnhmh ston opoio tha kratithoun ta arguments ths eperwthshs, +1 gia to NULL */
    if (( argv = malloc(( argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        return VIEW_GE;
    }

    /* initialization tou pinaka */
    for ( i = 0; i <= argc; i++ ) {
        argv[i] = NULL;
    }

    /*
       to megalytero attrlength pou epitrepetai einai MAXSTRINGLENGTH xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws:
    */

    argLength = MAXSTRINGLENGTH;

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    /* Desmeuoume mnhmh gia ta strings tou pinaka */
    for ( i = 0; i < argc; i++ ) {
        if (( argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    /* Twra tha dhmioyrghsoyme thn eperwthsh analoga ton typo toy view */

    switch ( viewInfo.type ) {
        case NONCOND_SELECT:
            /*
              "select" + 'newTempName' + attrcnt + attrcnt zeugaria ths morfhs <relName, attrName>
            */
            strcpy( argv[0], "select" );
            strcpy( argv[1], newTempName );
            sprintf( argv[2], "%d", viewInfo.attrcnt );

            /* gia ta provallomena attributes */
            for ( index = 3, attr = 0; attr < viewInfo.attrcnt; index += 2, attr++ ) {
                strcpy( argv[index], viewAttrs[attr].relname );
                strcpy( argv[index + 1], viewAttrs[attr].relattrname );
            }

            /* opou synantame to 'relName', prepei na to antikatastisoume me to tempName */
            for ( i = 3; i < argc; i++ ) {
                if ( strcmp( argv[i], relName ) == 0 ) {
                    strcpy( argv[i], tempName );
                }
            }

            #ifdef DEBUG
            printf("\n* Eimaste mesa sthn recursive update (no condition), query:\n" );
            printArgs( argc, argv );
            #endif

            #ifdef DEBUG
            printf("\n* Etoimoi na pame sthn dm select\n" );
            #endif

            /* ekteloyme thn eperwthsh */
            if ( DM_select( argc, argv ) != DME_OK ) {
                printf( "Error in recursive update: Error during select()\n" );
                cleanup();
                return VIEW_GE;
            }

            #ifdef DEBUG
            printf("\n* Molis bghkame apo thn dm select\n");
            #endif

            break;
        case COND_SELECT:
            /*
              "select" + 'newTempName' + attrcnt + attrcnt zeugaria ths morfhs <relName, attrName> + 'conditionRelName'
              + 'conditionAttrName' + 'op' + 'value'
            */
            strcpy( argv[0], "select" );
            strcpy( argv[1], newTempName );
            sprintf( argv[2], "%d", viewInfo.attrcnt );

            /* gia ta provallomena attributes */
            for ( index = 3, attr = 0; attr < viewInfo.attrcnt; index += 2, attr++ ) {
                strcpy( argv[index], viewAttrs[attr].relname );
                strcpy( argv[index + 1], viewAttrs[attr].relattrname );
            }

            /* mas menei to kommati + 'conditionRelName' + 'conditionAttrName' + 'op' + 'value' */
            strcpy( argv[index++], viewInfo.relname1 );
            strcpy( argv[index++], viewInfo.attrname1 );

            /* Analoga twn operator prepei n antigrapsoume kai to antistoixo string */
            switch ( viewInfo.op ) {
                case( EQUAL ):
                    strcpy( argv[index++], "=" );
                    break;
                case( LESS_THAN ):
                    strcpy( argv[index++], "<" );
                    break;
                case( GREATER_THAN ):
                    strcpy( argv[index++], ">" );
                    break;
                case( LESS_THAN_OR_EQUAL ):
                    strcpy( argv[index++], "<=" );
                    break;
                case( GREATER_THAN_OR_EQUAL ):
                    strcpy( argv[index++], ">=" );
                    break;
                case( NOT_EQUAL ):
                    strcpy( argv[index++], "!=" );
                    break;
                default:
                    printf( "Error in recursive update: Unknown operator for view '%s'\n", viewName );
                    cleanup();
                    return VIEW_GE;
            }

            /* opou synantame to 'relName', prepei na to antikatastisoume me to tempName */
            for ( i = 3; i < index; i++ ) {
                if ( strcmp( argv[i], relName ) == 0 ) {
                    strcpy( argv[i], tempName );
                }
            }

            strcpy( argv[index++], viewInfo.value );
            #ifdef DEBUG
            printf("\n* Eimaste mesa sthn recursive update (with condition), query:\n");
            printArgs( argc, argv );
            #endif

            #ifdef DEBUG
            printf("\n* Etoimoi na pame sthn dm select\n");
            #endif

            /* ekteloyme thn eperwthsh */
            if ( DM_select( argc, argv ) != DME_OK ) {
                printf( "Error in recursive update: Error during select()\n" );
                cleanup();
                return VIEW_GE;
            }

            #ifdef DEBUG
            printf("\n* Molis bghkame apo thn dm select\n" );
            #endif

            break;

        case JOIN_SELECT:
            /*
              "join" + 'newTempName' + attrcnt + attrcnt zeugaria ths morfhs <relName, attrName> +
              + 'conditionRelName1' + 'conditionAttrName1' + 'op' + 'conditionRelName2' + 'conditionAttrName2'
            */
            strcpy( argv[0], "join" );
            strcpy( argv[1], newTempName );
            sprintf( argv[2], "%d", viewInfo.attrcnt );

            /* gia ta provallomena attributes */
            for ( index = 3, attr = 0; attr < viewInfo.attrcnt; index += 2, attr++ ) {
                strcpy( argv[index], viewAttrs[attr].relname );
                strcpy( argv[index + 1], viewAttrs[attr].relattrname );
            }

            /* 'conditionRelName1' + 'conditionAttrName1' + 'op' + 'conditionRelName2' + 'conditionAttrName2' */
            strcpy( argv[index++], viewInfo.relname1 );
            strcpy( argv[index++], viewInfo.attrname1 );

            /* Analoga twn operator prepei n antigrapsoume kai to antistoixo string */
            switch ( viewInfo.op ) {
                case( EQUAL ):
                    strcpy( argv[index++], "=" );
                    break;
                case( LESS_THAN ):
                    strcpy( argv[index++], "<" );
                    break;
                case( GREATER_THAN ):
                    strcpy( argv[index++], ">" );
                    break;
                case( LESS_THAN_OR_EQUAL ):
                    strcpy( argv[index++], "<=" );
                    break;
                case( GREATER_THAN_OR_EQUAL ):
                    strcpy( argv[index++], ">=" );
                    break;
                case( NOT_EQUAL ):
                    strcpy( argv[index++], "!=" );
                    break;
                default:
                    printf( "Error in recursive update: Unknown operator for view '%s'\n", viewName );
                    cleanup();
                    return VIEW_GE;
            }

            strcpy( argv[index++], viewInfo.relname2 );
            strcpy( argv[index++], viewInfo.attrname2 );

            /* opou synantame to 'relName', prepei na to antikatastisoume me to tempName */
            for ( i = 3; i < argc; i++ ) {
                if ( strcmp( argv[i], relName ) == 0 ) {
                    strcpy( argv[i], tempName );
                }
            }

            #ifdef DEBUG
            printf("\n* Eimaste sthn recursive update (join), query:\n");
            printArgs( argc, argv );
            #endif

            /* ekteloyme thn eperwthsh */
            if ( DM_join( argc, argv ) != DME_OK ) {
                printf( "Error in recursive update: Error during join()\n" );
                cleanup();
                return VIEW_GE;
            }

            break;
    }

    #ifdef DEBUG
    printf("\n* 2o kommati ths recursive update\n");
    #endif


    /* 2. Για κάθε όψη V' πάνω στην V κάνε ενημέρωση ως εξής: */
    if (( views = getViewsOfRelation( viewName, &nviews ) ) != NULL && nviews != 0 ) {
        #ifdef DEBUG
        printf("\n* Oi opseis panw sthn opsh '%s'\n", viewName);
        printArgs( nviews, views );
        #endif

        for ( i = 0; i < nviews; i++ ) {
            /* 2.1 recursive_update(V',V,T, flag); */
            if ( recursive_update( views[i], viewName, newTempName, flag, ( tempIndex + 1 ) ) != VIEW_OK ) {
                printf( "Error in recursive update: Error in recursion (2.1)\n" );
            }
        }
    }

    #ifdef DEBUG
    printf("\n* Pame sto 3o/4o kommati ths recursive update\n");
    #endif

    /* 3. Αν το flag είναι 0, άρα έχουμε εισαγωγή */
    if ( flag == 0 ) {
        /* 3.1    Εισαγωγή στην V όλων των εγγραφών της T; */

        /* ta arguments gia thn DM_add einai: "add" + 'relName1' + 'relName2' */

        for ( i = 0; i < dmFunc_argc; i++ ) {
            if (( dmFunc_argv[i] = malloc( MAXNAME * sizeof( char ) ) ) == NULL ) {
                printf( MEM_ERROR );
                cleanup();
                return VIEW_GE;
            }
        }

        strcpy( dmFunc_argv[0], "add" );
        strcpy( dmFunc_argv[1], viewName );
        strcpy( dmFunc_argv[2], newTempName );
        dmFunc_argv[3] = NULL;

        if ( DM_add( dmFunc_argc, dmFunc_argv ) != DME_OK ) {
            printf( "Error in recursive update: Couldn't add to relation '%s'\n", viewName );
            cleanup();
            return VIEW_GE;
        }
    }
    else if ( flag == 1 ) {
        /*
        4. Αλλιώς
            4.1    Διαγραφή από την V όλων των εγγραφών της T;
        */

        /* ta arguments gia thn DM_subtract einai: "subtract" + 'relName1' + 'relName2' */

        for ( i = 0; i < dmFunc_argc; i++ ) {
            if (( dmFunc_argv[i] = malloc( MAXNAME * sizeof( char ) ) ) == NULL ) {
                printf( MEM_ERROR );
                cleanup();
                return VIEW_GE;
            }
        }

        strcpy( dmFunc_argv[0], "subtract" );
        strcpy( dmFunc_argv[1], viewName );
        strcpy( dmFunc_argv[2], newTempName );
        dmFunc_argv[3] = NULL;

        if ( DM_subtract( dmFunc_argc, dmFunc_argv ) != DME_OK ) {
            printf( "Error in recursive update: Couldn't subtract from relation '%s'\n", viewName );
            cleanup();
            return VIEW_GE;
        }
    }
    else {
        printf( "Error in recursive update: Invalid flag '%d' specified.\n", flag );
    }

    #ifdef DEBUG
    printf("\n* Pame sto 5o kommati ths recursive update\n");
    #endif

    /* 5. Κατέστρεψε τον πίνακα Τ. */

    /* ta arguments gia thn UT_destroy einai: "destroy" + 'relName' */

    for ( i = 0; i < utDestroy_argc; i++ ) {
        if (( utDestroy_argv[i] = malloc( MAXNAME * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    strcpy( utDestroy_argv[0], "destroy" );
    strcpy( utDestroy_argv[1], newTempName );
    utDestroy_argv[2] = NULL;

    if ( UT_destroy( utDestroy_argc, utDestroy_argv ) != OK ) {
        printf( "Error in recursive update: Could not destroy relation '%s'\n", newTempName );
        cleanup();
        return VIEW_GE;
    }

    cleanup();
    return VIEW_OK;
}


/*

int VW_destroy(int argc, char* argv[])

usage:
    DESTROY relName
    DESTROY MVIEW viewName

parser:
    argv[0]="destroy" H "destroyMView"
    argv[1]=όνομα-σχέσης
    argv[2]=NULL

operation:
       Diagrafetai mia sxesh h opsh (analoga to input mas), afou prwta ginoyn oi aparaithtoi shmasiologikoi elegxoi

return:
       * VIEW_OK, se periptwsh pou epityxei
       * VIEW_GE, se opoiadhpote allh periptwsh
*/

int VW_destroy( int argc, char *argv[] )
{
    int i, nviews, dmDelete_argc, UT_ret;
    char * relName, ** views, * dmDelete_argv[6];
    relDesc fromRelDesc;
    attrDesc * fromRelAttrs;
    viewDesc view;
    viewAttrDesc * viewAttrs;

    inline void cleanup() {
        /* Apeleytherwsh dynamikhs mnhmhs poy desmeysame */
        if ( fromRelAttrs != NULL ) {
            free( fromRelAttrs );
        }

        if ( viewAttrs != NULL ) {
            free( viewAttrs );
        }

        if ( views != NULL ) {
            for ( i = 0; i < nviews; i++ ) {
                if ( views[i] != NULL ) {
                    free( views[i] );
                }
            }

            free( views[i] );
        }

        for ( i = 0; i < dmDelete_argc; i++ ) {
            if ( dmDelete_argv[i] != NULL ) {
                free( dmDelete_argv[i] );
            }
        }
    }

    /* initialization */
    fromRelAttrs = NULL;
    viewAttrs = NULL;
    relName = argv[1];
    views = NULL;
    dmDelete_argc = 5;

    /* initialization stis times toy pinaka */
    for ( i = 0; i <= dmDelete_argc; i++ ) {
        dmDelete_argv[i] = NULL;
    }

    /*
      Shmasiologikoi elegxoi
    */

    /* Apaitoume 2 akrivws arguments */
    if ( argc != 2 ) {
        printf( "Error: Cannot destroy. Expected 2 arguments.\n" );
        return VIEW_GE;
    }

    /* An o xrhsths edwse destroy ws 1o argument */
    if ( strcmp( argv[0], "destroy" ) == 0 ) {
        /* Eksetazoume an yparxei h sxesh kai einai prwtarxikh */
        if ( UT_relInfo( relName, &fromRelDesc, &fromRelAttrs ) < 0 ) {
            printf( "Error: Cannot destroy. Could not retrieve information about relation '%s'\n", relName );
            cleanup();
            return VIEW_GE;
        }

        /* H sxesh mas prepei na einai prwtarxikh, dhladh na yparxei san sxesh alla na mhn einai opsh */
        if ( getViewRelation( relName, &view, &viewAttrs ) == VIEW_OK ) {
            printf( "Error: Cannot destroy. Relation '%s' is not a primary relation.\n", relName );
            cleanup();
            return VIEW_GE;
        }

        /* Telos, elegxoume oti h sxesh mas de xrhsimopoieitai ston orismo allwn opsewn */
        if (( views = getViewsOfRelation( relName, &nviews ) ) == NULL || nviews != 0 ) {
            printf( "Error: Cannot destroy. '%s' is being used by other views\n", relName );
            cleanup();
            return VIEW_GE;
        }

        /* proxwrame sth diagrafh ths sxeshs... */
    } /* if */

    /* An o xrhsths edwse destroy mview ws 1o argument */
    else if ( strcmp( argv[0], "destroyMView" ) == 0 ) {
        /* Eksetazoume an yparxei h sxesh kai einai opsh */
        if ( UT_relInfo( relName, &fromRelDesc, &fromRelAttrs ) < 0 ) {
            printf( "Error: Cannot destroy MView. Could not retrieve information about relation '%s'\n", relName );
            cleanup();
            return VIEW_GE;
        }

        /* H sxesh mas prepei na einai prwtarxikh, dhladh na yparxei san sxesh alla na mhn einai opsh */
        if ( getViewRelation( relName, &view, &viewAttrs ) != VIEW_OK ) {
            printf( "Error: Cannot destroy MView. Relation '%s' is not a view.\n", relName );
            cleanup();
            return VIEW_GE;
        }

        /* Eelegxoume oti h sxesh mas de xrhsimopoieitai ston orismo allwn opsewn */
        if (( views = getViewsOfRelation( relName, &nviews ) ) == NULL || nviews != 0 ) {
            printf( "Error: Cannot destroy MView. '%s' is being used by other views\n", relName );
            cleanup();
            return VIEW_GE;
        }

        /* Telos, prepei na diagrapsoume ta stoixeia ths sxeshs mas apo to viewCat & viewAttrCat */

        /* desmeyoyme xwro gia ta argument pou tha perastoun sthn delete */
        for ( i = 0; i < dmDelete_argc; i++ ) {
            if (( dmDelete_argv[i] = malloc( MAXNAME * sizeof( char ) ) ) == NULL ) {
                printf( MEM_ERROR );
                cleanup();
                return VIEW_GE;
            }
        }

        /* symplhrwnoume tis antistoixes times sta arguments pou tha perastoun sthn delete */
        strcpy( dmDelete_argv[0], "delete" );
        strcpy( dmDelete_argv[1], "viewCat" );
        strcpy( dmDelete_argv[2], "viewname" );
        strcpy( dmDelete_argv[3], "=" );
        strcpy( dmDelete_argv[4], relName );
        dmDelete_argv[5] = NULL;

        /* Ginetai h ananewsh tou viewCat arxeiou */
        if ( DM_delete( dmDelete_argc, dmDelete_argv ) != DME_OK ) {
            printf( "Error: Cannot destroy MView. Error while updating viewCat\n" );
            cleanup();
            return VIEW_GE;
        }

        /* Paromoiws gia to viewAttrCat arxeio... */
        strcpy( dmDelete_argv[0], "delete" );
        strcpy( dmDelete_argv[1], "viewAttrCat" );
        strcpy( dmDelete_argv[2], "viewname" );
        strcpy( dmDelete_argv[3], "=" );
        strcpy( dmDelete_argv[4], relName );
        dmDelete_argv[5] = NULL;

        /* Ginetai h ananewsh tou viewCat arxeiou */
        if ( DM_delete( dmDelete_argc, dmDelete_argv ) != DME_OK ) {
            printf( "Error: Cannot destroy MView. Error while updating viewAttrCat\n" );
            cleanup();
            return VIEW_GE;
        }

        strcpy( argv[0], "destroy" );
        /* proxwrame sth diagrafh ths sxeshs... */
    } /* else if */

    /* Se opoiadhpote allh periptwsh... */
    else {
        printf( "Error: Cannot destroy. Unknown first argument '%s'\n", argv[0] );
        return VIEW_GE;
    }

    /*
      Telos Shmasiologikwn elegxwn
    */

    /* twra tha ginei h diagrafh ths sxeshs apo to UT epipedo */

    if (( UT_ret = UT_destroy( argc, argv ) ) < 0 ) {
        printf( "Error: Can't delete (error code %d)\n", UT_ret );
        return VIEW_GE;
    }

    return VIEW_OK;
}

/**** int updateViewCat( viewDesc * viewInfo ) ****

 Input:
        * viewDesc * viewInfo     H eggrafh pros apo8hkeush

 Operation:
        * Pros8etei thn eggrafh "viewInfo" sto arxeio "viewCat"

 Return Value:
        * VIEW_OK, efoson epityxei
        * VIEW_GE, efoson apotyxei
*/

int updateViewCat( viewDesc * viewInfo , char valueType )
{
    int i, attrcnt, dmInsert_argc, argLength;
    char ** dmInsert_argv;

    inline void cleanup() {
        if ( dmInsert_argv != NULL ) {
            for ( i = 0; i <  dmInsert_argc; i++ ) {
                if ( dmInsert_argv[i] != NULL ) {
                    free( dmInsert_argv[i] );
                }
            }

            free( dmInsert_argv );
        }
    }

    if ( viewInfo == NULL ) {
        return VIEW_GE;
    }

    /* initialization */
    dmInsert_argv = NULL;

    /* O arithmos twn attributes tou viewCat isoutai me 9 */
    attrcnt = 9;

    /*
        * Dhmioyrgoyme th lista me ta arguments pou prepei na perastoun sthn DM_insert()
        * O arithmos twn arguments mas isoutai me =
            "insert" + "viewCat" + 'attrcnt' zeugaria (attribute, value) ths sxeshs viewCat
    */

    dmInsert_argc = attrcnt * 2 + 2;

    /* dhmioyrgoyme th lista apo ta arguments, +1 gia to NULL argument sto telos */
    if (( dmInsert_argv = malloc(( dmInsert_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return VIEW_GE;
    }

    /* Arxikopoioume ton pinaka mas */
    for ( i = 0; i < dmInsert_argc + 1; i++ ) {
        dmInsert_argv[i] = NULL;
    }

    /*
       to megalytero attrlength pou epitrepetai einai MAXSTRINGLENGTH xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws:
    */

    argLength = MAXSTRINGLENGTH;

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    for ( i = 0; i < dmInsert_argc; i++ ) {
        if (( dmInsert_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    strcpy( dmInsert_argv[0], "insert" );
    strcpy( dmInsert_argv[1], "viewCat" );
    strcpy( dmInsert_argv[2], "viewname" );
    strcpy( dmInsert_argv[3], viewInfo->viewname );
    strcpy( dmInsert_argv[4], "type" );
    sprintf( dmInsert_argv[5], "%d", viewInfo->type );
    strcpy( dmInsert_argv[6], "relname1" );
    strcpy( dmInsert_argv[7], viewInfo->relname1 );
    strcpy( dmInsert_argv[8], "attrname1" );
    strcpy( dmInsert_argv[9], viewInfo->attrname1 );
    strcpy( dmInsert_argv[10], "op" );
    sprintf( dmInsert_argv[11], "%d", viewInfo->op );
    strcpy( dmInsert_argv[12], "relname2" );
    strcpy( dmInsert_argv[13], viewInfo->relname2 );
    strcpy( dmInsert_argv[14], "attrname2" );
    strcpy( dmInsert_argv[15], viewInfo->attrname2 );
    strcpy( dmInsert_argv[16], "value" );

    if ( viewInfo->type == COND_SELECT ) {
        switch ( valueType ) {
            case 'c':
                sprintf( dmInsert_argv[17], "\"%s\"", viewInfo->value );
                break;
            case 'f':
                sprintf( dmInsert_argv[17], "%f", atof( viewInfo->value ) );
                break;
            case 'i':
                sprintf( dmInsert_argv[17], "%d", atoi( viewInfo->value ) );
                break;
        }
    }
    else {
        strcpy( dmInsert_argv[17], "summer glau" );
    }

    strcpy( dmInsert_argv[18], "attrcnt" );
    sprintf( dmInsert_argv[19], "%d", viewInfo->attrcnt );

    if ( DM_insert( dmInsert_argc, dmInsert_argv ) != DME_OK ) {
        printf( "Error in updateViewCat: Could not update viewCat relation\n" );
        cleanup();
        return VIEW_GE;
    }

    cleanup();
    return VIEW_OK;
}

/**** int updateViewAttrCat( viewAttrDesc * viewAttrInfo ) ****

 Input:
        * viewAttrDesc * viewAttrInfo     H eggrafh pros apo8hkeush

 Operation:
        * Pros8etei thn eggrafh "viewAttrInfo" sto arxeio "viewAttrCat"

 Return Value:
        * VIEW_OK, efoson epityxei
        * VIEW_GE, efoson apotyxei
*/

int updateViewAttrCat( viewAttrDesc * viewAttrInfo )
{
    int i, attrcnt, dmInsert_argc, argLength;
    char ** dmInsert_argv;

    inline void cleanup() {
        if ( dmInsert_argv != NULL ) {
            for ( i = 0; i <  dmInsert_argc; i++ ) {
                if ( dmInsert_argv[i] != NULL ) {
                    free( dmInsert_argv[i] );
                }
            }

            free( dmInsert_argv );
        }
    }

    if ( viewAttrInfo == NULL ) {
        return VIEW_GE;
    }

    /* initialization */
    dmInsert_argv = NULL;

    /* O arithmos twn attributes tou viewAttrCat isoutai me 4 */
    attrcnt = 4;

    /*
        * Dhmioyrgoyme th lista me ta arguments pou prepei na perastoun sthn DM_insert()
        * O arithmos twn arguments mas isoutai me =
            "insert" + "viewAttrCat" + 'attrcnt' zeugaria (attribute, value) ths sxeshs viewAttrCat
    */

    dmInsert_argc = attrcnt * 2 + 2;

    /* dhmioyrgoyme th lista apo ta arguments, +1 gia to NULL argument sto telos */
    if (( dmInsert_argv = malloc(( dmInsert_argc + 1 ) * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return VIEW_GE;
    }

    /* Arxikopoioume ton pinaka mas */
    for ( i = 0; i < dmInsert_argc + 1; i++ ) {
        dmInsert_argv[i] = NULL;
    }


    /*
       to megalytero attrlength pou epitrepetai einai MAXSTRINGLENGTH xaraktires. to megalytero attrname pou epitrepetai einai
       MAXNAME. Ara me asfaleia thetoyme to megethos enos argument ws:
    */

    argLength = MAXSTRINGLENGTH;

    if ( argLength < MAXNAME ) {
        argLength = MAXNAME;
    }

    for ( i = 0; i < dmInsert_argc; i++ ) {
        if (( dmInsert_argv[i] = malloc( argLength * sizeof( char ) ) ) == NULL ) {
            printf( MEM_ERROR );
            cleanup();
            return VIEW_GE;
        }
    }

    strcpy( dmInsert_argv[0], "insert" );
    strcpy( dmInsert_argv[1], "viewAttrCat" );
    strcpy( dmInsert_argv[2], "viewname" );
    strcpy( dmInsert_argv[3], viewAttrInfo->viewname );
    strcpy( dmInsert_argv[4], "viewattrname" );
    strcpy( dmInsert_argv[5], viewAttrInfo->viewattrname );
    strcpy( dmInsert_argv[6], "relname" );
    strcpy( dmInsert_argv[7], viewAttrInfo->relname );
    strcpy( dmInsert_argv[8], "relattrname" );
    strcpy( dmInsert_argv[9], viewAttrInfo->relattrname );

    /* Ananewnoume to viewAttrCat arxeio gia to attribute ths opshs */
    if ( DM_insert( dmInsert_argc, dmInsert_argv ) != DME_OK ) {
        printf( "Error in updateViewAttrCat: Could not update viewCat relation\n" );
        cleanup();
        return VIEW_GE;
    }

    cleanup();
    return VIEW_OK;
}

/**** int getViewRelation ( char * viewRelName, viewDesc * view ) ****

 Input:
        * char * viewRelName                    To onoma ths sxeshs thn opoia anazhtoume
        * viewDesc * view                       Edw topothetountai oi plhrofories gia thn opsh
        * viewAttrDesc ** viewAttrs             Edw topothetountai oi plhrofories gia ta attributes ths opshs

 Operation:
        * Symplhrwnei tis plhrofories ths opshs 'viewRelName' sto 'view'

 Return Value:
        * Kapoion arnhtiko arithmo efoson epityxei
        * VIEW_OK, efoson apotyxei
*/

int getViewRelation( char * viewRelName, viewDesc * view, viewAttrDesc ** viewAttrs )
{
    int scanDesc, recId, attr;
    char * record;
    relDesc fromRelDesc;
    attrDesc * fromRelAttrs;

    /* apeleytherwsh dynamikhs mnhmhs kai kleisimo scans */
    inline void cleanup() {
        if ( record != NULL ) {
            free( record );
        }

        if ( scanDesc >= 0 ) {
            if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in viewCat" );
            }
        }

        if ( fromRelAttrs != NULL ) {
            free( fromRelAttrs );
        }
    }

    /* initialization */
    record = NULL;
    scanDesc = -1;
    ( *viewAttrs ) = NULL;
    fromRelAttrs = NULL;

    /* Kanoume extract tis plhrofories tou viewCat apo to relCat & attrCat gia na mporesoume n arxisoume mia scan s ayto */
    if ( UT_relInfo( "viewCat", &fromRelDesc, &fromRelAttrs ) < 0 ) {
        printf( "Unable to retrieve information about viewCat\n" );
        return DME_FILE_NOT_EXISTS;
    }

    /* Desmeyoyme mnhmh gia to 'record' sto opoio tha apothikeuontai eggrafes apo to viewCat */
    if (( record = malloc( fromRelDesc.relwidth * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return -1;
    }


    /* Ksekiname ena scan sto viewCat gia na kanoyme retrieve tis plhrofories gia thn opsh mas me viewname 'viewRelName' */
    scanDesc = HF_OpenFileScan( viewcatFileDesc, fromRelDesc.relwidth, 'c', fromRelAttrs[0].attrlength,
                                fromRelAttrs[0].offset, EQUAL, viewRelName );

    if ( scanDesc < 0 ) {
        HF_PrintError( "Unable to start scan in viewCat" );
        cleanup();
        return -1;
    }

    /* psaxnoume na vroume th monadikh eggrafh me viewname = viewRelName */
    if ( HF_FindNextRec( scanDesc, record ) < 0 ) {
        /* An de th vroume, epistrefoume -1 */
        cleanup();
        return -1;
    }

    /* Alliws, apothikeuoume tis plhrofories ths opshs sto 'view' kai epistrefoume 0 */

    memcpy( view->viewname, record + fromRelAttrs[0].offset,  fromRelAttrs[0].attrlength );
    memcpy( &view->type, record + fromRelAttrs[1].offset,  fromRelAttrs[1].attrlength );
    memcpy( view->relname1, record + fromRelAttrs[2].offset,  fromRelAttrs[2].attrlength );
    memcpy( view->attrname1, record + fromRelAttrs[3].offset,  fromRelAttrs[3].attrlength );
    memcpy( &view->op, record + fromRelAttrs[4].offset,  fromRelAttrs[4].attrlength );
    memcpy( view->relname2, record + fromRelAttrs[5].offset,  fromRelAttrs[5].attrlength );
    memcpy( view->attrname2, record + fromRelAttrs[6].offset,  fromRelAttrs[6].attrlength );
    memcpy( view->value, record + fromRelAttrs[7].offset,  fromRelAttrs[7].attrlength );
    memcpy( &view->attrcnt, record + fromRelAttrs[8].offset,  fromRelAttrs[8].attrlength );

    /* Desmeuoume xwro ston opoio tha kratithoun ta attributes */
    if ((( *viewAttrs ) = malloc( view->attrcnt * sizeof( viewAttrDesc ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return -1;
    }

    /* apeleythwrnoume dynamikh mnhmh kai kleinoume to scan gia na anoiksoume ena kainoyrgio */
    cleanup();

    /* initialization */
    record = NULL;
    scanDesc = -1;
    fromRelAttrs = NULL;

    /* Kanoume extract tis plhrofories tou viewAttrCat apo to relCat & attrCat gia na mporesoume n arxisoume mia scan s ayto */
    if ( UT_relInfo( "viewAttrCat", &fromRelDesc, &fromRelAttrs ) < 0 ) {
        printf( "Unable to retrieve information about viewAttrCat\n" );
        cleanup();
        return -1;
    }

    /* Desmeyoyme mnhmh gia to 'record' sto opoio tha apothikeuontai eggrafes apo to viewAttrCat */
    if (( record = malloc( fromRelDesc.relwidth * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return -1;
    }

    /* Ksekiname ena scan sto viewAttrCat gia na kanoyme retrieve ola ta attributes ths opshs mas */
    scanDesc = HF_OpenFileScan( viewattrcatFileDesc, fromRelDesc.relwidth, 'c', fromRelAttrs[0].attrlength,
                                fromRelAttrs[0].offset, EQUAL, viewRelName );

    if ( scanDesc < 0 ) {
        HF_PrintError( "Unable to start scan in viewAttrCat" );
        cleanup();
        free( *viewAttrs );
        return -1;
    }

    attr = 0;

    while (( recId = HF_FindNextRec( scanDesc, record ) ) >= 0 ) {
        /* gia kathe attribute pou vriskoume, to apothikeuoume */
        memcpy(( *viewAttrs )[attr].viewname, record + fromRelAttrs[0].offset, fromRelAttrs[0].attrlength );
        memcpy(( *viewAttrs )[attr].viewattrname, record + fromRelAttrs[1].offset, fromRelAttrs[1].attrlength );
        memcpy(( *viewAttrs )[attr].relname, record + fromRelAttrs[2].offset, fromRelAttrs[2].attrlength );
        memcpy(( *viewAttrs )[attr].relattrname, record + fromRelAttrs[3].offset, fromRelAttrs[3].attrlength );
        attr++;
    }

    if ( recId != HFE_EOF ) {
        printf( "Error while scaning viewAttrCat for records\n" );
        cleanup();
        free( *viewAttrs );
        return -1;
    }

    if ( attr == 0 ) {
        printf( "Error: Did not find a single attribute for view '%s'\n", viewRelName );
        cleanup();
        free( *viewAttrs );
        return -1;
    }

    cleanup();
    return VIEW_OK;
}


/**** char ** getViewsOfRelation(char * relName) ****

 Input:
        * char * viewAttrName   To onoma ths sxeshs ths opoias ta views anazhtoyme

 Operation:
        * Epistrefei enan pinaka apo strings pou periexoun ta onomata twn views sth sxesh relName

 Return Value:
        * pointer ston pinaka me ta strings twn onomatwn twn views panw sth sxesh relName
        * NULL, efoson apotyxei
*/

char ** getViewsOfRelation( char * relName, int * nviews )
{
    int i, scanDesc, recId;
    char ** views, * record;
    relDesc fromRelDesc;
    attrDesc * fromRelAttrs;

    /* apeleytherwsh dynamikhs mnhmhs kai kleisimo scans */
    inline void cleanup() {
        if ( record != NULL ) {
            free( record );
        }

        if ( scanDesc >= 0 ) {
            if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
                HF_PrintError( "Could not close scan in viewCat" );
            }
        }

        if ( views != NULL ) {
            for ( i = 0; i < ( *nviews ); i++ ) {
                if ( views[i] != NULL ) {
                    free( views[i] );
                }
            }

            free( views );
        }

        if ( fromRelAttrs != NULL ) {
            free( fromRelAttrs );
        }
    }

    /* initialization */
    views = NULL;
    record = NULL;
    scanDesc = -1;
    *nviews = 0;
    fromRelAttrs = NULL;

    /* Kanoume extract tis plhrofories tou viewCat apo to relCat & attrCat gia na mporesoume n arxisoume mia scan s ayto */
    if ( UT_relInfo( "viewCat", &fromRelDesc, &fromRelAttrs ) < 0 ) {
        printf( "Error: Could not retrieve information about relation viewCat\n" );
        return NULL;
    }

    /* Desmeyoyme mnhmh gia to 'record' sto opoio tha apothikeuontai eggrafes apo to viewCat */
    if (( record = malloc( fromRelDesc.relwidth * sizeof( char ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return NULL;
    }


    /*
       * Ksekiname ena scan sto viewCat gia na kanoyme retrieve tis plhrofories gia tis opseis panw sth relation 'relName'
       * Mias kai to onoma ths sxeshs mporei na eite eite sto 'relname1' eite sto 'relname2', to scan mas tha exei
         ws value NULL gia na mas epistrafoun oles oi opseis kai na koitame EMEIS poies xreiazomaste, anti na arxisoume
         2 ksexwrista scans: mia fora gia to 'relname1' kai mia gia to 'relname2'
    */
    scanDesc = HF_OpenFileScan( viewcatFileDesc, fromRelDesc.relwidth, 'c', fromRelAttrs[0].attrlength,
                                fromRelAttrs[0].offset, EQUAL, NULL );

    if ( scanDesc < 0 ) {
        HF_PrintError( "Unable to start scan in viewCat" );
        cleanup();
        return NULL;
    }

    /* Epeidh de ginetai na kseroume poses opseis tha exoume panw sth sxesh mas, tha kanoume malloc arxika gia 10 strings
       kai an xreiastei, tha kanoume kathe fora realloc gia akoma 10. H allh epilogh tha htan na kanoume parapanw scans
       gia na metrisoume prwta tis opseis kai meta na tis apothikeusoume */

    if (( views = malloc( 10 * sizeof( char * ) ) ) == NULL ) {
        printf( MEM_ERROR );
        cleanup();
        return NULL;
    }

    /* initialization tou pinaka */
    for ( i = 0; i < 10; i++ ) {
        views[i] = NULL;
    }

    /* kanoyme retrieve oles tis views */
    while (( recId = HF_FindNextRec( scanDesc, record ) ) >= 0 ) {
        /* Elegxoume an eite to 'relname1', eite to 'relname2', ths twrinhs sxesh einai idia me th 'relName' sxesh mas */
        if ( !strcmp( relName, record + fromRelAttrs[2].offset ) || !strcmp( relName, record +  fromRelAttrs[5].offset ) ) {

            /* O pinakas mas me ta strings gemizei kathe 10 records, ara prepei na kanoume realloc gia alles 10 theseis */
            if ((( *nviews ) % 10 == 0 ) && (( *nviews ) != 0 ) ) {
                if (( views = realloc( views, ( *nviews ) + 10 ) ) == NULL ) {
                    printf( MEM_ERROR );
                    cleanup();
                    return NULL;
                }
            }

            /* Initialization gia tis nees theseis toy pinaka */
            for ( i = ( *nviews ); i < ( *nviews ) + 10; i++ ) {
                views[i] = NULL;
            }

            /* Desmeuoume mnhmh gia to twrino string */
            if (( views[*nviews] = malloc( MAXNAME * sizeof( char ) ) ) == NULL ) {
                printf( MEM_ERROR );
                cleanup();
                return NULL;
            }

            /* vrhkame akomh mia opsh panw sth sxesh mas kai thn apothikeuoume */
            strcpy( views[*nviews], record + fromRelAttrs[0].offset );
            ( *nviews )++;
        }
    }

    if ( recId != HFE_EOF ) {
        printf( "Error while scaning viewCat for records\n" );
        cleanup();
        return NULL;
    }

    /* Kanoume tis energeies poy prepei na ginoyn sto telos ths synarthshs, xwris na eleytherwnoume to 'views' */
    free( record );
    free( fromRelAttrs );

    if ( HF_CloseFileScan( scanDesc ) != HFE_OK ) {
        HF_PrintError( "Could not close scan in viewCat" );
    }

    /* epistrefoume ta onomata twn opsewn poy vrhkame */
    return views;
}
