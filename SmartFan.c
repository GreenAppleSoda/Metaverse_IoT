#include <wiringPi.h>
#include <softPwm.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wiringSerial.h>

// GPIO 핀 설정 (BCM 핀 번호)
#define PWM0 18   // GPIO18
#define PWM1 19   // GPIO19
#define LED1 16   // GPIO16
#define LED2 20   // GPIO20
#define LED3 21   // GPIO21
#define TRIG 23   // GPIO23
#define ECHO 24   // GPIO24
#define BUZZER 26 // GPIO26
#define SERVO 13  // GPIO13

// UART 블루투스 설정
#define BAUD_RATE 115200 //블루투스의 보율이 바뀔 경우 이 값을 변경해야함
static const char* UART1_DEV = "/dev/ttyAMA1"; //RPi5: UART1 연결을 위한 장치 파일

// UART 유선 설정
#define BAUD_RATE_UART 9600 // 유선 UART 보율
static const char* UART2_DEV = "/dev/ttyAMA2"; // 유선 UART 장치 파일

// 상태 변수
volatile int turn_enabled = 0;    // 서보모터 회전 활성화
volatile int current_position = 0; // 서보모터 현재 위치

// Mutex
pthread_mutex_t mutex;

void initPWM(int gpio1, int gpio2, int gpio3) {
    pinMode(gpio1, PWM_OUTPUT);
    pinMode(gpio2, PWM_OUTPUT);
    pinMode(gpio3, PWM_OUTPUT); // 하드웨어 PWM 사용

    // PWM 모드를 마크-스페이스 모드로 설정
    pwmSetMode(PWM_MODE_MS);

    // PWM 범위 설정 (주파수 = 19.2 MHz / (pwmClock * pwmRange))
    pwmSetClock(384);  // PWM 클럭 분주 설정 (19.2 MHz / 192 = 100 kHz)
    pwmSetRange(1000); // PWM 범위 설정 (100 kHz / 1000 = 100 Hz)

    // 초기값 설정
    pwmWrite(gpio1, 0);
    pwmWrite(gpio2, 0);
    pwmWrite(gpio3, 0);
}

//여러 바이트의 데이터를 씀
void serialWriteBytes (const int fd, const char *s)
{
    write (fd, s, strlen (s)) ;
}

//1바이트 데이터를 읽음
unsigned char serialRead(const int fd)
{
    unsigned char x;
    if(read (fd, &x, 1) != 1) //read 함수를 통해 1바이트 읽어옴
        return -1;
    return x; //읽어온 데이터 반환
}

//1바이트 데이터를 씀
void serialWrite(const int fd, const unsigned char c)
{
    write (fd, &c, 1); //write 함수를 통해 1바이트 씀
}


// LED 상태 업데이트
void update_leds(int speed) {
    digitalWrite(LED1, LOW);
    digitalWrite(LED2, LOW);
    digitalWrite(LED3, LOW);

    if (speed > 0 && speed <= 30) {
        digitalWrite(LED1, HIGH);
    } else if (speed > 30 && speed <= 70) {
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
    } else if (speed > 70) {
        digitalWrite(LED1, HIGH);
        digitalWrite(LED2, HIGH);
        digitalWrite(LED3, HIGH);
    }
}

// 모터 회전
// void motor_rotate(int speed, int direction) {
//     if (direction == 0) { // CW
//         softPwmWrite(PWM0, 0);
//         softPwmWrite(PWM1, speed);
//     } else if (direction == 1) { // CCW
//         softPwmWrite(PWM1, 0);
//         softPwmWrite(PWM0, speed);
//     }
// }

// 모터 회전 함수
void motor_rotate(int speed, int direction) {
    if (speed < 0 || speed > 100) {
        printf("속도는 0 ~ 100 사이로 설정해야 합니다.\n");
        return;
    }

    // 속도를 PWM 값으로 변환 (0 ~ 100 -> 0 ~ 1000)
    int pwm_value = speed * 5;

    if (direction == 0) { // CW (시계 방향)
        pwmWrite(PWM0, 0);
        pwmWrite(PWM1, pwm_value);
        printf("시계 방향으로 %d%% 속도로 회전합니다.\n", speed);
    } else if (direction == 1) { // CCW (반시계 방향)
        pwmWrite(PWM0, pwm_value);
        pwmWrite(PWM1, 0);
        printf("반시계 방향으로 %d%% 속도로 회전합니다.\n", speed);
    } else {
        printf("방향은 0 (CW) 또는 1 (CCW)로 설정해야 합니다.\n");
    }
    if (speed == 0 && direction == 0)
    {
        turn_enabled = 0;
    }
}

// 서보모터 제어
void *servo_control(void *arg) {
    int positions[] = {-45, 0, 45, 0}; // 각도 배열
    //int positions[] = {-90, 90}; // 각도 배열
    int index = 0;

    while (1) {
        pthread_mutex_lock(&mutex);
        if (turn_enabled) {
            current_position = positions[index];
            pthread_mutex_unlock(&mutex);

            // 각도를 PWM 신호로 변환 (-90도: 25, 0도: 75, 90도: 125)
            int duty_cycle = (int)((current_position + 90) * 50 / 90 + 25);
            pwmWrite(SERVO, duty_cycle); // 하드웨어 PWM 출력
            printf("Servo moved to position: %d (duty: %d)\n", current_position, duty_cycle);

            index = (index + 1) % 4; // 다음 위치로 이동
            //index = (index + 1) % 2; // 다음 위치로 이동
            sleep(1);
        } else {
            pthread_mutex_unlock(&mutex);
            usleep(100000);
        }
    }
    return NULL;
}

// 초음파 센서 제어
void *ultrasonic_sensor(void *arg) {
    
    int uart_fd; // 유선 UART 파일 디스크립터
    unsigned char dat;
    char buffer[100];
    buffer[0] = '0';
    buffer[1] = ',';
    buffer[2] = '5';
    buffer[3] = '\0';

    // 유선 UART 포트 열기
    uart_fd = serialOpen(UART2_DEV, BAUD_RATE_UART);
    if (uart_fd < 0) {
        fprintf(stderr, "Unable to open UART device: %s\n", UART2_DEV);
        return NULL;
    }

    while (1) {
        digitalWrite(TRIG, HIGH);
        usleep(10);
        digitalWrite(TRIG, LOW);

        while (digitalRead(ECHO) == LOW);
        long start_time = micros();

        while (digitalRead(ECHO) == HIGH);
        long travel_time = micros() - start_time;

        float distance = travel_time / 58.0; // 거리 계산
        printf("Distance: %.1f cm\n", distance);
        

        pthread_mutex_lock(&mutex);
        if (distance <= 17.9) {
            digitalWrite(BUZZER, HIGH);
            pwmWrite(PWM0, 0);
            pwmWrite(PWM1, 0);
            turn_enabled = 0;

            // UART를 통해 경고 메시지 전송
            serialWriteBytes(uart_fd, buffer);
            serialWriteBytes(uart_fd, "\n");

        } else {
            digitalWrite(BUZZER, LOW);
        }
        pthread_mutex_unlock(&mutex);

        usleep(200000);
    }
    close(uart_fd);
    return NULL;
}

// 문자열 양옆의 공백 및 줄 바꿈 문자를 제거하는 함수
void trim(char *str) {
    int len = strlen(str);
    int start = 0;
    int end = len - 1;

    // 앞쪽 공백 제거
    while (start < len && (str[start] == ' ' || str[start] == '\n' || str[start] == '\r')) {
        start++;
    }

    // 뒤쪽 공백 제거
    while (end >= start && (str[end] == ' ' || str[end] == '\n' || str[end] == '\r')) {
        end--;
    }

    // 잘린 문자열을 새로운 문자열로 변환
    memmove(str, str + start, end - start + 1);
    str[end - start + 1] = '\0'; // null terminator 추가
}

// 블루투스 명령 수신 처리
void *bluetooth_listener(void *arg) {
    int fd_serial; //블루투스
    int uart_fd; //유선
    unsigned char dat;
    char buffer[100];

    // 시리얼 포트 열기
    if ((fd_serial = serialOpen(UART1_DEV, BAUD_RATE)) < 0) {
        printf("Unable to open serial device.\n");
        return NULL;
    }

    // UART 포트 열기 (유선 UART)
    if ((uart_fd = serialOpen(UART2_DEV, BAUD_RATE_UART)) < 0) {
        fprintf(stderr, "Unable to open UART device: %s\n", UART2_DEV);
        return NULL;
    }

    while (1) {
        if (serialDataAvail(fd_serial)) {
            int i = 0;
            while (serialDataAvail(fd_serial) && i < sizeof(buffer)) {
                dat = serialRead(fd_serial);
                buffer[i++] = dat;
            }
            buffer[i] = '\0'; // 문자열 종료

            // 문자열 트리밍
            trim(buffer);

            printf("Received data: '%s'\n", buffer); // 받은 데이터 출력

            // 클라이언트로 회신
            serialWriteBytes(fd_serial, "Echo: ");
            serialWriteBytes(fd_serial, buffer);
            serialWriteBytes(fd_serial, "\n");

            // 유선 UART로 회신
            //serialWriteBytes(uart_fd, "From. SmartPhone : ");
            serialWriteBytes(uart_fd, buffer);
            serialWriteBytes(uart_fd, "\n");

            pthread_mutex_lock(&mutex);

            // 명령 처리
            if (strcasecmp(buffer, "0,1000") == 0) {
                turn_enabled = 1;
                printf("Servo rotation started.\n");
            } else if (strcasecmp(buffer, "0,2000") == 0) {
                turn_enabled = 0;
                printf("Servo rotation stopped.\n");
            }
            else {
                int speed, direction;
                if (sscanf(buffer, "%d,%d", &direction, &speed) == 2) {
                    if (speed >= 0 && speed <= 100 && (direction == 0 || direction == 1)) {
                        motor_rotate(speed, direction);
                        update_leds(speed);
                    } else {
                        printf("Invalid speed or direction values.\n");
                    }
                } else {
                    printf("Invalid command format.\n");
                }
            }

            pthread_mutex_unlock(&mutex);
        } 

        if (serialDataAvail(uart_fd)) {
            int i = 0;
            while (serialDataAvail(uart_fd) && i < sizeof(buffer)) {
                dat = serialRead(uart_fd);
                buffer[i++] = dat;
            }
            buffer[i] = '\0'; // 문자열 종료

            // 문자열 트리밍
            trim(buffer);

            printf("Received data: '%s'\n", buffer); // 받은 데이터 출력

            // 클라이언트로 회신
            serialWriteBytes(fd_serial, "Echo: ");
            serialWriteBytes(fd_serial, buffer);
            serialWriteBytes(fd_serial, "\n");

            // 유선 UART로 회신
            //serialWriteBytes(uart_fd, "From. SmartPhone : ");
            //serialWriteBytes(uart_fd, buffer);
            //serialWriteBytes(uart_fd, "\n");

            pthread_mutex_lock(&mutex);

            // 명령 처리
            if (strcasecmp(buffer, "0,1000") == 0) {
                turn_enabled = 1;
                printf("Servo rotation started.\n");
            } else if (strcasecmp(buffer, "0,2000") == 0) {
                turn_enabled = 0;
                printf("Servo rotation stopped.\n");
            } else {
                int speed, direction;
                if (sscanf(buffer, "%d,%d", &direction, &speed) == 2) {
                    if (speed >= 0 && speed <= 100 && (direction == 0)) {
                        motor_rotate(speed, direction);
                        update_leds(speed);
                    } else {
                        printf("Invalid speed or direction values.\n");
                    }
                } else {
                    printf("Invalid command format.\n");
                }
            }

            pthread_mutex_unlock(&mutex);
        }

        else {
            usleep(100000); // 데이터가 없으면 잠시 대기
        }
    }

    close(fd_serial);
    close(uart_fd);
    return NULL;
}

int main() {
    // GPIO 초기화
    wiringPiSetupGpio();

    // Mutex 초기화
    pthread_mutex_init(&mutex, NULL);


    // 핀 설정
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);
    pinMode(TRIG, OUTPUT);
    pinMode(ECHO, INPUT);
    pinMode(BUZZER, OUTPUT);

    initPWM(PWM0, PWM1, SERVO);

    // softPwmCreate(PWM0, 0, 100);
    // softPwmCreate(PWM1, 0, 100);

    // 스레드 생성
    pthread_t servo_thread, ultrasonic_thread, bluetooth_thread;
    pthread_create(&servo_thread, NULL, servo_control, NULL);
    pthread_create(&ultrasonic_thread, NULL, ultrasonic_sensor, NULL);
    pthread_create(&bluetooth_thread, NULL, bluetooth_listener, NULL);

    // 메인 루프
    while (1) {
        sleep(1);
    }

    // 자원 해제
    pthread_mutex_destroy(&mutex);
    return 0;
}