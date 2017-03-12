TO MAKE YOUR LIFE EASY

Instructions to run the program with the Makefile

-- To run the 2 tests type the following commands in the terminal.

make test1
./os_test1

make test2
./os_test2

-- Now to run the program individually and manually test the methods

make run
./os_run

You can also verify memory leaks with: valgrind ./os_run
Please type 'help' once in the program to display the main features.


Max values for comp310_a2_test.h (defaults are 4 and 256) and must match with config.h:
TEST_FORK_NUM = 30
TEST_MAX_KEY and TEST_MAX_POD_ENTRY = ~500 