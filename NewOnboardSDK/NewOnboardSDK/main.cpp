#include <iostream>
#include "LinkLayer/LinkLayer.h"
#include "AppLayer/DJIDrone.h"
#include <cstring>
#include <chrono>
#include <thread>

using namespace std;
using namespace DJIOnboardSDK; 

/*
DJIOnboardSDK::LinkLayer com2;

void sendFunc(uint8_t* data, size_t size)
{
    //    char* buf = (char*)malloc(size + 1); 
    //    memcpy(buf, data, size); 
    //    buf[size] = '\0'; 
    cout << "recv(" << size << "): " << data << endl;
    int input = stoi((char*)data);
    char buf[256];
    _itoa_s(input + 1, buf, 10);
    cout << "send: " << buf << endl;
    com2.send((uint8_t*)buf, strlen(buf)+1);
    this_thread::sleep_for(chrono::milliseconds(1000));
}
*/

int main()
{
    DJIDrone drone; 
    drone.initialize(); 
    
    getchar(); 
    cout << "Activate. " << endl; 
    drone.activate(); 

    getchar(); 
    cout << "Obtain control. " << endl; 
    drone.obtain_control(); 

    getchar(); 
    cout << "Take off. " <<endl; 
    drone.take_off(); 

    return 0;
}
