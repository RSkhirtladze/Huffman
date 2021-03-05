/**********************************************************
 * File: HuffmanEncoding.cpp
 *
 * Implementation of the functions from HuffmanEncoding.h.
 * Most (if not all) of the code that you write for this
 * assignment will go into this file.
 */

#include "HuffmanEncoding.h"
#include "pqueue.h"

/* Function: getFrequencyTable
 * Usage: Map<ext_char, int> freq = getFrequencyTable(file);
 * --------------------------------------------------------
 * Given an input stream containing text, calculates the
 * frequencies of each character within that text and stores
 * the result as a Map from ext_chars to the number of times
 * that the character appears.
 *
 * This function will also set the frequency of the PSEUDO_EOF
 * character to be 1, which ensures that any future encoding
 * tree built from these frequencies will have an encoding for
 * the PSEUDO_EOF character.
 */
Map<ext_char, int> getFrequencyTable(istream& file) {
	Map<ext_char, int> charCount;
	char tmpChar;
	while(file.get(tmpChar)){
		if(!charCount.containsKey(tmpChar)) charCount.put(tmpChar, 0);
		charCount.put(tmpChar, charCount.get(tmpChar) + 1);
	}
	charCount.put(PSEUDO_EOF, 1);
	return charCount;	
}

/* Function: buildEncodingTree
 * Usage: Node* tree = buildEncodingTree(frequency);
 * --------------------------------------------------------
 * Given a map from extended characters to frequencies,
 * constructs a Huffman encoding tree from those frequencies
 * and returns a pointer to the root.
 *
 * This function can assume that there is always at least one
 * entry in the map, since the PSEUDO_EOF character will always
 * be present.
 */
Node* buildEncodingTree(Map<ext_char, int>& frequencies) {
	PriorityQueue<Node*> pq;
	// Create all Nodes and put them into the priority queue
	foreach(ext_char ch in frequencies){
		Node* newNode = new Node;
		newNode->character = ch;
		newNode->weight = frequencies.get(ch);
		newNode->zero = NULL;
		newNode->one = NULL;
		pq.enqueue(newNode, frequencies.get(ch));
	}
	// Merge all Nodes using Huffman algorithm, until only 1 node is left
	while(pq.size() > 1){
		Node* zeroNode = pq.dequeue();
		Node* oneNode = pq.dequeue();
		Node* parent = new Node;
		int weight = zeroNode->weight + oneNode->weight;
		parent->weight = weight;
		parent->zero = zeroNode;
		parent->one = oneNode;
		parent->character = NOT_A_CHAR;
		pq.enqueue(parent, weight);
	}
	// Return one and only root node that left in queue
	return pq.dequeue();
}

/* Function: freeTree
 * Usage: freeTree(encodingTree);
 * --------------------------------------------------------
 * Deallocates all memory allocated for a given encoding
 * tree.
 */
void freeTree(Node* root) {
	if(root == NULL) return;

	freeTree(root->zero);
	freeTree(root->one);
	delete root;
}

/* Function: encodeFile
 * Usage: encodeFile(source, encodingTree, output);
 * --------------------------------------------------------
 * Encodes the given file using the encoding specified by the
 * given encoding tree, then writes the result one bit at a
 * time to the specified output file.
 *
 * This function can assume the following:
 *
 *   - The encoding tree was constructed from the given file,
 *     so every character appears somewhere in the encoding
 *     tree.
 *
 *   - The output file already has the encoding table written
 *     to it, and the file cursor is at the end of the file.
 *     This means that you should just start writing the bits
 *     without seeking the file anywhere.
 */ 
void encodeFile(istream& infile, Node* encodingTree, obstream& outfile) {
	Map<ext_char, string> encodedChars;
	getEncodedMap(encodingTree, encodedChars, "");

	char tmpChar;
	while(infile.get(tmpChar)){
		string code = encodedChars.get(tmpChar);
		for (int i = 0; i < code.size(); i++){
			int bit = code[i] - '0';
			outfile.writeBit(bit);
		}
	}
	string code = encodedChars.get(PSEUDO_EOF);
	for (int i = 0; i < code.size(); i++){
		int bit = code[i] - '0';
		outfile.writeBit(bit);
	}

}

void getEncodedMap(Node* tree, Map<ext_char, string> & mp, string code){
	if(tree == NULL) return;
	if(tree->character!=NOT_A_CHAR){
		mp.put(tree->character, code);
	}
	getEncodedMap(tree->zero, mp, code + "0");
	getEncodedMap(tree->one, mp, code + "1");
}

/* Function: decodeFile
 * Usage: decodeFile(encodedFile, encodingTree, resultFile);
 * --------------------------------------------------------
 * Decodes a file that has previously been encoded using the
 * encodeFile function.  You can assume the following:
 *
 *   - The encoding table has already been read from the input
 *     file, and the encoding tree parameter was constructed from
 *     this encoding table.
 *
 *   - The output file is open and ready for writing.
 */
void decodeFile(ibstream& infile, Node* encodingTree, ostream& file) {
	Map<string, ext_char> decodedBits;
	getDecodedMap(encodingTree, decodedBits, "");
		
	string currCode = "";

	while(true){
		int bit = infile.readBit();
	
		currCode+= integerToString(bit);
		if(decodedBits.containsKey(currCode)){
			if(decodedBits.get(currCode) == PSEUDO_EOF) {
				return;	
			}

			file.put(decodedBits.get(currCode));
			currCode = "";
		}
	}
}
void getDecodedMap(Node* tree, Map<string, ext_char> & mp, string code){
	if(tree == NULL) return;
	if(tree->character!=NOT_A_CHAR){
		mp.put(code, tree->character);
	}
	getDecodedMap(tree->zero, mp, code + "0");
	getDecodedMap(tree->one, mp, code + "1");
}

/* Function: writeFileHeader
 * Usage: writeFileHeader(output, frequencies);
 * --------------------------------------------------------
 * Writes a table to the front of the specified output file
 * that contains information about the frequencies of all of
 * the letters in the input text.  This information can then
 * be used to decompress input files once they've been
 * compressed.
 *
 * This function is provided for you.  You are free to modify
 * it if you see fit, but if you do you must also update the
 * readFileHeader function defined below this one so that it
 * can properly read the data back.
 */
void writeFileHeader(obstream& outfile, Map<ext_char, int>& frequencies) {
	/* The format we will use is the following:
	 *
	 * First number: Total number of characters whose frequency is being
	 *               encoded.
	 * An appropriate number of pairs of the form [char][frequency][space],
	 * encoding the number of occurrences.
	 *
	 * No information about PSEUDO_EOF is written, since the frequency is
	 * always 1.
	 */
	 
	/* Verify that we have PSEUDO_EOF somewhere in this mapping. */
	if (!frequencies.containsKey(PSEUDO_EOF)) {
		error("No PSEUDO_EOF defined.");
	}
	
	/* Write how many encodings we're going to have.  Note the space after
	 * this number to ensure that we can read it back correctly.
	 */
	outfile << frequencies.size() - 1 << ' ';
	
	/* Now, write the letter/frequency pairs. */
	foreach (ext_char ch in frequencies) {
		/* Skip PSEUDO_EOF if we see it. */
		if (ch == PSEUDO_EOF) continue;
		
		/* Write out the letter and its frequency. */
		outfile << char(ch) << frequencies[ch] << ' ';
	}
}

/* Function: readFileHeader
 * Usage: Map<ext_char, int> freq = writeFileHeader(input);
 * --------------------------------------------------------
 * Reads a table to the front of the specified input file
 * that contains information about the frequencies of all of
 * the letters in the input text.  This information can then
 * be used to reconstruct the encoding tree for that file.
 *
 * This function is provided for you.  You are free to modify
 * it if you see fit, but if you do you must also update the
 * writeFileHeader function defined before this one so that it
 * can properly write the data.
 */
Map<ext_char, int> readFileHeader(ibstream& infile) {
	/* This function inverts the mapping we wrote out in the
	 * writeFileHeader function before.  If you make any
	 * changes to that function, be sure to change this one
	 * too!
	 */
	Map<ext_char, int> result;
	
	/* Read how many values we're going to read in. */
	int numValues;
	infile >> numValues;
	
	/* Skip trailing whitespace. */
	infile.get();
	
	/* Read those values in. */
	for (int i = 0; i < numValues; i++) {
		/* Get the character we're going to read. */
		ext_char ch = infile.get();
		
		/* Get the frequency. */
		int frequency;
		infile >> frequency;
		
		/* Skip the space character. */
		infile.get();
		
		/* Add this to the encoding table. */
		result[ch] = frequency;
	}
	
	/* Add in 1 for PSEUDO_EOF. */
	result[PSEUDO_EOF] = 1;
	return result;
}

/* Function: compress
 * Usage: compress(infile, outfile);
 * --------------------------------------------------------
 * Main entry point for the Huffman compressor.  Compresses
 * the file whose contents are specified by the input
 * ibstream, then writes the result to outfile.  Your final
 * task in this assignment will be to combine all of the
 * previous functions together to implement this function,
 * which should not require much logic of its own and should
 * primarily be glue code.
 */
void compress(ibstream& infile, obstream& outfile) {
	Map<ext_char, int> charCount = getFrequencyTable(infile);

	Node* root =  buildEncodingTree(charCount);
	writeFileHeader(outfile, charCount);
	infile.rewind();

	encodeFile(infile, root, outfile);
	freeTree(root);

}

/* Function: decompress
 * Usage: decompress(infile, outfile);
 * --------------------------------------------------------
 * Main entry point for the Huffman decompressor.
 * Decompresses the file whose contents are specified by the
 * input ibstream, then writes the decompressed version of
 * the file to the stream specified by outfile.  Your final
 * task in this assignment will be to combine all of the
 * previous functions together to implement this function,
 * which should not require much logic of its own and should
 * primarily be glue code.
 */
void decompress(ibstream& infile, ostream& outfile) {
	Map<ext_char, int> charCount = readFileHeader(infile);
    Node* root = buildEncodingTree(charCount);
    decodeFile(infile, root, outfile);

    freeTree(root);
}

