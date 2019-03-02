/*Print series 010203040506.
Using multi-threading 1st thread will print only 0 2nd thread will print only even numbers and 3rd thread print only odd numbers.*/



#include<iostream>
#include<thread>
#include<pthread.h>
using namespace std;

pthread_mutex_t lock;
int turn = 1;

void trythis(int id)
{
    static int count = 1;
    bool flag = true;
    while(flag) {
        pthread_mutex_lock(&lock);
        if(count == 7) {
            break;
            flag = false;
        }
        if(turn == 1) {
            if(id == 1) {
                cout<<"0";
                turn = (count % 2) == 0 ? 2 : 3;
            }
        } else if (turn == 2) {
            if(id == 2) {
                cout<<count;
                count++;
                turn = 1;
            }
        } else if( turn == 3) {
            if(id == 2) {
                cout<<count;
                count++;
                turn = 1;
            }
        }
        pthread_mutex_unlock(&lock);
    }
    return;
}

int main(void)
{
    int i = 0;
    int rc;
    int error;
    thread t1(trythis, 1);
    thread t2(trythis, 2);
    thread t3(trythis, 3);
    t1.join();
    t2.join();
    t3.join();
    return 0;
}
