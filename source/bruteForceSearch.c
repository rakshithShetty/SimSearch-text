
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
#define MAX_CODE_LENGTH 40
#define MAX_TWEET_LEN 80

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

struct vocab_word {
  char *word;
  unsigned cn, twt_count;
  unsigned* tweetList;
};

struct tweet_dB_t {
  unsigned *vec;
  unsigned size;
  unsigned id;
};

char dbin_file[MAX_STRING_FNAME], train_file[MAX_STRING_FNAME], query_dB_file[MAX_STRING_FNAME];
char save_db_file[MAX_STRING_FNAME], read_vocab_file[MAX_STRING_FNAME], save_vocab_file[MAX_STRING_FNAME];
//struct image_vocab *imagenetvocab; 
struct vocab_word *vocab;
struct tweet_dB_t *tweetDb;
struct tweet_dB_t *querytweetDb;

unsigned twtdb_max_size = 15000000, querydb_max_size = 1000, indQuery = 0, indDB = 0;
int binary = 0, debug_mode = 2, min_count = 0, num_threads = 12, dim_red_type = 0, n_dim = 9000000, dBOnDisk = 0, vcbTwtMap = 0;
int *vocab_hash;
long long vocab_max_size = 9500000, tweet_count=0, query_count=0, image_vocab_max_size = 2500, vocab_size = 0,imgvocab_size = 0, layer1_size = 100;
long long train_words = 0, word_count_actual = 0, iter = 5, file_size = 0, classes = 0;
real avg_length=0, avg_tweet_length=0; 
clock_t start;

void randArray(unsigned N,unsigned M,unsigned *out) {
	int in, im;

	im = 0;

	for (in = 0; in < N && im < M; ++in) {
		int rn = N - in;
		int rm = M - im;
		if (rand() % rn < rm)    
			/* Take it */
			out[im++] = in; 
	}
}



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

// Returns hash value of a word
int GetWordHash(char *word) {
  unsigned long long a, hash = 0;
  for (a = 0; a < strlen(word); a++) hash = hash * 257 + word[a];
  hash = hash % vocab_hash_size;
  return hash;
}

// Returns position of a word in the vocabulary; if the word is not found, returns -1
/*int searchIdinVocab(unsigned long long id) {
  unsigned int i = 0;
  for (i = 0; i< imgvocab_size; i++) {
  	if(imagenetvocab[i].id == id) return i;
  }
  return -1;
}
*/
// Returns position of a word in the vocabulary; if the word is not found, returns -1
int SearchVocab(char *word) {
  unsigned int hash = GetWordHash(word);
  while (1) {
    if (vocab_hash[hash] == -1) return -1;
    if (!strcmp(word, vocab[vocab_hash[hash]].word)) return vocab_hash[hash];
    hash = (hash + 1) % vocab_hash_size;
  }
  return -1;
}

int SearchWordInTweet(unsigned *tweet,unsigned len, unsigned wordId) {
  unsigned int i = 0 ;
  while (i<len) {
    if (tweet[i] == wordId) return i;
	i++;
  }
  return -1;
}
// Reads a word and returns its index in the vocabulary
int ReadWordIndex(FILE *fin) {
  char word[MAX_STRING];
  ReadWord(word, fin);
  if (feof(fin)) return -1;
  return SearchVocab(word);
}

// Adds a word to the vocabulary
int AddWordToVocab(char *word) {
  unsigned int hash, length = strlen(word) + 1;
  
  if (length > MAX_STRING) 
  	length = MAX_STRING;
  
  vocab[vocab_size].word = (char *)calloc(length, sizeof(char));

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
  if (vocab_size + 2 >= vocab_max_size) {
    printf("Need More memory !! \n\n");
	vocab_max_size += 1000;
    vocab = (struct vocab_word *)realloc(vocab, vocab_max_size * sizeof(struct vocab_word));
    if(vocab == NULL) printf("NO More memory %lld %lld !! \n\n",vocab_max_size, train_words);
  }
  hash = GetWordHash(word);
  while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
  vocab_hash[hash] = vocab_size - 1;
  return vocab_size - 1;
}

// Used later for sorting by word counts
int VocabCompare(const void *a, const void *b) {
    return ((struct vocab_word *)b)->cn - ((struct vocab_word *)a)->cn;
}

void SortVocab() {
  int a, size;
  unsigned int hash;
  // Sort the vocabulary and keep </s> at the first position
  printf("Into Sorting Now !! \n\n");
  qsort(&vocab[1], vocab_size - 1, sizeof(struct vocab_word), VocabCompare);
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  size = vocab_size;
  train_words = 0;
  for (a = 0; a < size; a++) {
    // Words occuring less than min_count times will be discarded from the vocab
    if ((vocab[a].cn < min_count) && (a != 0)) {
      vocab_size--;
      free(vocab[a].word);
    } else {
      // Hash will be re-computed, as after the sorting it is not actual
      hash=GetWordHash(vocab[a].word);
      while (vocab_hash[hash] != -1) hash = (hash + 1) % vocab_hash_size;
      vocab_hash[hash] = a;
      train_words += vocab[a].cn;
    }
  }
 // vocab = (struct vocab_word *)realloc(vocab, (vocab_size + 1) * sizeof(struct vocab_word));
  // Allocate memory for the binary tree construction
  /* for (a = 0; a < vocab_size; a++) {
    vocab[a].code = (char *)calloc(MAX_CODE_LENGTH, sizeof(char));
    vocab[a].point = (int *)calloc(MAX_CODE_LENGTH, sizeof(int));
  }*/
}

void ReadVocab() {
  long long a, b,dummy;
  int start_point, end_point = 0;
  unsigned* indices,tot_vocab_size;
  char c;
  char word[MAX_STRING];
  struct vocab_word *temp_vocab; 
  FILE *fin = fopen(read_vocab_file, "rb");
  if (fin == NULL) {
    printf("Vocabulary file not found\n");
    exit(1);
  }

  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  a = 0;
  vocab_size = 0;
  fscanf(fin, "%u%c", &tot_vocab_size, &c);
  
  if( dim_red_type == 1) {
  	start_point = 0;
  	end_point = n_dim;
  } else if( dim_red_type == 2) {
  	start_point = tot_vocab_size-n_dim;
  	end_point = tot_vocab_size;
  } else if( dim_red_type == 3) {
  	start_point = 0;
  	end_point = tot_vocab_size;
  	indices = (unsigned *)calloc(n_dim, sizeof(unsigned));
	if (indices == NULL) {
		printf("Out of memory in Random\n\n");
		exit(-1);
	}
	randArray(tot_vocab_size,n_dim,indices);
  } else {
  	start_point = 0;
  	end_point = tot_vocab_size;
  }

  
  while (1) {
    ReadWord(word, fin);
    if (feof(fin)) break;
  	if (((a >= start_point) && (a<end_point)) && ((dim_red_type!=3) || (indices[vocab_size]==a))) {
    	b = AddWordToVocab(word);
    	fscanf(fin, "%u%c", &vocab[b].cn, &c);
	} else fscanf(fin, "%u%c", &dummy, &c);
    a++;
  }
  
  //Implement dimensionality reduction
 // if(dim_red_type)
//  	ReduceVocab();

  SortVocab();
  if (debug_mode > 0) {
    printf("Vocab size: %lld\n", vocab_size);
    printf("Words in train file: %lld\n", train_words);
  }
  fclose(fin);
}

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

void createDbFromTrainFile() {
  char word[MAX_STRING];
  unsigned curr_tweet_words[MAX_WORD_COUNT];
  FILE *fin;
  FILE *fout;
  long long a, i,j;
  unsigned curr_tweet = 0, curr_word_idx = 0,max_len = 0;
  
  fin = fopen(train_file, "rb");
  fout = fopen(save_db_file, "wb");
  
  if (fin == NULL) {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  tweet_count = 0;train_words = 0;
//  printf ("\n%d--> ",tweet_count);
  while (1) {
    ReadWord(word, fin);
    if (feof(fin)) break;
    train_words++;
    if ((debug_mode > 1) && (train_words % 100000 == 0)) {
      printf("%lldK  %lldk %lld%c", train_words / 1000,vocab_size/1000, tweet_count, 13);
      fflush(stdout);
    }
    i = SearchVocab(word);
	j = SearchWordInTweet(curr_tweet_words,curr_word_idx,i);
	//printf("%d,%s,%d ",t,word,i);

    if (i > -1 && (j == -1)) {
		curr_tweet_words[curr_word_idx] = i; 
		// Ignore white space
		curr_word_idx++;
	}
	if (tweet_count > curr_tweet) {
		if(curr_word_idx > 0) {
    		if (binary) {
				fwrite(&curr_tweet, sizeof(unsigned), 1, fout);
				fwrite(&curr_word_idx, sizeof(unsigned), 1, fout);
				fwrite(&curr_tweet_words[0], sizeof(unsigned), curr_word_idx, fout);
			} else {
				fprintf(fout, "%u ", curr_tweet);
				for(a=0;a<curr_word_idx;a++) 
					fprintf(fout, "%u ", curr_tweet_words[a]);
				fprintf(fout, "\n");
			}
		}
		//calculate average tweet length
  		avg_tweet_length = (avg_tweet_length * curr_tweet + curr_word_idx)/tweet_count;  
		max_len = (max_len>curr_word_idx) ? max_len: curr_word_idx;
		
		curr_word_idx = 0;
		curr_tweet = tweet_count;
	}
  }
  if (debug_mode > 0) {
    printf("Averge tweet length: %f\n", avg_tweet_length);
    printf("Max tweet length: %u\n", max_len);
    printf("No of tweets: %lld\n", tweet_count);
  }
  file_size = ftell(fin);
  fclose(fin);
  fclose(fout);
}

void SaveVocab() {
  long long i;
  FILE *fo = fopen(save_vocab_file, "wb");
  for (i = 0; i < vocab_size; i++) {
  	if(vocab[i].word[0] == 0)
		printf("What is happenening Here %lld!!\n",i);
  	
	fprintf(fo, "%s %u\n", vocab[i].word, vocab[i].cn);
  }
  fclose(fo);
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

unsigned ReadTweetDb(struct tweet_dB_t **tweetDbLp, unsigned *prealloc_size, char* db_file ) {
  unsigned tcnt = 0,tweet_id, length;
  struct tweet_dB_t *tweetDbL;
  FILE *fin = fopen(db_file, "rb");
  printf("Reading File %s\n",db_file);
  if (fin == NULL) {
    printf("DBfile file not found\n");
    exit(1);
  }
  printf("Addresses %u %u\n",tweetDbLp,(*tweetDbLp));
  tweetDbL = (*tweetDbLp);
  while (1) {
    fread(&tweet_id, sizeof(unsigned), 1, fin);
    if (feof(fin)) break;
    fread(&length, sizeof(unsigned), 1, fin);
    if (feof(fin)) break;
	tweetDbL[tcnt].id = tweet_id;
	tweetDbL[tcnt].size = length;

	if(length > 45)
		printf("WARNING Something wrong %u\n",length);

  	tweetDbL[tcnt].vec = (unsigned *)calloc(length, sizeof(unsigned));

	if(tweetDbL[tcnt].vec==NULL){
		printf("NO MEMORY REALLY %u %u %u\n",tweet_id,length,tcnt);
	}

    fread(tweetDbL[tcnt].vec, sizeof(unsigned), length, fin);

	tcnt++;

  	// Reallocate memory if needed
  	if (tcnt+ 2 >= *prealloc_size) {
  	  //printf("Need More memory !! \n\n");
  	  (*prealloc_size)+= 1000;
  	  tweetDbL = (struct tweet_dB_t *)realloc(tweetDbL, (*prealloc_size)*sizeof(struct tweet_dB_t));
  	  if(tweetDbL == NULL) printf("NO More memory %lld %u !! \n\n",(*prealloc_size), tcnt);
  	}
  }
  if (debug_mode > 0) {
    printf("Memory Allocated: %lld\n", (*prealloc_size));
    printf("No of tweets Read: %u\n", tcnt);
  }
  fclose(fin);
  *tweetDbLp = tweetDbL;
  return tcnt;
}

double computeCosineDist(struct tweet_dB_t t1,struct tweet_dB_t t2) {
	unsigned n_common = 0,i,j;
	float ratio;
	
	for(i=0;i<t1.size;i++)
	for(j=0;j<t2.size;j++)
	if(t1.vec[i] == t2.vec[j]) { 
		n_common++;
		break;
	}
	ratio = ((float)n_common)/(sqrt(t2.size) * sqrt(t1.size));
	return acos(ratio);
}

unsigned FindNearestNeighbour(struct tweet_dB_t query,struct tweet_dB_t tDb[], unsigned db_size) {
	float min_dist = 4.0, curr_dist;
	unsigned i, nn_id=-1;

	for(i=0;i< db_size;i++){
		curr_dist = computeCosineDist(query,tDb[i]);
		//printf("curr_dist = %f%c",curr_dist,13);
		if(curr_dist < min_dist){
			nn_id = tDb[i].id;
			min_dist = curr_dist;
		}
	}
	return nn_id;
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

void DoOneNNSearch(struct tweet_dB_t *Db,struct tweet_dB_t *qDb, unsigned tweet_count, unsigned query_count) {
  unsigned i, nn = 0, j=0;
  float *min_dist_arr;
  float curr_dist;
  unsigned* nn_id; 
  FILE *tweetDbFile;

	tweetDbFile = fopen(dbin_file, "rb");

  	GetNthDb(indDB, 0, Db, tweetDbFile);
	if(Db[0].size == 0)
		printf("ERROR!!!\n");
	curr_dist = computeCosineDist(qDb[indQuery],Db[0]);
  	
  	printf("Dist between tweet %u, len = %u and tweet %u  %u len = %u is %f\n",qDb[indQuery].id, qDb[indQuery].size,indDB,Db[0].id, Db[0].size,curr_dist);
	fclose(tweetDbFile);
}

void DoFullNearestNeighbourSearch(struct tweet_dB_t *Db,struct tweet_dB_t *qDb, unsigned tweet_count, unsigned query_count) {
  unsigned i, nn = 0, j=0;
  float *min_dist_arr;
  float curr_dist;
  unsigned* nn_id; 
  FILE *tweetDbFile;
  clock_t t0, t1;

  printf("\n\n -------------------Doing FULLNearest neigbhbout now ----------\n\n");
  
  if(dBOnDisk == 0){
  	for(i=0;i<query_count;i++) {
  	  nn = FindNearestNeighbour(qDb[i],Db,tweet_count);
  	  printf("Nearest Neighbour for tweet %u, is %u\n",qDb[i].id,nn);
  	}
  } 
  else {
  	min_dist_arr = (float *)calloc(query_count, sizeof(float));
  	nn_id = (unsigned *)calloc(query_count, sizeof(unsigned));
	tweetDbFile = fopen(dbin_file, "rb");

   	t0 = clock();
	for(i=0;i<query_count;i++) 
		min_dist_arr[i] = 4.0;

	while(GetNextDb(Db, tweetDbFile)) {
  		for(i=0;i<query_count;i++) {
			curr_dist = computeCosineDist(qDb[i],Db[0]);
			if(curr_dist < min_dist_arr[i]) {
				nn_id[i] = Db[0].id;
				min_dist_arr[i] = curr_dist;
				//printf(" %8u, %8u%c",i,nn_id[i],13);
			}
		}
		j++;
    	if (j % 100000 == 0) {
    	  printf("On  %uk %c", j, 13);
    	  fflush(stdout);
    	}
	}
	
	printf("\nTotal DB tweets read %u\n\n",j);
  	
	for(i=0;i<query_count;i++) {
  	  printf("NN for tweet %u, is %u\n",qDb[i].id,nn_id[i]);
	}
   
   t1 = clock();
   printf("\n\nTotal Execution took %.6f seconds.\n",(float)(t1 - t0)/CLOCKS_PER_SEC);
   fclose(tweetDbFile);
  }
}

int main(int argc, char **argv) {
  
  int i;
  if (argc == 1) {
    printf("mapImgnet to Vocab tool v0.1a\n\n");
    return 0;
  }
  srand(time(NULL)); 
  if ((i = ArgPos((char *)"-readDb", argc, argv)) > 0) strcpy(dbin_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-queryfile", argc, argv)) > 0) strcpy(query_dB_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-train-file", argc, argv)) > 0) strcpy(train_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-read-vocab", argc, argv)) > 0) strcpy(read_vocab_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-saveDb", argc, argv)) > 0) strcpy(save_db_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-dRed", argc, argv)) > 0) dim_red_type = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-ndim", argc, argv)) > 0) n_dim = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-doOnDisk", argc, argv)) > 0) dBOnDisk = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-vocabTweetList", argc, argv)) > 0) vcbTwtMap = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-save-vocab", argc, argv)) > 0) strcpy(save_vocab_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-indQ", argc, argv)) > 0) indQuery = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-indDB", argc, argv)) > 0) indDB = atoi(argv[i + 1]);
		
  
  if (dbin_file[0] == 0) {
  	vocab = (struct vocab_word *)calloc(n_dim+2, sizeof(struct vocab_word));
  	
  	printf("size of Vocab struct is %ld %ld %d \n",sizeof(struct vocab_word),sizeof(unsigned),RAND_MAX);
  	vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
  	
  	if (read_vocab_file[0] != 0) ReadVocab();
  	
  	if (train_file[0] != 0) 
  		createDbFromTrainFile();
  	if (save_vocab_file[0] != 0) SaveVocab();
  } else {
 	
	if (dBOnDisk==0) { 
  		tweetDb = (struct tweet_dB_t *)calloc(twtdb_max_size, sizeof(struct tweet_dB_t));
	}
	else {
  		tweetDb = (struct tweet_dB_t *)calloc(1, sizeof(struct tweet_dB_t));
  		tweetDb[0].vec = (unsigned *)calloc(MAX_TWEET_LEN, sizeof(unsigned));
	}

  	querytweetDb = (struct tweet_dB_t *)calloc(querydb_max_size, sizeof(struct tweet_dB_t));
	//printf("Addresses %u %u \n",tweetDb,querytweetDb);
	
	if((tweetDb == NULL) || (querytweetDb == NULL))
		printf("NO MEMORY\n");
	
	if (dBOnDisk==0)
		tweet_count = ReadTweetDb(&tweetDb, &twtdb_max_size, dbin_file); 
	//printf("Addresses %u %u \n",tweetDb,querytweetDb);
	
	query_count = ReadTweetDb(&querytweetDb, &querydb_max_size, query_dB_file); 

	if (indQuery == 0 )
		DoFullNearestNeighbourSearch(tweetDb,querytweetDb,tweet_count,query_count);	
	else
		DoOneNNSearch(tweetDb,querytweetDb,tweet_count,query_count);	
  }
  
  return 0;
}
