# Metaverse IoT: 스마트 선풍기와 자동문 시스템

## 프로젝트 개요

**Metaverse IoT**는 IoT 기술과 메타버스 환경을 결합한 프로젝트로, 현실에서 동작하는 스마트 기기들을 Unity로 구현한 가상현실에서 실시간으로 시뮬레이션한다.
본 프로젝트는 스마트 선풍기와 자동문 두 가지 주요 시스템으로 구성되어 있으며, 총 3대의 Raspberry Pi를 활용하여 시스템 간의 유기적인 연결과 통신을 구현했다.

![KakaoTalk_20241220_111349252](https://github.com/user-attachments/assets/3c85586a-171f-4fdb-be99-41098872f360)
---
# Tech Stack

## Cross-Platform Integration
1. **임베디드 시스템**:
   - **OS**: ![Static Badge](https://img.shields.io/badge/Raspberry%20Pi-A22846?style=flat&logo=raspberrypi&logoColor=white)
   - **Hardware**: ![Static Badge](https://img.shields.io/badge/Raspberry%20Pi-A22846?style=flat&logo=raspberrypi&logoColor=white)
   - **Programming Language**: ![Static Badge](https://img.shields.io/badge/C-A8B9CC?style=flat&logo=c&logoColor=white)
   - **Communication**: 
        - Bluetooth: HC-06 Bluetooth 모듈을 사용해 중앙 Raspberry Pi와 Unity 가상환경 및 스마트폰 간의 무선 데이터 송수신.
        - UART (유선 통신): 중앙 Raspberry Pi가 스마트 선풍기와 자동문 제어 Raspberry Pi와 데이터를 주고받기 위해 사용.

2. **가상환경**:
   - **OS**: ![Static Badge](https://img.shields.io/badge/Android-34A853?style=flat&logo=android&logoColor=white)
   - **Engine**: ![Static Badge](https://img.shields.io/badge/unity3D-FFFFFF?style=flat&logo=unity&logoColor=black)
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
  - Unity 엔진을 사용하여 현실의 동작(선풍기 on/off/회전, 자동문 열림/닫힘)을 가상현실에서 실시간 반영
  - 가상현실에서의 기기조작(선풍기 on/off/회전, 가까이 접근 및 위험 감지, 자동문 접근)을 현실에서 실시간 반영

- **통신 방식**:
  - 중앙 Raspberry Pi와 선풍기, 자동문을 조작하는 Raspberry Pi와 유선 UART통신을 통해 
    기기에 입력되는 값(선풍기 on/off/회전, 초음파센서 감지 값, 자동문 초음파센서 감지 값)을 전달받음
  - 중앙 Raspberry Pi와 Bluetooth로 통신하여 기기의 상태를 전달받아 Unity에서 반영
---


