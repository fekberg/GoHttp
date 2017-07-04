all: 
	gcc ../src/daemonize.c daemonize_test.c test_suite.c -o test_suite ../lib/libmock.a `pkg-config --cflags --libs check` `../bin/generate_mock_linker_flags.sh daemonize_test.c`
