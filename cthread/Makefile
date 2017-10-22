#
# Makefile de EXEMPLO
#
# OBRIGATÓRIO ter uma regra "all" para geração da biblioteca e de uma
# regra "clean" para remover todos os objetos gerados.
#
# É NECESSARIO ADAPTAR ESSE ARQUIVO de makefile para suas necessidades.
#  1. Cuidado com a regra "clean" para não apagar o "support.o"
#
# OBSERVAR que as variáveis de ambiente consideram que o Makefile está no diretótio "cthread"
# 

CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src
TST_DIR=./testes

all: objeto movetoBin gloriousLib movetoLib

objeto: $(SRC_DIR)/cthread.c $(INC_DIR)/cdata.h $(INC_DIR)/cthread.h $(INC_DIR)/support.h
	$(CC) -c $(SRC_DIR)/cthread.c -Wall

movetoBin:
	mv cthread.o $(BIN_DIR)

gloriousLib:
	ar crs libcthread.a $(BIN_DIR)/*.o

movetoLib:
	mv libcthread.a $(LIB_DIR)

clean:
	rm -rf $(BIN_DIR)/cthread.o $(LIB_DIR)/libcthread.a

