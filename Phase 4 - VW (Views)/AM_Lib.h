#ifndef AM_LIB_H
#define AM_LIB_H

#define AME_OK 0
#define AME_UNDERLAYING_LIBRARY_ERROR -1
#define AME_INDEX_EXISTS -2
#define AME_INDEX_NOT_EXISTS -3
#define AME_INDEXOPEN -4
#define AME_OS -5
#define AME_INVALID_SCANDESC -6
#define AME_INDEX_CLOSED -7
#define AME_SCAN_IN_PROGRESS -8
#define AME_ENTRY_NOT_FOUND -9
#define AME_EOF -10
#define AME_INVALID_INDEX_DESCRIPTOR -11
#define AME_INVALID_ATTRTYPE -12

int AM_errno;

void AM_Init(void);
int AM_CreateIndex(char *fileName, int indexNo, char attrType, int attrLength);
int AM_DestroyIndex(char *fileName, int indexNo);
int AM_OpenIndex (char *fileName, int indexNo);
int AM_CloseIndex (int fileDesc);
int AM_InsertEntry(int fileDesc, char attrType, int attrLength, char *value, int recId);
int AM_DeleteEntry(int fileDesc, char attrType, int attrLength, char *value, int recId);
int AM_OpenIndexScan(int fileDesc, char attrType, int attrLength, int op, char *value);
int AM_FindNextEntry(int scanDesc);
int AM_CloseIndexScan(int scanDesc);
void AM_PrintError(char *errString);

#endif
