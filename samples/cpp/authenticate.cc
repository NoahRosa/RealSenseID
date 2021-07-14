// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2021 Intel Corporation. All Rights Reserved.

#include "RealSenseID/FaceAuthenticator.h"
#include <iostream>
#include <stdio.h>
#include "rs232.h"
#include <chrono>
#include <signal.h>
#include <errno.h>

#define IMX_GPIO_NR(port, index)    ((((port)-1)*32)+((index)&31))
#define ADMIN_ID "noah" //SET HERE THE ADMIN USER ID


//GPIO Class implementation for standard Linux system, it is recommended to use system specific libraries

class Gpio {
    int pinNumber;
    char bufg[100];
    public:
        enum direction {IN, OUT};
        enum pinState {HIGH, LOW};
        Gpio(int chip, int number){
            memset(bufg, 0, sizeof(bufg));
            pinNumber = IMX_GPIO_NR(chip, number);
            int fd = open("/sys/class/gpio/export", O_WRONLY);
            sprintf(bufg, "%d", pinNumber); 
            write(fd, bufg, strlen(bufg));
            close(fd);
        }
        ~Gpio(){
            memset(bufg, 0, sizeof(bufg));
            int fd = open("/sys/class/gpio/unexport", O_WRONLY);
            sprintf(bufg, "%d", pinNumber);
            write(fd, bufg, strlen(bufg));
            close(fd);
        }
        void setDirection(Gpio::direction dir){
            memset(bufg, 0, sizeof(bufg));
            sprintf(bufg, "/sys/class/gpio/gpio%d/direction", pinNumber);
            int fd = open(bufg, O_WRONLY);
            switch (dir){
                case Gpio::direction::IN : write(fd, "in", 2); break; 
                case Gpio::direction::OUT : write(fd, "out", 3); break; 
            }
            close(fd);
        }
        void setValue(Gpio::pinState state){
            memset(bufg, 0, sizeof(bufg));
            sprintf(bufg, "/sys/class/gpio/gpio%d/value", pinNumber);
            int fd = open(bufg, O_WRONLY);
            switch (state){
                case Gpio::pinState::HIGH : write(fd, "1", 1);
                    break; 
                case Gpio::pinState::LOW : write(fd, "0", 1);
                    break; 
            }
            close(fd);
        }
};

void sigint_handler(int sig); //interrupt handler
volatile int die = 0;  //Ctrl-C interrupt flag, when active makes the program end
char user[20]; //Last user authenticated
volatile int newUser = 0; //new user flag, when active tries to authenticate the admin, when low opens the gate


class MyAuthClbk : public RealSenseID::AuthenticationCallback
{
    Gpio *gpio; //Gpio connected to the relay, use system specific library
public:
    MyAuthClbk(Gpio &pin){  //override the standard constructor to add the gpio
        gpio = &pin;
    }
    void OnResult(const RealSenseID::AuthenticateStatus status, const char* user_id) override
    {
        if (status == RealSenseID::AuthenticateStatus::Success){  //if the authentication is succesfull prints message
            std::cout << "Authenticated " << user_id << std::endl;
            strcpy(user, user_id);
            if (!newUser){ //if not registering a new user opens the gate
                gpio->setValue(Gpio::pinState::HIGH);
            }
        }
    }

    void OnHint(const RealSenseID::AuthenticateStatus hint) override
    {
        std::cout << "OnHint " << hint << std::endl;
    }

    void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces, const unsigned int ts) override
    {
        for (auto& face : faces)
        {
            printf("** Detected face %u,%u %ux%u (timestamp %u)\n", face.x, face.y, face.w, face.h, ts);
        }
    }
};

class MyEnrollClbk : public RealSenseID::EnrollmentCallback
{
public:
    void OnResult(const RealSenseID::EnrollStatus status) override
    {
        std::cout << "Result " << status << std::endl;
    }

    void OnProgress(const RealSenseID::FacePose pose) override
    {
        std::cout << "OnProgress " << pose << std::endl;
    }

    void OnHint(const RealSenseID::EnrollStatus hint) override
    {
        std::cout << "Hint " << hint << std::endl;
    }

    void OnFaceDetected(const std::vector<RealSenseID::FaceRect>& faces, const unsigned int ts) override
    {
        for (auto& face : faces)
        {
            printf("** Detected face %u,%u %ux%u (timestamp %u)\n", face.x, face.y, face.w, face.h, ts);   
        }
    }
};



int waitForCommand(unsigned char *buf){
    using namespace std::chrono;
    int cport_nr=38,        /* /dev/ttymxc1 (COM1 on windows) */
    bdrate=115200;       /* 115200 baud */
    char mode[]={'8','N','1',0}; // 8bit, no parity, 1 stop bit
    int i = 0;
    if(RS232_OpenComport(cport_nr, bdrate, mode, 0)) //try opening serial to nxp board
    {
        printf("Can not open comport\n");

        return(0);
    }
    long int time, newtime;
    time = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count(); //get start time
    newtime = time;
    while (strlen((const char *)buf) != 3){ //poll com port until command length is recieved
        if (die) return 0;
        i += RS232_PollComport(38, &buf[i], (4095- strlen((const char *)buf)));
        newtime = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
        if ((newtime - time) > 2000){ //if timeout reset buffer (TODO not working)
            memset(buf, 0, sizeof(buf));
            time = duration_cast< milliseconds >(system_clock::now().time_since_epoch()).count();
            newtime = time;
        }
    }
    RS232_CloseComport(cport_nr);
    return 0;
}

int main()
{
    RealSenseID::FaceAuthenticator authenticator;
#ifdef _WIN32
    auto status = authenticator.Connect({"COM9"});
#elif LINUX
    auto status = authenticator.Connect({"/dev/ttyACM0"}); //Select camera serial port
#endif
    if (status != RealSenseID::Status::Ok)
    {
        std::cout << "Failed connecting with status " << status << std::endl;
        return 1;
    }
    Gpio gpio = Gpio(1, 6); /* Just an example */ //initialize relay GPIO
    
    MyAuthClbk auth_clbk(gpio); //initialize callbacks

    MyEnrollClbk enroll_clbk;

    signal(SIGINT, sigint_handler); //initialize interrupt

    gpio.setDirection(Gpio::direction::OUT); //set gpio to output 

    unsigned char buf[4096];
    memset(buf, 0, sizeof(buf));
    long int time, newtime;
    while (1){//start main loop
        while (strcmp("ON1", (const char *) buf)){//wait for activation command
            memset(buf, 0, sizeof(buf));
            waitForCommand(buf);
            if (die) return 0;
        }
        while (strcmp("OFF", (const char *) buf)){//wait for deactivation command
            memset(buf, 0, sizeof(buf));
            waitForCommand(buf);
            if (die) return 0;
            if (!strcmp("003", (const char *) buf)){//if open command recieved
                    newUser = 0; //set the flag to open the gate
                    authenticator.Authenticate(auth_clbk);
                }
            if (!strcmp("004", (const char *) buf)){//if close command recieved
                gpio.setValue(Gpio::pinState::LOW);//close the gate
            }
            if (!strcmp("025", (const char *) buf)){//if new user command is recieved
                memset(user, 0, sizeof(user));
                newUser = 1; //set for new user registration
                authenticator.Authenticate(auth_clbk);
                if (!strcmp(user, ADMIN_ID)){//if the admin is authenticated
                    memset(buf, 0, sizeof(buf));
                    while (strcmp("026", (const char *) buf)){//wait until the start enrolling command is recieved
                        memset(buf, 0, sizeof(buf));
                        waitForCommand(buf);
                        if (die) return 0;
                    }
                    unsigned int nUser = 0;
                    authenticator.QueryNumberOfUsers(nUser);//request the current number of users
                    char uid[20];
                    sprintf(uid, "User%d", nUser+1); //give the new user a name
                    authenticator.Enroll(enroll_clbk, uid);  //enroll the new user
                }
            }
        }
    }
}

void sigint_handler(int sig) {  //handle the interrupt signal
    printf("\nCTRL-C detected\n");
    die = 1; //set the termination flag
}
