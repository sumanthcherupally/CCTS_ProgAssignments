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
map<int,vector<int> > writeList;
map<int,map<int,int> > varVersions; //[variable][tx_id][value]
ofstream output;
pthread_mutex_t idCounterLock,varLock,printLock,measureLock;
map<int,int> maxRscheduled;
map<int,int> status;  //0->live 1->abort 2->commited
vector<int> temp;
int m;//Number of variables
int numThreads;//Number of fixed threads
int numTrans;//Number of transactions per thread
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
    for(int i=0;i<m;i++)
    {
        for(int j=0;j<writeList[i].size();j++)
        {
            if(status(writeList[i][j])==2)
            {
                if(writeList[i][j+1]>writeList[i][j-1])
                {
                    if(status[writeList[i][j+1]]==2)
                    {
                        status.erase(writeList[i][j]);
                        writeList[i].erase(j);
                    }
                }
            }
        }
    }
}
bool read(int tx_id,int data_item, int* ptrLocVal)//reads the data-item value into local variable for a tx
{
    pthread_mutex_lock(&varLock);
    int prevGreatest = -1;
    for(int i=0;i<writeList[data_item].size();i++)
    {
        if(writeList[data_item][i]<=tx_id)
        {
            if(writeList[data_item][i]>prevGreatest)//first one to find is not the next highest txid
            {
                prevGreatest = writeList[data_item][i];
            }
        }
    }
    if(prevGreatest == -1)
    {
        status[tx_id] = 1;
        cleanup(tx_id);
        pthread_mutex_unlock(&varLock);
        return 0;
    }
    if(tx_id>maxRscheduled[data_item])
    {
        maxRscheduled[data_item] = tx_id;
    }
    *ptrLocVal = varVersions[data_item][prevGreatest];
    pthread_mutex_unlock(&varLock);
    return 1;
}

bool write(int tx_id,int data_item)//writes the data-item to the new value of the local variable for a tx
{
    pthread_mutex_lock(&varLock);
    if(maxRscheduled[data_item]>tx_id)
    {
        status[tx_id] = 1;
        cleanup(tx_id);
        pthread_mutex_unlock(&varLock);
        return 0;
    }
    writeList[data_item].push_back(tx_id);
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
            varVersions[i][tx_id] = commitValues[i];
        }
    }
    status[tx_id] = 2;
    cleanup(tx_id);
    pthread_mutex_unlock(&varLock);
    return 1;
}
void* updtMem(void* unused)
{
    int abortCount = 0;
    struct timeval critStartTime,critEndTime;
    for(int currTrans=0;currTrans<numTrans;currTrans++)
    {
        abortCount=0;
        int status = 0;
        do
        {
            gettimeofday(&critStartTime,NULL);
            status = 0;
            int id = begin_trans();
            int numIters = rand()%m;
            vector<int> varValues;
            for(int i=0;i<m;i++)
            {
                varValues.push_back(-1);
            }
            for(int i=0;i<numIters+1;i++)
            {
                //locVal = rand()%m;
                int locVal=-1;
                int randVariable = rand()%m;
                int randVal = rand()%constVal;
                randVal++;
                if(read(id,randVariable,&locVal)==0)
                {
                    status = 1;
                    break;
                }
                // cout<<locVal<<"\n";
                //varValues[randVariable] = locVal;
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
            // for(int i=0;i<m;i++)
            // {
            //     cout<<varValues[i]<<"\n";
            // }
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
        //record times
    }
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
    input>> numTrans;
    numTrans = numTrans/numThreads;
    input>>constVal;
    input>>l;
    temp.push_back(0);
    for(int i=0;i<m;i++)
    {
        writeList[i] = temp;
        maxRscheduled[i] = -1;
        varVersions[i][0]=0;
    }
    pthread_t tId[numThreads];
    output.open("MVTO-log.txt");
    for(int i=0;i<numThreads;i++)
    {
        pthread_create(&tId[i],NULL,updtMem,NULL);
    }
    for(int i=0;i<numThreads;i++)
    {
        pthread_join(tId[i],NULL);   
    }
    cout<<"Average commit time - "<<commitTime/(float)(numThreads*numTrans)<<"\n";
    cout<<"Average abort count - "<<abortCountGlobal/(float)(numThreads*numTrans)<<"\n";
    output.close();
    return 0;
}