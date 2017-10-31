#include <iostream>
#include <fstream>
#include <pthread.h>
#include <time.h>
#include <vector>
#include <chrono>
#include <unistd.h>
#include <queue>
#include <stack>

using namespace std;

typedef struct lock_and_waiting law;
typedef struct waiting_list waiting_list;

struct lock_and_waiting{
	pthread_rwlock_t rwlock;
	int wait_count;
	waiting_list *wali;
};

struct waiting_list{
	int tid;
	char rw;
	waiting_list *next;
};


vector<law> lock_table;
pthread_mutex_t global_lock=PTHREAD_MUTEX_INITIALIZER;
vector<pthread_cond_t> cond;

//N : Number of threads
//R : Number of records
//E : Global Execution order
int N, R, E;
int global_execution_order=0;

int *rec_check;
int *record;
int *cond_flag;

int find_wr(waiting_list *temp);
void delete_node(int i, int tid);
waiting_list * find_tid(waiting_list *root,int tid);
waiting_list * find_prev(waiting_list *root, int tid);
void *ijk_func(void *arg);
waiting_list * find_last(waiting_list *root);
int deadlock_check(int tid, int ijk);
int find_waiting_tid(int tid, int ijk);
void rec_reset();

int main(int argc, char* argv[]){
	//number of arguments is must be three!
	if(argc!=4){
		cout << "3 arg"<<endl;
		return 0;
	}

	//change the form from alphabet to integer
	N = atoi(argv[1]);
	R = atoi(argv[2]);
	E = atoi(argv[3]);
	

	int rc;	//return value of create threads
	
	//record initialize
	record = (int*)malloc(sizeof(int)*R);
	rec_check = (int*)malloc(sizeof(int)*R);
	cond_flag = (int*)malloc(sizeof(int)*N);

	//fill lock_table as much as number of records
	for(int i=0; i<R; i++){
		record[i]=0;
		rec_check[i]=0;
		law _law;
		//root node making
		waiting_list *head = (waiting_list *)malloc(sizeof(waiting_list));
		head->tid =-1;
		head->next = NULL;
		pthread_rwlock_init(&(_law.rwlock),NULL);
		_law.wali = head;
		_law.wait_count = 0;
		lock_table.push_back(_law);
	}

	//cond variable initialize
	for(int i=0; i<N; i++){
		cond_flag[i]=1;
		pthread_cond_t con=PTHREAD_COND_INITIALIZER;
		cond.push_back(con);
	}

	//thread create
	pthread_t p[N];
	for(int i=0; i<N; i++){
		int j[1];
		j[0]=i;
		rc = pthread_create(&p[i], NULL, ijk_func, (void * ) j);
		if(rc!=0){
			cout <<"thread_making failed"<<endl;
			return 0;
		}
	}
	

	//thread finish
	for(int i=0; i<N; i++){
		pthread_join(p[i],NULL);
	}
	
	return 0;
}



void *ijk_func(void *arg){
	int numm=0;
	//record address
	int *tid_p = (int *)arg;
	int tid = *tid_p;
	int i,j,k;
	int rec_i,rec_j,rec_k;
	while(1){
		//random seed
		int seed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		srand(seed);
		usleep(2000);
		//random number generator
		i = rand()%R;
		j = rand()%R;
		k = rand()%R;
		while(i==j){
			j=rand()%R;
		}
		while(i==k ||j==k){
			k=rand()%R;
		}
		// read i
		pthread_mutex_lock(&global_lock);
		if(lock_table[i].wait_count ==0){
			pthread_rwlock_rdlock(&lock_table[i].rwlock);
			lock_table[i].wait_count++;
			waiting_list *node = (waiting_list *)malloc(sizeof(waiting_list));
			node->tid = tid;
			node->rw = 'r';
			node->next = NULL;
			waiting_list *ptr = find_last(lock_table[i].wali);
			ptr->next=node;
			pthread_mutex_unlock(&global_lock);
			
		}
		else{
			//deadlock checking if deadlock 발생 give up the resource
			if(deadlock_check(tid,i)){
				pthread_mutex_unlock(&global_lock);
				continue;
			}
			lock_table[i].wait_count++;
			waiting_list *node = (waiting_list *)malloc(sizeof(waiting_list));
			node->tid = tid;
			node->rw = 'r';
			node->next = NULL;
			waiting_list *ptr = find_last(lock_table[i].wali);
			ptr->next = node;
			while(cond_flag[tid]==1)
				pthread_cond_wait(&cond[tid],&global_lock);
			cond_flag[tid]=1;
			pthread_rwlock_rdlock(&lock_table[i].rwlock);
			pthread_mutex_unlock(&global_lock);
		}
		rec_i = record[i];


		// write j
		pthread_mutex_lock(&global_lock);
		if(lock_table[j].wait_count ==0){
			pthread_rwlock_wrlock(&lock_table[j].rwlock);
			lock_table[j].wait_count++;
			waiting_list *node = (waiting_list *)malloc(sizeof(waiting_list));
			node->tid = tid;
			node->rw = 'w';
			node->next = NULL;
			waiting_list *ptr = find_last(lock_table[j].wali);
			ptr->next=node;
			pthread_mutex_unlock(&global_lock);
		}
		else{
			//deadlock check
			if(deadlock_check(tid,j)){
				// i 해제 하고 continue
				lock_table[i].wait_count--;
				if(lock_table[i].wait_count==0){
					free(lock_table[i].wali->next);
					lock_table[i].wali->next=NULL;
					pthread_rwlock_unlock(&lock_table[i].rwlock);
					pthread_mutex_unlock(&global_lock);
				}
				else{
					waiting_list *temp = lock_table[i].wali->next->next;
					free(lock_table[i].wali->next);
					lock_table[i].wali->next=temp;
					cond_flag[temp->tid]=0;
					pthread_rwlock_unlock(&lock_table[i].rwlock);
					pthread_cond_signal(&cond[temp->tid]);
					pthread_mutex_unlock(&global_lock);
				}
				continue;
			}
			lock_table[j].wait_count ++;
			waiting_list *node = (waiting_list *)malloc(sizeof(waiting_list));
			node->tid = tid;
			node->rw = 'w';
			node->next = NULL;
			waiting_list *ptr = find_last(lock_table[j].wali);
			ptr->next = node;
			while(cond_flag[tid]==1)
				pthread_cond_wait(&cond[tid],&global_lock);
			cond_flag[tid]=1;
			pthread_rwlock_wrlock(&lock_table[j].rwlock);
			pthread_mutex_unlock(&global_lock);
			
		}
		rec_j = record[j];
		record[j]+=record[i]+1;


		// write k
		pthread_mutex_lock(&global_lock);
		if(lock_table[k].wait_count ==0){
			pthread_rwlock_wrlock(&lock_table[k].rwlock);
			lock_table[k].wait_count++;
			waiting_list *node = (waiting_list *)malloc(sizeof(waiting_list));
			node->tid = tid;
			node->rw = 'w';
			node->next = NULL;
			waiting_list *ptr = find_last(lock_table[k].wali);
			ptr->next=node;
			pthread_mutex_unlock(&global_lock);
		}
		else{
			//deadlock check
			if(deadlock_check(tid,k)){
				// record[j] 값 원상 복구 해주고 i,j 해제 하고 continue
				lock_table[i].wait_count--;
				lock_table[j].wait_count--;
				record[j]=rec_j;
				pthread_rwlock_unlock(&lock_table[i].rwlock);
				pthread_rwlock_unlock(&lock_table[j].rwlock);
				if(lock_table[i].wait_count==0){
					free(lock_table[i].wali->next);
					lock_table[i].wali->next=NULL;
				}
				else{
					waiting_list *temp = lock_table[i].wali->next->next;
					free(lock_table[i].wali->next);
					lock_table[i].wali->next=temp;
					cond_flag[temp->tid]=0;
					pthread_cond_signal(&cond[temp->tid]);
				}
				if(lock_table[j].wait_count==0){
					free(lock_table[j].wali->next);
					lock_table[j].wali->next=NULL;
					pthread_mutex_unlock(&global_lock);
					
				}
				else{
					waiting_list *temp = lock_table[j].wali->next->next;
					free(lock_table[j].wali->next);
					lock_table[j].wali->next=temp;
					cond_flag[temp->tid]=0;
					pthread_cond_signal(&cond[temp->tid]);
					pthread_mutex_unlock(&global_lock);
				}
				continue;
			}
			lock_table[k].wait_count ++;
			waiting_list *node = (waiting_list *)malloc(sizeof(waiting_list));
			node->tid = tid;
			node->rw = 'w';
			node->next = NULL;
			waiting_list *ptr = find_last(lock_table[k].wali);
			ptr->next = node;
			while(cond_flag[tid]==1)
				pthread_cond_wait(&cond[tid],&global_lock);
			cond_flag[tid]=1;
//			while()
			pthread_rwlock_wrlock(&lock_table[k].rwlock);
			pthread_mutex_unlock(&global_lock);
			
		}
		rec_k = record[k];
		record[k]-=record[i];

		//commit
		int commit_id;
		pthread_mutex_lock(&global_lock);
		pthread_rwlock_unlock(&lock_table[i].rwlock);
		pthread_rwlock_unlock(&lock_table[j].rwlock);
		pthread_rwlock_unlock(&lock_table[k].rwlock);
		lock_table[i].wait_count--;
		if(lock_table[i].wait_count==0)
			delete_node(i,tid);
		else{
			delete_node(i,tid);
			cond_flag[lock_table[i].wali->next->tid]=0;
			pthread_cond_signal(&cond[lock_table[i].wali->next->tid]);
		}
		lock_table[j].wait_count--;
		if(lock_table[j].wait_count==0)
			delete_node(j,tid);
		else{
			delete_node(j,tid);
			cond_flag[lock_table[j].wali->next->tid]=0;
			pthread_cond_signal(&cond[lock_table[j].wali->next->tid]);
		}
		lock_table[k].wait_count--;
		if(lock_table[k].wait_count==0)
			delete_node(k,tid);
		else{
			delete_node(k,tid);
			cond_flag[lock_table[k].wali->next->tid]=0;
			pthread_cond_signal(&cond[lock_table[k].wali->next->tid]);
		}
		global_execution_order++;
		commit_id = global_execution_order;
		if(commit_id > E){
			record[j]=rec_j;
			record[k]=rec_k;
			pthread_mutex_unlock(&global_lock);
			return NULL;
		}
		string filepath = "thread";
		filepath.append(to_string(tid));
		filepath.append(".txt");
		ofstream writeFile;
		writeFile.open(filepath.data(),fstream::app);
		if(writeFile.is_open()){
			writeFile<<commit_id<<" "<<i<<" "<<j<<" "<<k<<" ";
			writeFile<<record[i]<<" "<<record[j]<<" "<<record[k]<<"\n";
		}
		writeFile.close();
		pthread_mutex_unlock(&global_lock);
		
		

	}
	return NULL;
}
int deadlock_check(int tid, int ijk){
	if(lock_table[ijk].wait_count== 0)
		return 0;
	else
	{
		stack<int> stack_t;
		int arr[N][N]={0};
		waiting_list *ijkl = find_last(lock_table[ijk].wali);
		
		arr[tid][ijkl->tid]=1;
		//deadlock check array ready

		for(int i=0; i<R;i++){
			waiting_list * temp = lock_table[i].wali->next;
			if( temp!=NULL){
				while(temp->next!=NULL){
					arr[temp->next->tid][temp->tid]=1;
					temp=temp->next;
				}
			}
		}
		int arr_check[N]={0};
		arr_check[tid]=1;
		for(int i=0; i<N;i++){
			if(arr[tid][i]==1)
				stack_t.push(i);
		}
		while(!stack_t.empty()){
			int temp = stack_t.top();
			stack_t.pop();
			if(arr_check[temp] == 1)
				return 1;
			else{
				arr_check[temp]=1;
				for(int i=0; i<N; i++){
					if(arr[temp][i]==1)
						stack_t.push(i);
				}
			}

		}
		return 0;

		/*
		queue<int> checking_list;
		waiting_list * ptr=lock_table[ijk].wali->next;
		while(ptr!=NULL	){
			checking_list.push(ptr->tid);
			ptr=ptr->next;
		}
		int size = checking_list.size();
		for(int x=0; x<size;x++){
			queue<int> temp;

			temp.push(checking_list.front());
			checking_list.pop();

			rec_check[ijk]=1;
			while(!temp.empty()){
				int pop = temp.front();
				temp.pop();
				//lock table을 검색해서 pop 된 값이 테이블내에 있는지 찾는다.
				for(int i=0; i<R; i++){
					if(rec_check[i]==0){
						//lock table의 tid 가 다른 자원을 기다리고 있는지 조건문
						if(find_waiting_tid(pop, i)){
							if(lock_table[i].wali->next->tid==tid){
								rec_reset();
								return 1;
							}
							rec_check[i]=1;
							temp.push(lock_table[i].wali->next->tid);
						}
					}
					
				}
			}
			rec_reset();
		}
		return 0;*/
	}


}
int find_waiting_tid(int tid, int ijk){
	waiting_list *temp = lock_table[ijk].wali;
	while(temp!=NULL)
	{
		if(temp->tid == tid)
			return 1;
		else
			temp=temp->next;
	}
	return 0;
}

int find_wr(waiting_list *temp){
	while(temp!=NULL){
		if(temp->rw=='w'){
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

waiting_list * find_last(waiting_list *root){
	waiting_list * temp= root;
	while(temp->next!=NULL){
		temp=temp->next;
	}
	return temp;
}
void delete_node(int i, int tid){
	waiting_list *node = find_tid(lock_table[i].wali, tid);
	waiting_list *prev = find_prev(lock_table[i].wali,tid);
	prev->next = node->next;
	free(node);
}
void rec_reset(){
	for(int i =0; i< R; i++)
		rec_check[i]=0;
}

waiting_list * find_tid(waiting_list *root,int tid){
	waiting_list * temp= root;
	while(temp->tid!=tid)
		temp = temp->next;
	return temp;
}
waiting_list * find_prev(waiting_list *root, int tid){
	waiting_list * temp= root;
	while(temp->next->tid!=tid)
		temp = temp->next;
	return temp;
}

