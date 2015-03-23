#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

#define MAX_TWEETS 15000000
#define MAX_WORD_COUNT 100
#define MAX_STRING 40
#define MAX_STRING_FNAME 100
#define EXP_TABLE_SIZE 1000
#define MAX_EXP 6
#define MAX_SENTENCE_LENGTH 1000
#define MAX_CODE_LENGTH 40
#define MAX_TWEET_LEN 80
#define BATCH_SIZE 400 

typedef float real;                    // Precision of float numbers

struct vocab_word {
  char *word;
  unsigned cn, twt_count, posInDb;
  int twtlenLoc[50];
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

char dbin_file[MAX_STRING_FNAME], save_dB_file[MAX_STRING_FNAME], save_sort_key[MAX_STRING_FNAME];
//struct image_vocab *imagenetvocab; 
struct vocab_word *vocab;
struct tweet_dB_t *tweetDb;
struct tweet_dB_t *querytweetDb;

unsigned twtdb_max_size = 1, querydb_max_size = 1000;
int *vocab_hash;
long long vocab_max_size = 9500000, tweet_count=0, query_count=0, image_vocab_max_size = 2500, vocab_size = 0,imgvocab_size = 0, layer1_size = 100;
long long train_words = 0, word_count_actual = 0, iter = 5, file_size = 0, classes = 0;
real avg_length=0, avg_tweet_length=0; 
clock_t start;

struct tweetList_t *tweetList;

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

int GetNthDb(unsigned n, unsigned prev, struct tweet_dB_t *tweetDbL, FILE* fin) {
  unsigned tcnt = 0, length;
  
  if (fin == NULL) {
    printf("DBfile file not found\n");
    exit(1);
  }

  if (n < prev) {
  	fseek (fin, 0 , SEEK_SET);
  	prev = 0;	
  }
  
  for(tcnt = 0; tcnt < (n-prev); tcnt++) {
  	fseek (fin, 1*sizeof(unsigned) , SEEK_CUR);
  	if (feof(fin)) return 0;
  	fread(&length, sizeof(unsigned), 1, fin);
  	if (feof(fin)) return 0;
  	fseek (fin, length*sizeof(unsigned) , SEEK_CUR);
  	if(length > 45)
  		printf("WARNING Something wrong %u\n",length);
   }
   
   GetNextDb(tweetDbL, fin);

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

void sortDb(struct tweet_dB_t *Db) {
  char word[MAX_STRING], c;
  FILE *finDb;
  FILE *foutDb;
  FILE *foutKey;
  long long i,j;
  unsigned tcnt = 0;
  unsigned prev = 0;
  
  finDb = fopen(dbin_file, "rb");
  foutDb = fopen(save_dB_file, "wb");
  foutKey = fopen(save_sort_key, "wb");

  if ((finDb == NULL) ) {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  
  while (GetNextDb(Db, finDb)) {
	tweetList[tcnt].id = tcnt;
	tweetList[tcnt].len = Db[0].size;
	tcnt++;
  }
  
  printf("Beginning Sorting!! \n");
  qsort(&tweetList[0], tcnt, sizeof(struct tweetList_t), ListCompare);
  printf("Done Sorting!! \n Now to write\n");
  fseek (finDb, 0 , SEEK_SET);
  prev = 0;

  fwrite(&tcnt, sizeof(unsigned), 1, foutKey);
  for (j=0; j< tcnt ; j++) {
  	GetNthDb(tweetList[j].id, prev, Db, finDb);
	prev = tweetList[j].id+1;
	if(1){
		fwrite(&Db[0].id, sizeof(unsigned), 1, foutDb);
		fwrite(&Db[0].size, sizeof(unsigned), 1, foutDb);
		fwrite(&Db[0].vec[0], sizeof(unsigned), Db[0].size, foutDb);
		
		// Save the sort key
		//fwrite(&j, sizeof(unsigned), 1, foutKey);
		fwrite(&Db[0].id, sizeof(unsigned), 1, foutKey);
	} else {
		fprintf(foutDb, "%u  %u ", Db[0].id,Db[0].size);
		for(i=0;i<Db[0].size;i++) 
			fprintf(foutDb, "%u ", Db[0].vec[i]);
		fprintf(foutDb, "\n");
	}
	if ( (j%10000) == 0)
		printf("On %u%c",j,13);
  }

  fclose(foutKey);
  fclose(finDb);
  fclose(foutDb);
}


int main(int argc, char **argv) {
  
  int i;
  if (argc == 1) {
    return 0;
  }
  srand(time(NULL)); 
  if ((i = ArgPos((char *)"-readDb", argc, argv)) > 0) strcpy(dbin_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-saveDb", argc, argv)) > 0) strcpy(save_dB_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-saveSortKey", argc, argv)) > 0) strcpy(save_sort_key, argv[i + 1]);
 
	tweetList = (struct tweetList_t *)calloc(MAX_TWEETS, sizeof(struct tweetList_t));
	if (tweetList == NULL) {
		printf("OUT OF MEMORY!! \n");
		exit (1);
	}
  
 tweetDb = (struct tweet_dB_t *)calloc(twtdb_max_size, sizeof(struct tweet_dB_t));
 if(tweetDb == NULL)
 	printf("NULL WTF!!!!!!!!!!\n");
 
 tweetDb[0].vec = (unsigned *)calloc(MAX_TWEET_LEN, sizeof(unsigned));

 sortDb(tweetDb);
  	
  
  return 0;
}
