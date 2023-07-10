20 points now
<details>
<summary>stdout.txt</summary>
        
```text
        -------- main started --------
        -------- test_open started --------
ok - error when no such file
ok - errno is 'no_file'
ok - use 'create' now
ok - close immediately
ok - now open works without 'create'
ok - 'create' is not an error when file exists
ok - open second descriptor
ok - it is not the same in value
ok - close the second
ok - and the first
ok - deletion
ok - now 'create' is needed again
        -------- test_open done --------
        -------- test_close started --------
ok - close invalid fd
ok - errno is set
ok - close with seemingly normal fd
ok - close with huge invalid fd
ok - close normal descriptor
ok - close it second time
ok - errno is set
        -------- test_close done --------
        -------- test_io started --------
ok - write into invalid fd
ok - errno is set
ok - write into seemingly valid fd
ok - read from invalid fd
ok - errno is set
ok - read from seemingly valid fd
ok - data (only needed) is written
ok - data is read
ok - the same data
ok - read
ok - got data from start
ok - write more
ok - overwrite
ok - read rest
ok - got the tail
ok - read all
ok - check all
ok - data with zeros
ok - read data with zeros
ok - check with zeros
ok - write big data in parts
ok - read big data in parts
ok - data is correct
        -------- test_io done --------
        -------- test_delete started --------
ok - delete when opened descriptors exist
ok - write into an fd opened before deletion
ok - read from another opened fd - it sees the data
ok - exactly the same data
ok - write into it and the just read data is not overwritten
ok - read from the first one
ok - read from the first one again
ok - it reads data in correct order
ok - the existing 'ghost' file is not visible anymore for new opens
ok - errno is set
ok - the file is created back, no data
ok - but the ghost still lives
ok - and gives correct data
ok - delete it again
        -------- test_delete done --------
        -------- test_stress_open started --------
# open 1000 read and write descriptors, fill with data
# read the data back
        -------- test_stress_open done --------
        -------- test_max_file_size started --------
ok - can not write over max file size
ok - errno is set
# read works
        -------- test_max_file_size done --------
        -------- test_rights started --------
ok - file is opened with 'create' flag only
ok - it is allowed to read from it
ok - as well as write into it
ok - now opened without flags at all
ok - can read
ok - can write
ok - opened with 'read only'
ok - can read
ok - can not write
ok - errno is set
ok - can again read
ok - and data was not overwritten
ok - opened with 'write only
ok - can not read
ok - errno is set
ok - can write
ok - opened with 'read write
ok - can read
ok - data is correct
ok - can write
ok - write something
ok - write again but less
ok - data still here
ok - data is ok
        -------- test_rights done --------
        -------- main done --------
```
</details>

На моей системе код с heap_help виснет вечно, проверить не получилось, если получится то обновлю.

Но так как количество аллокаций равно количеству освобождений, то думаю что утечек нет. За не имением лучшего прикладываю valgrind:

```text
==239812== HEAP SUMMARY:
==239812==     in use at exit: 0 bytes in 0 blocks
==239812==   total heap usage: 415,677 allocs, 415,677 frees, 114,211,105 bytes allocated
==239812== 
==239812== All heap blocks were freed -- no leaks are possible
```
