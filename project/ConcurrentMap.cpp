#include <iostream>
#include <bits/stdc++.h>
#include <mutex>
#include <unistd.h>
#include <pthread.h>
#include<sys/time.h>
#include<fstream>

using namespace std;

double l;
int n,p;
ofstream output;
default_random_engine generator;
exponential_distribution<double> distribution(l);
map<int,int> sharedMap;
pthread_mutex_t* OwnershipArray[100];
pthread_mutex_t idCounterLock, WriteToFile,measureLock;
int globalId = 0;
double TotalcommitTime = 0;
int RestartCount = 0;


string convertTime(time_t epoch_time){
	tm *t = localtime(&epoch_time);
	string ct = to_string(t->tm_hour)+":"+to_string(t->tm_min)+":"+to_string(t->tm_sec);
	return ct;
}

int begin_trans() {      //begin transaction and returns an unique id
    pthread_mutex_lock(&idCounterLock);
    globalId++;
    int localId = globalId;
    pthread_mutex_unlock(&idCounterLock);
    return localId;
}

void ReleaseLocks(vector<int> list) {
	for(int i=0;i<list.size();i++) {
		pthread_mutex_unlock(OwnershipArray[list[i]]);
	}
	return;
}

bool Commit(vector<int> L) {
	ReleaseLocks(L);
}

void Acquire(int location) {
	pthread_mutex_lock(OwnershipArray[location]);
	return ;
}

int TryAcquire(int location) {
	if(pthread_mutex_trylock(OwnershipArray[location])==0) {
		return 1;
	}
	else {
		return 0;
	}
}

void Abort(vector<int> L) {
    // take relevant args and do cleanup
	ReleaseLocks(L);
}

int hash_(int val) {
    return val%p;
}

int Acess(int id, int variable, struct timeval AcessTime, int updtVal) {
	int oper = rand()%2;
	if(oper==0) { //read only
		pthread_mutex_lock(&WriteToFile);
		output<<"Thread id "<<pthread_self()<<" Transaction id "<<id<<" reads from location "<<variable<<" value "<<sharedMap[variable]<<" at "<<convertTime(AcessTime.tv_sec)<<"\n";
		pthread_mutex_unlock(&WriteToFile);
		return -1;
	}
	else { // read and write
		pthread_mutex_lock(&WriteToFile);
		output<<"Thread id "<<pthread_self()<<" Transaction id "<<id<<" reads from location "<<variable<<" value "<<sharedMap[variable]<<" at "<<convertTime(AcessTime.tv_sec)<<"\n";
		pthread_mutex_unlock(&WriteToFile);
		int locVal = sharedMap[variable];
		sharedMap[variable]+=updtVal;
		output<<"Thread id "<<pthread_self()<<" Transaction id "<<id<<" writes to location "<<variable<<" value "<<sharedMap[variable]<<" at "<<convertTime(AcessTime.tv_sec)<<"\n";
		return locVal;
	}
}

void Rollback(map<int,int> writeSet) {
	for(auto itr = writeSet.begin(); itr != writeSet.end(); ++itr) {
		if(itr->second != -1) {
			sharedMap[itr->first] = itr->second;
		}
	}
	return;
}

void *TransactionTest(void* arg) {
	int m = *((int*)(&arg));
	struct timeval StartTime,CommitTime,AcessTime;
	gettimeofday(&StartTime,NULL);
	int tx_id = begin_trans();
	map<int,int> accessedLocations;
	vector<int> L;
	map<int,int> writeSet;
	int iters = rand()%20;	
	int restarts = 0;
	for(int i = 0; i < m; i++) {
		accessedLocations[i] = 0;
	}
	for(int i = 0; i < iters ;i++) {
		int r = rand()%m;
		if(accessedLocations[r]) {
			continue;
		}
		else {
			accessedLocations[r] = 1;
			int hx = hash_(r);
			vector<int> M;
			vector<int>::iterator k = find(L.begin(), L.end(), hx);
			if(k==L.end()) {
				for(auto it=L.begin();it!=L.end();it++) {
					if(*it>hx) {
						M.push_back(*it);
					}
				}
				sort(M.begin(), M.end());
				L.push_back(hx);
				if(M.size()==0) {
					Acquire(hx);
				}
				else if(!TryAcquire(hx)) {
					Rollback(writeSet);
					Abort(M);
					Acquire(hx);
					for(int j=0;j<M.size();j++) {
						Acquire(M[j]);
					}
					pthread_mutex_lock(&WriteToFile);
					output <<"Restarting transaction - "<<tx_id<< "\n";
					restarts++;
					pthread_mutex_unlock(&WriteToFile);
				}
			}
		}
		gettimeofday(&AcessTime,NULL);
		writeSet[r] = Acess(tx_id, r, AcessTime, rand()%10);
		float randTime = distribution(generator);
        usleep(randTime*1000000);
	}
	Commit(L);
	gettimeofday(&CommitTime,NULL);
	pthread_mutex_lock(&WriteToFile);
	output << "Transaction "<<tx_id<<" commited" << "\n";
	pthread_mutex_unlock(&WriteToFile);
	pthread_mutex_lock(&measureLock);
    RestartCount+=restarts;
    TotalcommitTime += CommitTime.tv_sec - StartTime.tv_sec + CommitTime.tv_usec/1000000.0 - StartTime.tv_usec/1000000.0;
    pthread_mutex_unlock(&measureLock);
}

int main() {
	int numTran;
	ifstream input;
	input.open("inp-params.txt");
    input>>numTran;
    input>>n;
    input>>p;
    input>>l;
	// cout<<n<<" "<<numTran<<" "<<p<<" "<<l;
	for(int i = 0 ; i < n; i++) {
		sharedMap[i] = rand()%50;
	}
    pthread_mutex_init(&idCounterLock, NULL);
    pthread_mutex_init(&WriteToFile, NULL);
	pthread_mutex_init(&measureLock, NULL);
	for(int i = 0 ; i < p; i++) {
		OwnershipArray[i] = new pthread_mutex_t;
		pthread_mutex_init(OwnershipArray[i], NULL);
	}
	output.open("log.txt");
	pthread_t tId[numTran];
	for(int i = 0 ; i < numTran; i++) {
		pthread_create(&tId[i],NULL,TransactionTest,(void*)100);
	} 
	for(int i = 0 ; i < numTran ; i++){
		pthread_join(tId[i],NULL);
	}
	cout<<"Avg Commit time - "<<TotalcommitTime/numTran<<"\n";
	cout<<"Avg abort count - "<<float(RestartCount)/numTran<<"\n";
}