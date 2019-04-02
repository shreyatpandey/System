#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include<ctype.h>
#include<time.h>
#include<sys/time.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>
#include<math.h>
#include "my402list.h"
#include "cs402.h"

#define maxsize 2147483647

pthread_t tokenbucket,q1,s1,s2,interrupt; 
pthread_mutex_t threadlock = PTHREAD_MUTEX_INITIALIZER; 
pthread_cond_t qu2;
sigset_t set;



int flag = 0;
int pflag=0;
int flagincrement = 0;
int flaginterrupt =0 ;
long tokencount =0 ;
long tempcount = 0;
long initialtokenbuck;
double initialtime,endtime;
double timeinQ1=0;
double totaltimespentinQ1 = 0;
double timeinQ2=0;
double totaltimespentinQ2 = 0;
double totaltimefull = 0;
double totalpacketinterarrivaltime = 0;
double totalservicetimeS1 = 0;
double totalservicetimeS2 = 0;
double totaltime = 0;
double timesquaresys = 0;
int tfilepresent=0;
double ratetoken=0;
long bucketdepth=0;
long noofpackets;
double inter_arrival_time,no_of_tokens,service_time;
long droppedtokens,droppedpackets,compackets,packetcount;
double packetleavetimeQ1,packetarrivaltimeQ2;
My402List *queue1 = NULL;
My402List *queue2 = NULL;
My402List *server1 = NULL;
My402List *server2 = NULL;
FILE *fp;
char buf[1024];
long deternoofpackets;
double lambda,murate;
long notokensmain = 0;
struct timeval initial;

typedef struct packet_parameters
{
   double interarrivaltime,servicetime;
   long nooftokens;
   long numberofpackets;
   double timespentinQ1;
   double timespentinQ2;
   double timeenterQ1,timeenterQ2;
   double timeleaveQ1,timeleaveQ2;
   double interarrivaldifference;
   long packetno;

}packet;




double tsleepdiff(struct timeval now)
{
	double seconddiff = now.tv_sec - initial.tv_sec;
	double usecdiff = now.tv_usec - initial.tv_usec;
	double total = (seconddiff*1000000) + (usecdiff);
	return(total);
    
}
void readnoofpacket(FILE *fp)
{
   char no_of_packets[1024];
   fgets(buf,sizeof(buf),fp);
    if(sizeof(buf)>1024)
  {
    fprintf(stderr,"Size of line is very large\n");
    exit(0);
  }
   char *p = strtok(buf,"\t");
   strncpy(no_of_packets,p,sizeof(no_of_packets));
   noofpackets = atoi(no_of_packets);
   
 }

void printdroppedtokens(double timdiff,long tokencount)
{
  fprintf(stdout,"%012.3lfms: token t %ld arrives,dropped\n",timdiff/1000,tokencount);
}

void printtokenarrival1(double timediff,long tokencount,long tokenbuck)
{
   fprintf(stdout,"%012.3lfms: token t%ld arrives, token bucket has now %ld token\n",timediff/1000,tokenbuck,tokencount);
}


void printpacketleave(double timediff,long pacno,double  timespentinQ1,long tokencount)
{
   fprintf(stdout,"%012.3lfms: p%ld leaves Q1,Time spent in Q1:%0.3lfms,token bucket now has %ld token\n",(timediff/1000),pacno,(timespentinQ1/1000),tokencount);
}

void printpacketarrivalQ2(double timediff,long pacno)
{
  fprintf(stdout,"%012.3lfms: p%ld enters Q2\n",timediff/1000,pacno);
}

void *tokgen(void *args)  //Implementation is done with the help of slide Timeout in Bill Cheng's Thread Safety lecture
{
    struct timeval now;
    initialtokenbuck=0;
    double starttokentime = initialtime;
    double previoustokentime = 0;
    double tokentimeaftersleep =0;
    unsigned long sleeptokentime = 0;
    double timediff=0;   
    packetleavetimeQ1=0;
    My402ListElem *elem = NULL;
    packet *pac;
    pac = malloc(sizeof(packet));
	queue1 = malloc(sizeof(My402List));
	queue2 = malloc(sizeof(My402List));
	double iat = 1/(ratetoken);
	double iatmicro = ((1/ratetoken)*1000000);    //ratetoken is a global variable
      

if(tfilepresent==0)
{
	for(;;)
{
  if(((long)iat)>10)
{
	iatmicro = 10;
}
  gettimeofday(&now,NULL);
   timediff = tsleepdiff(now);
  sleeptokentime = (long)(iatmicro - timediff + previoustokentime);
  
  if(sleeptokentime<0)
  	usleep(0);
  else
  usleep(sleeptokentime);

  pthread_mutex_lock(&threadlock);

  gettimeofday(&now,NULL);
  tokentimeaftersleep = tsleepdiff(now);

  previoustokentime = tokentimeaftersleep;
  initialtokenbuck = initialtokenbuck + 1;

if(bucketdepth <= tokencount)
   {
       droppedtokens = droppedtokens + 1;
       printdroppedtokens(previoustokentime,initialtokenbuck);
       pthread_mutex_unlock(&threadlock);
       continue;

   }

if(bucketdepth>tokencount)
     {
       tokencount=tokencount+1;
       printtokenarrival1(previoustokentime,tokencount,initialtokenbuck);
      } 
 pthread_mutex_unlock(&threadlock);
elem = My402ListFirst(queue1);
   //int resultemptycheck = listemptycheck(elem);
   if(elem == NULL)
   {
   	pthread_mutex_unlock(&threadlock);
   	continue;
   }
   pac = (struct packet_parameters*)elem->obj;
        
    if(tokencount >= (notokensmain))   
    {
        int checkempty2 = 0;
        gettimeofday(&now,NULL);
        packetleavetimeQ1 = tsleepdiff(now);
        //printf("packetleavetimeQ1:- %lf\n",packetleavetimeQ1);
        pac->timespentinQ1 = packetleavetimeQ1 - (pac->timeenterQ1); //both pac->packetarrivaltime & pac->packetno
        totaltimespentinQ1 = totaltimespentinQ1 + pac->timespentinQ1;
        My402ListUnlink(queue1,elem);

        tokencount = tokencount - (notokensmain);
        printpacketleave(packetleavetimeQ1,pac->packetno,pac->timespentinQ1,tokencount);
        
        
        checkempty2 = My402ListEmpty(queue2);
        My402ListAppend(queue2,pac);
        tempcount = tempcount + 1;
        flagincrement = flagincrement + 1;

        //printf("Value of temp count is:- %d\n",tempcount);
        gettimeofday(&now,NULL);
        packetarrivaltimeQ2 = tsleepdiff(now);
        printpacketarrivalQ2(packetarrivaltimeQ2,pac->packetno);
        //printf("packetarrivaltimeQ2:- %lf\n",packetarrivaltimeQ2);
        if(checkempty2!=0)
        {
        	pthread_cond_broadcast(&qu2);
        }

   }
   pthread_mutex_unlock(&threadlock);
    if(( flagincrement == deternoofpackets) && My402ListEmpty(queue1))
		{
			break;
		}
   }  // this is for loop
   //int emptycheckq2 = My402ListEmpty(queue2);
   pflag = 1;
   if(My402ListEmpty(queue2))
	{
		//flag =1;
		pthread_cond_broadcast(&qu2);
	}
   pthread_exit(NULL);
}
if(tfilepresent ==1)	
{
for(;;)
{
  gettimeofday(&now,NULL);
   timediff = tsleepdiff(now);
  //sleeptokentime = (long)(iatmicro - timediff + previoustokentime);
  //printf("sleeptokentime is:-%ld\n",sleeptokentime);
  //if(sleeptokentime<0)
  	//usleep(0);
 // else
   long m = (long)(iatmicro);
  usleep(m);

  pthread_mutex_lock(&threadlock);

  gettimeofday(&now,NULL);
  tokentimeaftersleep = tsleepdiff(now);

  previoustokentime = tokentimeaftersleep;
  initialtokenbuck = initialtokenbuck + 1;

if(bucketdepth <= tokencount)
   {
       droppedtokens = droppedtokens + 1;
       printdroppedtokens(previoustokentime,initialtokenbuck);
       pthread_mutex_unlock(&threadlock);
       continue;

   }

if(bucketdepth>tokencount)
     {
       tokencount=tokencount+1;
       printtokenarrival1(previoustokentime,tokencount,initialtokenbuck);
      } 
 pthread_mutex_unlock(&threadlock);
elem = My402ListFirst(queue1);
   //int resultemptycheck = listemptycheck(elem);
   if(elem == NULL)
   {
   	pthread_mutex_unlock(&threadlock);
   	continue;
   }
   pac = (struct packet_parameters*)elem->obj;
        
    if(tokencount >= (pac->nooftokens))   
    {
        int checkempty2 = 0;
        gettimeofday(&now,NULL);
        packetleavetimeQ1 = tsleepdiff(now);
        //printf("packetleavetimeQ1:- %lf\n",packetleavetimeQ1);
        pac->timespentinQ1 = packetleavetimeQ1 - (pac->timeenterQ1); //both pac->packetarrivaltime & pac->packetno
        totaltimespentinQ1 = totaltimespentinQ1 + pac->timespentinQ1;
        My402ListUnlink(queue1,elem);

        tokencount = tokencount - (pac->nooftokens);
        printpacketleave(packetleavetimeQ1,pac->packetno,pac->timespentinQ1,tokencount);
        
        
        checkempty2 = My402ListEmpty(queue2);
        My402ListAppend(queue2,pac);
        tempcount = tempcount + 1;
        flagincrement = flagincrement + 1;

        //printf("Value of temp count is:- %d\n",tempcount);
        gettimeofday(&now,NULL);
        packetarrivaltimeQ2 = tsleepdiff(now);
        printpacketarrivalQ2(packetarrivaltimeQ2,pac->packetno);
        //printf("packetarrivaltimeQ2:- %lf\n",packetarrivaltimeQ2);
        if(checkempty2!=0)
        {
        	pthread_cond_broadcast(&qu2);
        }

   }
   pthread_mutex_unlock(&threadlock);
    if(( flagincrement == noofpackets) && My402ListEmpty(queue1))
		{
			break;
		}
   }  // this is for loop
   //int emptycheckq2 = My402ListEmpty(queue2);
   pflag = 1;
   if(My402ListEmpty(queue2))
	{
		//flag =1;
		pthread_cond_broadcast(&qu2);
	}
   pthread_exit(NULL);
 }
}




void printdroppedpackets(double packettimdiff,long packetno,int nooftokens,double interarrivaldifference)
{
  fprintf(stdout,"%012.3lfms: packets t%ld arrives,needs %d tokens,inter-arrival time = %0.3lfms,dropped\n",packettimdiff/1000,packetno,nooftokens,interarrivaldifference/1000);
}
void printpacketarrival(double packettimediff,long packetno,int nooftokens,double interarrivaldifference)   //precise inter arrival time
{
   fprintf(stdout,"%012.3lfms: p%ld arrives,needs %d tokens,inter-arrival time:%0.3lfms\n",packettimediff/1000,packetno,nooftokens,interarrivaldifference/1000);

}
void printpacketenterqueue(double timeafterenteringqueue,long packetno)
{
   fprintf(stdout,"%012.3lfms: p%ld Enters q1\n",timeafterenteringqueue/1000,packetno);
}
void packetcancel(int tfilepresent,long packetcount,long tempcount,long returnemptycheckq1)  //changes required
{
	if(tfilepresent==0)
  {
	if((packetcount== deternoofpackets)&&(flagincrement == (packetcount- droppedpackets))&&(returnemptycheckq1!=0))
	pthread_cancel(tokenbucket);
   }
    //printf("Have you been executed\n");
   if(tfilepresent==1)
  {
	if((packetcount== noofpackets)&&(flagincrement == (packetcount- droppedpackets))&&(returnemptycheckq1!=0))
	pthread_cancel(tokenbucket);
   }
}


void *packgen(void *args)
{

    struct timeval now;
	My402ListElem *elem;
	packet *pac1 = NULL;
	queue1 = malloc(sizeof(My402List));
	queue2 = malloc(sizeof(My402List));
	int returnemptycheckq1;
	double packettimediff=0;
	packetcount = 0;
	double timediff = 0;
	double packetsleeptime = 0;
    double packettimeaftersleep = 0;
    double previouspacketarrivaltime = 0;
    double timeafterenteringqueue = 0;
    double packetunknowtime ;
    double droppedpackettime = 0;
    long lambdaratetime;
    long i;
    char buf[1024];
    char *temp;
    long countofpackets = 0;
    double interarrivalrate = (1/lambda);
    double deterinterarrivaltime = ((1/lambda)*1000000ULL);

 if(tfilepresent==0)
 {
 	for(i=0;i<deternoofpackets;i++)
    {
    	 if(((long)interarrivalrate)>10)
    	 {
    	 	deterinterarrivaltime = 10;
    	 }
    	
        pac1 = malloc(sizeof(packet));
        countofpackets ++;
        pac1->packetno = countofpackets;
        gettimeofday(&now,NULL);
        timediff = tsleepdiff(now);
        

        packetsleeptime = (long)(deterinterarrivaltime - timediff + previouspacketarrivaltime); //source of error
        //printf("packetsleeptime is :- %lf\n",packetsleeptime);
   
        if(packetsleeptime<0)
          usleep(0);
        else
        usleep(packetsleeptime);
        packetcount = packetcount + 1;
		
		
	    gettimeofday(&now,NULL);
		packettimeaftersleep = tsleepdiff(now); //packettimeaftersleep is nothing but timearrivalstamp
	    pac1->timeenterQ1 = packettimeaftersleep;
        pac1->interarrivaldifference  = pac1->timeenterQ1 - previouspacketarrivaltime;
        previouspacketarrivaltime = packettimeaftersleep;
	    
	    totalpacketinterarrivaltime = totalpacketinterarrivaltime + pac1->timeenterQ1;
        pthread_mutex_lock(&threadlock);

        if(notokensmain >bucketdepth)
		{
			
			printdroppedpackets(pac1->timeenterQ1,pac1->packetno,notokensmain,pac1->interarrivaldifference);
			droppedpackets = droppedpackets + 1;
		    pthread_mutex_unlock(&threadlock);
		    free(pac1);
		    i++;
		    continue;
		}
		printpacketarrival(pac1->timeenterQ1,pac1->packetno,notokensmain,pac1->interarrivaldifference);
        
        My402ListAppend(queue1,pac1);
        
         gettimeofday(&now,NULL);
         packettimediff = tsleepdiff(now);
        

        printpacketenterqueue(packettimediff,pac1->packetno);
        elem = My402ListFirst(queue1);
        if(elem==NULL)
        {
        	pthread_mutex_unlock(&threadlock);
        	continue;
        }
       
        pac1 = (struct packet_parameters*)elem->obj;
        
        if(tokencount >= notokensmain)
    {
        int checkempty2 = 0;
        gettimeofday(&now,NULL);
        //packetleavetimeQ1 = (now.tv_sec*1000000ULL + now.tv_usec);
        packetleavetimeQ1 = tsleepdiff(now);
        //printf("packetleavetimeQ1:- %lf\n",packetleavetimeQ1);
        pac1->timespentinQ1 = packetleavetimeQ1 - (pac1->timeenterQ1); //both pac->packetarrivaltime & pac->packetno
        totaltimespentinQ1 = totaltimespentinQ1 + pac1->timespentinQ1;
        My402ListUnlink(queue1,elem);

        tokencount = tokencount - (notokensmain);
        printpacketleave(packetleavetimeQ1,pac1->packetno,pac1->timespentinQ1,tokencount);
        tempcount = tempcount + 1;
        flagincrement = flagincrement + 1;
        
        //printf("Value of temp count is:- %d\n",tempcount);
        checkempty2 = My402ListEmpty(queue2);
        My402ListAppend(queue2,pac1);
        gettimeofday(&now,NULL);
        //packetarrivaltimeQ2 = (now.tv_sec*1000000ULL + now.tv_usec);
        packetarrivaltimeQ2 = tsleepdiff(now);
        //printf("packetarrivaltimeQ2:- %lf\n",packetarrivaltimeQ2);
        printpacketarrivalQ2(packetarrivaltimeQ2,pac1->packetno);
        if(checkempty2!=0)
        {
        	pthread_cond_broadcast(&qu2);
        }

    }
    pthread_mutex_unlock(&threadlock);
    //returnemptycheckq1 = My402ListEmpty(queue1);
	//packetcancel(tfilepresent,packetcount,flagincrement,returnemptycheckq1);
    
   }//for loop end
   //returnemptycheckq1 = My402ListEmpty(queue1);
		//packetcancel(tfilepresent,packetcount,flagincrement,returnemptycheckq1);
   if(My402ListEmpty(queue2))
	{
		//flag =1;
		pthread_cond_broadcast(&qu2);
	}
	pflag = 1;
   pthread_exit(NULL);
 }  
  
 if(tfilepresent==1)
 {
 	double tfileinterarrivaltime = 0;
 	for(i=0;i<noofpackets;i++)
    {
    	pac1 = malloc(sizeof(packet));
        fgets(buf,sizeof(buf),fp);
      { 
        if(sizeof(buf)>1024)
        {
          fprintf(stderr,"Size of line is very large\n");
          exit(0);
        }   
        sscanf(buf,"%lf %lf %lf %[^\t\n] ",&inter_arrival_time,&no_of_tokens,&service_time,temp);
       { 
       	countofpackets++;
        pac1->interarrivaltime = (inter_arrival_time*1000);  // this is the main cause of error has it will be stored completely, we need to extract every step of it
        pac1->nooftokens = no_of_tokens;
        pac1->servicetime = service_time;
        pac1->packetno = countofpackets;
       }
     }
    	
        
        gettimeofday(&now,NULL);
        timediff = tsleepdiff(now);
        
        
        packetsleeptime = (long)(pac1->interarrivaltime - timediff + previouspacketarrivaltime); //source of error
        //printf("packetsleeptime is :- %lf\n",packetsleeptime);
   
        //if(packetsleeptime<0)
          //usleep(0);
        //else
        long m = (long)(pac1->interarrivaltime);
        usleep(m);
        packetcount = packetcount + 1;
		
		
	    gettimeofday(&now,NULL);
		packettimeaftersleep = tsleepdiff(now); //packettimeaftersleep is nothing but timearrivalstamp
	    pac1->timeenterQ1 = packettimeaftersleep;
        pac1->interarrivaldifference  = pac1->timeenterQ1 - previouspacketarrivaltime;
        previouspacketarrivaltime = packettimeaftersleep;
	    
	    totalpacketinterarrivaltime = totalpacketinterarrivaltime + pac1->timeenterQ1;
        pthread_mutex_lock(&threadlock);

        if((pac1->nooftokens) >bucketdepth)
		{
			
			printdroppedpackets(pac1->timeenterQ1,pac1->packetno,pac1->nooftokens,pac1->interarrivaldifference);
			droppedpackets = droppedpackets + 1;
		    pthread_mutex_unlock(&threadlock);
		    free(pac1);
		    i++;
		    continue;
		}
		printpacketarrival(pac1->timeenterQ1,pac1->packetno,pac1->nooftokens,pac1->interarrivaldifference);
        
        My402ListAppend(queue1,pac1);
        
         gettimeofday(&now,NULL);
         packettimediff = tsleepdiff(now);
        

        printpacketenterqueue(packettimediff,pac1->packetno);
        elem = My402ListFirst(queue1);
        if(elem==NULL)
        {
        	pthread_mutex_unlock(&threadlock);
        	continue;
        }
       
        pac1 = (struct packet_parameters*)elem->obj;
        
        if(tokencount >= (pac1->nooftokens))
    {
        int checkempty2 = 0;
        gettimeofday(&now,NULL);
        packetleavetimeQ1 = tsleepdiff(now);
        //printf("packetleavetimeQ1:- %lf\n",packetleavetimeQ1);
        pac1->timespentinQ1 = packetleavetimeQ1 - (pac1->timeenterQ1); //both pac->packetarrivaltime & pac->packetno
        totaltimespentinQ1 = totaltimespentinQ1 + pac1->timespentinQ1;
        My402ListUnlink(queue1,elem);

        tokencount = tokencount - (pac1->nooftokens);
        printpacketleave(packetleavetimeQ1,pac1->packetno,pac1->timespentinQ1,tokencount);
        tempcount = tempcount + 1;
        flagincrement = flagincrement + 1;
        
        //printf("Value of temp count is:- %d\n",tempcount);
        checkempty2 = My402ListEmpty(queue2);
        My402ListAppend(queue2,pac1);
        gettimeofday(&now,NULL);
        //packetarrivaltimeQ2 = (now.tv_sec*1000000ULL + now.tv_usec);
        packetarrivaltimeQ2 = tsleepdiff(now);
        //printf("packetarrivaltimeQ2:- %lf\n",packetarrivaltimeQ2);
        printpacketarrivalQ2(packetarrivaltimeQ2,pac1->packetno);
        if(checkempty2!=0)
        {
        	pthread_cond_broadcast(&qu2);
        }

    }
    pthread_mutex_unlock(&threadlock);
    returnemptycheckq1 = My402ListEmpty(queue1);
	packetcancel(tfilepresent,packetcount,flagincrement,returnemptycheckq1); 
    
   }//for loop end
   returnemptycheckq1 = My402ListEmpty(queue1);
		packetcancel(tfilepresent,packetcount,flagincrement,returnemptycheckq1); 
   if(My402ListEmpty(queue2))
	{
		//flag =1;
		pthread_cond_broadcast(&qu2);
	}
	pflag = 1;
   pthread_exit(NULL);
   
 } 
  	
}
void printpacketleaveQ2(double timediff,long packetno,double timespentinQ2)
{
    fprintf(stdout,"%012.3fms: p%ld leaves Q2, time in Q2:%0.3lfms\n",timediff/1000,packetno,timespentinQ2/1000);

}
void printpacketenterS1(double timediff,long packetno,long servicetime)
{
	fprintf(stdout,"%012.3fms: p%ld begins service at S1,requesting :%ldms\n",timediff/1000,packetno,servicetime);
}

void printtimeinsystem(double timediff,long packetno,double servicetime,double systemtime)
{
	fprintf(stdout,"%012.3fms: p%ld departs from S1,service time = %0.3lfms, time in system = %0.3lfms\n ",timediff/1000,packetno,servicetime/1000,systemtime/1000);
}



void *serverfunction1(void *args) 
{
   struct timeval now;
   double startservertime = initialtime;
   packet *pacserv = NULL;
   pacserv = malloc(sizeof(packet));
   queue2 = malloc(sizeof(My402List));
   server1 = malloc(sizeof(My402List));
   queue1 = malloc(sizeof(My402List));
   double timediff = 0;
   double packetleaveQ2 = 0;
   double packetenterS1 = 0;
   double packetleavetimeS1 = 0;
   double totaltimeinsystem = 0;
   double packettimeaftersleepS1 = 0;
   long i;
   My402ListElem *elem = NULL;
   double muratetime = 1/(murate);
   long deterservicetime = (long)((1/(murate))*1000000ULL);
    
   if(tfilepresent==0) 
  {
  	for(i=0;i<deternoofpackets;i++) //code from this has been implemented with the help of Bill Cheng's Cancellation and Conditions slide from Thread Safety lecture
   
   {
      if(((long)(muratetime))>10)
      {
      	deterservicetime = 10;
      }
      
      pthread_mutex_lock(&threadlock);
  
      while(!(tempcount>0))
      {
      	if((My402ListEmpty(queue1)&&(pflag==1))||(flaginterrupt==1))
      	{
      		pthread_mutex_unlock(&threadlock);
      		pthread_exit(NULL);
          
      	}
      	
        pthread_cond_wait(&qu2,&threadlock);
        
      }
	 
      elem = My402ListFirst(queue2);
      if(elem == NULL)
        {
        	pthread_mutex_unlock(&threadlock);
        	break;
       }

       pacserv = (struct packet_parameters*)elem->obj;
       
       My402ListUnlink(queue2,elem);
       tempcount = tempcount - 1;       //the key variable in my function
       //printf("Value of tempcount in serverthreadS1 is:- %d\n",tempcount);
       pthread_mutex_unlock(&threadlock);

       gettimeofday(&now,NULL);
       packetleaveQ2 = tsleepdiff(now);
       pacserv->timeleaveQ2 = packetleaveQ2;
       pacserv->timespentinQ2 = pacserv->timeleaveQ2-pacserv->timeenterQ2;
       totaltimespentinQ2 = totaltimespentinQ2 + pacserv->timespentinQ2;
       //printf("Totaltimespent in Q2 %lf\n",totaltimespentinQ2);
       printpacketleaveQ2(pacserv->timeleaveQ2,pacserv->packetno,pacserv->timespentinQ2);

       
       My402ListAppend(server1,pacserv);
       
       gettimeofday(&now,NULL);
       packetenterS1 = tsleepdiff(now);
       printpacketenterS1(packetenterS1,pacserv->packetno,deterservicetime); //changes are required
     
       usleep(deterservicetime);

       gettimeofday(&now,NULL);
       packettimeaftersleepS1 = tsleepdiff(now);
       packetleavetimeS1 = packettimeaftersleepS1 - packetenterS1;
       totalservicetimeS1 = totalservicetimeS1 + packetleavetimeS1;
       totaltimeinsystem = packettimeaftersleepS1 - pacserv->timeenterQ1;
       totaltimefull = totaltimefull + totaltimeinsystem;
       timesquaresys = timesquaresys + (totaltimeinsystem*totaltimeinsystem);
       compackets = compackets + 1;
      
       printtimeinsystem(packettimeaftersleepS1,pacserv->packetno,packetleavetimeS1,totaltimeinsystem);
    
		pthread_mutex_unlock(&threadlock);
      
    }
    pthread_exit(NULL);
  }
 if(tfilepresent==1)
 {
   for(i=0;i<noofpackets;i++) //code from this has been implemented with the help of Bill Cheng's Cancellation and Conditions slide from Thread Safety lecture
   
   {
      
      
      pthread_mutex_lock(&threadlock);
  
      while(!(tempcount>0))
      {
      	if((My402ListEmpty(queue1)&&(pflag==1))||(flaginterrupt==1))
      	{
      		pthread_mutex_unlock(&threadlock);
      		pthread_exit(NULL);
          
      	}
      	
        pthread_cond_wait(&qu2,&threadlock);
        
      }
	 
      elem = My402ListFirst(queue2);
      if(elem == NULL)
        {
        	pthread_mutex_unlock(&threadlock);
        	break;
       }

       pacserv = (struct packet_parameters*)elem->obj;
       
       My402ListUnlink(queue2,elem);
       tempcount = tempcount - 1;       //the key variable in my function
       //printf("Value of tempcount in serverthreadS1 is:- %d\n",tempcount);
       pthread_mutex_unlock(&threadlock);

       gettimeofday(&now,NULL);
       packetleaveQ2 = tsleepdiff(now);
       pacserv->timeleaveQ2 = packetleaveQ2;
       pacserv->timespentinQ2 = pacserv->timeleaveQ2-pacserv->timeenterQ2;
       totaltimespentinQ2 = totaltimespentinQ2 + pacserv->timespentinQ2;
       //printf("Totaltimespent in Q2 %lf\n",totaltimespentinQ2);
       printpacketleaveQ2(pacserv->timeleaveQ2,pacserv->packetno,pacserv->timespentinQ2);

       
       My402ListAppend(server1,pacserv);
       long tfileservicetime = (long)((pacserv->servicetime)*1000);
       gettimeofday(&now,NULL);
       packetenterS1 = tsleepdiff(now);
       printpacketenterS1(packetenterS1,pacserv->packetno,tfileservicetime); //changes are required
     
       usleep(tfileservicetime);

       gettimeofday(&now,NULL);
       packettimeaftersleepS1 = tsleepdiff(now);
       packetleavetimeS1 = packettimeaftersleepS1 - packetenterS1;
       totalservicetimeS1 = totalservicetimeS1 + packetleavetimeS1;
       totaltimeinsystem = packettimeaftersleepS1 - pacserv->timeenterQ1;
       totaltimefull = totaltimefull + totaltimeinsystem;
       timesquaresys = timesquaresys + (totaltimeinsystem*totaltimeinsystem);
       compackets = compackets + 1;
      
       printtimeinsystem(packettimeaftersleepS1,pacserv->packetno,packetleavetimeS1,totaltimeinsystem);
    
		pthread_mutex_unlock(&threadlock);
      
    }
    pthread_exit(NULL);

 }  	
    
 }
void printpacketenterS2(double timediff,long packetno,long servicetime)
{
	fprintf(stdout,"%012.3fms: p%ld begins service at S2,requesting :%ldms\n",timediff/1000,packetno,servicetime);
}

void printtimeinsystem2(double timediff,long packetno,double servicetime,double systemtime)
{
	fprintf(stdout,"%012.3fms: p%ld departs from S2,service time = %0.3lfms, time in system = %0.3lfms\n ",timediff/1000,packetno,servicetime/1000,systemtime/1000);
}
void *serverfunction2(void *args) 
{
   struct timeval now;
   double startservertime = initialtime;
   packet *pacserv2 = NULL;
   pacserv2 = malloc(sizeof(packet));
   queue2 = malloc(sizeof(My402List));
   queue1 = malloc(sizeof(My402List));
   server2 = malloc(sizeof(My402List));
   double timediff = 0;
   double packetleaveQ2 = 0;
   double packetenterS2 = 0;
   double packetleavetimeS2 = 0;
   double totaltimeinsystem = 0;
   double packettimeaftersleepS2 = 0;
   long i;
   My402ListElem *elem = NULL;

    double deterservicetime = 0;
    
  if(tfilepresent==0)
  {
  	for(i=0;i<deternoofpackets;i++)  //code from this has been implemented with the help of Bill Cheng's Cancellation and Conditions slide from Thread Safety lecture
   {
      

   	  pthread_mutex_lock(&threadlock);
      while(!(tempcount>0))
      {
      	if((My402ListEmpty(queue1)&&(pflag==1))||(flaginterrupt==1))
      	{
      		pthread_mutex_unlock(&threadlock);
      		pthread_exit(NULL);
          
      	}
        pthread_cond_wait(&qu2,&threadlock);
        
      }
	 
      elem = My402ListFirst(queue2);
      if(elem == NULL)
        {
        	pthread_mutex_unlock(&threadlock);
        	break;
       }

       pacserv2 = (struct packet_parameters*)elem->obj;
       
       My402ListUnlink(queue2,elem);
       tempcount = tempcount - 1;       //the key variable in my function
       //printf("Value of tempcount in serverthreadS2 is:- %d\n",tempcount);
       pthread_mutex_unlock(&threadlock);

       gettimeofday(&now,NULL);
       packetleaveQ2 = tsleepdiff(now);
       pacserv2->timeleaveQ2 = packetleaveQ2;
       pacserv2->timespentinQ2 = pacserv2->timeleaveQ2-(pacserv2->timeenterQ2);
       totaltimespentinQ2 = totaltimespentinQ2 + (pacserv2->timespentinQ2);
       //printf("Totaltimespent in Q2 %lf\n",totaltimespentinQ2);
       printpacketleaveQ2(pacserv2->timeleaveQ2,pacserv2->packetno,pacserv2->timespentinQ2);

       
       My402ListAppend(server2,pacserv2);
       long deterservicetime = (long)((1/(murate))*1000000ULL);
       gettimeofday(&now,NULL);
       packetenterS2 = tsleepdiff(now);
       printpacketenterS2(packetenterS2,pacserv2->packetno,deterservicetime); //changes are required
     
       usleep(deterservicetime);

       gettimeofday(&now,NULL);
       packettimeaftersleepS2 = tsleepdiff(now);
       packetleavetimeS2 = packettimeaftersleepS2 - packetenterS2;
       totalservicetimeS2 = totalservicetimeS2 + packetleavetimeS2;
       totaltimeinsystem = packettimeaftersleepS2 - pacserv2->timeenterQ1;
       totaltimefull = totaltimefull + totaltimeinsystem;
       timesquaresys = timesquaresys + (totaltimeinsystem*totaltimeinsystem);
       compackets = compackets + 1;
      
       printtimeinsystem2(packettimeaftersleepS2,pacserv2->packetno,packetleavetimeS2,totaltimeinsystem);
    
      
      int returnemptycheck = My402ListEmpty(queue2);
		pthread_mutex_unlock(&threadlock);
    }
    pthread_exit(NULL);
  }
  if(tfilepresent==1)
  {
  	for(i=0;i<noofpackets;i++)  //code from this has been implemented with the help of Bill Cheng's Cancellation and Conditions slide from Thread Safety lecture
   {
      //printf("serverthreadstart2\n");

   	  pthread_mutex_lock(&threadlock);
      while(!(tempcount>0))
      {
      	if((My402ListEmpty(queue1)&&(pflag==1))||(flaginterrupt==1))
      	{
      		pthread_mutex_unlock(&threadlock);
      		pthread_exit(NULL);
          
      	}
        pthread_cond_wait(&qu2,&threadlock);
        
      }
	 
      elem = My402ListFirst(queue2);
      if(elem == NULL)
        {
        	pthread_mutex_unlock(&threadlock);
        	break;
       }

       pacserv2 = (struct packet_parameters*)elem->obj;
       
       My402ListUnlink(queue2,elem);
       tempcount = tempcount - 1;       //the key variable in my function
       //printf("Value of tempcount in serverthreadS2 is:- %d\n",tempcount);
       pthread_mutex_unlock(&threadlock);

       gettimeofday(&now,NULL);
       packetleaveQ2 = tsleepdiff(now);
       pacserv2->timeleaveQ2 = packetleaveQ2;
       pacserv2->timespentinQ2 = pacserv2->timeleaveQ2-(pacserv2->timeenterQ2);
       totaltimespentinQ2 = totaltimespentinQ2 + (pacserv2->timespentinQ2);
       //printf("Totaltimespent in Q2 %lf\n",totaltimespentinQ2);
       printpacketleaveQ2(pacserv2->timeleaveQ2,pacserv2->packetno,pacserv2->timespentinQ2);

       
       My402ListAppend(server2,pacserv2);
       long deterservicetime = (long)((pacserv2->servicetime)*1000);
       gettimeofday(&now,NULL);
       packetenterS2 = tsleepdiff(now);
       printpacketenterS2(packetenterS2,pacserv2->packetno,deterservicetime); //changes are required
     
       usleep(deterservicetime);

       gettimeofday(&now,NULL);
       packettimeaftersleepS2 = tsleepdiff(now);
       packetleavetimeS2 = packettimeaftersleepS2 - packetenterS2;
       totalservicetimeS2 = totalservicetimeS2 + packetleavetimeS2;
       totaltimeinsystem = packettimeaftersleepS2 - pacserv2->timeenterQ1;
       totaltimefull = totaltimefull + totaltimeinsystem;
       timesquaresys = timesquaresys + (totaltimeinsystem*totaltimeinsystem);
       compackets = compackets + 1;
      
       printtimeinsystem2(packettimeaftersleepS2,pacserv2->packetno,packetleavetimeS2,totaltimeinsystem);
    
      
      int returnemptycheck = My402ListEmpty(queue2);
		pthread_mutex_unlock(&threadlock);
    }
    pthread_exit(NULL);
  }  
    
 }





void calculatestatistics()
{
   fprintf(stdout,"Statistics:\n");	
   double averagetimespentinsystm = (totaltimefull)/(compackets);  // need to define compelete packets;
   double avgnoofpacketsQ1 = totaltimespentinQ1/totaltime;
   double avgnoofpacketsQ2 = totaltimespentinQ2/totaltime;
   double avgnoofpacketsS1 = totalservicetimeS1 / totaltime;
   double avgnofpacketsS2 =  totalservicetimeS2 / totaltime;
   double avgtimesquaresys = timesquaresys/(compackets);

   double avgpacketinterarrivaltime = totalpacketinterarrivaltime / (packetcount);  //deterministic has different name
   double avgpacketservicetime = (totalservicetimeS1 + totalservicetimeS2)  / (compackets) ; 

   double standardeviation = sqrt(avgtimesquaresys - pow(averagetimespentinsystm,2));
   //printf("Dropped tokens is:- %d\n",droppedtokens);
   //printf("Tokencount is:- %d\n",tokencount);
   
   

   
     if(packetcount == 0)
     fprintf(stdout,"average packet inter-arrival time = 0\n");
    
     if(compackets == 0)
     fprintf(stdout,"average packet service time = 0\n\n\n");
     
     if(packetcount!=0)
     fprintf(stdout,"average packet inter-arrival time = %0.6gsec\n",avgpacketinterarrivaltime/(1000000L));
     
     if(compackets != 0)
     fprintf(stdout,"average packet service time = %0.6gsec\n\n\n",avgpacketservicetime/(1000000L));

     if(totaltime == 0 )
     {
      fprintf(stdout,"average number of packets in Q1=0\n");
      fprintf(stdout,"average number of packets in Q2=0\n");
      fprintf(stdout,"average number of packets at S1=0\n");
      fprintf(stdout,"average number of packets at S2=0\n\n\n"); //dont forget to add these numbers in S2 as well
    }
    if(totaltime !=0)
    {
      fprintf(stdout,"average number of packets in Q1=%0.6g\n",avgnoofpacketsQ1);
      fprintf(stdout,"average number of packets in Q2=%0.6g\n",avgnoofpacketsQ2);
      fprintf(stdout,"average number of packets at S1=%0.6g\n",avgnoofpacketsS1);
      fprintf(stdout,"average number of packets at S2=%0.6g\n\n\n",avgnofpacketsS2); //dont forget to add these numbers in S2 as well
    }
    if(compackets == 0)
    { 	
     fprintf(stdout,"average time a packet spent in system = 0sec\n");
     fprintf(stdout,"standard deviation for time spent in system = 0sec\n");
    }
    if(compackets != 0)
    { 
     fprintf(stdout,"average time a packet spent in system = %0.6gsec\n",averagetimespentinsystm/(1000000L));
     fprintf(stdout,"standard deviation for time spent in system = %0.6gsec\n\n\n",standardeviation/(1000000L));
    }
    if(tokencount == 0)
    fprintf(stdout,"token drop probability = 0\n");
     
    if(packetcount == 0)
    fprintf(stdout,"packet drop probability = 0sec\n\n\n");
 
    if(tokencount != 0)
    {
      double tokendropprobability = (double)(droppedtokens/tokencount);	
    fprintf(stdout,"token drop probability = %0.6g\n",tokendropprobability);
    }
    if(packetcount != 0)
   {
    double packetdropprobability = (double)(droppedpackets/packetcount); 	
    fprintf(stdout,"packet drop probability = %0.6g\n\n\n",packetdropprobability);
    }
}

void cleanup_routine()
{
	flaginterrupt = 1;
	My402ListElem *elem;
    packet *pacrem = NULL;
    //pthread_mutex_lock(&threadlock);
	for(elem=My402ListFirst(queue1);elem!=NULL;elem=My402ListNext(queue1,elem))
	{
		pacrem = malloc(sizeof(packet));
        pacrem = (struct packet_parameters*)elem->obj;
        pthread_mutex_lock(&threadlock);
		fprintf(stdout,"p%ld removed from Q1\n",pacrem->packetno);
		
		pthread_mutex_unlock(&threadlock);
		
	}
	for(elem=My402ListFirst(queue2);elem!=NULL;elem=My402ListNext(queue2,elem))
	{
	    pacrem = malloc(sizeof(packet));
        pacrem = (struct packet_parameters*)elem->obj;
        pthread_mutex_lock(&threadlock);
       fprintf(stdout,"p%ld removed from Q2\n",pacrem->packetno);
       pthread_mutex_unlock(&threadlock);
      
	}
    My402ListUnlinkAll(queue1);
    My402ListUnlinkAll(queue2);
    
    //printf("Get lost monkey\n");
	pthread_mutex_unlock(&threadlock);
	//printf("Get lost donkey\n");
	

}

void *signalhandler(void *args)
{   
	//flaginterrupt = 1;
	int signal = 0;
	pthread_cleanup_push(cleanup_routine,NULL);
	//flaginterrupt = 1;
	int sigwaitret = sigwait(&set,&signal);
	if(sigwaitret !=0)
	{
       fprintf(stderr,"Interrupt function has failed\n");

	}
    pthread_mutex_lock(&threadlock);
    fprintf(stdout,"\n");
    fprintf(stdout,"SIGINT caught,no new packets or tokens will be allowed\n");
    flaginterrupt = 1;
    pthread_cancel(tokenbucket);
    pthread_cancel(q1);
    //pthread_cancel(s1);
    //pthread_cancel(s2);
   
   
      //int returnemptycheck = My402ListEmpty(queue2);
      if(My402ListEmpty(queue2))
        { 
         pthread_cond_broadcast(&qu2);
        }
    pflag = 1;
    pthread_mutex_unlock(&threadlock);
    //printf("hello\n");
    pthread_exit(NULL);
    pthread_cleanup_pop(0);
}

void check(double value)
{
   if(value<0 || value>maxsize)
   {
   	 fprintf(stdout,"Value is either less than zero or greater than the maximum value\n");
   	 exit(0);
   }
}


void real_value()
{
   fprintf(stdout,"Value is not a real value, check the input again\n");
   exit(0);
}
void emustart()
{
  double starttime = 0; 
  fprintf(stdout,"%012.3fms: emulation begins\n",starttime);
}



void emulation_output(int result,double ratetoken,long bucketdepth,double lm,double mu,long n,long p,char *filename)
{
    if(result==0)
    {
    	printf("Emulation parameters:\n\tnumber to arrive = %ld\n\tlambda = % 0.6g\n\tmu = %0.6g\n\tr = %0.6g\n\tB = %ld\n\tP = %ld\n",n,lm,mu,ratetoken,bucketdepth,p);
    }
    if(result==1)
    {
    	printf("Emulation parameters:\n\tr = %0.1lf\n\tB = %ld\n\ttsfile = %s\n",ratetoken,bucketdepth,filename);
    }

}


int main(int argc, char *argv[])
{
   int totalargument  = argc;
   char *filename;
   My402List *list = malloc(sizeof(My402List));
   packet *pac = NULL;
   pac = malloc(sizeof(packet));
   
   double lm,mu,p;
   long n;
   struct stat dir;
   
   gettimeofday(&initial,NULL);
  
   if(argc<3)
   {
   	 fprintf(stdout,"Command lines are less\n");
   	 exit(-1);
   }
 
  if(argc==3)
  {
  	int tresult = strcmp(argv[1],"-t");
      int lresult = strcmp(argv[1],"-lambda");
      int mresult = strcmp(argv[1],"-mu");
      int nresult = strcmp(argv[1],"-n");
      int rresult = strcmp(argv[1],"-r");
      int bresult = strcmp(argv[1],"-B");
      int presult = strcmp(argv[1],"-P");

  	if(tresult == 0)
  	{  
  		tfilepresent=1;
  		if(stat(argv[2],&dir)==0)
  		{
  			if(dir.st_mode & S_IFDIR)
  			{
  				fprintf(stderr,"The 3rd argument is a directory and needs to be a file\n");
  				exit(-1);
  			}
  		}	
  		filename = argv[2];
  		fp=fopen(argv[2],"r");
  		if(fp==NULL)
  		{
  			fprintf(stdout,"Error in opening the file\n");
  			exit(0);
  		}
  		readnoofpacket(fp);

  	}
  	if(lresult!=0 && mresult!=0 && nresult !=0 && rresult !=0 && bresult !=0 && presult !=0 && tresult!=0)
  	{
  		fprintf(stdout,"First argument is not correct\n");
  	    exit(-1);
  	}
  	if(argv[2][0] =='-')
  	{
  		fprintf(stdout,"Input is not a real value\n");
  		exit(-1);
  	}

  }
 
 

if(argc>3)
{
  
  long i;
  i=1;
  while(i<argc)
   {
      int tresult = strcmp(argv[i],"-t");
      int lresult = strcmp(argv[i],"-lambda");
      int mresult = strcmp(argv[i],"-mu");
      int nresult = strcmp(argv[i],"-n");
      int rresult = strcmp(argv[i],"-r");
      int bresult = strcmp(argv[i],"-B");
      int presult = strcmp(argv[i],"-P");
      
     if(lresult!=0 && mresult!=0 && nresult !=0 && rresult !=0 && bresult !=0 && presult !=0 && tresult!=0)
  	{
  		fprintf(stdout,"This is a wrong input argument\n");
  	    exit(-1);
  	}

      if(tresult==0)
      { 
      	 tfilepresent=1;

      	int m=i+1;
      	if(m>=totalargument)
   	   {
   	   	fprintf(stderr,"The value is NULL\n");
   	   	exit(0);
   	   }
      	filename = argv[m];
        fp=fopen(argv[m],"r");
  		if(fp==NULL)
  		{
  			fprintf(stdout,"Error in opening the file\n");
  			exit(0);
  		}
  		readnoofpacket(fp);
  	  }
      if(rresult==0)
      {
      	 
      	 int m=i+1;
      	 if(m>=totalargument)
   	   {
   	   	fprintf(stderr,"The value is NULL\n");
   	   	exit(0);
   	   }
      	  if(argv[m][0]=='-')
      	  {
      	  	 real_value();
      	  }
         ratetoken=atof(argv[m]);
         //check for the condition

      }
      if(bresult==0)
      {
      	int m=i+1;
      	if(m>=totalargument)
   	   {
   	   	fprintf(stderr,"The value is NULL\n");
   	   	exit(0);
   	   }
      	if(argv[i+1][0]=='-')
      	real_value();
      	  	
        bucketdepth=atoi(argv[m]);
        check(bucketdepth);

      }
    if(lresult==0)
    {
    	char *typeinput="-lambda";
    	 int m=i+1;
    	 if(m>=totalargument)
   	   {
   	   	fprintf(stderr,"The value is NULL\n");
   	   	exit(0);
   	   }
    	if(argv[m][0]=='-')
      	  {
      	  	 real_value();
      	  }
        lm = atof(argv[m]);
        lambda = (double)lm;
    
    }
   if(mresult==0)
   {
   	   
   	   int m=i+1;
   	   if(m>=totalargument)
   	   {
   	   	fprintf(stderr,"The value is NULL\n");
   	   	exit(0);
   	   }
       if(argv[m][0]==('-'))
       real_value();
       
       mu=atof(argv[m]);
       murate = mu;
       

   }    
   if(nresult==0)
   {
     int m=i+1;
     if(m>=totalargument)
   	   {
   	   	fprintf(stderr,"The value is NULL\n");
   	   	exit(0);
   	   }
   	   if(argv[m][0]=='-')
      	real_value();
        n = atof(argv[m]);
        check(n);
        deternoofpackets = (long)n;
        
   }
   if(presult==0)
   {
   	  char *typeinput="-P";
   	 int m=i+1;
   	 if(m>=totalargument)
   	   {
   	   	fprintf(stderr,"The value is NULL\n");
   	   	exit(0);
   	   }
   	  if(argv[m][0]=='-')
      real_value();
      p = atoi(argv[m]);
      check(p);
      notokensmain = (long)p;
        //printf("No of tokens is : %ld\n",notokensmain);
        //pac->nooftokens = p;
        //value_insert(typeinput,p);

   }
  i=i+2;
  
 }
}
if(bucketdepth == 0)
{
	bucketdepth = 10;
}
if(ratetoken==0)
    {
    	ratetoken= (double)1.5;
     }
   if(lm==0)
    {
    	lambda = (1/0.5)*1000;
    	
    }
  if(mu == 0)
    {
    	 murate = (1/0.35)*1000;
    	
    }
   if(n==0)
   {
   	 
   	 deternoofpackets = 20;
   }
   if(notokensmain==0)
   {
   	notokensmain = 3;
   	//pac->nooftokens = 3;
   }
emulation_output(tfilepresent,ratetoken,bucketdepth,lambda,murate,deternoofpackets,notokensmain,filename);

if(argc>15)
  {
  	fprintf(stdout,"Too many arguments\n");
  	exit(-1);
  }


emustart();
sigemptyset(&set);
sigaddset(&set, SIGINT);
sigprocmask(SIG_BLOCK,&set,0);


pthread_create(&tokenbucket,NULL,tokgen,NULL);

pthread_create(&q1,NULL,packgen,NULL);

pthread_create(&s1,NULL,serverfunction1,NULL);

pthread_create(&s2,NULL,serverfunction2,NULL);

pthread_create(&interrupt,NULL,signalhandler,NULL);



pthread_join(tokenbucket,NULL);
pthread_join(q1,NULL);
pthread_join(s1,NULL);
pthread_join(s2,NULL);


if(flaginterrupt == 0)
{
	pthread_cancel(interrupt);
}

pthread_join(interrupt,NULL);

fprintf(stdout,"emulation ends\n");
struct timeval end;
gettimeofday(&end,NULL);
totaltime = tsleepdiff(end);

calculatestatistics();


return (0);
}
