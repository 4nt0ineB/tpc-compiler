# tpc-compiler
Project Small compiler for the TPC language, a subset of C.
The purpose of this project during the final year of my cs degree was to introduce the concepts and methods of compilation.

produces the executable in bin/tpcc
```sh
make
```

runs the tests and gives a report in test/tpc-test.txt
```sh
make test
```


### tpcc help
```sh
SYNOPSIS
	./tpcas [OPTION]
	./tpcas [OPTION] [FILE]

DESCRIPTION
	 Parses inputs (from stdin or FILE) as TPC formated text

	-h, --help
		display this help and exit

	-t, --tree
		parse the input and display the abstract syntax tree

	-d [file]
		produce a dot formated file from the abstract syntax tree

	-s display symbol tables
```
