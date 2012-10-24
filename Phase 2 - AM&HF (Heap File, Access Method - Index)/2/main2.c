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

	int eventsIndexFd0;

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

	int scanDesc1;
	int scanDesc2;
	int scanDesc3;



	HF_Init();

	AM_Init();



	athletesFd = HF_OpenFile("ATHLETES");

	if (athletesFd < 0)
	{
		HF_PrintError("Error in HF_OpenFile called on ATHLETES.");
		return -1;
	}


	eventsFd = HF_OpenFile("EVENTS");

	if (eventsFd < 0)
	{
		HF_PrintError("Error in HF_OpenFile called on EVENTS.");
		return -1;
	}


	partsFd = HF_OpenFile("PARTICIPATIONS");

	if (partsFd < 0)
	{
		HF_PrintError("Error in HF_OpenFile called on PARTICIPATIONS.");
		return -1;
	}

	athletesIndexFd0 = AM_OpenIndex("ATHLETES", 0);

	if (athletesIndexFd0 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on ATHLETES.0.");
		return -1;
	}

	athletesIndexFd1 = AM_OpenIndex("ATHLETES", 1);

	if (athletesIndexFd1 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on ATHLETES.1.");
		return -1;
	}


	eventsIndexFd0 = AM_OpenIndex("EVENTS", 0);

	if (eventsIndexFd0 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on EVENTS.0.");
		return -1;
	}

	partsIndexFd2 = AM_OpenIndex("PARTICIPATIONS", 2);

	if (partsIndexFd2 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on PARTICIPATIONS.2.");
		return -1;
	}


	athleteRec = (char *) malloc(AM_MAIN_ATHLETE_REC_SIZE);
	if (athleteRec == NULL)
	{
		printf("Athlete record malloc failed\n");
		return -1;
	}

	eventRec = (char *) malloc(AM_MAIN_EVENT_REC_SIZE);
	if (eventRec == NULL)
	{
		printf("Event record malloc failed\n");
		return -1;
	}


	partRec = (char *) malloc(AM_MAIN_PARTICIPATION_REC_SIZE);
	if (partRec == NULL)
	{
		printf("Participation record malloc failed\n");
		return -1;
	}


	printf("Searching for athletes with Surname = HLIADHS:\n");
	strcpy(surname, "HLIADHS");
	scanDesc1 = AM_OpenIndexScan(athletesIndexFd1, 'c', AM_MAIN_NAME_SIZE, EQUAL, surname);
	while((recId = AM_FindNextEntry(scanDesc1)) != AME_EOF)
	{
		if (HF_GetThisRec(athletesFd, recId, athleteRec, AM_MAIN_ATHLETE_REC_SIZE) != HFE_OK)
		{
			HF_PrintError("Indexed athlete not found.");
		}
		else
		{
			memcpy((void *) &athId, (void *) athleteRec, sizeof(int));
			athleteRec += sizeof(int);

			strcpy(surname, athleteRec);
			athleteRec += AM_MAIN_NAME_SIZE;

			strcpy(name, athleteRec);
			athleteRec += AM_MAIN_NAME_SIZE;

			athleteRec -= AM_MAIN_ATHLETE_REC_SIZE;
			printf("%d. %d %s %s\n", recId, athId, surname, name);
		}
	}
	AM_CloseIndexScan(scanDesc1);

	printf("Searching for events carried out on '06/04/1999':\n");
	strcpy(partDate, "06/04/1999");
	scanDesc1 = AM_OpenIndexScan(partsIndexFd2, 'c', AM_MAIN_DATE_SIZE, EQUAL, partDate);
	while((recId = AM_FindNextEntry(scanDesc1)) != AME_EOF)
	{
		int found = 1;

		if (HF_GetThisRec(partsFd, recId, partRec, AM_MAIN_PARTICIPATION_REC_SIZE) != HFE_OK)
		{
			HF_PrintError("Indexed participation not found.");
			found = 0;
		}
		else
		{
			memcpy((void *) &athId, (void *) partRec, sizeof(int));
			partRec += sizeof(int);
			memcpy((void *) &eventId, (void *) partRec, sizeof(int));
			partRec -= sizeof(int);
		}
	
		scanDesc2 = AM_OpenIndexScan(athletesIndexFd0, 'i', sizeof(int), EQUAL, (char *) &athId);
		if ((recId = AM_FindNextEntry(scanDesc2)) != AME_EOF)
		{
			if (HF_GetThisRec(athletesFd, recId, athleteRec, AM_MAIN_ATHLETE_REC_SIZE) != HFE_OK)
			{
				HF_PrintError("Indexed athlete not found.");
				found = 0;
			}
			else
			{
				athleteRec += sizeof(int);
				strcpy(surname, athleteRec);
				athleteRec += AM_MAIN_NAME_SIZE;
				strcpy(name, athleteRec);
				athleteRec += AM_MAIN_NAME_SIZE;
				athleteRec -= AM_MAIN_ATHLETE_REC_SIZE;
			}
		}
		AM_CloseIndexScan(scanDesc2);
	
		scanDesc3 = AM_OpenIndexScan(eventsIndexFd0, 'i', sizeof(int), EQUAL, (char *) &eventId);
		if ((recId = AM_FindNextEntry(scanDesc3)) != AME_EOF)
		{
			if (HF_GetThisRec(eventsFd, recId, eventRec, AM_MAIN_EVENT_REC_SIZE) != HFE_OK)
			{
				HF_PrintError("Indexed event not found.");
				found = 0;
			}
			else
			{
				eventRec += sizeof(int);
				strcpy(eventName, eventRec);
				eventRec += AM_MAIN_EVENT_NAME_SIZE;
				eventRec -= AM_MAIN_EVENT_REC_SIZE;
			}
		}
		AM_CloseIndexScan(scanDesc3);

		if (found) printf("%s %s played %s on %s\n", surname, name, eventName, partDate);	
		else printf("Failed to find who played what on %s\n", partDate);	
	}
	AM_CloseIndexScan(scanDesc1);


	free(athleteRec);
	free(eventRec);
	free(partRec);


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
	if (AM_CloseIndex(eventsIndexFd0) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on EVENTS.0.");
	if (AM_CloseIndex(partsIndexFd2) != AME_OK)
		AM_PrintError("Error in AM_CloseIndex called on PARTICIPATIONS.2.");

	return 0;
}

