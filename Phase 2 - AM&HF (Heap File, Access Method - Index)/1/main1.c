#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AM_Lib.h"
#include "HF_Lib.h"

#define AM_MAIN_NAME_SIZE 30
#define AM_MAIN_EVENT_NAME_SIZE 60
#define AM_MAIN_DATE_SIZE 11

/*Μέγεθος της εγγραφής τύπου Athlete*/
#define AM_MAIN_ATHLETE_REC_SIZE sizeof(int) + 2*(sizeof(char) * AM_MAIN_NAME_SIZE)

/*Μέγεθος της εγγραφής τύπου Event*/
#define AM_MAIN_EVENT_REC_SIZE sizeof(int) + (sizeof(char) * AM_MAIN_EVENT_NAME_SIZE)

/*Μέγεθος της εγγραφής τύπου Participation*/
#define AM_MAIN_PARTICIPATION_REC_SIZE 2*sizeof(int) + (sizeof(char) * AM_MAIN_DATE_SIZE)

int main()
{
	int athletesFd;
	int eventsFd;
	int partsFd;

	int athletesIndexFd0;
	int athletesIndexFd1;
	int athletesIndexFd2;

	int eventsIndexFd0;
	int eventsIndexFd1;

	int partsIndexFd0;
	int partsIndexFd1;
	int partsIndexFd2;

	int recId;
	char *athleteRec;
	char *eventRec;
	char *partRec;

	int athId;
	char surname[AM_MAIN_NAME_SIZE];
	char name[AM_MAIN_NAME_SIZE];
	int eventId;
	char eventName[AM_MAIN_EVENT_NAME_SIZE];
	char partDate[AM_MAIN_DATE_SIZE];


	/* Αρχικοποίηση Επιπέδου HF */
	HF_Init();
	/* Αρχικοποίηση Επιπέδου AM */
	AM_Init();

	/* ’νοιγμα Αρχείου ATHLETES */
	athletesFd = HF_OpenFile("ATHLETES");

	if (athletesFd < 0)
	{
		HF_PrintError("Error in HF_OpenFile called on ATHLETES.");
		return -1;
	}

	/* ’νοιγμα Αρχείου EVENTS */
	eventsFd = HF_OpenFile("EVENTS");

	if (eventsFd < 0)
	{
		HF_PrintError("Error in HF_OpenFile called on EVENTS.");
		return -1;
	}

	/* ’νοιγμα Αρχείου PARTICIPATIONS */
	partsFd = HF_OpenFile("PARTICIPATIONS");

	if (partsFd < 0)
	{
		HF_PrintError("Error in HF_OpenFile called on PARTICIPATIONS.");
		return -1;
	}




	
	/*Δημιουργία ευρετηρίου στο 1ο πεδίο του αρχείου ATHLETES*/
	if (AM_CreateIndex("ATHLETES", 0, 'i', sizeof(int)) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on ATHLETES.0.");
	}

	/*’νοιγμα του 1ου ευρετηρίου στο αρχείο ATHLETES*/
	athletesIndexFd0 = AM_OpenIndex("ATHLETES", 0);

	if (athletesIndexFd0 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on ATHLETES.0.");
		return -1;
	}

	/*Δημιουργία ευρετηρίου στο 2ο πεδίο του αρχείου ATHLETES*/
	if (AM_CreateIndex("ATHLETES", 1, 'c', AM_MAIN_NAME_SIZE) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on ATHLETES.1.");
	}

/*	//’νοιγμα του 2ου ευρετηρίου στο αρχείο ATHLETES*/
	athletesIndexFd1 = AM_OpenIndex("ATHLETES", 1);

	if (athletesIndexFd1 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on ATHLETES.1.");
		return -1;
	}

/*	//Δημιουργία ευρετηρίου στο 3ο πεδίο του αρχείου ATHLETES */
	if (AM_CreateIndex("ATHLETES", 2, 'c', AM_MAIN_NAME_SIZE) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on ATHLETES.2.");
	}

/*	//’νοιγμα του 3ου ευρετηρίου στο αρχείο ATHLETES*/
	athletesIndexFd2 = AM_OpenIndex("ATHLETES", 2);

	if (athletesIndexFd2 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on ATHLETES.2.");
		return -1;
	}

	athleteRec = (char *) malloc(AM_MAIN_ATHLETE_REC_SIZE);
	if (athleteRec == NULL)
	{
		printf("Athlete record malloc failed\n");
		return -1;
	}

/*	// Εισαγωγή δεδομένων στα ευρετήρια του αρχείου ATHLETES*/
	recId = HF_GetFirstRec(athletesFd, athleteRec, AM_MAIN_ATHLETE_REC_SIZE);
	while (recId >= 0)
	{
		memcpy((void *) &athId, (void *) athleteRec, sizeof(int));
		athleteRec += sizeof(int);

		strcpy(surname, athleteRec);
		athleteRec += AM_MAIN_NAME_SIZE;

		strcpy(name, athleteRec);
		athleteRec += AM_MAIN_NAME_SIZE;

		athleteRec -= AM_MAIN_ATHLETE_REC_SIZE;

		if (AM_InsertEntry(athletesIndexFd0, 'i', sizeof(int), (char *) &athId, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");
		if (AM_InsertEntry(athletesIndexFd1, 'c', AM_MAIN_NAME_SIZE, surname, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");
		if (AM_InsertEntry(athletesIndexFd2, 'c', AM_MAIN_NAME_SIZE, name, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");

		recId = HF_GetNextRec(athletesFd, recId, athleteRec, AM_MAIN_ATHLETE_REC_SIZE);
	}




/*	//Δημιουργία ευρετηρίου στο 1ο πεδίο του αρχείου EVENTS*/
	if (AM_CreateIndex("EVENTS", 0, 'i', sizeof(int)) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on EVENTS.0.");
	}

/*	//’νοιγμα του 1ου ευρετηρίου στο αρχείο EVENTS*/
	eventsIndexFd0 = AM_OpenIndex("EVENTS", 0);

	if (eventsIndexFd0 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on EVENTS.0.");
		return -1;
	}

/*	//Δημιουργία ευρετηρίου στο 2ο πεδίο του αρχείου EVENTS*/
	if (AM_CreateIndex("EVENTS", 1, 'c', AM_MAIN_EVENT_NAME_SIZE) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on EVENTS.1.");
	}

/*	//’νοιγμα του 2ου ευρετηρίου στο αρχείο EVENTS*/
	eventsIndexFd1 = AM_OpenIndex("EVENTS", 1);

	if (eventsIndexFd1 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on EVENTS.1.");
		return -1;
	}

	eventRec = (char *) malloc(AM_MAIN_EVENT_REC_SIZE);
	if (eventRec == NULL)
	{
		printf("Event record malloc failed\n");
		return -1;
	}

/*	// Εισαγωγή δεδομένων στα ευρετήρια του αρχείου EVENTS*/
	recId = HF_GetFirstRec(eventsFd, eventRec, AM_MAIN_EVENT_REC_SIZE);
	while (recId >= 0)
	{
		memcpy((void *) &eventId, (void *) eventRec, sizeof(int));
		eventRec += sizeof(int);

		strcpy(eventName, eventRec);
		eventRec += AM_MAIN_EVENT_NAME_SIZE;

		eventRec -= AM_MAIN_EVENT_REC_SIZE;

		if (AM_InsertEntry(eventsIndexFd0, 'i', sizeof(int), (char *) &eventId, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");
		if (AM_InsertEntry(eventsIndexFd1, 'c', AM_MAIN_EVENT_NAME_SIZE, eventName, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");

		recId = HF_GetNextRec(eventsFd, recId, eventRec, AM_MAIN_ATHLETE_REC_SIZE);
	}



	
/*	//Δημιουργία ευρετηρίου στο 1ο πεδίο του αρχείου PARTICIPATIONS*/
	if (AM_CreateIndex("PARTICIPATIONS", 0, 'i', sizeof(int)) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on PARTICIPATIONS.0.");
	}

/*	//’νοιγμα του 1ου ευρετηρίου στο αρχείο PARTICIPATIONS*/
	partsIndexFd0 = AM_OpenIndex("PARTICIPATIONS", 0);

	if (partsIndexFd0 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on PARTICIPATIONS.0.");
		return -1;
	}

/*	//Δημιουργία ευρετηρίου στο 2ο πεδίο του αρχείου PARTICIPATIONS*/
	if (AM_CreateIndex("PARTICIPATIONS", 1, 'i', sizeof(int)) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on PARTICIPATIONS.1.");
	}

/*	//’νοιγμα του 2ου ευρετηρίου στο αρχείο PARTICIPATIONS*/
	partsIndexFd1 = AM_OpenIndex("PARTICIPATIONS", 1);

	if (partsIndexFd1 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on PARTICIPATIONS.1.");
		return -1;
	}

/*	//Δημιουργία ευρετηρίου στο 3ο πεδίο του αρχείου PARTICIPATIONS*/
	if (AM_CreateIndex("PARTICIPATIONS", 2, 'c', AM_MAIN_DATE_SIZE) != AME_OK)
	{
		AM_PrintError("Error in AM_CreateIndex called on PARTICIPATIONS.2.");
	}

/*	//’νοιγμα του 3ου ευρετηρίου στο αρχείο PARTICIPATIONS*/
	partsIndexFd2 = AM_OpenIndex("PARTICIPATIONS", 2);

	if (partsIndexFd2 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on PARTICIPATIONS.2.");
		return -1;
	}

	partRec = (char *) malloc(AM_MAIN_PARTICIPATION_REC_SIZE);
	if (partRec == NULL)
	{
		printf("Participation record malloc failed\n");
		return -1;
	}

/*	// Εισαγωγή δεδομένων στα ευρετήρια του αρχείου PARTICIPATIONS*/
	recId = HF_GetFirstRec(partsFd, partRec, AM_MAIN_PARTICIPATION_REC_SIZE);
	while (recId >= 0)
	{
		memcpy((void *) &athId, (void *) partRec, sizeof(int));
		partRec += sizeof(int);

		memcpy((void *) &eventId, (void *) partRec, sizeof(int));
		partRec += sizeof(int);

		strcpy(partDate, partRec);
		partRec += AM_MAIN_DATE_SIZE;

		partRec -= AM_MAIN_PARTICIPATION_REC_SIZE;

		if (AM_InsertEntry(partsIndexFd0, 'i', sizeof(int), (char *) &athId, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");
		if (AM_InsertEntry(partsIndexFd1, 'i', sizeof(int), (char *) &eventId, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");
		if (AM_InsertEntry(partsIndexFd2, 'c', AM_MAIN_DATE_SIZE, partDate, recId) != AME_OK)
			AM_PrintError("Failed to insert entry.");

		recId = HF_GetNextRec(partsFd, recId, partRec, AM_MAIN_PARTICIPATION_REC_SIZE);
	}


	free(athleteRec);
	free(eventRec);
	free(partRec);

/*	//Κλείσιμο Αρχείων*/
	if (HF_CloseFile(athletesFd) != HFE_OK)
		HF_PrintError("Error in HF_CloseFile called on ATHLETES.");
	if (HF_CloseFile(eventsFd) != HFE_OK)
		HF_PrintError("Error in HF_CloseFile called on EVENTS.");
	if (HF_CloseFile(partsFd) != HFE_OK)
		HF_PrintError("Error in HF_CloseFile called on PARTICIPATIONS.");
	if (AM_CloseIndex(athletesIndexFd0) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on ATHLETES.0.");
	if (AM_CloseIndex(athletesIndexFd1) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on ATHLETES.1.");
	if (AM_CloseIndex(athletesIndexFd2) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on ATHLETES.2.");
	if (AM_CloseIndex(eventsIndexFd0) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on EVENTS.0.");
	if (AM_CloseIndex(eventsIndexFd1) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on EVENTS.1.");
	if (AM_CloseIndex(partsIndexFd0) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on PARTICIPATIONS.0.");
	if (AM_CloseIndex(partsIndexFd1) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on PARTICIPATIONS.1.");
	if (AM_CloseIndex(partsIndexFd2) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on PARTICIPATIONS.2.");

	return 0;
}

