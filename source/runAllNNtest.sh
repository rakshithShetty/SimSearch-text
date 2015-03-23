#!/bin/sh
# Change the path for Databases here.
path='./'
ResDir='Results'
typestr='_FULL'
cp $path\queryDb$typestr\.txt $path\Db$typestr\_srt.txt $path\sortkey$typestr\.txt $path\VocabMapHeader$typestr\.txt $path\VocabMap$typestr\.bin . 
echo "-------------- FULL ---------------------------"
touch $ResDir\/Brute_SearchResult$typestr\.txt
./bruteForceSearch -readDb Db$typestr\_srt.txt -queryfile queryDb$typestr\.txt -doOnDisk 1 > $ResDir\/Brute_SearchResult$typestr\.txt
touch $ResDir\/Fast_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -sortKey sortkey$typestr\.txt > $ResDir\/Fast_SearchResult$typestr\.txt
touch $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.5 -sortKey sortkey$typestr\.txt > $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.2 -sortKey sortkey$typestr\.txt > $ResDir\/Approx20_SearchResult$typestr\.txt
rm queryDb$typestr\.txt Db$typestr\_srt.txt sortkey$typestr\.txt VocabMapHeader$typestr\.txt VocabMap$typestr\.bin
#
max=7
lsi='100 400 1600 6400 25600 102400 409600 1638400'
for i in $lsi; 
do
dim=$i
echo $dim

typestr='Freq_'$dim
cp $path\queryDb$typestr\.txt $path\Db$typestr\_srt.txt $path\sortkey$typestr\.txt $path\VocabMapHeader$typestr\.txt $path\VocabMap$typestr\.bin .
echo "-------------- Freq ---------------------------"
touch $ResDir\/Brute_SearchResult$typestr\.txt
./bruteForceSearch -readDb Db$typestr\_srt.txt -queryfile queryDb$typestr\.txt -doOnDisk 1 > $ResDir\/Brute_SearchResult$typestr\.txt
touch $ResDir\/Fast_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -sortKey sortkey$typestr\.txt > $ResDir\/Fast_SearchResult$typestr\.txt
touch $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.5 -sortKey sortkey$typestr\.txt > $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.2 -sortKey sortkey$typestr\.txt > $ResDir\/Approx20_SearchResult$typestr\.txt
rm queryDb$typestr\.txt Db$typestr\_srt.txt sortkey$typestr\.txt VocabMapHeader$typestr\.txt VocabMap$typestr\.bin


typestr='InFreq_'$dim
cp $path\queryDb$typestr\.txt $path\Db$typestr\_srt.txt $path\sortkey$typestr\.txt $path\VocabMapHeader$typestr\.txt $path\VocabMap$typestr\.bin .
echo "-------------- In Freq ---------------------------"
touch $ResDir\/Brute_SearchResult$typestr\.txt
./bruteForceSearch -readDb Db$typestr\_srt.txt -queryfile queryDb$typestr\.txt -doOnDisk 1 > $ResDir\/Brute_SearchResult$typestr\.txt
touch $ResDir\/Fast_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -sortKey sortkey$typestr\.txt > $ResDir\/Fast_SearchResult$typestr\.txt
touch $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.5 -sortKey sortkey$typestr\.txt > $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.2 -sortKey sortkey$typestr\.txt > $ResDir\/Approx20_SearchResult$typestr\.txt
rm queryDb$typestr\.txt Db$typestr\_srt.txt sortkey$typestr\.txt VocabMapHeader$typestr\.txt VocabMap$typestr\.bin


typestr='Rand_'$dim
cp $path\queryDb$typestr\.txt $path\Db$typestr\_srt.txt $path\sortkey$typestr\.txt $path\VocabMapHeader$typestr\.txt $path\VocabMap$typestr\.bin .
echo "-------------- Random ---------------------------"
touch $ResDir\/Brute_SearchResult$typestr\.txt
./bruteForceSearch -readDb Db$typestr\_srt.txt -queryfile queryDb$typestr\.txt -doOnDisk 1 > $ResDir\/Brute_SearchResult$typestr\.txt
touch $ResDir\/Fast_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -sortKey sortkey$typestr\.txt > $ResDir\/Fast_SearchResult$typestr\.txt
touch $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.5 -sortKey sortkey$typestr\.txt > $ResDir\/Approx_SearchResult$typestr\.txt
./exactSearchOpt -doOnDisk 1 -queryfile queryDb$typestr\.txt -vocabMapHeader VocabMapHeader$typestr\.txt -vocabMapDb VocabMap$typestr\.bin -readDb Db$typestr\_srt.txt -approx 1.2 -sortKey sortkey$typestr\.txt > $ResDir\/Approx20_SearchResult$typestr\.txt
rm queryDb$typestr\.txt Db$typestr\_srt.txt sortkey$typestr\.txt VocabMapHeader$typestr\.txt VocabMap$typestr\.bin
done
