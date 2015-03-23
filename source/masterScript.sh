
#Build All files
make

#Now create the vocabulary from database 

./createVocab -read-vocab dbTweets.txt -save-vocab vocabMasterDb.txt


# Create ll the databases required by search algorithms for all d = 100 to 1638400
./createAlldB.sh
# Run the nearest neighbour tests on all the databases
./runAllNNtest.sh
