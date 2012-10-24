#ifndef BF_LIB_H
#define BF_LIB_H


#define BF_BLOCK_SIZE 1024

#define BFE_OK 0                    /* OK */
#define BFE_NOMEM -1                /* έλλειψη μνήμης */
#define BFE_NOBUF -2                /* έλλειψη χώρου ενδιάμεσης μνήμης */
#define BFE_BLOCKFIXED -3           /* μπλοκ ήδη ``καρφωμένο'' στη μνήμη */
#define BFE_BLOCKNOTINBUF -4        /* μπλοκ για ``ξεκάρφωμα'' δεν είναι στη μνήμη */
#define BFE_BLOCKINBUF -5           /* μπλοκ ήδη στη μνήμη */
#define BFE_OS -6                   /* γενικό σφάλμα Λειτουργικού Συστήματος */
#define BFE_INCOMPLETEREAD -7       /* ατελής ανάγνωση μπλοκ */
#define BFE_INCOMPLETEWRITE -8      /* ατελές γράψιμο σε μπλοκ */
#define BFE_INCOMPLETEHDRREAD -9    /* ατελής ανάγνωση μπλοκ-κεφαλίδας */
#define BFE_INCOMPLETEHDRWRITE -10  /* ατελές γράψιμο σε μπλοκ-κεφαλίδα */
#define BFE_INVALIDBLOCK -11        /* μη έγκυρος αναγνωριστικός αριθμός μπλοκ */
#define BFE_FILEOPEN -12            /* αρχείο ήδη ανοιχτό */
#define BFE_FTABFULL -13            /* λίστα ανοιχτών αρχείων πλήρης */
#define BFE_FD -14                  /* μη έγκυρος αναγνωριστικός αριθμός αρχείου */
#define BFE_EOF -15                 /* τέλος αρχείου */
#define BFE_BLOCKFREE -16           /* μπλοκ ήδη διαθέσιμο */
#define BFE_BLOCKUNFIXED -17        /* μπλοκ ήδη ``ξεκαρφωμένο'' */
#define BFE_FILE_EXISTS -18         /* αρχειο υπάρχει ήδη */
#define BFE_FILE_NOT_EXISTS -19     /* αρχειο δεν υπάρχει */
#define BFE_FOPEN_ERROR  -20        /* σφάλμα στο άνοιγμα του αρχείου */
#define BFE_FILE_CLOSED -21         /* αρχείο ήδη κλειστό */
#define BFE_INVALID_FILEDESC -22    /* μη έγγυρο αναγνωριστικό αρχείου */
#define BFE_MALLOC_ERROR -23        /* σφάλμα malloc */
#define BFE_FWRITE_ERROR -24        /* σφάλμα fwrite */
#define BFE_FCLOSE_ERROR -25        /* σφάλμα fclose */
#define BFE_NULL_POINTER -26        /* κενός δείκτης */
#define BFE_FSEEK_ERROR -27         /* σφάλμα fseek */
#define BFE_FREAD_ERROR -28         /* σφάλμα fread */
#define BFE_FTELL_ERROR -29         /* σφάλμα ftell */
#define BFE_REALLOC_ERROR -30       /* σφάλμα realloc */
#define BFE_TOO_BIG_FILENAME -31    /* πολύ μεγάλο όνομα αρχείου */

/* Ορισμός καθολικής μεταβλητής σφάλματος */
int BF_errno;

/* Δηλώσεις συναρτήσεων Επιπέδου Block */
void BF_Init( void );
int BF_CreateFile( char *filename );
int BF_DestroyFile( char *filename );
int BF_OpenFile( char *filename );
int BF_CloseFile( int fileDesc );
int BF_GetFirstBlock( int fileDesc, int *blockNum, char **blockBuf );
int BF_GetNextBlock( int fileDesc, int *blockNum, char **blockBuf );
int BF_GetThisBlock( int fileDesc, int blockNum, char **blockBuf );
int BF_AllocBlock( int fileDesc, int *blockNum, char **blockBuf );
int BF_DisposeBlock( int fileDesc, int *blockNum );
int BF_UnpinBlock( int fileDesc, int blockNum, int dirty );
void BF_PrintError( char *errString );

#endif
