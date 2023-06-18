CC = gcc
CFLAGS = -Wall
LDFLAGS = -lfl 
BIN = bin/
OBJ = obj/
SRC = src/

# Nom Projet
NPROJ=tpc
# Nom Analyseur Lexical
AL=$(NPROJ)-l
# Nom Analyseur Syntaxique
AS=$(NPROJ)-s
# Nom parser
EXEC=$(BIN)tpcc
ARGS=test/good/4-while.tpc

# Test
TESTDIR=test
GOOD_TESTS= $(shell find $(TESTDIR)/good/ -type f | sort -n);
SYN_ERR_TESTS= $(shell find $(TESTDIR)/syn-err/ -type f | sort -n);
SEM_ERR_TESTS= $(shell find $(TESTDIR)/sem-err/ -type f | sort -n);
WARN_ERR_TESTS=$(shell find $(TESTDIR)/warn/ -type f | sort -n);
TEST_REPORT=$(TESTDIR)/$(NPROJ)-test.txt


# Other module for the parser
MODULES=tree SymbolTable Compile

# makefile non verbeux
MAKEFLAGS += #--silent

all: compiler

###### AnaLyseur lexical #####
al: makeoutdir $(SRC)$(AL)

# exe
$(SRC)$(AL): $(SRC)$(AL).yy.o
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRC)$(AL).yy.c: $(SRC)$(NPROJ).lex
	flex -o $@ $< 

##### Analyseur syntaxique #####
as: makeoutdir $(SRC)$(AS).tab.c

$(SRC)$(AS).tab.c: $(SRC)$(NPROJ).y 
	bison -d -v -o $(SRC)$(AS).tab.c $< 

##### Parser ####
# as + al + other module
compiler: makeoutdir $(EXEC)

$(EXEC): $(OBJ)$(AS).tab.o $(OBJ)$(AL).yy.o  $(addprefix $(OBJ), $(addsuffix .o, $(MODULES)))   
	$(CC) -o $(EXEC) $^ $(LDFLAGS)


test: compiler
##### Test parser ####
# @echo "# 0 mean no error\n# 1 mean error" > $(TEST_REPORT)
# @echo "#[Good tests] all tests should give 0\n" >> $(TEST_REPORT)
# for t in $(GOOD_TESTS)  do \
# 	./$(EXEC) < $$t > /dev/null 2>&1 ; \
# 	echo "$(basename $$t) : $$?" >> $(TEST_REPORT) ; \
# done
# @echo "\n\n#[Syn-err tests] all tests should give 1\n" >> $(TEST_REPORT)
# for t in $(SYN_ERR_TESTS)  do \
# 	./$(EXEC) < $$t  > /dev/null 2>&1 ; \
# 	echo "$(basename $$t) : $$?" >> $(TEST_REPORT) ; \
# done
##### Test compiler #### 
	@echo "\n# 0 mean no error\n# any other value means error" > $(TEST_REPORT)
	@echo "#[Sem-err tests] all tests should give 2\n" >> $(TEST_REPORT)
	for t in $(SEM_ERR_TESTS) do \
		./$(EXEC) < $$t > /dev/null 2>&1 ; \
		echo "$(basename $$t) : $$?" >> $(TEST_REPORT) ; \
	done
	@echo "\n\n# 0 mean no error\n# any other value means error" >> $(TEST_REPORT)
	@echo "#[Warn tests] all tests should give 0\n" >> $(TEST_REPORT)
	for t in $(WARN_ERR_TESTS) do \
		./$(EXEC) < $$t > /dev/null 2>&1 ; \
		echo "$(basename $$t) : $$?" >> $(TEST_REPORT) ; \
	done
 

##### AST to dot #####
todot: parser
	./$(EXEC) < $(ARGS)
	dot -Tpdf "ast.dot" -o "ast.pdf"
	evince ast.pdf

##### Util #####
$(OBJ)%.o: $(SRC)%.c
	$(CC) -o $@ -c $< $(CFLAGS) $(LDFLAGS)

clean:
	@rm -rf $(SRC)$(AL).yy.* $(SRC)*.tab.* $(OBJ)*.o ast.dot ast.pdf \
	 $(EXEC) $(TREPORT) $(AS) $(AL) test/*-test.txt $(SRC)*.output
	@rm -rf *.o _anonymous bin/_anonymous
	@rm -rf *.dot
	@rm -rf *.pdf
	@rm -rf $(ASM_EXEC).asm
	@rm -rf ./bin/utils.asm

# Dependances
$(OBJ)$(AS).tab.o: $(SRC)$(AS).tab.c $(SRC)$(AS).tab.h $(SRC)tree.h
$(OBJ)$(AL).yy.o: $(SRC)$(AL).yy.c $(SRC)$(AS).tab.h
$(OBJ)tree.o: $(SRC)tree.c $(SRC)tree.h
$(OBJ)SymbolTable.o: $(SRC)SymbolTable.c $(SRC)SymbolTable.h
$(OBJ)Compile.o: $(SRC)Compile.c $(SRC)Compile.h

makeoutdir:
	@mkdir -p $(BIN) $(OBJ)
	@cp src/utils.asm bin/utils.asm

####### ASM
ASM_OBJ= _anonymous.o

ASM_EXEC= _anonymous

$(ASM_EXEC): $(ASM_OBJ)
	$(CC) $^ -no-pie -nostartfiles -g -o bin/$@

%.o:%.asm
	nasm -F dwarf -g -lc  -felf64 -o $@ $<

exo%.o: exo%.asm
utils.o: utils.asm
_anonymous.o: _anonymous.asm
