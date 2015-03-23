#!/bin/sh
# Change the path for Databases here.
path='./'
typestr='_FULL'

./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file queryTweets.txt -saveDb queryDb_FULL.txt -debug 2 -dRed 0  -binary 1
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file dbTweets.txt -save-vocab Vocab_FULL.txt -saveDb Db_FULL.txt -debug 2 -dRed 0 -binary 1
./sortDb -readDb Db_FULL.txt -saveDb Db_FULL_srt.txt -saveSortKey sortkey_FULL.txt
rm Db_FULL.txt
./createTweetVocabMap -read-vocab Vocab_FULL.txt -save-vocab VocabMapHeader_FULL.txt -readDb Db_FULL_srt.txt -save-map VocabMap_FULL.bin -binary 1

mv queryDb_FULL.txt Vocab_FULL.txt Db_FULL_srt.txt sortkey_FULL.txt VocabMapHeader_FULL.txt VocabMap_FULL.bin $path
max=7
lsi='100 400 1600 6400 25600 102400 409600 1638400'
for i in $lsi; 
do
dim=$i
echo $dim

echo "-------------- Freq ---------------------------"
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file queryTweets.txt -saveDb queryDbFreq_$dim\.txt -debug 2 -dRed 1 -ndim $dim -binary 1
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file dbTweets.txt -save-vocab VocabFreq_$dim\.txt -saveDb DbFreq_$dim\.txt -debug 2 -dRed 1 -ndim $dim -binary 1
./sortDb -readDb DbFreq_$dim\.txt -saveDb DbFreq_$dim\_srt.txt -saveSortKey sortkeyFreq_$dim\.txt
rm DbFreq_$dim\.txt
./createTweetVocabMap -read-vocab VocabFreq_$dim\.txt -save-vocab VocabMapHeaderFreq_$dim\.txt -readDb DbFreq_$dim\_srt.txt -save-map VocabMapFreq_$dim\.bin -binary 1

mv queryDbFreq_$dim\.txt VocabFreq_$dim\.txt DbFreq_$dim\_srt.txt sortkeyFreq_$dim\.txt VocabMapHeaderFreq_$dim\.txt VocabMapFreq_$dim\.bin $path

echo "-------------- In Freq ---------------------------"
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file queryTweets.txt -saveDb queryDbInFreq_$dim\.txt -debug 2 -dRed 2 -ndim $dim -binary 1
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file dbTweets.txt -save-vocab VocabInFreq_$dim\.txt -saveDb DbInFreq_$dim\.txt -debug 2 -dRed 2 -ndim $dim -binary 1
./sortDb -readDb DbInFreq_$dim\.txt -saveDb DbInFreq_$dim\_srt.txt -saveSortKey sortkeyInFreq_$dim\.txt
rm DbInFreq_$dim\.txt
./createTweetVocabMap -read-vocab VocabInFreq_$dim\.txt -save-vocab VocabMapHeaderInFreq_$dim\.txt -readDb DbInFreq_$dim\_srt.txt -save-map VocabMapInFreq_$dim\.bin -binary 1

mv queryDbInFreq_$dim\.txt VocabInFreq_$dim\.txt DbInFreq_$dim\_srt.txt sortkeyInFreq_$dim\.txt VocabMapHeaderInFreq_$dim\.txt VocabMapInFreq_$dim\.bin $path 

echo "-------------- Random ---------------------------"
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file queryTweets.txt -saveDb queryDbRand_$dim\.txt -debug 2 -dRed 3 -ndim $dim -binary 1
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file dbTweets.txt -save-vocab VocabRand_$dim\.txt -saveDb DbRand_$dim\.txt -debug 2 -dRed 3 -ndim $dim -binary 1
./sortDb -readDb DbRand_$dim\.txt -saveDb DbRand_$dim\_srt.txt -saveSortKey sortkeyRand_$dim\.txt
rm DbRand_$dim\.txt
./createTweetVocabMap -read-vocab VocabRand_$dim\.txt -save-vocab VocabMapHeaderRand_$dim\.txt -readDb DbRand_$dim\_srt.txt -save-map VocabMapRand_$dim\.bin -binary 1

mv queryDbRand_$dim\.txt VocabRand_$dim\.txt DbRand_$dim\_srt.txt sortkeyRand_$dim\.txt VocabMapHeaderRand_$dim\.txt VocabMapRand_$dim\.bin $path
done
