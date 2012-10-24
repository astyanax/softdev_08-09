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
	int eventsFd;
	int partsFd;

	int eventsIndexFd1;

	int partsIndexFd0;
	int partsIndexFd1;
	int partsIndexFd2;

	int recId;
	char *eventRec;
	char *partRec;

	int athId;
	int eventId;
	char eventName[AM_MAIN_EVENT_NAME_SIZE];
	char partDate[AM_MAIN_DATE_SIZE];

	int scanDesc1;
	int scanDesc2;
	int scanDesc3;

	int counter;


	HF_Init();
	
	AM_Init();


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


	eventsIndexFd1 = AM_OpenIndex("EVENTS", 1);

	if (eventsIndexFd1 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on EVENTS.1.");
		return -1;
	}



	partsIndexFd0 = AM_OpenIndex("PARTICIPATIONS", 0);

	if (partsIndexFd0 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on PARTICIPATIONS.0.");
		return -1;
	}



	partsIndexFd1 = AM_OpenIndex("PARTICIPATIONS", 1);

	if (partsIndexFd1 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on PARTICIPATIONS.1.");
		return -1;
	}



	partsIndexFd2 = AM_OpenIndex("PARTICIPATIONS", 2);

	if (partsIndexFd2 < 0)
	{
		AM_PrintError("Error in AM_OpenIndex called on PARTICIPATIONS.2.");
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


	scanDesc2 = AM_OpenIndexScan(eventsIndexFd1, 'c', AM_MAIN_EVENT_NAME_SIZE, NOT_EQUAL, "GYMNASTIKH");
	counter = 0;
	while((recId = AM_FindNextEntry(scanDesc2)) != AME_EOF)
	{
		if (HF_GetThisRec(eventsFd, recId, eventRec, AM_MAIN_EVENT_REC_SIZE) != HFE_OK)
		{
			HF_PrintError("Indexed event not found.");
		}
		else
		{
			memcpy((void *) &eventId, (void *) eventRec, sizeof(int));
			eventRec += sizeof(int);
			strcpy(eventName, eventRec);
			eventRec -= sizeof(int);

			printf("Found event: (%d, %s)\n", eventId, eventName);
			counter++;
		}
	}
	AM_CloseIndexScan(scanDesc2);
	printf("Number of found events: %d\n", counter);


	athId = 79;
	scanDesc1 = AM_OpenIndexScan(partsIndexFd0, 'i', sizeof(int), NOT_EQUAL, (char *) &athId);
	counter = 0;
	while((recId = AM_FindNextEntry(scanDesc1)) != AME_EOF)
	{
		if (HF_GetThisRec(partsFd, recId, partRec, AM_MAIN_PARTICIPATION_REC_SIZE) != HFE_OK)
		{
			HF_PrintError("Indexed participation not found.");
		}
		else
		{
			memcpy((void *) &athId, (void *) partRec, sizeof(int));
			partRec += sizeof(int);
			memcpy((void *) &eventId, (void *) partRec, sizeof(int));
			partRec += sizeof(int);
			strcpy(partDate, partRec);
			partRec += AM_MAIN_DATE_SIZE;
			partRec -= AM_MAIN_PARTICIPATION_REC_SIZE;

			if (AM_DeleteEntry(partsIndexFd0, 'i', sizeof(int), (char *) &athId, recId) != AME_OK)
				AM_PrintError("Failed to delete indexed participation.");
			if (AM_DeleteEntry(partsIndexFd1, 'i', sizeof(int), (char *) &eventId, recId) != AME_OK)
				AM_PrintError("Failed to delete indexed participation.");
			if (AM_DeleteEntry(partsIndexFd2, 'c', AM_MAIN_DATE_SIZE, partDate, recId) != AME_OK)
				AM_PrintError("Failed to delete indexed participation.");
			counter++;
		}
	}
	AM_CloseIndexScan(scanDesc1);
	printf("Number of deleted participations: %d\n", counter);

	scanDesc3 = AM_OpenIndexScan(partsIndexFd0, 'i', sizeof(int), EQUAL, NULL);
	counter = 0;
	while((recId = AM_FindNextEntry(scanDesc3)) != AME_EOF)
	{
		if (HF_GetThisRec(partsFd, recId, partRec, AM_MAIN_PARTICIPATION_REC_SIZE) != HFE_OK)
		{
			HF_PrintError("Indexed participation not found.");
		}
		else
		{
			memcpy((void *) &athId, (void *) partRec, sizeof(int));
			memcpy((void *) &eventId, partRec + sizeof(int), sizeof(int));
			memcpy(partDate, partRec + 2*sizeof(int), AM_MAIN_DATE_SIZE);
			printf("Found participation: (%d, %d, %s)\n",athId,eventId,partDate);
			counter++;
		}
	}
	
	AM_CloseIndexScan(scanDesc3);
	printf("Number of found participations: %d\n", counter);

	free(eventRec);
	free(partRec);


	if (HF_CloseFile(eventsFd) != HFE_OK)
		HF_PrintError("Error in HF_CloseFile called on EVENTS.");
	if (HF_CloseFile(partsFd) != BFE_OK)
		HF_PrintError("Error in HF_CloseFile called on PARTICIPATIONS.");


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
