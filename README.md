# Simple-Key-Value-Store

A key value store database written in C that allows a user to read and write to the database. To store the value, the program follows the concept of a shared memory (to be able to retrieve the values from different process life) which later are saved to the disk. Also, to solve the problem of the readers and writers, and allow concurrency, the program is implemented with semaphores.
