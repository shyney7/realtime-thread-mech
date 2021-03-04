# realtime-thread-mech
Approach of reaching realtime code execution with the help of a controller thread. The Controller Thread wakes up after a predefined time e.g. 200ms and checks if the task was executed in time. If not the task thread is cancalled and restarted again.
