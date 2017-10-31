# **Welcome to Two phase Locking with Read and Write lock**
  
This program designed in ubuntu 17.04 and linux kernel version 3.x. <br>
I used some c++ standard library <iostream, pthread.h, time.h, vector, chrono, unistd.h, queue, stack> <br>
and also make my own struct called lock_and waiting, and waiting list that is actually linked list.

# **Project specification**
  
<pre>Input = you should need three standard arguments which are N, R, E. 
(N is thread numbers, R is Record numbers, E is Global Execution order. 
Output = #thread.txt files that each thread make their own text file and they commit their modification of records.   </pre>

//N : Number of threads<br>
//R : Number of records<br>
//E : Global Execution order<br>

If have three number of N, R, E, then this program creates N threads and each thread have their own works.

<pre>Works = each threads choose i, j, k and read ith record and write j as j+i+1 and write as k - i. 
This looks really simple, but this program have two phase lock which means each thread sequentially take 
ith rwlock and jth rwlock and kth rwlock, so it is really vulnerable from deadlock. 
That's why deadlock handling is one of most important part in this project.
</pre>

# **Program Features**

Each thread run with ijk_func, records, and Lock table which maintain which thread wait this records or not. <br>
This information is important because if this thread waits, then it should be signaled by other thread. <br>
In addition, deadlock_check function is handling deadlock condition.

# **Code review**
#### 1.Global variables
```c++

vector<law> lock_table;
pthread_mutex_t global_lock=PTHREAD_MUTEX_INITIALIZER;
vector<pthread_cond_t> cond;

//N : Number of threads
//R : Number of records
//E : Global Execution order
int N, R, E;
int global_execution_order=0;

int *record;
int *cond_flag;

```
<hr>
global_lock is a mutex supported by POSIX standard library, and lock_table is a vector<br>
which includes the information of occupying thread and waiting threads of each records.<br>
cond is a vector which has condition variable. <br>
N, R, E are standard arguments. <br>
global_execution_order is current order and thread executes until global_execution order becomes E. <br>
record is record that thread access every steps and have lock of records and length is R. <br>
cond_flag is a int type array and array size is number of threads because thread wait and wake with this variable.<br>
<hr/>
#### 2.main function
```c++
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
```
In main function, first initialize every variable and stack, rw_lock, condition variable, linked list and allocate memory with them. <br>
After then, main thread give responsibility to child thread!!<br>
<hr/>
#### 3.Thread function
  
```c++
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
```
  
In thread function, each thread has tid which is thread number. There are R number of records, and first choose three random numbers between 0 and R. First number is i, second is j, and the third is k. first thread hold read_write_lock[i], and read record[i], then hold read_write_lock[j] and write record[j] as record[j]+i+1, finally hold read_write_lock[k] and write record[k] as record[k]-i. If thread finish this steps, then commit record and i, j, k with commit number called global execution number with +1 that means write on the #thread.txt file.

# **Deadlock handling**
```c++
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
}
```
For deadlock checking, this program use this function. In this function, it finds deadlock with the recursive way. First, it makes two dimensional array that shows which thread wait the other thread. And with this array, this program hired stack and push the inserting value in the stack. As soon as pop stack, it checks this value with array. The thread number that this thread waits is pushed in the stack. and so on recursively. If pop value is equal to first thread id, it means deadlock.
# **Limitation**
Because of this program use record value as integer type, that makes overflow condition. <br>

# **Thank You**

  
