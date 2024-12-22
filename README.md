# Metaverse IoT: 스마트 선풍기와 자동문 시스템

## 개발 배경 및 필요성
IoT 기술과 가상현실을 융합하여 기존 스마트 기기의 활용도를 확장하고, 현실과 가상 공간을 연결해 스마트 기기의 제어 및 동기화 가능성을 탐구하고자 이 프로젝트를 진행하게 되었다.

## 프로젝트 개요

**Metaverse IoT**는 IoT 기술과 메타버스 환경을 결합한 프로젝트로, 현실에서 동작하는 스마트 기기들을 Unity로 구현한 가상현실에서 실시간으로 시뮬레이션하며, Unity에서 일어나는 동작을 현실에서도 동일하게 재현한다.
본 프로젝트는 스마트 선풍기와 자동문 두 가지 주요 시스템으로 구성되어 있으며, 총 3대의 Raspberry Pi와 Unity를 활용하여 시스템 간의 유기적인 연결과 통신을 구현했다.

![KakaoTalk_20241220_111349252](https://github.com/user-attachments/assets/3c85586a-171f-4fdb-be99-41098872f360)
---
# Tech Stack

## Cross-Platform Integration
1. **임베디드 시스템**:
   - **OS**: ![Static Badge](https://img.shields.io/badge/Raspberry%20Pi%20OS-A22846?style=flat&logo=raspberrypi&logoColor=white)
   - **Hardware**: ![Static Badge](https://img.shields.io/badge/Raspberry%20Pi%205-A22846?style=flat&logo=raspberrypi&logoColor=white)
   - **Programming Language**: ![Static Badge](https://img.shields.io/badge/C-A8B9CC?style=flat&logo=c&logoColor=white)
   - **Communication**: 
        - Bluetooth: HC-06 Bluetooth 모듈 및 Arduino Bluetooth Plugin (Unity)을 사용하여 중앙 Raspberry Pi와 Unity 가상환경 및 스마트폰 간의 무선 데이터 송수신.
        - UART (유선 통신): 중앙 Raspberry Pi가 스마트 선풍기와 자동문 제어 Raspberry Pi와 데이터를 주고받기 위해 사용.

2. **가상환경**:
   - **OS**: ![Static Badge](https://img.shields.io/badge/Android-34A853?style=flat&logo=android&logoColor=white)
   - **Engine**: ![Static Badge](https://img.shields.io/badge/unity%203D-FFFFFF?style=flat&logo=unity&logoColor=black)
   - **Programming Language**: ![Static Badge](https://img.shields.io/badge/C%23-512BD4?style=flat&logoColor=white)

# 시스템 설계

## 하드웨어 구성 및 기능

### 1. 스마트 선풍기
- **기능**:  
  - Bluetooth 통신을 통한 on/off, 회전 조작  
  - 선풍기에 가까이 접근하면 선풍기 정지 및 경고음 출력
  - LED를 이용한 풍속 가시화

- **하드웨어 구성**:  
  - Raspberry Pi (제어용)  
  - 능동부저
  - LED
  - Keyes 140C04 모듈 (L9110 모터드라이버, DC 모터)
  - SG90 서보모터  
  - HC-06 Bluetooth 모듈  
  - HC-SR04 초음파 센서
---

### 2. 자동문
- **기능**:  
  - 초음파 센서를 통해 문 앞에 사람이 감지되면 문이 자동으로 열림  
  - 사람이 없을 경우 문이 닫힘  

- **하드웨어 구성**:  
  - Raspberry Pi (제어용)  
  - SG90 서보모터  
  - HC-SR04 초음파 센서
---

### 3. 가상현실 조작 조이스틱
- **기능**:  
  - 가상현실의 플레이어는 조이스틱 모듈을 이용해 이동 및 점프가능

- **하드웨어 구성**:  
  - Raspberry Pi (제어용) 
  - HC-06 Bluetooth 모듈
  - 조이스틱 모듈
---

### 4. 가상현실
- **기능**:  
  - Unity 엔진과 Arduino Bluetooth Plugin을 사용하여 현실의 동작(선풍기 on/off/회전, 자동문 열림/닫힘)을 가상현실에서 실시간 반영
  - 가상현실에서의 기기조작(선풍기 on/off/회전, 가까이 접근 및 위험 감지, 자동문 접근)을 현실에서 실시간 반영

- **빌드 환경**:
   - Unity 버전: **2019.4.40f**
   - 안드로이드 애플리케이션으로 빌드
---

# 시스템 구성도
![시스템 구성도](https://github.com/user-attachments/assets/91a337bc-2f2b-4937-9009-a42c51dc158f)
---

# 제한 조건 구현

## Multi Thread 사용
   - 서보모터 제어
   - 초음파센서 제어
   - UART통신 제어
   - 조이스틱 및 버튼 입력 처리
     
## Mutex 사용
   - 서보모터 회전 활성화 여부
   - 서보모터의 현재 회전 위치
---

# 개발 시 문제점 및 해결방안

## 유니티와의 통신, 블루투스 통신, 라즈베리파이 간 통신
   - UART통신은 1 : 1 통신, 라즈베리파이의 저전압 경고 문제, 내장블루투스 연결 실패
      - 라즈베리파이 추가 TXD, RXD GPIO핀 직접 연결
    
## Unity와 라즈베리파이 간의 통신에서 동작상태 불일치
   - 명령과 센서 이벤트의 처리 로직 통합, 상태 확인 부족
      - 명령 분리, 상태 확인 및 중복 동작 방지
   - Unity에서 `IsTriggerStay`를 사용해 매 프레임 라즈베리파이로 데이터를 전송하다 보니, 과도한 정보가 전달되어 통신 과부하가 발생.
      - `IsTriggerStay`에서 매 프레임 데이터를 전송하지 않고, **Coroutine**을 사용해 데이터를 전송을 최적화.
    
## 서보모터의 오작동
   - 설정된 각도로 이동하는 과정에서 불필요하게 움찔거리는 현상이 발생
      - 서보모터가 설정된 위치로 이동한 뒤, 추가적인 신호가 전달되지 않도록 setServoAngle(-1)을 호출하여 PWM 신호를 완전히 차단
    
## 초음파센서의 인식 범위
   - 정면 인식 시 사각지대 발생
      - 밑에서 위로 인식하는 방식
---

# 성과
- 임베디드 시스템과 가상 환경의 원활한 통합
- 현실과 가상 공간 사이의 장치 상태 실시간 동기화 구현
- 현실과 가상현실의 융합을 통해 IoT와 메타버스 기술이 결합된 미래지향적인 프로그래밍 가능성을 제시

  ---
# 데모영상
https://youtu.be/P1sDLRWjP3w
