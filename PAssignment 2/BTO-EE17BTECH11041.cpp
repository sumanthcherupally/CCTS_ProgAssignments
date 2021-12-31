#include<iostream>
#include<fstream>
#include<stdlib.h>
#include <unistd.h>
#include<pthread.h>
#include<sys/time.h>
#include<vector>
#include<math.h>
#include <algorithm>
#include<map>
using namespace std;

double l;
default_random_engine generator;
exponential_distribution<double> distribution(l);
ofstream output;
pthread_mutex_t idCounterLock,varLock,printLock,measureLock;
int maxRscheduled = 0,maxWscheduled = 0;
map<int,int> status; //0->live 1->abort 2->commited
map<int,int> varVersions; //[variable][tx_id][value]
int m;//Number of variables
int numThreads;//Number of fixed threads
int globalId = 0;//used for giving unique id's to transactions
int constVal;//used to generate random number for write operations
double commitTime = 0;
int abortCountGlobal = 0;

string convertTime(time_t epoch_time){
	tm *t = localtime(&epoch_time);
	string ct = to_string(t->tm_hour)+":"+to_string(t->tm_min)+":"+to_string(t->tm_sec);
	return ct;
}

int begin_trans(){//begin transaction and returns an unique id
    pthread_mutex_lock(&idCounterLock);
    globalId++;
    int localId = globalId;
    pthread_mutex_unlock(&idCounterLock);
    pthread_mutex_lock(&varLock);
    status[localId] = 0;
    pthread_mutex_unlock(&varLock);
    return localId;
}

void cleanup(int tx_id)
{
    //status.erase(tx_id);
}
bool read(int tx_id,int data_item,int* ptrLocVal)//reads the data-item value into local variable for a tx
{
    pthread_mutex_lock(&varLock);
    if(tx_id<maxWscheduled)
    {
        status[tx_id] = 1;
    }
    else
    {
        maxRscheduled = tx_id;
    }
    if(status[tx_id]==1)
    {
        cleanup(tx_id);
        pthread_mutex_unlock(&varLock);
        return 0;
    }
    *ptrLocVal = varVersions[data_item];
    pthread_mutex_unlock(&varLock);
    return 1;
}

bool write(int tx_id,int data_item)//writes the data-item to the new value of the local variable for a tx
{
    pthread_mutex_lock(&varLock);
    if(tx_id<maxRscheduled)
    {
        status[tx_id] = 1;
    }
    else if(tx_id<maxWscheduled)
    {
        status[tx_id] = 1;
    }
    else
    {
        maxWscheduled = tx_id;
    }
    if(status[tx_id]==1)
    {
        cleanup(tx_id);
        pthread_mutex_unlock(&varLock);
        return 0;
    }
    pthread_mutex_unlock(&varLock);
    return 1;
}
bool tryCommit(int tx_id, vector<int> commitValues)//returns 0 - abort or 1 - commit
{
    pthread_mutex_lock(&varLock);
    for(int i=0;i<m;i++)
    {
        if(commitValues[i]!=-1)
        {
            varVersions[i] = commitValues[i];
        }
    }
    cleanup(tx_id);
    pthread_mutex_unlock(&varLock);
    return 1;
}
void* updtMem(void* unused)
{
    int abortCount = 0;
    struct timeval critStartTime,critEndTime;
    abortCount=0;
    int status = 0;
    do
    {
        gettimeofday(&critStartTime,NULL);
        status = 0;
        int id = begin_trans();
        int numIters = rand()%m;
        // int locVal;
        vector<int> varValues;
        for(int i=0;i<m;i++)
        {
            varValues.push_back(-1);
        }
        for(int i=0;i<numIters;i++)
        {
            int locVal = -1;
            int randVariable = rand()%m;
            int randVal = rand()%constVal;
            randVal++;
            if(read(id,randVariable,&locVal)==0)
            {
                status = 1;
                break;
            }
            struct timeval readTime,writeTime;
            gettimeofday(&readTime,NULL);
            pthread_mutex_lock(&printLock);
            output<<"Thread id "<<pthread_self()<<" Transaction id "<<id<<" reads from "<<randVariable<<" value "<<locVal<<" at "<<convertTime(readTime.tv_sec)<<"\n";
            pthread_mutex_unlock(&printLock);
            locVal+= randVal;
            //randVariable = rand()%m;
            if(write(id,randVariable)==0)
            {
                status = 1;
                break;
            }
            varValues[randVariable] = locVal;
            gettimeofday(&writeTime,NULL);
            pthread_mutex_lock(&printLock);
            output<<"Thread id "<<pthread_self()<<" Transaction id "<<id<<" writes to "<<randVariable<<" value "<<locVal<<" at "<<convertTime(writeTime.tv_sec)<<"\n";
            pthread_mutex_unlock(&printLock);
            float randTime = distribution(generator);
            usleep(randTime*1000000);
        }
        string printStatus;
        if(status!=1)
        {
            if(tryCommit(id,varValues)==1)
            {
                status = 2;
                printStatus = "commit";
            }
            else
            {
                status = 1;
            }
        }
        if(status==1)
        {
            printStatus = "abort";
            abortCount++;
        }
        struct timeval commitTime;
        gettimeofday(&commitTime,NULL);
        pthread_mutex_lock(&printLock);
        output<<"Transaction id "<<id<<" tries to commit with result "<<printStatus<<" at "<<convertTime(commitTime.tv_sec)<<"\n";
        pthread_mutex_unlock(&printLock);
    }
    while(status!=2);
    gettimeofday(&critEndTime,NULL);
    pthread_mutex_lock(&measureLock);
    abortCountGlobal+=abortCount;
    commitTime += critEndTime.tv_sec - critStartTime.tv_sec + critEndTime.tv_usec/1000000.0 - critStartTime.tv_usec/1000000.0;
    pthread_mutex_unlock(&measureLock);
}
int main(){
    srand (time(NULL));
    pthread_mutex_init(&varLock, NULL);
    pthread_mutex_init(&idCounterLock, NULL);
    pthread_mutex_init(&printLock, NULL);
    pthread_mutex_init(&measureLock, NULL);
    ifstream input;
    input.open("inp-params.txt");
    input>>numThreads;
    input>>m;
    input>>constVal;
    input>>l;
    pthread_t tId[numThreads];
    output.open("BTO-log.txt");
    for(int i=0;i<m;i++)
    {
        varVersions[i]=0;
    }
    for(int i=0;i<numThreads;i++)
    {
        pthread_create(&tId[i],NULL,updtMem,NULL);
    }
    for(int i=0;i<numThreads;i++)
    {
        pthread_join(tId[i],NULL);   
    }
    cout<<"Average commit time - "<<commitTime/(float)(numThreads)<<"\n";
    cout<<"Average abort count - "<<abortCountGlobal/(float)(numThreads)<<"\n";
    output.close();
    return 0;
}