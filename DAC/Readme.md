The program implements the fast integer retrieval method based on Directly Addressable variable-length Codes (DAC) [1].
The Succinct data structure library (SDSL) is required, https://github.com/simongog/sdsl/.

Command line to compile:
g++ dac.cpp -O3 -lsdsl -o DAC -I /sdsl-lite-include-file-directory/ -L /the-directory-including-the-sdsl-lib/ -std=c++17 -march=native

Run program with 2 command line arguments: 
1) the number of iterations of the retrieval test;
2) the maximal index of an integer to be retrieved.

Indices in the given range are generated randomly.

The input test data should be stored in files in the program directory:
- a0.bin,...,aK.bin - 4- or 8-bit integer chunks;
- b0.bin,...,bK.bin - bitstreams denoting the chunk existance.

[1] Nieves R. Brisaboa, Susana Ladra, and Gonzalo Navarro. Directly addressable variable-length codes. In Jussi Karlgren, Jorma Tarhio, and Heikki Hyyr¨o, editors, String Processing and Information Retrieval, pages 122–130, Berlin, Heidelberg, 2009. Springer Berlin Heidelberg.
