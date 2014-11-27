#define INCL_DOSSEMAPHORES
#define INCL_DOSASYNCTIMER
#define INCL_DOSDATETIME
#include <os2.h>

#include <stdio.h>
#include <string.h>

void usage(void) {
	printf("eventer - wait until a specified time\n");
	printf("usage: eventer HH:MM\n");
}

int parseTime(const char *hhmm, UCHAR *hour, UCHAR *minute) {
	*hour=0;
	if(hhmm[0]<'0' || hhmm[0]>'9' ||
	   hhmm[1]<'0' || hhmm[1]>'9' ||
	   hhmm[2]!=':'               ||
	   hhmm[3]<'0' || hhmm[3]>'9' ||
	   hhmm[4]<'0' || hhmm[4]>'9' ||
	   hhmm[5]!='\0')
		return -1;

	*hour  =(hhmm[0]-'0')*10+(hhmm[1]-'0');
	*minute=(hhmm[3]-'0')*10+(hhmm[4]-'0');

	return 0;
}


int cmpDT(UCHAR hour1, UCHAR minute1, UCHAR hour2, UCHAR minute2) {
	if(hour1<hour2)     return -1;
	if(hour1>hour2)     return  1;
	if(minute1<minute2) return -1;
	if(minute1>minute2) return  1;
	return 0;
}


long ms_from_time2time(UCHAR hour1, UCHAR minute1, UCHAR hour2, UCHAR minute2) {
	if(cmpDT(hour1,minute1,hour2,minute2)>=0) hour2+=24;
	long sek= ((hour2-hour1)*60L + minute2-minute1)*60;
	long ms=sek*1000L;
	return ms;
}


int main(int argc, char *argv[]) {
	int rv=0;
	if(argc!=2) {
		usage();
		return -1;
	}
	UCHAR eventHour,eventMinute;
	if(parseTime(argv[1],&eventHour,&eventMinute)) {
		printf("eventer: '%s' is not in HH:MM format (eg: 01:00 18:37 00:00)\n",argv[1]);
		return -1;
	}

	APIRET rc;

	HSEM hsemTimerSem;
	char szName[20];
	strcpy(szName,"\\SEM32\\EVENTER");
	rc=DosCreateEventSem((PSZ)szName, (PHEV)&hsemTimerSem, 0, FALSE);
	if(rc!=0) {
		printf("eventer: DosCreateEventSem() returned %ld\n",rc);
		return -1;
	}


	DATETIME dt;
	DosGetDateTime(&dt);

	//compute ms until event
	long ms=ms_from_time2time(dt.hours,dt.minutes,eventHour,eventMinute);
#ifndef NDEBUG
	printf("ms till event: %ld\n",ms);
#endif

	HTIMER htimerTimer;
	rc=DosAsyncTimer((ULONG)ms, hsemTimerSem, &htimerTimer);
	if(rc!=0) {
		printf("eventer: DosAsyncTimer() returned %ld\n",rc);
		rv=-1;
		goto freeSem;
	}

	//semaphore created, timer started, now go to sleep...
	printf("Counting sheep until %s\n",argv[1]); fflush(stdout);
	rc=DosWaitEventSem((HEV)hsemTimerSem, (ULONG)-1);	//count sheep
	if(rc!=0) {
		printf("eventer: DosWaitEventSem() returned %ld\n",rc);
		rv=-1;
	}


freeSem:
	rc=DosCloseEventSem((HEV)hsemTimerSem);
	if(rc!=0) {
		printf("eventer: DosCloseEventSem() returned %ld\n",rc);
		rv=-1;
	}

end:
	return rv;
}
