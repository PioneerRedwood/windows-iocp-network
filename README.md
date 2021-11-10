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



### #3, 4 Advanced Server

감사하게도 많은걸 배우고 있다.. 🙏



#### Middle check

##### zeliard/GSP Homework #1~4

지금껏 윈도우 프로그래밍에서 쓰이는 테크닉, 각종 자료구조와 알고리즘들을 따라 코딩해보며 옅볼수 있었다. 단순히 코딩한다고 해서 내것이 되는 것이 아니기 때문에 추후에 여기서 쓰인 기술들을 참고해 적용할 것이며 처음 봐서 몰랐거나 헷갈리는 작업들은 모두 기록해놓을 예정이다.

IOCP 기본.

Input Output Completion Port



지금은 ObjectPool, MemoryPool, 

##### MSSQL DB usage from zeliard/GSP Homework #5





## 학습에 참고가 된 감사한 분들 👏

- WinSock IOCP 펌 https://onecellboy.tistory.com/126 
- 학습 기초 자료 https://github.com/zeliard/GSP
- WinNT.h에 있는 단방향 연결 리스트(SLIST)에 대한 정리 
  - 다른 분의 한글 설명 https://lacti.github.io/2011/08/03/interlocked-singly-linked-lists/
  - 공식 문서 https://docs.microsoft.com/en-us/windows/win32/sync/interlocked-singly-linked-lists

