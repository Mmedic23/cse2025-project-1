#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <locale.h>
#include <string.h>
#include <math.h>

// MAX_WORD_LEN is defined to represent the maximum length of a single word in the dataset.
#define MAX_WORD_LEN 25
int docs = 0;
int categorySizes[3] = {0, 0, 0};
int terms = 0;

// Used for storing the most frequently used terms in 2 different orders for parts b and c
struct tableNode
{
	struct term *assignedTerm;
	int tf;
	double idf;
	struct tableNode *next;
	struct tableNode *nextInverse;
};

// These arrays each represent 3 linked lists which are sorted by their tf or tf*idf values.
struct tableNode *categoryHeads[3];
struct tableNode *categoryInverseHeads[3];

// Used for storing the connections (first order relations) to other terms inside each term.
// This struct also proved useful for constructing any kind of secondary linked list from the MLL.
struct conNode
{
	struct term *connection;
	struct conNode *next;
};

//O(n)
// Takes a pointer to the head of a conNode linked list, and adds the termToConnect term to the linked list.
int addCon(struct conNode **connections, struct term *termToConnect)
{
	struct conNode *conToAdd = malloc(sizeof(struct conNode));
	conToAdd->connection = termToConnect;
	conToAdd->next = NULL;
	// If the linked list is empty, the termToConnect is now the head of the linked list.
	if (*connections == NULL)
	{
		*connections = conToAdd;
		return 0;
	}
	else
	{
		struct conNode *curNode = *connections;
		while (1)
		{
			// We loop through each node until we reach the tail of the linked list and add the termToConnect to the tail.
			if (curNode->next == NULL)
			{
				curNode->next = conToAdd;
				return 0;
			}
			else
			{
				curNode = curNode->next;
			}
		}
	}
}

// O(n)
// Loops through each item in a given conNode linked list and returns 1 if the given termToSearch exists in the linked list, returns 0 if not.
int connectionExists(struct conNode *connections, struct term *termToSearch)
{
	if (connections == NULL)
	{
		return 0;
	}

	struct conNode *curNode = connections;
	while (1)
	{
		if (curNode->connection == termToSearch)
		{
			return 1;
		}
		if (curNode->next == NULL)
		{
			return 0;
		}
		curNode = curNode->next;
	}
}

// Used to store each unique term in the dataset in a linked list. This struct forms the MLL.
struct term
{
	char content[MAX_WORD_LEN];
	struct term *next;
	char *includedInDoc;
	struct conNode *connections;
};

struct term *headTerms;

// Adds the given word to the MLL, keeps the linked list alphabetically sorted.
// NOTE: tr_TR.utf8 locale needs to be installed in order for the function to sort Turkish characters properly.
int addTerm(char wordToAdd[MAX_WORD_LEN], int docIndex)
{
	struct term *termToAdd = malloc(sizeof(struct term));
	strncpy(termToAdd->content, wordToAdd, MAX_WORD_LEN);
	termToAdd->includedInDoc = calloc(docs, sizeof(char));
	termToAdd->includedInDoc[docIndex] = 1;

	if (headTerms == NULL)
	{
		headTerms = termToAdd;
		return 1;
	}
	else
	{
		struct term *curTerm = headTerms;
		if (strcoll(wordToAdd, curTerm->content) < 0)
		{
			// wordToAdd comes BEFORE the head
			termToAdd->next = headTerms;
			headTerms = termToAdd;
			return 1;
		}
		while (1)
		{
			if (curTerm->next == NULL)
			{
				break;
			}

			int result = strcoll(wordToAdd, curTerm->next->content);
			if (result < 0)
			{
				// wordToAdd comes BEFORE curTerm->next->content
				break;
			}
			curTerm = curTerm->next;
		}
		termToAdd->next = curTerm->next;
		curTerm->next = termToAdd;
		// We keep the total term count to later allocate appropriate memory.
		terms++;
	}
}

// Finds the given word in the MLL. Returns the pointer to the term if found, NULL if not.
struct term *findTerm(char wordToSearch[MAX_WORD_LEN])
{
	if (headTerms == NULL)
	{
		return 0;
	}

	struct term *curTerm = headTerms;
	while (1)
	{
		if (curTerm->next == NULL)
		{
			break;
		}

		int result = strcoll(wordToSearch, curTerm->next->content);
		if (result < 0)
		{
			// wordToAdd comes BEFORE curTerm->next->content
			break;
		}

		curTerm = curTerm->next;
	}
	if (strcoll(wordToSearch, curTerm->content) == 0)
	{
		return curTerm;
	}
	else
	{
		return 0;
	}
}

// Used to keep document's paths in a linked list to read each files.
struct document
{
	char path[256];
	struct document *next;
};

struct document *headDocs;
// A tail pointer is used to add docs more quickly.
struct document *tailDocs;

// Adds the given path to the documents linked list.
int addDoc(char *path)
{
	struct document *docToAdd = malloc(sizeof(struct document));
	strcpy(docToAdd->path, path);
	docToAdd->next = NULL;
	if (headDocs == NULL)
	{
		headDocs = docToAdd;
		tailDocs = docToAdd;
	}
	else
	{
		tailDocs->next = docToAdd;
		tailDocs = docToAdd;
	}
	docs++;
}

// Prints the two given terms in a formatted manner.
int printOrder(struct term *firstTerm, struct term *secondTerm)
{
	char *buffer = calloc(60, sizeof(char));
	strcat(buffer, "/");
	strcat(buffer, firstTerm->content);
	strcat(buffer, "-");
	strcat(buffer, secondTerm->content);
	strcat(buffer, "/");
	puts(buffer);
	free(buffer);
	buffer = NULL;
}

// Gets the tf value given a term and the category index.
int getCategoryFrequency(struct term *term, int categoryIndex)
{
	int sum = 0;
	int offset = 0;
	if (categoryIndex > 0)
	{
		offset = categorySizes[categoryIndex - 1];
	}

	for (int i = 0; i < categorySizes[categoryIndex]; i++)
	{
		sum += term->includedInDoc[offset + i];
	}
	return sum;
}

// Gets the idf value given a term and the category index.
double getInverseDocumentFrequency(struct term *term, int categoryIndex)
{
	double totalDocs = categorySizes[categoryIndex];
	double docsIncludingTerm = 0;
	int offset = 0;
	if (categoryIndex > 0)
	{
		offset = categorySizes[categoryIndex - 1];
	}

	for (int i = 0; i < categorySizes[categoryIndex]; i++)
	{
		if (term->includedInDoc[offset + i] != 0)
		{
			docsIncludingTerm++;
		}
	}
	if (docsIncludingTerm == 0)
	{
		return 0;
	}

	return log(totalDocs / docsIncludingTerm);
}

// Adds a given term to the both the term frequency list and the inverse document frequency list in the correct order.
int addTableNode(int categoryIndex, struct term *termToAdd)
{
	struct tableNode *tnToAdd = malloc(sizeof(struct tableNode));
	tnToAdd->assignedTerm = termToAdd;
	tnToAdd->tf = getCategoryFrequency(termToAdd, categoryIndex);
	tnToAdd->idf = getInverseDocumentFrequency(termToAdd, categoryIndex);
	tnToAdd->next = NULL;
	tnToAdd->nextInverse = NULL;

	// Add term to the term frequency list
	if (categoryHeads[categoryIndex] == NULL)
	{
		categoryHeads[categoryIndex] = tnToAdd;
	}
	else
	{
		struct tableNode *curTn = categoryHeads[categoryIndex];
		if (curTn->tf < tnToAdd->tf)
		{
			tnToAdd->next = curTn;
			categoryHeads[categoryIndex] = tnToAdd;
		}
		else
		{
			while (1)
			{
				if (curTn->next == NULL)
				{
					break;
				}

				if (curTn->next->tf <= tnToAdd->tf)
				{
					// tnToAdd should be added after curTn
					break;
				}

				curTn = curTn->next;
			}
			tnToAdd->next = curTn->next;
			curTn->next = tnToAdd;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	// Add term to the inverse document frequency list.
	if (categoryInverseHeads[categoryIndex] == NULL)
	{
		categoryInverseHeads[categoryIndex] = tnToAdd;
	}
	else
	{
		struct tableNode *curTn = categoryInverseHeads[categoryIndex];
		if ((curTn->tf * curTn->idf) < (tnToAdd->tf * tnToAdd->idf))
		{
			tnToAdd->nextInverse = curTn;
			categoryInverseHeads[categoryIndex] = tnToAdd;
		}
		else
		{
			while (1)
			{
				if (curTn->nextInverse == NULL)
				{
					break;
				}

				if ((curTn->nextInverse->tf * curTn->nextInverse->idf) <= (tnToAdd->tf * tnToAdd->idf))
				{
					// tnToAdd should be added after curTn
					break;
				}

				curTn = curTn->nextInverse;
			}
			tnToAdd->nextInverse = curTn->nextInverse;
			curTn->nextInverse = tnToAdd;
		}
	}
}

// Wrapper function to addd a term to the frequency lists for every available category.
int addTableNodes(struct term *termToAdd)
{
	addTableNode(0, termToAdd);
	addTableNode(1, termToAdd);
	addTableNode(2, termToAdd);
}

int main()
{
	// setLocale is used to set the collation language to Turkish, so that Turkish characters can be sorted properly.
	// The tr_TR.utf8 locale must be available to the C library on the system for this to work.
	if (setlocale(LC_COLLATE, "tr_TR.utf8") == NULL)
	{
		puts("WARNING! The Turkish locale for collation is not installed on this system. Words containing Turkish characters cannot be sorted alphabetically.");
	}

	// START
	// First we open the dataset directory to read all files under sub-directories and populate the documents linked list with the documents.
	DIR *d;
	struct dirent *dir;
	d = opendir("../dataset");
	if (d)
	{
		int categoryIndex = -1;
		while ((dir = readdir(d)) != NULL)
		{
			if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, ".."))
			{
				continue;
			}
			categoryIndex++;
			char *subPath = calloc(30, sizeof(char));
			strcat(subPath, "../dataset/");
			strcat(subPath, dir->d_name);
			DIR *subD = opendir(subPath);
			if (subD)
			{
				struct dirent *subDir;
				while ((subDir = readdir(subD)) != NULL)
				{
					if (!strcmp(subDir->d_name, ".") || !strcmp(subDir->d_name, ".."))
					{
						continue;
					}

					char buff[256];
					strcat(buff, "dataset/");
					strcat(buff, dir->d_name);
					strcat(buff, "/");
					strcat(buff, subDir->d_name);
					addDoc(buff);
					categorySizes[categoryIndex]++;
					memset(buff, 0, sizeof(buff));
				}
				closedir(subD);
			}
			free(subPath);
		}
		closedir(d);
	}

	// Then we loop through each document in the linked list to read each document word by word.
	struct document *curDoc = headDocs;
	int docIndex = 0;
	while (1)
	{
		char *fileDir = calloc(50, sizeof(char));
		strcat(fileDir, "../");
		strcat(fileDir, curDoc->path);
		FILE *fileToRead = fopen(fileDir, "r");
		char inWord[MAX_WORD_LEN];
		while (fscanf(fileToRead, " %25s", inWord) == 1)
		{
			struct term *foundTerm = findTerm(inWord);
			if (!foundTerm)
			{
				// We only add the new term if it doesn't already exist.
				addTerm(inWord, docIndex);
			}
			else
			{
				// If the term already exists, we increase the number of times it is used in the current document.
				foundTerm->includedInDoc[docIndex]++;
			}
		}
		printf("\n");

		curDoc = curDoc->next;
		docIndex++;
		fclose(fileToRead);
		free(fileDir);
		if (curDoc == NULL)
		{
			break;
		}
	}

	// Then we loop through each term in the MLL to build connections between terms that are seen together in the same document.
	// These connections are actually 1st order relationships.
	struct term *curTerm = headTerms;
	while (1)
	{
		addTableNodes(curTerm);
		for (int i = 0; i < docs; i++) // foreach included doc in curTerm's includedDocs
		{
			if (curTerm->includedInDoc[i] == 0)
			{
				continue;
			}
			struct term *termToConnect = headTerms;
			while (1)
			{
				if (termToConnect == curTerm)
				{
					termToConnect = termToConnect->next;
					continue;
				}
				if (termToConnect == NULL)
				{
					break;
				}
				if (termToConnect->includedInDoc[i] != 0) // termToConnect and curTerm exist in the same file at least once.
				{
					// We add the connection as a two-way link, and only add the connection if it already doesn't exists
					if (!connectionExists(curTerm->connections, termToConnect))
					{
						addCon(&(curTerm->connections), termToConnect);
					}
					if (!connectionExists(termToConnect->connections, curTerm))
					{
						addCon(&(termToConnect->connections), curTerm);
					}
				}
				termToConnect = termToConnect->next;
			}
		}
		curTerm = curTerm->next;
		if (curTerm == NULL)
		{
			break;
		}
	}

	// Then we start printing 1st, 2nd and 3rd order relations.
	// For each type of relation, we run through each term.
	// For 1st order co-occurances, we loop through each term and each term's connections, printing each term and its connected termm.

	// For 2nd order co-occurances, we loop through each term, each term's connections and each connection's connections.
	// We check if the resulting relationship is 1st order or not. If it is, the relationship is not printed.
	// We also need to check if the relationship has been already printed, in the inverse order. If it has, we do not need to print it.

	// The same algorithm is used to print 3rd order co-occurances, but we need to make some extra checks.
	curTerm = headTerms;
	struct conNode *alreadyPrinted = calloc(terms, sizeof(struct conNode));
	puts("==========FIRST ORDER START==========");
	while (1)
	{
		// Looping through each term
		struct conNode *curNode = curTerm->connections;
		while (1)
		{
			// Looping through each connection of a term
			if (!connectionExists(alreadyPrinted, curNode->connection))
			{
				printOrder(curTerm, curNode->connection);
			}

			curNode = curNode->next;
			if (curNode == NULL)
			{
				break;
			}
		}
		addCon(&alreadyPrinted, curTerm);
		curTerm = curTerm->next;
		if (curTerm == NULL)
		{
			break;
		}
	}

	curTerm = headTerms;
	free(alreadyPrinted);
	alreadyPrinted = calloc(terms, sizeof(struct conNode));
	puts("==========SECOND ORDER START==========");
	while (1)
	{
		//Looping through each term
		struct conNode *curNode = curTerm->connections;
		while (1)
		{
			//Looping through each connection of the first term
			struct conNode *secondNode = curNode->connection->connections;
			while (1)
			{
				//Looping through each connection of the connected term
				if ((!(secondNode->connection == curTerm)) &&
					(!(connectionExists(curTerm->connections, secondNode->connection))) &&
					(!(connectionExists(alreadyPrinted, secondNode->connection))))
				{
					printOrder(curTerm, secondNode->connection);
				}
				secondNode = secondNode->next;
				if (secondNode == NULL)
				{
					break;
				}
			}
			curNode = curNode->next;
			if (curNode == NULL)
			{
				break;
			}
		}
		addCon(&alreadyPrinted, curTerm);
		curTerm = curTerm->next;
		if (curTerm == NULL)
		{
			break;
		}
	}

	curTerm = headTerms;
	free(alreadyPrinted);
	alreadyPrinted = calloc(terms, sizeof(struct conNode));
	puts("==========THIRD ORDER START==========");
	while (1)
	{
		//Looping through each term
		struct conNode *curNode = curTerm->connections;
		while (1)
		{
			//Looping through each connection of the first term
			struct conNode *secondNode = curNode->connection->connections;
			while (1)
			{
				//Looping through each connection of the connected term
				if ((!(secondNode->connection == curTerm)) &&
					(!(connectionExists(curTerm->connections, secondNode->connection))))
				{
					//Looping through each connection of the connected term's connected term
					struct conNode *thirdNode = secondNode->connection->connections;
					while (1)
					{
						// This pair of terms is a third order connection only if;
						//		the connection line didn't loop back to the first connected term
						//		there is no connection between the first connected term and the third term
						//		the connection was not already printed.
						if ((!(thirdNode->connection == curNode->connection)) &&
							(!(connectionExists(curNode->connection->connections, thirdNode->connection))) &&
							(!(connectionExists(alreadyPrinted, thirdNode->connection))))
						{
							printOrder(curTerm, thirdNode->connection);
						}

						thirdNode = thirdNode->next;
						if (thirdNode == NULL)
						{
							break;
						}
					}
				}
				secondNode = secondNode->next;
				if (secondNode == NULL)
				{
					break;
				}
			}
			curNode = curNode->next;
			if (curNode == NULL)
			{
				break;
			}
		}
		addCon(&alreadyPrinted, curTerm);
		curTerm = curTerm->next;
		if (curTerm == NULL)
		{
			break;
		}
	}
	free(alreadyPrinted);
	alreadyPrinted = NULL;
	curTerm = NULL;
	puts("");

	// Here, we simply loop through each item of each frequency linked list to print the top 5 items.
	// Assumes at least 5 words in each category
	for (int r = 0; r < 5; r++)
	{
		for (int i = 0; i < 3; i++)
		{
			struct tableNode *frequentTerm = categoryHeads[i];
			for (int j = 0; j < r; j++)
			{
				frequentTerm = frequentTerm->next;
			}
			char *buffer = calloc(25, sizeof(char));
			sprintf(buffer, "%s,%d", frequentTerm->assignedTerm->content, frequentTerm->tf);
			printf("|%-25s|", buffer);
			free(buffer);
		}
		printf("\n");
	}
	puts("");

	for (int r = 0; r < 5; r++)
	{
		for (int i = 0; i < 3; i++)
		{
			struct tableNode *frequentTerm = categoryInverseHeads[i];
			for (int j = 0; j < r; j++)
			{
				frequentTerm = frequentTerm->nextInverse;
			}
			char *buffer = calloc(30, sizeof(char));
			sprintf(buffer, "%s, %.2lf, %d", frequentTerm->assignedTerm->content, frequentTerm->idf, frequentTerm->tf);
			printf("|%-25s|", buffer);
			free(buffer);
		}
		printf("\n");
	}
	return 0;
}