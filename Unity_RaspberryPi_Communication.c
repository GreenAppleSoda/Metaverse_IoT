#include <stdio.h>      // 표준 입출력 함수
#include <stdlib.h>     // 표준 라이브러리 함수
#include <string.h>     // 문자열 처리 함수
#include <unistd.h>     // UNIX 표준 함수
#include <fcntl.h>      // 파일 제어 함수
#include <pthread.h>    // 쓰레드 처리
#include <linux/i2c-dev.h>  // I2C 통신용
#include <sys/ioctl.h>  // ioctl 함수
#include <wiringPi.h>   // WiringPi 라이브러리
#include <wiringSerial.h> // WiringPi 직렬 통신 함수
#include <wiringPiSPI.h>
#include <math.h> // 기울기 각도 계산을 위해 포함

#define CS_GPIO 7 //CS
#define SPI_CH 0
#define SPI_SPEED 1000000 // 1MHz
#define SPI_MODE 3
#define BW_RATE 0x2C //
#define POWER_CTL 0x2D //Power Control Register
#define DATA_FORMAT 0x31
#define DATAX0 0x32 //X-Axis Data 0
#define DATAX1 0x33 //X-Axis Data 1
#define DATAY0 0x34 //Y-Axis Data 0
#define DATAY1 0x35 //Y-Axis Data 1
#define DATAZ0 0x36 //Z-Axis Data 0
#define DATAZ1 0x37 //Z-Axis Data 1


#define BAUD_RATE 115200
#define UART1_DEV "/dev/ttyAMA1"
#define I2C_ADDR 0x48
#define BUTTON_PIN 18  // GPIO 18

#define DEVICE "/dev/ttyAMA2" // UART2에 해당하는 디바이스 파일
#define BAUD_RATE2 9600       // 통신 속도
#define BAUD_RATE3 4800       // 통신 속도
#define DEVICE2 "/dev/ttyAMA3" // UART2에 해당하는 디바이스 파일

#define BUFFER_SIZE 32
int uart_fd;// UART 파일 디스크립터
int uart_fd2;// UART 파일 디스크립터
int fd_serial;

unsigned char serialRead(const int fd);
void serialWrite(const int fd, const unsigned char c) ;

//ADXL345
void readRegister_ADXL345(char registerAddress, int numBytes, char * values){
    //read 1 .
    values[0] = 0x80 | registerAddress;
    // 6 1
    if(numBytes > 1) values[0] = values[0] | 0x40;
    digitalWrite(CS_GPIO, LOW); // Low : CS Active
    // values
    wiringPiSPIDataRW(SPI_CH, values, numBytes + 1);
    digitalWrite(CS_GPIO, HIGH); // High : CS Inactive
}
//ADXL345
void writeRegister_ADXL345(char address, char value)
{
    unsigned char buff[2];
    buff[0] = address;
    buff[1] = value;
    digitalWrite(CS_GPIO, LOW); // Low : CS Active
    wiringPiSPIDataRW(SPI_CH, buff, 2);
    digitalWrite(CS_GPIO, HIGH); // High : CS Inactive
}


unsigned char serialRead(const int fd) {
    unsigned char x;
    if (read(fd, &x, 1) != 1)
        return -1;
    return x;
}

void serialWrite(const int fd, const unsigned char c) //1Byte 데이터를 송신하는 함수
{
write (fd, &c, 1); //write 함수를 통해 1바이트 씀
}

void *receiveData(void *arg) {

    char received[256];

    int index = 0;



    while (1) {

        if (serialDataAvail(uart_fd)) { // 수신 데이터가 있는지 확인

            char c = serialGetchar(uart_fd);
            //serialFlush(uart_fd);
            if (c == '\n') { // 메시지 끝을 개행 문자로 구분
                received[index] = '\0';

                printf("[RX] Received: %s\n", received);
                fflush(stdout); // 출력 버퍼 비우기

                // 공백을 기준으로 문자열 분리
                char *token = strtok(received, ",");
                char *tokens[2]; // 최대 2개의 값을 저장할 배열
                int part_count = 0;

                while (token != NULL && part_count < 2) {
                    tokens[part_count] = token;
                    printf("Part %d: %s\n", part_count, token);
                    part_count++;
                    token = strtok(NULL, ",");
                }

                if (part_count == 2) { // 두 값이 모두 존재할 경우에만 메시지 생성
                    char message[16];
                    snprintf(message, sizeof(message), "%s,%s\n", tokens[0], tokens[1]); // 좌표 값을 문자열로 변환
                    write(fd_serial, message,strlen(message)); // 전체 문자열 전송
                    fflush(stdout); // 출력 버퍼 비우기
                } else {
                    printf("Invalid message format\n");
                }

                index = 0; // 버퍼 초기화
            } else {
                received[index++] = c;
            }


        }

        usleep(1000); // CPU 과부하 방지를 위해 약간의 대기

    }

    return NULL;

}

void *receiveData2(void *arg) {

    char received[256];

    int index = 0;



    while (1) {

        if (serialDataAvail(uart_fd2)) { // 수신 데이터가 있는지 확인

            char c = serialGetchar(uart_fd2);
            //serialFlush(uart_fd);
            if (c == '\n') { // 메시지 끝을 개행 문자로 구분
                received[index] = '\0';

                printf("[RX] Received: %s\n", received);
                fflush(stdout); // 출력 버퍼 비우기

                // 공백을 기준으로 문자열 분리
                char *token = strtok(received, ",");
                char *tokens[2]; // 최대 2개의 값을 저장할 배열
                int part_count = 0;

                while (token != NULL && part_count < 2) {
                    tokens[part_count] = token;
                    printf("Part %d: %s\n", part_count, token);
                    part_count++;
                    token = strtok(NULL, ",");
                }

                if (part_count == 2) { // 두 값이 모두 존재할 경우에만 메시지 생성
                    char message[16];
                    snprintf(message, sizeof(message), "%s,%s\n", tokens[0], tokens[1]); // 좌표 값을 문자열로 변환
                    write(fd_serial, message,strlen(message)); // 전체 문자열 전송
                    fflush(stdout); // 출력 버퍼 비우기
                } else {
                    printf("Invalid message format\n");
                }

                index = 0; // 버퍼 초기화
            } else {
                received[index++] = c;
            }


        }

        usleep(1000); // CPU 과부하 방지를 위해 약간의 대기

    }

    return NULL;

}

void* uart_thread(void* arg) {
    fd_serial;
    unsigned char dat;
    char str[2]; // 문자열 변환용 배열
    int index=0;
    char buffer[BUFFER_SIZE] = {0};
    if ((fd_serial = serialOpen(UART1_DEV, BAUD_RATE)) < 0) {
        printf("Unable to open serial device.\n");
        return NULL;
    }
    
    while (1) {
        if (serialDataAvail(fd_serial)) { // 읽을 데이터가 있다면
            dat = serialRead(fd_serial); // 데이터를 읽어옴

            if (dat != (unsigned char)-1) {
                if (dat != '\n') { // '\n'은 끝을 의미
                    if (index < BUFFER_SIZE - 1) { // 버퍼가 가득 차지 않았을 때만 저장
                        buffer[index++] = dat; // 버퍼에 문자 저장
                        buffer[index] = '\0'; // null 문자로 종료
                    }
                } else {
                    // 줄 바꿈 문자를 받았으므로 전체 메시지를 처리
                    printf("[RX] Received: %s\n", buffer);

                    // 받은 메시지를 그대로 전송
                    serialPuts(uart_fd, buffer);
                    serialPuts(uart_fd, "\n"); // 개행 추가

                    serialPuts(uart_fd2, buffer);
                    serialPuts(uart_fd2, "\n"); // 개행 추가
                    serialFlush(uart_fd);
                    serialFlush(uart_fd2);
                    printf("[TX] Sent: %s\n", buffer);

                    // 버퍼 초기화
                    memset(buffer, 0, BUFFER_SIZE);
                    index = 0;
                }
            }
        }
        delay(10);
    }

    return NULL;
}


void* i2c_thread(void* arg) {
    int file;
    char* filename = "/dev/i2c-1";
    if ((file = open(filename, O_RDWR)) < 0) {
        perror("Failed to open the i2c bus");
        return NULL;
    }

    if (ioctl(file, I2C_SLAVE, I2C_ADDR) < 0) {
        perror("Failed to acquire bus access and/or talk to slave");
        return NULL;
    }

    pinMode(BUTTON_PIN, INPUT);
    pullUpDnControl(BUTTON_PIN, PUD_UP);  // 풀업 저항 설정

    while (1) {
        char config[1] = {0x40};
        write(file, config, 1);

        char data[1];
        read(file, data, 1);
        int x_val = data[0];

        config[0] = 0x41;
        write(file, config, 1);
        read(file, data, 1);
        int y_val = data[0];

        int button_state = digitalRead(BUTTON_PIN);

        printf("X: %d, Y: %d, Button: %s\n", x_val, y_val, button_state == LOW ? "Released" : "Pressed");
        unsigned char dat;
        //dat=(unsigned char)0;
        fflush (stdout) ;
        char message[16];
        snprintf(message, sizeof(message), "%d,%d\n", x_val, y_val+20); // 좌표 값을 문자열로 변환
        write(fd_serial, message, strlen(message)); // 전체 문자열 전송

        //s//erialWrite(fd_serial, (unsigned char)x_val);
        //serialWrite(fd_serial, ' '); // 값 사이에 공백 추가
        //serialWrite(fd_serial, (unsigned char)y_val);
        //serialWrite(fd_serial, '\n'); // 메시지 끝에 개행 추가
        //serialWrite(fd_serial, dat); // UART1에 echo
        
        
        //fflush (stdout) ;
        //serialWrite(fd_serial, dat); // UART1에 echo
        //dat=(unsigned char)y_val;
        //fflush (stdout) ;
        //serialWrite(fd_serial, dat); // UART1에 echo
        //unsigned char data2[3];
        //data2[0] = (unsigned char)0; // 신호 값 (0~255)
        //data2[1] = (unsigned char)x_val;   // x 값
        //data2[2] = (unsigned char)y_val;   // y 값
        //fflush (stdout) ;
        //serialWrite(fd_serial, data2);// UART1에 echo
        //serialWrite(fd_serial, data2);// UART1에 echo
        if(button_state!=LOW)
        {
            char message2[16];
            snprintf(message2, sizeof(message2), "3,0\n"); // 좌표 값을 문자열로 변환
            write(fd_serial, message2, strlen(message2)); // 전체 문자열 전송
            printf("[TX] Sent: %s\n", message2);
        }
        
        delay (10);
    }

    return NULL;
}
void* ButtonPress(void* arg)
{
    int i;
    pinMode(19, INPUT); 
    while(1)
    {
         char message2[16];

        if (digitalRead(19)==0) 
        {
            snprintf(message2, sizeof(message2), "3,1\n"); // 좌표 값을 문자열로 변환
            write(fd_serial, message2, strlen(message2)); // 전체 문자열 전송
            printf("[TX] Sent: %s\n", message2);
            delay(1000);
        }

        
    }
    return NULL;
}
/*void* spi_thread(void* arg)
{
    unsigned char buffer[100];
    short x, y= 0 , z= 0;
    float x_g, y_g, z_g;
    float roll, pitch;

    if(wiringPiSPISetupMode(SPI_CH, SPI_SPEED,SPI_MODE) == -1) return NULL;
    pinMode(CS_GPIO, OUTPUT);
    digitalWrite(CS_GPIO, HIGH);
    writeRegister_ADXL345(DATA_FORMAT, 0x09); 
    writeRegister_ADXL345(BW_RATE,0x0C); 
    writeRegister_ADXL345(POWER_CTL,0x08); 
    while(1)
    {
        readRegister_ADXL345(DATAX0,6,buffer); 
        x = ((short)buffer[2]<<8)|(short)buffer[1]; //X
        y = ((short)buffer[4]<<8)|(short)buffer[3]; //Y
        z = ((short)buffer[6]<<8)|(short)buffer[5]; //Z

        x_g = x * 0.0039; 
        y_g = y * 0.0039; 
        z_g = z * 0.0039;
        roll = atan2(y_g, z_g) * 57.2958; 
        pitch = atan2(-x_g, sqrt(y_g * y_g + z_g * z_g)) * 57.2958;

        //printf("%f %f %f\n", x_g, y_g, z_g); // X, Y, Z
        //printf("%.2f %.2f\n", roll, pitch);
        delay(500);

    }
    

    return NULL;
}*/

int main() {
    pthread_t uart, i2c, spi,receiverThread,receiverThread2,Button;

    wiringPiSetupGpio();

    // UART 포트 열기

    if ((uart_fd = serialOpen(DEVICE, BAUD_RATE2)) < 0) {

        fprintf(stderr, "Unable to open UART device: %s\n", DEVICE);

        return 1;

    }

    if ((uart_fd2 = serialOpen(DEVICE2, BAUD_RATE3)) < 0) {

        fprintf(stderr, "Unable to open UART device: %s\n", DEVICE2);

        return 1;

    }
    
    pthread_create(&receiverThread, NULL, receiveData, NULL);
    pthread_create(&receiverThread2, NULL, receiveData2, NULL);
    pthread_create(&uart, NULL, uart_thread, NULL);
    pthread_create(&i2c, NULL, i2c_thread, NULL);
    
    pthread_create(&Button, NULL, ButtonPress, NULL);

    pthread_join(receiverThread, NULL);
    pthread_join(uart, NULL);
    pthread_join(i2c, NULL);
    pthread_join(Button, NULL);         
    pthread_join(receiverThread2, NULL);

    return 0;
}
//메세지 규칙 공백을 기준으로 일단 보냄
//1 0 선풍기를 킴.
//1 1 선풍기를 회전.
//1 2 선풍기를 멈춤

//2 0 선풍기 1단계
//2 1 선풍기 2단계
//2 3 선풍기 3단계

//3 0 문을 열겠다.
//3 1 문을 닫음.
//206 170
