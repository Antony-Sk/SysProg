Test on my mac with 6 files 40k each, 6 coroutines and 0 target latency:
```text
pojiloyproger@pojiloyproger-x 1 % ./a.out 0 6 test1.txt test2.txt test3.txt test4.txt test5.txt test6.txt
Coro #0: time: 0.019295 sec, switch count: 19169, finished with status 0
Coro #1: time: 0.019346 sec, switch count: 19187, finished with status 0
Coro #2: time: 0.019391 sec, switch count: 19201, finished with status 0
Coro #3: time: 0.019405 sec, switch count: 19204, finished with status 0
Coro #4: time: 0.019510 sec, switch count: 19291, finished with status 0
Coro #5: time: 0.020265 sec, switch count: 20084, finished with status 0
All files sorted and merged. Overall time: 0.156430
```
6 files 40k each, 3 coroutines and 1000 target latency:
```text
pojiloyproger@pojiloyproger-x 1 % ./a.out 1000 3 test1.txt test2.txt test3.txt test4.txt test5.txt test6.txt 
Coro #0: time: 0.036392 sec, switch count: 108, finished with status 0
Coro #1: time: 0.036595 sec, switch count: 109, finished with status 0
Coro #2: time: 0.036548 sec, switch count: 109, finished with status 0
All files sorted and merged. Overall time: 0.148806
```

I check result in code and print "All files sorted and merged" if everything is okay. I do not create additional file for result.