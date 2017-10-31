#include <iostream>
#include <string>
#include <set>
#include <map>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#define NUM_THREAD 26

using namespace std;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
multimap<size_t,string> result;
/*pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static map<size_t,set<string>> result;

vector<set<string>::iterator> beg_words;
vector<int> nof_words;
int thread_ret[NUM_THREAD];
*/

struct targs{
		set<string>::iterator wl_beg;
		set<string>::iterator wl_end;
};
void* ThreadFunc(void* arg);
string buf;


int main(){
    int N, index_vec=0;
    char cmd;

	vector<set<string>> words;
	for(int i = 0 ; i < NUM_THREAD ; i++){
		set<string> se;
		words.push_back(se);
	}

    std::ios::sync_with_stdio(false);

    cin >> N;
	
    for (int i = 0; i < N; i++){
		cin >> buf;
		int num = buf[0];
		num -=97;
		words[num].insert(buf);
    }

    cout << "R" << std::endl;
    while(cin >> cmd){
        cin.get();
        getline(cin, buf);
        switch(cmd){
            case 'Q':
                {
					pthread_t threads[NUM_THREAD];
                   

					for(int i=0; i < NUM_THREAD; i++){
						struct targs *t = (struct targs *)malloc(sizeof(struct targs));
						t->wl_beg = words[i].begin();
						t->wl_end = words[i].end();
						if(pthread_create(&threads[i], 0, ThreadFunc, (void*)t)<0){
							printf("pthread_create error!\n");
							return 0;
						}		
					}
					long val;
					for(int i =0; i < NUM_THREAD; i++){
						pthread_join(threads[i], (void**)&val);
					}
	
					
                    map<size_t, string>::iterator it = result.begin();
                    int cnt = result.size();
                    if (cnt == 0){
                        cout << -1 << endl;
                        break;
                    }
                    for (; cnt != 0; cnt--, it++){
						cout<< it->second;
						
                        if(cnt!=1){
                            cout <<"|";
                        }
                    }
                    result.clear();
                    cout << endl;
                }
                break;
            case 'A':
				{

					int x = buf[0];
					x-=97;
	                words[x].insert(buf);
                	break;
				}
            case 'D':
				int x = buf[0];
				x-=97;
                words[x].erase(buf);
                break;
        }
    }
    return 0;
}


void* ThreadFunc(void* arg){
	struct targs *p = (struct targs*)arg;
    set<string>:: iterator it = p->wl_beg;
	set<string>:: iterator it_2 = p->wl_end;

	for(;it!=it_2;it++){
        size_t pos = buf.find(*it);
		if (pos != string::npos){
			pthread_mutex_lock(&mutex);
			result.insert(make_pair(pos,*it));
			pthread_mutex_unlock(&mutex);
		}
	}

	return NULL;
}


/*
	int tid = *(int*)arg;
	map<size_t,set<string>>::iterator mps;
    set<string>:: iterator it = beg_words[tid];
	pthread_mutex_lock(&mutex);
	thread_ret[tid] = -1;
	pthread_cond_wait(&cond, &mutex);
	pthread_mutex_unlock(&mutex);

	while(1){
	    for(int i = 0;i<nof_words[tid];it++,i++){
	        size_t pos = buf.find(*it);
	        if (pos != string::npos){
				

				pthread_mutex_lock(&mutex);
				mps	=result.find(pos);
				pthread_mutex_unlock(&mutex);
				if(mps == result.end()){
					set<string>  temp;
					temp.insert(*it);
					pthread_mutex_lock(&mutex);
					result.insert(make_pair(pos,temp));
					pthread_mutex_unlock(&mutex);
				}
				else{
					pthread_mutex_lock(&mutex);
					mps->second.insert(*it);
					pthread_mutex_unlock(&mutex);
				}
			}
	    }

		pthread_mutex_lock(&mutex);
		thread_ret[tid] = 1;
		pthread_cond_wait(&cond, &mutex);
		it = beg_words[tid];	
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}
*/
