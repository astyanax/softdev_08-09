#ifndef _AMINIREL_H_
#define _AMINIREL_H_

enum {FALSE, TRUE};

#define MAXNAME 32
/* General Error */
#define AMINIREL_GE -1

#define AMINIREL_OK 0
#define AMINIREL_INVALID -1

/* Error strings */
#define MEM_ERROR "Error: Memory Allocation failed.\n"

/* Otan de dinetai kommati "where", xeirizomaste oles tis eggrafes */
#define GET_ALL 10

/**********************
 * Struct Definitions *
 **********************/

/* Perigrafei tis pleiades mias sxeshs */
typedef struct
{
    char relname[MAXNAME];  /* ����� ������ */
    int relwidth;           /* ����� �������� ������ �� bytes */
    int attrcnt;            /* ������� ������ �������� */
    int indexcnt;           /* ������� ���������� ������ */
} relDesc;

/* Perigrafei ena gnwrisma (attribute) mias sygkekrimenhs eggrafhs mias sxeshs */
typedef struct
{
    char relname[MAXNAME];  /* ����� ������ */
    char attrname[MAXNAME]; /* ����� ������ ��� ������ */
    int offset;             /* �������� ����� ������ ��� ��� ���� ��� �������� �� bytes */
    int attrlength;         /* ����� ������ �� bytes */
    char attrtype;          /* ����� ������ ('i', 'f', � 'c' */
    int indexed;            /* TRUE �� �� ����� ���� ��������� */
    int indexno;            /* ����� ������� ��� ���������� �� indexed=TRUE */
} attrDesc;

/* Struct pou krataei tous file descriptors gia ta 4 vasika arxeia ths database pou xeirizomaste,
   oso to aminirel einai energo. */
struct db
{
    int attrCatDesc;
    int relCatDesc;
    int viewAttrCatDesc;
    int viewCatDesc;
};

/***********************************
** UT layer function declarations **
***********************************/
int UT_Init( char * dbname );
void printArgs( int argc, char **argv );
int UT_create( int argc, char* argv[] );
int UT_buildindex( int argc, char* argv[] );
int UT_destroy( int argc, char* argv[] );
void UT_quit( void );

int updateRelCat( relDesc * relInfo );
int updateAttrCat( attrDesc * attrInfo );
relDesc * getRelation( char * relName, int * recId );
attrDesc * getAttribute( char * relName, char * attrName, int * recId );

/***********************************
** DM layer function declarations **
***********************************/
int DM_select( int argc, char* argv[] );
int DM_join( int argc, char* argv[] );
int DM_delete( int argc, char* argv[] );
int DM_subtract( int argc, char* argv[] );
int DM_insert( int argc, char* argv[] );
int DM_add( int argc, char* argv[] );

int printRelation( char * relName );

#endif
