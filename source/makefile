CC = gcc
#Using -Ofast instead of -O3 might result in faster code, but is supported only by newer GCC versions
#CFLAGS = -lm -pthread -O3 -march=native -Wall -funroll-loops -Wno-unused-result
CFLAGS = -lm -pthread -O3 -march=native -mtune=native -Wall -funroll-loops -Wno-unused-result

all: createVocab bruteForceSearch createTweetVocabMap sortDb exactSearchOpt

exactSearchOpt: exactSearchOpt.c
	$(CC) exactSearchOpt.c -o exactSearchOpt $(CFLAGS)
sortDb: sortDb.c
	$(CC) sortDb.c -o sortDb $(CFLAGS)
createTweetVocabMap: createTweetVocabMap.c
	$(CC) createTweetVocabMap.c -o createTweetVocabMap $(CFLAGS)
createVocab: createVocab.c
	$(CC) createVocab.c -o createVocab $(CFLAGS)
bruteForceSearch: bruteForceSearch.c
	$(CC) bruteForceSearch.c -o bruteForceSearch $(CFLAGS)
	chmod +x *.sh

clean:
	rm -rf createVocab bruteForceSearch createTweetVocabMap sortDb exactSearchOpt
