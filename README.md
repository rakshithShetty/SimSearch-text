# SimSearch-text
This code implements fast cosine similarity search to find the nearest neighbor to a query sentence in a database. 
Input is a database of sentences (can be tweets/ comments etc ) and a list of query sentences.
This code searches the database and finds the nearest neighbhor to the given queries using cosine similarity (Similar to Jaccard similarity).
The code implements:
1) brute force search 
2) Fast exact search by building smart indexes 
3) Fast approximate search (given the error tolerance).

It is written in C language and you just need basic c compiler to use the code. It has been tested on linux 64 bit machines.

Fast search methods need to first build indexes on the database, which help speed up the search.

For a database of 14 million tweets and 1000 queries fast exact search takes about 160 seconds to search the nearest neighbour
while Approximate search takes about 90 seconds. (Both times don't take into account index building phase which can 
take pretty long but is a one time thing).

Read the report.pdf for more technical details and read the README in src directory for usage instruction.
Best place to start is with the master_deploy.sh script.
