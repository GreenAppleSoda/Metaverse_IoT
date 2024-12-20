#include "auto_door.h"
#include <wiringPi.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringSerial.h>
#include <softPwm.h>
#include <math.h>
#include <fcntl.h>
#define UART2_DEV "/dev/ttyAMA3" // UART2 device file
#define BAUD_RATE 4800
#define OPEN_DISTANCE 10 // Door opening distance threshold (in cm)
#define WAIT_TIME 5000   // Time to wait before closing the door (in milliseconds)
#define ledPin 18

static int trigPin;
static int echoPin;
static int servoPin;
//static int ledPin = 18; // LED 핀 번호
static int isDoorOpen = 0; // Door state (0: closed, 1: open)
static long startWaitTime = 0;


int fd_serial;

char buffer[20] = { 0 };

// Function prototypes
unsigned char serialRead(const int fd);
void serialWrite(const int fd, const unsigned char c);
void setServoAngle(int angle);
double getDistance();
void controlDoor(int open);
void initAutomaticDoor(int tPin, int ePin, int sPin);
void checkAndControlDoor();
void* Whilethread(void* arg);
void* receiveData(void* arg);
void initLED();
void controlLED(int state);

// Read 1-byte data from serial
unsigned char serialRead(const int fd) {
    unsigned char x;
    if (read(fd, &x, 1) != 1)
        return -1;
    return x;
}

// Write 1-byte data to serial
void serialWrite(const int fd, const unsigned char c) {
    write(fd, &c, 1);
}

// Set servo motor angle
void setServoAngle(int angle) {
    if (angle >= 0)
    {
        int pulseWidth = (angle * 11 / 180) + 5;
        softPwmWrite(servoPin, pulseWidth); // Send PWM signal to servo pin
        delay(500);
    }
    else
    {
        softPwmWrite(servoPin, 0);
    }
}

// Measure distance using ultrasonic sensor
double getDistance() {
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    while (digitalRead(echoPin) == LOW);
    long startTime = micros();
    while (digitalRead(echoPin) == HIGH);
    long travelTime = micros() - startTime;

    return travelTime / 58.0; // Distance in cm
}

// ***********************************************
void initLED() {

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW); // LED 초기 상태: OFF

}

void controlLED(int state) {
    char str[6]; // 데이터를 저장할 문자열 배열

    if (state) {
        printf("LED ON\n");
        digitalWrite(ledPin, HIGH); // LED 켜기
        snprintf(str, sizeof(str), "2,0"); // LED 켜짐 데이터
    }
    else {
        printf("LED OFF\n");
        digitalWrite(ledPin, LOW); // LED 끄기
        snprintf(str, sizeof(str), "2,1"); // LED 꺼짐 데이터
    }

    // LED 상태 데이터 전송
    serialPuts(fd_serial, str);
    serialPuts(fd_serial, "\n");
    serialFlush(fd_serial);
}

void controlLED2(int state) {
    char str[6]; // 데이터를 저장할 문자열 배열

    if (state) {
        printf("LED ON\n");
        digitalWrite(ledPin, HIGH); // LED 켜기
        snprintf(str, sizeof(str), "2,0"); // LED 켜짐 데이터
    }
    else {
        printf("LED OFF\n");
        digitalWrite(ledPin, LOW); // LED 끄기
        snprintf(str, sizeof(str), "2,1"); // LED 꺼짐 데이터
    }

}


// Control door (open/close)
void controlDoor(int open) {
    char str[6]; // 데이터를 저장할 문자열 배열

    if (open) {
        snprintf(str, sizeof(str), "1,0"); // 문 열림 데이터
        printf("Door opening\n");
        controlLED(1);
        setServoAngle(120); // Open angle

        delay(500);// Close angle
        setServoAngle(-1);

        serialPuts(fd_serial, str);
        serialPuts(fd_serial, "\n");
        serialFlush(fd_serial);
    }
    else {
        snprintf(str, sizeof(str), "1,1"); // 문 닫힘 데이터
        printf("Door closing\n");
        controlLED(0);
        setServoAngle(0);

        delay(500);// Close angle
        setServoAngle(-1);

        serialPuts(fd_serial, str);
        serialPuts(fd_serial, "\n");
        serialFlush(fd_serial);
    }

    // 데이터 전송


    isDoorOpen = open; // 문 상태 업데이트
}

void controlDoor2(int open) {
    startWaitTime = millis();
    char str[6]; // 데이터를 저장할 문자열 배열

    if (open) {
        if (isDoorOpen)
        {
            startWaitTime = millis();
        }
        else {

            printf("Door opening\n");
            controlLED2(1);
            setServoAngle(120); // Open angle

            delay(500);// Close angle
            setServoAngle(-1);
        }

    }

    else {
        printf("Door closing\n");
        controlLED2(0);
        setServoAngle(0);

        delay(500);// Close angle
        setServoAngle(-1);

    }

    // 데이터 전송


    isDoorOpen = open; // 문 상태 업데이트
}

// Initialize automatic door
void initAutomaticDoor(int tPin, int ePin, int sPin) {
    trigPin = tPin;
    echoPin = ePin;
    servoPin = sPin;

    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    softPwmCreate(servoPin, 0, 200); // Initialize servo pin
    digitalWrite(trigPin, LOW);

    printf("Automatic door initialized\n");
}

// Check distance and control the door accordingly
void checkAndControlDoor() {
    double distance = getDistance();
    printf("Measured distance: %.2f cm\n", distance);

    if (distance > 0 && distance <= OPEN_DISTANCE) {
        if (!isDoorOpen) {
            controlDoor(1); // Open door
            startWaitTime = millis();

        }
        else
        {
            printf("afdsaf");
            startWaitTime = millis();
        }
    }
    else {
        if (isDoorOpen && millis() - startWaitTime >= WAIT_TIME) {
            controlDoor(0); // Close door

        }
    }
}

// Thread function to handle door control
void* Whilethread(void* arg) {
    printf("왜");
    while (1) {
        checkAndControlDoor();
        delay(100); // Check every 100 ms
    }
    return NULL;
}

// Thread function to handle serial data reception
void* receiveData(void* arg) {
    char received[256];
    int index = 0;

    while (1) {
        if (serialDataAvail(fd_serial)) {
            char c = serialGetchar(fd_serial);

            if (c == '\n') {
                received[index] = '\0';
                printf("[RX] Received: %s\n", received);

                char* token = strtok(received, ",");
                char* tokens[2];
                int part_count = 0;

                while (token != NULL && part_count < 2) {
                    tokens[part_count] = token;
                    printf("Part %d: %s\n", part_count, token);
                    part_count++;
                    token = strtok(NULL, ",");
                }

                if (part_count == 2) {
                    if (strcmp(tokens[0], "1") == 0 && strcmp(tokens[1], "0") == 0) {
                        printf("Door opening\n");
                        controlLED2(1); // Turn on LED
                        controlDoor2(1); // Open door
                    }
                    else if (strcmp(tokens[0], "1") == 0 && strcmp(tokens[1], "1") == 0) {
                        printf("Door closing\n");
                        controlLED2(0); // Turn off LED
                        controlDoor2(0); // Close door
                    }
                    else if (strcmp(tokens[0], "2") == 0 && strcmp(tokens[1], "0") == 0) {
                        printf("Light OFF\n");
                        // Add your light off logic here
                    }
                    else if (strcmp(tokens[0], "2") == 0 && strcmp(tokens[1], "1") == 0) {
                        printf("Light ON\n");
                        // Add your light on logic here
                    }
                }

                index = 0; // Reset the index after processing the message
            }
            else {
                received[index++] = c;
                if (index >= sizeof(received) - 1) {
                    // Prevent buffer overflow
                    index = 0;
                    printf("Error: Input too long\n");
                }
            }
        }

        usleep(1000); // Prevent CPU overuse
    }
    return NULL;
}

void* receiveData2(void* arg) {
    char received[256];
    int index = 0;

    while (1) {
        if (serialDataAvail(fd_serial)) {
            char c = serialGetchar(fd_serial);

            if (c == '\n') {
                received[index] = '\0'; // Null-terminate the string
                printf("[RX] Received: %s\n", received);

                // Check for commands
                if (strcmp(received, "1,0") == 0) {
                    printf("Door opening\n");
                    controlLED(1); // Turn on LED
                    controlDoor(1); // Open door
                }
                else if (strcmp(received, "1,1") == 0) {
                    printf("Door closing\n");
                    controlLED(0); // Turn off LED
                    controlDoor(0); // Close door
                }
                else if (strcmp(received, "2, 0") == 0) {
                    printf("Light ON\n");

                }
                else if (strcmp(received, "2, 1") == 0) {
                    printf("Light OFF\n");

                }
                else {
                    printf("Invalid command received\n");
                }

                index = 0; // Reset index for next message
            }
            else {
                received[index++] = c; // Accumulate characters
            }
        }

        usleep(1000); // Prevent CPU overuse
    }
    return NULL;
}

int main() {
    if (wiringPiSetupGpio() == -1) {
        printf("WiringPi initialization failed\n");
        return 1;
    }

    if ((fd_serial = serialOpen(UART2_DEV, BAUD_RATE)) < 0) {
        printf("Unable to open serial device\n");
        return 1;
    }
    pthread_t controlThread, receiveThread;

    // Initialize the LED
    printf("d");
    initLED();
    printf("da");
    // Initialize the automatic door
    initAutomaticDoor(23, 24, 25);
    printf("dsasaa");
    // Create threads for door control and data reception
    pthread_create(&controlThread, NULL, Whilethread, NULL);
    pthread_create(&receiveThread, NULL, receiveData, NULL);

    // Wait for threads to complete (infinite loop in this case)
    pthread_join(controlThread, NULL);
    pthread_join(receiveThread, NULL);

    return 0;
}
