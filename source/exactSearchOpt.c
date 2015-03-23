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
#define MAX_TWEET_LEN 50

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

char dbin_file[MAX_STRING_FNAME], query_dB_file[MAX_STRING_FNAME];
char vocab_map_header_file[MAX_STRING_FNAME], sort_key_file[MAX_STRING_FNAME], vocab_map_dB_file[MAX_STRING_FNAME];
//struct image_vocab *imagenetvocab; 
struct vocab_word *vocab;
struct tweet_dB_t *tweetDb;
struct tweet_dB_t *querytweetDb;

unsigned twtdb_max_size = 15000000, querydb_max_size = 1000;
int binary = 0, debug_mode = 2, min_count = 0, num_threads = 12, dim_red_type = 0, n_dim = 9000000, dBOnDisk = 0, vcbTwtMap = 0;
int *vocab_hash;
long long vocab_max_size = 9500000, tweet_count=0, query_count=0, image_vocab_max_size = 2500, vocab_size = 0,imgvocab_size = 0, layer1_size = 100;
long long train_words = 0, word_count_actual = 0, iter = 5, file_size = 0, classes = 0;
real avg_length=0, avg_tweet_length=0; 
clock_t start;

int vocab_lenIdxlist[MAX_TWEET_LEN][MAX_TWEET_LEN];
unsigned vocab_dBSize[MAX_TWEET_LEN];
unsigned vocab_dBPosIdx[MAX_TWEET_LEN];
unsigned *vocab_twtMap_Buf;
unsigned localMemPos[MAX_TWEET_LEN];

unsigned *tweetSortKey;

float approxFactor = 1.0;

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

unsigned ReadWordHeader(FILE* finHeader, unsigned wordId, unsigned currLoadPos) {

	
	fseek (finHeader, (long long)(wordId*(2+MAX_TWEET_LEN)*sizeof(unsigned)), SEEK_SET);
  	
	fread(&vocab_dBSize[currLoadPos], sizeof(unsigned), 1, finHeader);
	fread(&vocab_dBPosIdx[currLoadPos], sizeof(unsigned), 1, finHeader);
	fread(&vocab_lenIdxlist[currLoadPos][0], sizeof(unsigned), MAX_TWEET_LEN, finHeader);

	return (currLoadPos + 1);
}

unsigned LoadQueryHeadMap(FILE* finHeader, FILE* finMap ,struct tweet_dB_t t1) {
	
	unsigned i, currLoadPos=0;

	for(i=0;i<t1.size;i++) {
	
		fseek(finHeader, (long long)(t1.vec[i]*(2+MAX_TWEET_LEN)*sizeof(unsigned)), SEEK_SET);
  	
		fread(&vocab_dBSize[i], sizeof(unsigned), 1, finHeader);
		fread(&vocab_dBPosIdx[i], sizeof(unsigned), 1, finHeader);
		fread(&vocab_lenIdxlist[i][0], sizeof(unsigned), MAX_TWEET_LEN, finHeader);

		fseek(finMap, (long long)(vocab_dBPosIdx[i]*sizeof(unsigned)), SEEK_SET);
		fread(&vocab_twtMap_Buf[currLoadPos], sizeof(unsigned), vocab_dBSize[i], finMap);

		localMemPos[i] = currLoadPos; 
		currLoadPos += vocab_dBSize[i];
		//printf("id = %u sz = %u, pos = %u, L = %d, lcl = %u\n",t1.vec[i], vocab_dBSize[i], vocab_dBPosIdx[i], vocab_lenIdxlist[i][1],localMemPos[i]);
	}

	return (currLoadPos + 1);
}

int numCompare(const void *a, const void *b) {
    return (*((unsigned*)a) - *((unsigned *)b));
}

float computeBestDist(unsigned lenBlk, unsigned tweetLen, unsigned* bestIdx,unsigned minIntNeeded)
{
#define LARGENUM 22232322 
	unsigned curr_best_el_cnt = 0, curr_best_el = 0 ;
	int pntrs[MAX_TWEET_LEN];
	unsigned currMinEl = LARGENUM, currMinElIdx[MAX_TWEET_LEN], currMinElSecLarge = LARGENUM ;
	unsigned currMinCnt = 0;
	unsigned i,j = 0, k = 0, curr_actv = 0;
	unsigned done = 0;
	unsigned lenBlkSize[MAX_TWEET_LEN];

	if ((minIntNeeded > tweetLen)||(minIntNeeded > (lenBlk+1)))
		return 0;

	for(i=0;i<tweetLen;i++) {
		if (vocab_lenIdxlist[i][lenBlk] != -1) {
			pntrs[k] = localMemPos[i] + (vocab_lenIdxlist[i][lenBlk] - 1);
			lenBlkSize[k] = 0;
			for( j = lenBlk+1; j < MAX_TWEET_LEN; j++) 
				if((vocab_lenIdxlist[i][j] != -1)) {
					lenBlkSize[k] = pntrs[k] + (vocab_lenIdxlist[i][j]) - (vocab_lenIdxlist[i][lenBlk]);
					break;
				}
			if (lenBlkSize[k] == 0)
				lenBlkSize[k] = pntrs[k] + vocab_dBSize[i] - (vocab_lenIdxlist[i][lenBlk]-1);
			k++;
		}
		//printf("i = %u, word = %u,  k = %u lenBlkSize = %u len = %d\n",i, querytweetDb[149].vec[i],k ,lenBlkSize[k-1],vocab_lenIdxlist[i][lenBlk]);
	}
	j = 0;	
	while (!done) {
		done = 1;
		//time to remove the minElement from the heap
		currMinEl = LARGENUM;
		currMinElSecLarge = LARGENUM;
		currMinCnt = 0;
		curr_actv = 0;
		
		for(i = 0; i < k; i++) {
			if (pntrs[i] < lenBlkSize[i]) {
				curr_actv++;
				done = 0;
				if (currMinEl == vocab_twtMap_Buf[pntrs[i]]) {
					currMinElIdx[currMinCnt] = i; 
					currMinCnt++;
				} else if (currMinEl > vocab_twtMap_Buf[pntrs[i]]) {
					currMinElSecLarge = currMinEl;
					currMinEl = vocab_twtMap_Buf[pntrs[i]];
					currMinCnt = 0;
					currMinElIdx[currMinCnt] = i; 
					currMinCnt++;
				} else if (currMinElSecLarge > vocab_twtMap_Buf[pntrs[i]]) {
					currMinElSecLarge = vocab_twtMap_Buf[pntrs[i]];
				}
			}
		}
		

		for (i = 0; i < currMinCnt; i++) {
		//	printf("i = %u, id = %u min = %u sec = %u %u %u\n", curr_actv, currMinElIdx[i],currMinEl,currMinElSecLarge, pntrs[currMinElIdx[i]], lenBlkSize[currMinElIdx[i]]);
			while ( (currMinElSecLarge > vocab_twtMap_Buf[pntrs[currMinElIdx[i]]]) && (pntrs[currMinElIdx[i]] < lenBlkSize[currMinElIdx[i]]))
				pntrs[currMinElIdx[i]]++;
		}

		if (currMinCnt > curr_best_el_cnt) {
			curr_best_el = currMinEl;
			curr_best_el_cnt = currMinCnt;
			//printf("Best El = %u, BestCnt = %u j = %u \n",curr_best_el, curr_best_el_cnt,j);
			if ((curr_best_el_cnt > lenBlk) || (curr_best_el_cnt >= tweetLen)) {
				break;
			}
		}
		j++;
		if ((curr_actv < minIntNeeded ) || (curr_actv < curr_best_el_cnt))
			break;
	}
//	printf(" dist = %f, minNeeded = %u, curr_actv = %u, k = %u, len = %d Best El = %u, BestCnt = %u j = %u \n", ((float)curr_best_el_cnt)/(sqrt(lenBlk+1)),minIntNeeded,curr_actv, k,lenBlk,curr_best_el, curr_best_el_cnt,j);
	*bestIdx = curr_best_el;
	//return ((float)curr_best_el_cnt)/sqrt(lenBlk+1);
	return (float)curr_best_el_cnt;
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
  	  if(tweetDbL == NULL) printf("NO More memory %u %u !! \n\n",(*prealloc_size), tcnt);
  	}
  }
  if (debug_mode > 0) {
    printf("Memory Allocated: %u\n", (*prealloc_size));
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

void DoFullNearestNeighbourSearch(struct tweet_dB_t *Db,struct tweet_dB_t *qDb, unsigned tweet_count, unsigned query_count) {
  unsigned i, j=0, keySize = 0;
  float curr_dist, bestDist = 0, qLen = 0, constraint = 0; 
  FILE *tweetDbFile;
  FILE *finSortKey;
  FILE *finVocabHead, *finMap;
  unsigned tot_mem_reqd=0; 
  unsigned bestResult = 0, currResult;
  float blkLenSqrt[MAX_TWEET_LEN];
  clock_t t0, t1;

  finVocabHead = fopen(vocab_map_header_file, "rb");
  finMap = fopen(vocab_map_dB_file, "rb");
  finSortKey = fopen(sort_key_file, "rb");
  tweetDbFile = fopen(dbin_file, "rb");

  	printf("\n\n -------------------Doing FULLNearest neigbhbout now ----------\n\n");
  
	
	for( i = 0; i < MAX_TWEET_LEN; i++) { 
   		ReadWordHeader(finVocabHead, i, i);
		tot_mem_reqd +=  vocab_dBSize[i];
		blkLenSqrt[i] = sqrt(i+1);
		//printf("Len is %u %d \n\n", i, vocab_lenIdxlist[i][i]);
	}
  	
	// Buffer for loading tweets of each word
	vocab_twtMap_Buf = (unsigned *)calloc( tot_mem_reqd, sizeof(unsigned));
	if (vocab_twtMap_Buf== NULL)
		printf("OUT OF MEMORY!! %u %u \n\n", i, tot_mem_reqd);

	// Buffer for loading sort key of tweets. This is to print answer from original DB
  	fread(&keySize, sizeof(unsigned), 1, finSortKey);
	printf("Key Size is  %u \n\n", keySize);
	tweetSortKey = (unsigned *)calloc(keySize, sizeof(unsigned));
	if (tweetSortKey == NULL)
		printf("OUT OF MEMORY KEY!! %u \n\n", keySize);
  	fread(&tweetSortKey[0], sizeof(unsigned), keySize, finSortKey);

	printf ("MEMORY ALLOCATION DONE!! :-) \n");
	
	printf ("Query Count = %u \n",query_count);
	//query_count = 0;
  
   t0 = clock();
  	
	for(i=0;i<query_count;i++) {
		LoadQueryHeadMap(finVocabHead, finMap ,qDb[i]);
		bestDist = 0;
		constraint = 0;
		qLen = sqrt(qDb[i].size);
		for (j=0; j < MAX_TWEET_LEN; j++) {
			curr_dist  = computeBestDist(j, qDb[i].size, &currResult, ceil(constraint * blkLenSqrt[j]))/blkLenSqrt[j]; //ceil(bestDist * sqrt(j+1) * approxFactor));
  	  		//printf(" curr Dist %f %f \n", curr_dist,bestDist);
			if (curr_dist > bestDist) {
				bestDist = curr_dist; 
				bestResult = currResult;
				/* Condition is for floating point precision issues
					Inexplicably sometimes bestDist is slightly > qLen !!
				*/
  	  		//	printf(" j = %u  bd = %f \t mn = %f  mnOld = %f qLen =%d\n ",j, bestDist,ceil(constraint), ceil(bestDist * approxFactor),(bestDist >= qLen));
				if (bestDist < qLen)
					constraint =  cos(acos(bestDist/qLen)/approxFactor)*qLen;
				else
					break;
			}
		}
  	  	printf("NN for tweet %u, is %u d = %f\n",qDb[i].id,tweetSortKey[bestResult],bestDist/qLen);
  	  //	printf("NN for tweet %u, is %u\n",qDb[i].id,tweetSortKey[bestResult]);
	}
   t1 = clock();
   printf("\n\nTotal Execution took %.6f seconds.\n",(float)(t1 - t0)/CLOCKS_PER_SEC);

   fclose(finVocabHead);
   fclose(finMap);
   fclose(finSortKey);
   fclose(tweetDbFile); 
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
  if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-binary", argc, argv)) > 0) binary = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-vocabTweetList", argc, argv)) > 0) vcbTwtMap = atoi(argv[i + 1]);
  if ((i = ArgPos((char *)"-vocabMapHeader", argc, argv)) > 0) strcpy(vocab_map_header_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-vocabMapDb", argc, argv)) > 0) strcpy(vocab_map_dB_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-approx", argc, argv)) > 0) approxFactor = atof(argv[i + 1]);
  if ((i = ArgPos((char *)"-sortKey", argc, argv)) > 0) strcpy(sort_key_file, argv[i + 1]);
		
  tweetDb = (struct tweet_dB_t *)calloc(1, sizeof(struct tweet_dB_t));
  tweetDb[0].vec = (unsigned *)calloc(MAX_TWEET_LEN, sizeof(unsigned));

  querytweetDb = (struct tweet_dB_t *)calloc(querydb_max_size, sizeof(struct tweet_dB_t));
  printf("Approx Factor %f \n",approxFactor);
  
  if((tweetDb == NULL) || (querytweetDb == NULL))
  	printf("NO MEMORY\n");
  
  query_count = ReadTweetDb(&querytweetDb, &querydb_max_size, query_dB_file); 
  
  DoFullNearestNeighbourSearch(tweetDb,querytweetDb,tweet_count,query_count);	
  
  return 0;
}
