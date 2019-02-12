#include <thread>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include <vector>
#include <unistd.h>

using namespace std;

vector<int> buffer;

uint64_t produced_count = 0;
uint64_t consumed_count = 0;
mutex counts_mutex;
condition_variable counts_cv;

void producer()
{
	while (true) {
		unique_lock<mutex> counts_lock(counts_mutex);
		while (produced_count - consumed_count == buffer.size()) {
			counts_cv.wait(counts_lock);
		}
		
		int product = rand();
		buffer[produced_count++ % buffer.size()] = product;
		cout << "produced: " << product << "\n";
		
		counts_cv.notify_all();
	}
}

void consumer()
{
	while (true) {
		unique_lock<mutex> counts_lock(counts_mutex);
		while (produced_count - consumed_count == 0) {
			counts_cv.wait(counts_lock);
		}
		
		int product = buffer[consumed_count++ % buffer.size()];
		cout << "consumed: " <<  product << "\n";
		
		counts_cv.notify_all();
	}
}

int main(int argvc, char const **argv)
{
    buffer.resize(16, 0);
	// to handle the uint64_t counts overflow, the following must be true:
	// (0xffffffffffffffff + 1) % buffer.size() == 0
 
    vector<thread> threads;
    for (int i = 0; i < 3; ++i) {
        threads.push_back(thread(producer));
    }
	sleep(3);
    for (int i = 0; i < 3; ++i) {
        threads.push_back(thread(consumer));
    }
    for (int i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    return 0;
}
