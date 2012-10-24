#ifndef __DEFN_H__
#define __DEFN_H__

#define MAXNAME 15
#define MAXSTRINGLENGTH 255

#define VIEW_OK 0
#define VIEW_GE -1
#define VIEW_INVALID -1

/* Error strings */
#define MEM_ERROR "Error: Memory Allocation failed.\n"

/* Comment the following line in order to remove debug messages */
/* #define DEBUG */

enum {FALSE, TRUE};
enum {NONCOND_SELECT, COND_SELECT, JOIN_SELECT};

/*
#define RELSIZE (MAXNAME + 12)
#define ATTRSIZE (MAXNAME + MAXNAME + 17)
#define VIEWSIZE (5 * MAXNAME + 12 + MAXSTRINGLENGTH)
#define VIEWATTRSIZE (4 * MAXNAME)
*/

typedef struct {
    char relname[MAXNAME];  /* ����� ������ */
    int relwidth;           /* ����� �������� ������ �� bytes */
    int attrcnt;            /* ������� ������ �������� */
    int indexcnt;           /* ������� ���������� ������ */
} relDesc;

typedef struct {
    char relname[MAXNAME];    /* ����� ������ */
    char attrname[MAXNAME];   /* ����� ������ ��� ������ */
    int offset;               /* �������� ����� ������ ��� ��� ���� ��� �������� �� bytes */
    int attrlength;           /* ����� ������ �� bytes */
    char attrtype;            /* ����� ������ ('i', 'f', � 'c' */
    int indexed;              /* TRUE �� �� ����� ���� ��������� */
    int indexno;              /* ����� ������� ��� ���������� �� indexed=TRUE */
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

int relcatFileDesc;
int attrcatFileDesc;
int viewcatFileDesc;
int viewattrcatFileDesc;

#endif  /*__DEFN_H__*/

