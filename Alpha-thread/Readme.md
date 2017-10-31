# **Welcome to Alpha Thread**
  
Alpha thread is a program to search some words in a super long sentence. It is designed by c++11 std and made in linux kernel 3.x version, so if you want to test this program you should use the same linux kernel version. When it comes to testing the performance of this program, it is tested in the circumstance as CPU : 2 x Intel(R) Xeon(R) CPU E5-2699 v3 @ 2.30GHz, Config : 36 cores / Hyperthread disabled, main memory : 450 GB.

# **Project specification**
  
<pre>Input = Number of words that you want to find, words set, Query('Q', 'A', 'D')  
Output = the words that you find in the long sentence ordered by the time when you find the exact word  </pre>
  
First, you should have input of the Number which is how many words you want to find in a long sentence, and next N input string is the words that you want to find in a long sentence. If you finish to have the words set, then you can have a query which is consisted of three possible sorts as 'Q', 'A', 'D'. 'Q' is you want to find the words in the long sentence, so you should have a query sentence. 'A' is you want to add other word in your words set. 'D' is you want to delete this word in your words set.

# **Program Features**
 You can guess the inner process by its name "alpha thread" which has 26+1 threads ( the number of alphabet is 26 and one is main thread ). Each thread has their words set start by their alphabet. For example, thread 1 has words set which starts with 'a' and thread 2 has words set which starts with 'b' and so on. Finally, thread 26 has words set starting with 'z'. And they find words they have and store multi map called "result". Finally, standard out the words that they find ordered by the text sequence.( in the long sentence if one word precedes another, words that first appear should be the first output. 

# **Code review**
#### 1.Global variables
```c++
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
multimap<size_t,string> result;

struct targs{
		set<string>::iterator wl_beg;
		set<string>::iterator wl_end;
};
void* ThreadFunc(void* arg);
string buf;
```
Mutex is a mutex supported by POSIX standard library, and result is multi map which is stored actual words found by the query sentence and clear right after it is standard out. struct targs is a struct which store the location of word begin and end in the entire words set. string buf is a buf that store string.
<hr/>
#### 2.main function
```c++
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
```
In main function, first it has input number and string which is making starting words set vector. In a vector words, there are 26 set and each set is a words set starting with alphabet. Next, it starts with command character which is consisted with 'Q', 'A', 'D'. If you have 'Q', then it makes 26 threads, find their words in each thread and finally aggregate the results in multi map. If you have 'A', it just adds words set vector in each alphabet set. If you have 'D', it just deletes that word in each alphabet set.
<hr/>
#### 3.Thread function
  
```c++
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
```
  
In thread function, it has a argument which has two iterator the begin and end of thread's words set. With this iterator, each thread has responsible to find their words starting with their alphabet. **The main point of this function is locking parts when insert the word in multi map named result.** Multi map is a STL which is abstraction of data structure, so it is not done atomically. That's why it is surrounded by mutex lock.
  
# **Performance**
It is tested in 36 CPU cores and 450 GB memory, and use the small, medium, large data set. It takes 12.574 seconds in small set , it takes 45.798 seconds in medium set , it takes 127.754 seconds in large set. It takes 186.126 seconds in total. As a result, it is much faster than using only one thread. However, It is not faster 26 times even if this program is using 26 more threads. That is because locking problem.

# **Limitation**
I also tried to create more and more threads but it is really hard in my design. If I want to use two alphabet in the words set, in that case, I can make 26*26 threads but it makes much more problem and limitation like how can I handle the one character word and it can also make order problem. I made the program with solving these problem that have 26*26 threads, but it cannot have better performance.
