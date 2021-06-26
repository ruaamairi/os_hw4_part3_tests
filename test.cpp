//
// Created by ariel on 25/06/2021.
//

#include <cstdlib>
#include <sstream>
#include <sys/wait.h>
#include "printMemoryList.h"
#include "malloc_3.h"

/////////////////////////////////////////////////////

#define MAX_ALLOC 23

// Copy your type here
// dont change anything from the one in malloc_3.c !!not even the order of args!!!
typedef struct MallocMetadata3 {
	size_t size;
	bool is_free;
	bool on_heap;
	MallocMetadata3 *next;
	MallocMetadata3 *prev;
} Metadata3;


///////////////////////////////////////////////////


#define NUM_FUNC 8

typedef std::string (*TestFunc)(void *[MAX_ALLOC]);

void *memory_start_addr;

int max_test_name_len;


///////////////test functions/////////////////////
void freeAll(void *array[MAX_ALLOC]) {
	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		sfree(array[i]);
	}
}


std::string allocNoFree(void *array[MAX_ALLOC]) {
	std::string expected = "|U:100|U:100000|U:10|\n";
	if (MAX_ALLOC < 4) {
		std::cout << "Test Wont work with MAX_ALLOC < 4";
		return expected;
	}

	array[0] = smalloc(100);
	array[1] = smalloc(100000);
	array[2] = smalloc(10);
	array[3] = smalloc(11e8);


	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string allocandFree(void *array[MAX_ALLOC]) {

	std::string expected = "";
	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		if (i % 5 == 0) {
			expected += "|F:10";
		} else {
			expected += "|U:10";
		}
	}
	expected += "|\n";

	if (MAX_ALLOC < 6) {
		std::cout << "Test Wont work with MAX_ALLOC < 6";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		array[i] = smalloc(10);
	}
	for (int i = 0 ; i < MAX_ALLOC ; i += 5) {
		sfree(array[i]);
	}

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string allocandFreeMerge(void *array[MAX_ALLOC]) {

	std::string block_of2 = std::to_string(2 * 10 + sizeof(Metadata3));
	std::string block_of3 = std::to_string(3 * 10 + 2 * sizeof(Metadata3));
	std::string expected = "";
	int j = 1;
	for (; j + 8 < MAX_ALLOC ; j += 10) {
		expected += "|U:10";
		expected += "|F:" + block_of2;
		expected += "|U:10";
		expected += "|F:" + block_of2;
		expected += "|U:10";
		expected += "|F:" + block_of3;
	}
	for (; j - 1 < MAX_ALLOC ; ++j) {
		expected += "|U:10";
	}
	expected += "|\n";

	if (MAX_ALLOC < 10) {
		std::cout << "Test Wont work with MAX_ALLOC < 10";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		array[i] = smalloc(10);
		if (!array[i]) {
			std::cout << "Failed to allocate INDEX: " << i << std::endl;
			return expected;
		}
	}
	for (int i = 1 ; i + 8 < MAX_ALLOC ; i += 10) {
		sfree(array[i]);
		sfree(array[i + 1]);
		//keep +2 allocated

		//check if order matter
		sfree(array[i + 4]);
		sfree(array[i + 3]);

		//keep +5 allocated

		//check if merge 3
		sfree(array[i + 6]);
		sfree(array[i + 8]);
		sfree(array[i + 7]);
	}
	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testRealloc(void *array[MAX_ALLOC]) {
	std::string block_of2 = std::to_string(2 * 10 + sizeof(Metadata3));
	std::string expected = "aaaaaaaaaa|U:" + block_of2;
	for (int i = 2 ; i < MAX_ALLOC ; ++i) {
		expected += "|U:10";
	}
	expected += "|\n";

	if (MAX_ALLOC < 2) {
		std::cout << "Test Wont work with MAX_ALLOC < 2";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		array[i] = smalloc(10);
		if (!array[i]) {
			std::cout << "Failed to allocate INDEX: " << i << std::endl;
			return expected;
		}
	}

	for (int i = 0 ; i < 10 ; ++i) {
		((char *) array[0])[i] = 'b';
		((char *) array[1])[i] = 'a';
	}
	sfree(array[0]);
	array[1] = srealloc(array[1], 20);
	for (int i = 0 ; i < 10 ; ++i) {
		std::cout << ((char *) array[0])[i];
	}

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

/**
 * test if we used the small option for realloc
 * @param array
 * @return
 */
std::string testRealloc2(void *array[MAX_ALLOC]) {
	std::string expected = "bbbbbbbbbb";
	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		if (i == 3 || i == 5 || i == 7 || i == 9) {
			expected += "|F:10";
		} else {
			expected += "|U:10";
		}
	}
	expected += "|F:30|U:40|U:20|\n";

	if (MAX_ALLOC < 10) {
		std::cout << "Test Wont work with MAX_ALLOC < 10";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		array[i] = smalloc(10);
		if (!array[i]) {
			std::cout << "Failed to allocate INDEX: " << i << std::endl;
			return expected;
		}
	}

	sfree(array[5]);
	array[5] = smalloc(30);
	sfree(array[9]);
	array[9] = smalloc(40);
	sfree(array[7]);
	array[7] = smalloc(20);

	for (int i = 0 ; i < 10 ; ++i) {
		((char *) array[3])[i] = 'b';
		((char *) array[5])[i] = 'a';
		((char *) array[7])[i] = 'd';
	}
	sfree(array[5]);
	sfree(array[7]);

	array[3] = srealloc(array[3], 20);
	for (int i = 0 ; i < 10 ; ++i) {
		std::cout << ((char *) array[7])[i];
	}

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testWild(void *array[MAX_ALLOC]) {
	std::string expected = "";
	for (int i = 0 ; i < MAX_ALLOC - 10 ; ++i) {
		expected += "|U:10";
	}
	expected += "|U:30";
	expected += "|U:20|\n";

	if (MAX_ALLOC < 10) {
		std::cout << "Test Wont work with MAX_ALLOC < 10";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC - 9 ; ++i) {
		array[i] = smalloc(10);
		if (!array[i]) {
			std::cout << "Failed to allocate INDEX: " << i << std::endl;
			return expected;
		}
	}

	sfree(array[MAX_ALLOC - 10]);
	array[MAX_ALLOC - 10] = smalloc(30);
	array[MAX_ALLOC - 9] = smalloc(10);
	array[MAX_ALLOC - 9] = srealloc(array[MAX_ALLOC - 9], 20);

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testSplitAndMerge(void *array[MAX_ALLOC]) {
	std::string expected = "|U:30|F:" + std::to_string(200 - 30 - sizeof(Metadata3));
	expected += "|U:200|U:200|U:230|U:230|F:140|F:200|U:200|U:200|U:230|F:170|U:200|U:230|F:170|U:200|U:200|U:200|U:200|U:200|U:200|U:200|U:200|U:200|\n";

	if (MAX_ALLOC < 14) {
		std::cout << "Test Wont work with MAX_ALLOC < 14";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		array[i] = smalloc(200);
		if (!array[i]) {
			std::cout << "Failed to allocate INDEX: " << i << std::endl;
			return expected;
		}
	}

	sfree(array[0]);
	smalloc(30);
	sfree(array[3]);
	sfree(array[6]);
	srealloc(array[4], 230);
	srealloc(array[5], 230);

	sfree(array[10]);
	srealloc(array[9], 230);

	sfree(array[12]);
	srealloc(array[13], 230);


	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testCalloc(void *array[MAX_ALLOC]) {
	std::string expected = "|U:100|U:20|U:100|\n";
	if (MAX_ALLOC < 4) {
		std::cout << "Test Wont work with MAX_ALLOC < 4";
		return expected;
	}

	array[0] = scalloc(1, 100);
	for (int i = 0 ; i < 100 ; ++i) {
		if (((char *) array[0])[i] != 0) {
			std::cout << "array[0] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}
	array[1] = scalloc(2, 10);
	for (int i = 0 ; i < 20 ; ++i) {
		if (((char *) array[1])[i] != 0) {
			std::cout << "array[1] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}
	array[2] = scalloc(10, 10);
	for (int i = 0 ; i < 100 ; ++i) {
		if (((char *) array[2])[i] != 0) {
			std::cout << "array[2] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}
	array[3] = scalloc(100, 10000);
	for (int i = 0 ; i < 1000000 ; ++i) {
		if (((char *) array[3])[i] != 0) {
			std::cout << "array[3] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}


	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}


/////////////////////////////////////////////////////
void *getMemoryStart() {
	void *first = smalloc(2);
	if (!first) { return nullptr; }
	void *start = (char *) first - sizeof(Metadata3);
	sfree(first);
	return start;
}

void printTestName(std::string &name) {
	std::cout << name;
	for (int i = (int) name.length() ; i < max_test_name_len ; ++i) {
		std::cout << " ";
	}
}


bool checkFunc(std::string (*func)(void *[MAX_ALLOC]), void *array[MAX_ALLOC], std::string &test_name) {
	std::stringstream buffer;
	// Redirect std::cout to buffer
	std::streambuf *prevcoutbuf = std::cout.rdbuf(buffer.rdbuf());

	// BEGIN: Code being tested
	std::string expected = func(array);
	// END:   Code being tested

	// Use the string value of buffer to compare against expected output
	std::string text = buffer.str();
	int result = text.compare(expected);
	// Restore original buffer before exiting
	std::cout.rdbuf(prevcoutbuf);
	if (result != 0) {
		printTestName(test_name);
		std::cout << ": FAIL" << std::endl;
		std::cout << "expected: '" << expected << "\'" << std::endl;
		std::cout << "recived:  '" << text << "\'" << std::endl;
		return false;
	} else {
		printTestName(test_name);
		std::cout << ": PASS" << std::endl;
	}
	return true;
}
/////////////////////////////////////////////////////

TestFunc functions[NUM_FUNC] = {allocNoFree, allocandFree, allocandFreeMerge, testRealloc, testRealloc2, testWild, testSplitAndMerge, testCalloc};
std::string function_names[NUM_FUNC] = {"allocNoFree", "allocandFree", "allocandFreeMerge", "testRealloc", "testRealloc2", "testWild",
										"testSplitAndMerge", "testCalloc"};


void initTests() {
	memory_start_addr = getMemoryStart();
	max_test_name_len = function_names[0].length();
	for (int i = 0 ; i < NUM_FUNC ; ++i) {
		if (max_test_name_len < (int) function_names[i].length()) {
			max_test_name_len = function_names[i].length();
		}
	}
	max_test_name_len++;
	std::cout << "RUNNING TESTS: (MALLOC PART 3)" << std::endl;
	std::string header = "TEST NAME";
	std::string line = "";
	int offset = (max_test_name_len - (int) header.length()) / 2;
	header.insert (0, offset, ' ');
	line.insert(0,max_test_name_len + 9,'-');
	std::cout << line << std::endl;
	printTestName(header);
	std::cout << " STATUS" << std::endl;
	std::cout << line << std::endl;

}

int main() {
	void *allocations[MAX_ALLOC];
	initTests();
	int wait_status;
	for (int i = 0 ; i < NUM_FUNC ; ++i) {
		pid_t pid = fork();
		if (pid == 0) {
			checkFunc(functions[i], allocations, function_names[i]);
			exit(0);
		} else {
			wait(&wait_status);
			if (!WIFEXITED(wait_status) || (WEXITSTATUS(wait_status)) != 0) {
				printTestName(function_names[i]);
				std::cout << ": FAIL + CRASHED";
				if (WCOREDUMP (wait_status)) {
					std::cout << " (Core Dumped)";
				} else if (WIFSIGNALED(wait_status)) {
					std::cout << " Exit Signal:" << WTERMSIG(wait_status) << std::endl;
				}
				std::cout << std::endl;
			}
		}
	}

	return 0;
}