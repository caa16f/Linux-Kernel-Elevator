
Project2

Part 1
-System-call Tracing

Part 2
-Kernel Module

Part 3

makefile 
-compile with make sudo

elevator_calls_wrappers.c 
- used for syscall wrappers

elevator.c 
- runs elevator

Known Bugs: 
elevator.c: seems to compile with producer.c and consumer.c, but test.c has sporadic behavior.
            We believe the issue is caused by either int elevator_open or ssize_t elevator_read.
            Start and Stop commands seem to work but the issue request tests have a problem when outputing proc.
            
