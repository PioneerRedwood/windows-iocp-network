# Self study for Windows IOCP

#### boost-asio를 익힌 뒤에 [Parent git repository](https://github.com/zeliard/GSP) 를 참고하여 windows IOCP를 학습, 주관적으로 두 네트워크 라이브러리의 차이점에 대해서도 논할 예정

boost-asio는 C++ 크로스 플랫폼 네트워크 라이브러리이며, 크게 커널 버전에 신경 쓰지 않고 윈도우 실행환경에선 IOCP를 사용하며 리눅스 실행환경에선 Epoll을 사용한다. 



## zeliard/GSP

### #1 EchoServer

- EduEchoServer
  - 프로젝트 메인 진입점
  - 클라인언트 연결을 수락하는 ACCEPT, 입출력을 처리하는 IO_WORKER 두개의 스레드로 동작
- **ClientSession**
  - 입출력, 연결 등의 작업에 필요한 세션 클래스, 아래는 세션에 있는 변수들
    - OverlappedIOContext 구조체; **OVERLAPPED** 객체와 버퍼가 있다.
    - SOCKET, mutex
- SessionManager
  - 세션을 생성하고 세션 연결을 관리하는 매니저
- Exception
  - process.h, Dbghelp.h, minidumpapiset.h, handleapi.h, debugapi.h, winnt.h, minwindef.h 등 windows api들이 사용된다.
  - CRASH_ASSERT와 예외처리를 위한 헬퍼 클래스
- FastSpinlock
  - timeapi.h, syncapi.h
  - 멀티 스레드에서 상호배제를 위한 클래스
- **IocpManager**
  - 핵심 IOCP 매니저
  - 초기화, 종료, IO스레드 시작, accept 시작, 송수신 핸들러 (Completion)



### #2 Enhanced EchoServer

- CircularBuffer
  - A, B 구역이 나뉜 버퍼 클래스









## 학습에 참고가 된 감사한 분들 👏

- https://onecellboy.tistory.com/126 
- https://github.com/zeliard/GSP