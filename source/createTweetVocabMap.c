#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

#define MAX_WORD_COUNT 100
#define MAX_STRING 40
#define MAX_STRING_FNAME 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_LENGTHS 50
#define MAX_TWEET_LEN 80
#define BATCH_SIZE 8800000 

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

struct vocab_word {
  char *word;
  unsigned cn, twt_count, posInDb;
//  int twtlenLoc[50];
};

struct tweet_dB_t {
  unsigned *vec;
  unsigned size;
  unsigned id;
};

struct tweetList_t {
  unsigned id;
  unsigned len;
};

char dbin_file[MAX_STRING_FNAME], read_vocab_file[MAX_STRING_FNAME], save_vocab_file[MAX_STRING_FNAME],save_vocab_twt_map[MAX_STRING_FNAME];; 
//struct image_vocab *imagenetvocab; 
struct vocab_word *vocab;
struct tweet_dB_t *tweetDb;
struct tweet_dB_t *querytweetDb;

unsigned twtdb_max_size = 1, querydb_max_size = 1000;
int binary = 0;
int *vocab_hash;
long long vocab_max_size = 9500000, tweet_count=0, query_count=0, image_vocab_max_size = 2500, vocab_size = 0,imgvocab_size = 0, layer1_size = 100;
long long train_words = 0, word_count_actual = 0, iter = 5, file_size = 0, classes = 0;
real avg_length=0, avg_tweet_length=0; 
clock_t start;


void ReadWord(char *word, FILE *fin) {
  int a = 0, ch;
  while (!feof(fin)) {
    ch = fgetc(fin);
    if (ch == 13) continue;
    if ((ch == ' ') || (ch == '\t') || (ch == '\n')) {
      if (a > 0) {
        if (ch == '\n') { 
		tweet_count++;
		ungetc(ch, fin);
		}
        break;
      }
      if (ch == '\n') {
        strcpy(word, (char *)"</s>");
        return;
      } else continue;
    }
    word[a] = ch;
    a++;
    if (a >= MAX_STRING - 1) a--;   // Truncate too long words
  }
  word[a] = 0;
}

// Adds a word to the vocabulary
int AddWordToVocab(char *word) {
  unsigned int length = strlen(word) + 1;
  
  if (length > MAX_STRING) 
  	length = MAX_STRING;
  
  if((vocab[vocab_size].word == NULL)) {
  	printf("No More Memory!! %f %lld %lld %lld\n",avg_length,vocab_size,train_words,tweet_count);
  }
  
  strcpy(vocab[vocab_size].word, word);
  if(vocab[vocab_size].word[0]==0){
  	printf("Something Fishy! %s %s %u\n",vocab[vocab_size].word, word,length);
  }
  vocab[vocab_size].cn = 0;
  vocab_size++;
  avg_length = (avg_length * (vocab_size-1) + length)/vocab_size;  

  // Reallocate memory if needed
  return vocab_size - 1;
}

int ListCompare(const void *a, const void *b) {
    return ((struct tweetList_t *)a)->len - ((struct tweetList_t *)b)->len;
}

// Reduces the vocabulary using the technique input


int ArgPos(char *str, int argc, char **argv) {
  int a;
  for (a = 1; a < argc; a++) if (!strcmp(str, argv[a])) {
    if (a == argc - 1) {
      printf("Argument missing for %s\n", str);
      exit(1);
    }
    return a;
  }
  return -1;
}

int GetNextDb(struct tweet_dB_t *tweetDbL, FILE* fin) {
  unsigned tcnt = 0,tweet_id, length;
  
  if (fin == NULL) {
    printf("DBfile file not found\n");
    exit(1);
  }
  
  fread(&tweet_id, sizeof(unsigned), 1, fin);
  if (feof(fin)) return 0;
  fread(&length, sizeof(unsigned), 1, fin);
  if (feof(fin)) return 0;
  tweetDbL[tcnt].id = tweet_id;
  tweetDbL[tcnt].size = length;

  if(length > 45)
  	printf("WARNING Something wrong %u\n",length);

  fread(tweetDbL[tcnt].vec, sizeof(unsigned), length, fin);
  tcnt++;

  return tcnt;
}

unsigned searchWordInTweet(unsigned t1,struct tweet_dB_t t2) {
	unsigned j;
	
	for(j=0;j<t2.size;j++)
		if(t1 == t2.vec[j]) { 
			return 1;
		}
	return 0;
}

int updateWordTwtMap(unsigned id, unsigned word, FILE* fout) {
  long long bytePosInDb;
  
  bytePosInDb = (vocab[word].posInDb + vocab[word].twt_count)*sizeof(unsigned); 
  vocab[word].twt_count++;
  fseek (fout, bytePosInDb , SEEK_SET);
  fwrite(&id, sizeof(unsigned), 1, fout);
   
  return 1;
}

int updateWordLenList(unsigned len, unsigned word, FILE* fInOut) {
  long long bytePosInDb;
  int curr_entry;
  
  bytePosInDb = (word*(2+MAX_LENGTHS)+ 1 + len)*sizeof(unsigned); 
  fseek (fInOut, bytePosInDb , SEEK_SET);

  fread(&curr_entry, sizeof(unsigned), 1, fInOut);
 
  if(curr_entry==-1) { 

  	fseek (fInOut, bytePosInDb , SEEK_SET);
  	fwrite(&vocab[word].twt_count, sizeof(unsigned), 1, fInOut);
  }
   
  return 1;
}

void ReadVocabAndCreateBlankFiles(FILE* foutVoc, FILE* foutMap) {
  int lenLoc[MAX_LENGTHS];
  unsigned twtlstLen = 0;
  unsigned posinDb = 0, twt_id = 0;
  unsigned i =0;
  char c;
  char word[MAX_STRING];
  
  FILE *fin = fopen(read_vocab_file, "rb");

  if (fin == NULL) {
    printf("Vocabulary file not found\n");
    exit(1);
  }

  for(i=0;i<MAX_LENGTHS;i++)
  	lenLoc[i] = -1;

  vocab_size = 0;
  while (1) {
    ReadWord(word, fin);
    if (feof(fin)) break;
    fscanf(fin, "%u%c", &vocab[vocab_size].cn, &c);
	vocab[vocab_size].posInDb  = posinDb;
	vocab[vocab_size].twt_count = 0;
	
	fwrite(&twtlstLen, sizeof(unsigned), 1, foutVoc);
	fwrite(&vocab[vocab_size].posInDb, sizeof(unsigned), 1, foutVoc);
	fwrite(&lenLoc[0], sizeof(unsigned), MAX_LENGTHS, foutVoc);
	
	for (i = 0; i < vocab[vocab_size].cn; i++)
		fwrite(&twt_id, sizeof(unsigned), 1, foutMap);
	
	posinDb += vocab[vocab_size].cn;
    vocab_size++;
  }


  fseek (foutVoc, 0 , SEEK_SET);
  fseek (foutMap, 0 , SEEK_SET);
  
  printf("Vocab size: %lld\n", vocab_size);
  fclose(fin);
}

void createVocabTwtMapping(struct tweet_dB_t *Db) {
  char word[MAX_STRING], c;
  FILE *finDb, *finVocab;
  FILE *foutVocab, *foutMap;
  long long i,j;
  unsigned tcnt = 0; 
  
  finDb = fopen(dbin_file, "rb");
  finVocab = fopen(read_vocab_file, "rb");
  foutVocab = fopen(save_vocab_file, "wb+");
  foutMap = fopen(save_vocab_twt_map, "wb+");

  if ((finDb == NULL) || (finVocab == NULL)) {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  
  ReadVocabAndCreateBlankFiles(foutVocab, foutMap);
  
  //Now get all the tweets in which this word appears 
  tcnt = 0;
  
  while (GetNextDb(Db, finDb)) {
  	for(j=0;j<Db[0].size;j++) {
		updateWordTwtMap(tcnt,Db[0].vec[j], foutMap);
		updateWordLenList(Db[0].size, Db[0].vec[j], foutVocab);
  	}
  	tcnt++;
  	if((tcnt%100000) == 0)  {
		printf("On  %u%c", tcnt,13);
      	fflush(stdout);
	}
  }

  fseek (foutVocab, 0 , SEEK_SET);
  
  // update Lengths in Vocab file
  for (j =0; j< vocab_size; j++) {
  	
	fseek (foutVocab, (long long)(j*(2+MAX_LENGTHS)*sizeof(unsigned)), SEEK_SET);
	fwrite(&vocab[j].twt_count, sizeof(unsigned), 1, foutVocab);
		
  	
	if((j %1000000) == 0)  {
		printf("%s %u %u %u\n", vocab[j].word, vocab[j].cn, vocab[j].twt_count, vocab[j].posInDb);
      	fflush(stdout);
	}
  }

  fclose(finDb);
  fclose(finVocab);
  fclose(foutVocab);
  fclose(foutMap);
}

int main(int argc, char **argv) {
  
  int i;
  if (argc == 1) {
    return 0;
  }
 srand(time(NULL)); 
 if ((i = ArgPos((char *)"-readDb", argc, argv)) > 0) strcpy(dbin_file, argv[i + 1]);
 if ((i = ArgPos((char *)"-read-vocab", argc, argv)) > 0) strcpy(read_vocab_file, argv[i + 1]);
 if ((i = ArgPos((char *)"-save-vocab", argc, argv)) > 0) strcpy(save_vocab_file, argv[i + 1]);
 if ((i = ArgPos((char *)"-save-map", argc, argv)) > 0) strcpy(save_vocab_twt_map, argv[i + 1]);
 if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
		
 vocab = (struct vocab_word *)calloc(BATCH_SIZE, sizeof(struct vocab_word));
 if (vocab == NULL) {
 	printf("OUT OF MEMORY!! \n");
	exit (1);
 }

 printf("size of Vocab struct is %ld %ld %d \n",sizeof(struct vocab_word),sizeof(unsigned),RAND_MAX);
 
 tweetDb = (struct tweet_dB_t *)calloc(twtdb_max_size, sizeof(struct tweet_dB_t));
 if(tweetDb == NULL)
 	printf("NULL WTF!!!!!!!!!!\n");
 
 tweetDb[0].vec = (unsigned *)calloc(MAX_TWEET_LEN, sizeof(unsigned));

 createVocabTwtMapping(tweetDb);
  	
  
  return 0;
}
