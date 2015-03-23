
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

#define MAX_STRING 40
#define MAX_STRING_FNAME 100

const int vocab_hash_size = 30000000;  // Maximum 30 * 0.7 = 21M words in the vocabulary

typedef float real;                    // Precision of float numbers

struct vocab_word {
  char *word;
  unsigned tweet_cn;
  unsigned cn;
};

char train_file[MAX_STRING_FNAME];
char save_vocab_file[MAX_STRING_FNAME], read_vocab_file[MAX_STRING_FNAME];
//struct image_vocab *imagenetvocab; 
struct vocab_word *vocab;
int binary = 0, debug_mode = 2, window = 5, min_count = 0;
int *vocab_hash;
long long vocab_max_size = 9500000,tweet_count=0, vocab_size = 0;
long long train_words = 0, file_size = 0;
real avg_length=0; 

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
int SearchVocab(char *word) {
  unsigned int hash = GetWordHash(word);
  while (1) {
    if (vocab_hash[hash] == -1) return -1;
    if (!strcmp(word, vocab[vocab_hash[hash]].word)) return vocab_hash[hash];
    hash = (hash + 1) % vocab_hash_size;
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
void breakout() {
 SaveVocab();
 exit(-1);
}
// Adds a word to the vocabulary
int AddWordToVocab(char *word) {
  unsigned int hash, length = strlen(word) + 1;
  if (length > MAX_STRING) length = MAX_STRING;
  //if (length > 4){
  	vocab[vocab_size].word = (char *)calloc(length, sizeof(char));
 /* } else {
  	vocab[vocab_size].word = &(vocab[vocab_size].filler_char[0]);
  }*/
  if((vocab[vocab_size].word == NULL)) {
  
  	printf("No Memory!! %f %lld %lld %lld\n",avg_length,vocab_size,train_words,tweet_count);
  	breakout();
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

void LearnVocabFromTrainFile() {
  char word[MAX_STRING];
  FILE *fin;
  long long a, i;
  for (a = 0; a < vocab_hash_size; a++) vocab_hash[a] = -1;
  fin = fopen(train_file, "rb");
  if (fin == NULL) {
    printf("ERROR: training data file not found!\n");
    exit(1);
  }
  vocab_size = 0;
  AddWordToVocab((char *)"</s>");
  while (1) {
    ReadWord(word, fin);
    if (feof(fin)) break;
    train_words++;
    if ((debug_mode > 1) && (train_words % 100000 == 0)) {
      printf("%lldK  %lldk %lldk%c", train_words / 1000,vocab_size/1000, tweet_count, 13);
      fflush(stdout);
    }
    i = SearchVocab(word);
    if (i == -1) {
		a = AddWordToVocab(word);
		vocab[i].tweet_cn = tweet_count;
		vocab[a].cn = 1;
	} else {
		vocab[i].cn ++;//= (vocab[i].tweet_cn != tweet_count);
		vocab[i].tweet_cn = tweet_count;
	}
    if (vocab_size > vocab_hash_size * 0.7) printf("Things getting Dangerous !! \n\n");
  }
  SortVocab();
  if (debug_mode > 0) {
    printf("Vocab size: %lld tweet_count = %lld\n", vocab_size,tweet_count);
    printf("Words in train file: %lld\n", train_words);
  }
  file_size = ftell(fin);
  fclose(fin);
}

void SaveVocab() {
  long long i;
  FILE *fo = fopen(save_vocab_file, "wb");
  fprintf(fo, "%u\n", vocab_size-1);
  //omitting writing \<s\> into the vocabulary file
  for (i = 1; i < vocab_size; i++) {
	fprintf(fo, "%s %u\n", vocab[i].word, vocab[i].cn);
  }
  fclose(fo);
}

int main(int argc, char **argv) {
  int i;
  if (argc == 1) {
    printf("mapImgnet to Vocab tool v0.1a\n\n");
    return 0;
  }
  if ((i = ArgPos((char *)"-save-vocab", argc, argv)) > 0) strcpy(save_vocab_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-read-vocab", argc, argv)) > 0) strcpy(train_file, argv[i + 1]);
  if ((i = ArgPos((char *)"-debug", argc, argv)) > 0) debug_mode = atoi(argv[i + 1]);
  
  vocab = (struct vocab_word *)calloc(vocab_max_size, sizeof(struct vocab_word));
  
 printf("size of Vocab struct is %ld %ld\n",sizeof(struct vocab_word),sizeof(unsigned));
 vocab_hash = (int *)calloc(vocab_hash_size, sizeof(int));
  
  if (train_file[0] != 0) LearnVocabFromTrainFile();
//  ReadImageVocab();
 
  if (save_vocab_file[0] != 0) SaveVocab();
  
  return 0;
}
