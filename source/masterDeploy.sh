
#Build All files
make

#Now create the vocabulary from database 

./createVocab -read-vocab $1 -save-vocab vocabMasterDb.txt


# Create ll the databases required by search algorithms for all d = 100 to 1638400

path='./'
typestr='_FULL'

./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file $2 -saveDb queryDb_FULL.txt -debug 2 -dRed 0 -binary 1
./bruteForceSearch -read-vocab vocabMasterDb.txt -train-file $1 -save-vocab Vocab_FULL.txt -saveDb Db_FULL.txt -debug 2 -dRed 0 -binary 1
./sortDb -readDb Db_FULL.txt -saveDb Db_FULL_srt.txt -saveSortKey sortkey_FULL.txt
rm Db_FULL.txt
./createTweetVocabMap -read-vocab Vocab_FULL.txt -save-vocab VocabMapHeader_FULL.txt -readDb Db_FULL_srt.txt -save-map VocabMap_FULL.bin -binary 1

# Run the nearest neighbour tests on all the databases

path='./'
ResDir='Results'
typestr='_FULL'
cp $path\queryDb$typestr\.txt $path\Db$typestr\_srt.txt $path\sortkey$typestr\.txt $path\VocabMapHeader$typestr\.txt $path\VocabMap$typestr\.bin . 
echo "-------------- FULL ---------------------------"
#touch $ResDir\/Brute_SearchResult$typestr\.txt
#./bruteForceSearch -readDb Db$typestr\_srt.txt -queryfile queryDb$typestr\.txt -doOnDisk 1 > $ResDir\/Brute_SearchResult$typestr\.txt

touch $ResDir\/Fast_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -sortKey sortkey$typestr\.txt > $ResDir\/Fast_SearchResult$typestr\.txt
#touch $ResDir\/Approx_SearchResult$typestr\.txt
#./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.5 -sortKey sortkey$typestr\.txt > $ResDir\/Approx_SearchResult$typestr\.txt
#./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.2 -sortKey sortkey$typestr\.txt > $ResDir\/Approx20_SearchResult$typestr\.txt
#rm queryDb$typestr\.txt Db$typestr\_srt.txt sortkey$typestr\.txt VocabMapHeader$typestr\.txt VocabMap$typestr\.bin
