# SimSearch-text
This code implements fast cosine similarity search between text sentences. 
If have a database of sentences (can be tweets/ documents etc ) and a list of query sentences. 
This code searches the database and finds the nearest neighbhor to the given queries using cosine similarity. 
The code supports : 
1) brute force search 
) Fast exact search by building smart indexes 
3) Fast approximate search (given the error tolerance) . 

It is written in C language and you just need basic c compiler to use the code. It has been tested on linux 64 bit machines

For a database of 14 million tweets and 1000 queries fast exact search takes about 160 seconds to search the nearest neighbour
while Approximate search takes about 90 seconds. (Both times don't take into account index building phase which can 
take pretty long but is a one time thing).


Read the report.pdf for more technical details and read the README in src for usage instruction.
Best place to start is with the master_deploy.sh script.

