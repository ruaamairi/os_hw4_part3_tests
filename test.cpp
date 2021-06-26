//
// Created by ariel on 25/06/2021.
//

#include <cstdlib>
#include <sstream>
#include <sys/wait.h>
#include "printMemoryList.h"
#include "malloc_3.h"
#include "colors.h"

/////////////////////////////////////////////////////

#define MAX_ALLOC 23

//if you see garbage when printing remove this line or comment it
#define USE_COLORS

// Copy your type here
// don't change anything from the one in malloc_3.c !!not even the order of args!!!
typedef struct MallocMetadata3 {
	size_t size;
	bool is_free;
	bool on_heap;
	MallocMetadata3 *next;
	MallocMetadata3 *prev;
} Metadata3;


///////////////////////////////////////////////////


#define NUM_FUNC 11

typedef std::string (*TestFunc)(void *[MAX_ALLOC]);

void *memory_start_addr;

int max_test_name_len;

int size_of_metadata;

int default_block_size;

std::string default_block;
std::string block_of_2;
std::string block_of_3;

#define DO_MALLOC(x) do{ \
        if(!(x)){                \
           std::cout << "Failed to allocate at line: "<< __LINE__ << ". command: "<< std::endl << #x << std::endl; \
           exit(1) ;\
        }                \
}while(0)

///////////////test functions/////////////////////
void freeAll(void *array[MAX_ALLOC]) {
	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		sfree(array[i]);
	}
}


std::string allocNoFree(void *array[MAX_ALLOC]) {
	std::string expected = "|U:100|U:100000|U:10|";
	if (MAX_ALLOC < 4) {
		std::cout << "Test Wont work with MAX_ALLOC < 4";
		return expected;
	}

	DO_MALLOC(array[0] = smalloc(100));
	DO_MALLOC(array[1] = smalloc(100000));
	DO_MALLOC(array[2] = smalloc(10));
	DO_MALLOC(array[3] = smalloc(11e6));


	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string allocandFree(void *array[MAX_ALLOC]) {

	std::string expected = "";
	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		if (i % 5 == 0) {
			expected += "|F:" + default_block;
		} else {
			expected += "|U:" + default_block;
		}
	}
	expected += "|";

	if (MAX_ALLOC < 6) {
		std::cout << "Test Wont work with MAX_ALLOC < 6";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		DO_MALLOC(array[i] = smalloc(default_block_size));
	}
	for (int i = 0 ; i < MAX_ALLOC ; i += 5) {
		sfree(array[i]);
	}

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string allocandFreeMerge(void *array[MAX_ALLOC]) {

	std::string expected = "";
	int j = 1;
	for (; j + 8 < MAX_ALLOC ; j += 10) {
		expected += "|U:" + default_block;
		expected += "|F:" + block_of_2;
		expected += "|U:" + default_block;
		expected += "|F:" + block_of_2;
		expected += "|U:" + default_block;
		expected += "|F:" + block_of_3;
	}
	for (; j - 1 < MAX_ALLOC ; ++j) {
		expected += "|U:" + default_block;
	}
	expected += "|";

	if (MAX_ALLOC < 10) {
		std::cout << "Test Wont work with MAX_ALLOC < 10";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		DO_MALLOC(array[i] = smalloc(default_block_size));
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

/**
 * test if the info was copied to the right place
 * @param array
 * @return
 */
std::string testRealloc(void *array[MAX_ALLOC]) {
	std::string expected = "aaaaaaaaaa|U:" + block_of_2;
	for (int i = 2 ; i < MAX_ALLOC ; ++i) {
		expected += "|U:" + default_block;
	}
	expected += "|";

	if (MAX_ALLOC < 2) {
		std::cout << "Test Wont work with MAX_ALLOC < 2";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		DO_MALLOC(array[i] = smalloc(default_block_size));
	}

	for (int i = 0 ; i < 10 ; ++i) {
		((char *) array[0])[i] = 'b';
		((char *) array[1])[i] = 'a';
	}
	sfree(array[0]);
	DO_MALLOC(array[1] = srealloc(array[1], default_block_size * 2));
	for (int i = 0 ; i < 10 ; ++i) {
		std::cout << ((char *) array[0])[i];
	}

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

/**
 * test if we used the small option for realloc
 * and if the info was copied to the right place
 * @param array
 * @return
 */
std::string testRealloc2(void *array[MAX_ALLOC]) {
	std::string expected = "";
	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		if (i == 3 || i == 5 || i == 7 || i == 9) {
			expected += "|F:" + default_block;
		} else {
			expected += "|U:" + default_block;
		}
	}
	expected += "|F:" + std::to_string(default_block_size * 3) + "|U:" + std::to_string(default_block_size * 4) + "|U:" +
				std::to_string(default_block_size * 2) + "|";

	if (MAX_ALLOC < 10) {
		std::cout << "Test Wont work with MAX_ALLOC < 10";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		DO_MALLOC(array[i] = smalloc(default_block_size));
	}

	sfree(array[5]);
	DO_MALLOC(array[5] = smalloc(default_block_size * 3));
	sfree(array[9]);
	DO_MALLOC(array[9] = smalloc(default_block_size * 4));
	sfree(array[7]);
	DO_MALLOC(array[7] = smalloc(default_block_size * 2));

	for (int i = 0 ; i < default_block_size ; ++i) {
		((char *) array[3])[i] = 'b';
		((char *) array[5])[i] = 'a';
		((char *) array[7])[i] = 'd';
	}
	sfree(array[5]);
	sfree(array[7]);

	DO_MALLOC(array[3] = srealloc(array[3], default_block_size * 2));
	for (int i = 0 ; i < default_block_size ; ++i) {
		if (((char *) array[7])[i] != 'b') {
			std::cout << "realloc didnt copy the char b to index " << i << std::endl;
			break;
		}
	}

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testWild(void *array[MAX_ALLOC]) {
	std::string expected = "";
	for (int i = 0 ; i < MAX_ALLOC - 10 ; ++i) {
		expected += "|U:" + default_block;
	}
	expected += "|U:" + std::to_string(default_block_size * 3);
	expected += "|U:" + std::to_string(default_block_size * 2) + "|";

	if (MAX_ALLOC < 10) {
		std::cout << "Test Wont work with MAX_ALLOC < 10";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC - 9 ; ++i) {
		DO_MALLOC(array[i] = smalloc(default_block_size));
	}

	sfree(array[MAX_ALLOC - 10]);
	DO_MALLOC(array[MAX_ALLOC - 10] = smalloc(default_block_size * 3));
	DO_MALLOC(array[MAX_ALLOC - 9] = smalloc(default_block_size));
	DO_MALLOC(array[MAX_ALLOC - 9] = srealloc(array[MAX_ALLOC - 9], default_block_size * 2));

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testSplitAndMerge(void *array[MAX_ALLOC]) {
	int small_part_of_block = default_block_size / 3;
	std::string expected =
			"|U:" + std::to_string(small_part_of_block) + "|F:" + std::to_string(default_block_size - small_part_of_block - size_of_metadata);
	expected += "|U:" + default_block;
	expected += "|U:" + default_block;
	expected += "|U:" + std::to_string(small_part_of_block + default_block_size);
	expected += "|U:" + std::to_string(small_part_of_block + default_block_size);
	expected += "|F:" + std::to_string(default_block_size - 2 * small_part_of_block);
	expected += "|F:" + default_block;
	expected += "|U:" + default_block;
	expected += "|U:" + default_block;
	expected += "|U:" + std::to_string(small_part_of_block + default_block_size);
	expected += "|F:" + std::to_string(default_block_size - small_part_of_block);
	expected += "|U:" + default_block;
	expected += "|U:" + std::to_string(small_part_of_block + default_block_size);
	expected += "|F:" + std::to_string(default_block_size - small_part_of_block);
	for (int i = 14 ; i < MAX_ALLOC ; ++i) {
		expected += "|U:" + default_block;
	}
	expected += "|";;


	if (MAX_ALLOC < 14) {
		std::cout << "Test Wont work with MAX_ALLOC < 14";
		return expected;
	}

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		DO_MALLOC(array[i] = smalloc(default_block_size));
	}

	sfree(array[0]);
	DO_MALLOC(smalloc(default_block_size / 3));
	sfree(array[3]);
	sfree(array[6]);
	DO_MALLOC(srealloc(array[4], default_block_size + default_block_size / 3));
	DO_MALLOC(srealloc(array[5], default_block_size + default_block_size / 3));

	sfree(array[10]);
	DO_MALLOC(srealloc(array[9], default_block_size + default_block_size / 3));

	sfree(array[12]);
	DO_MALLOC(srealloc(array[13], default_block_size + default_block_size / 3));


	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testCalloc(void *array[MAX_ALLOC]) {
	std::string expected = "|U:100|U:20|U:100|";
	if (MAX_ALLOC < 4) {
		std::cout << "Test Wont work with MAX_ALLOC < 4";
		return expected;
	}

	DO_MALLOC(array[0] = scalloc(1, 100));
	for (int i = 0 ; i < 100 ; ++i) {
		if (((char *) array[0])[i] != 0) {
			std::cout << "array[0] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}
	DO_MALLOC(array[1] = scalloc(2, 10));
	for (int i = 0 ; i < 20 ; ++i) {
		if (((char *) array[1])[i] != 0) {
			std::cout << "array[1] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}
	DO_MALLOC(array[2] = scalloc(10, 10));
	for (int i = 0 ; i < 100 ; ++i) {
		if (((char *) array[2])[i] != 0) {
			std::cout << "array[2] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}
	DO_MALLOC(array[3] = scalloc(100, 10000));
	for (int i = 0 ; i < 1000000 ; ++i) {
		if (((char *) array[3])[i] != 0) {
			std::cout << "array[3] isn't all 0 (first bad index:" << i << ")\n";
			break;
		}
	}


	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testFreeAllAndMerge(void *array[MAX_ALLOC]) {
	if (MAX_ALLOC > 10e8) {
		std::cout << "Test Wont work with MAX_ALLOC > 10e8";
		return "";
	}
	int allsize = 0;
	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		DO_MALLOC(array[i] = smalloc(i + 1));
		allsize += i + 1 + (int) sizeof(Metadata3);
	}
	allsize -= (int) sizeof(Metadata3);
	std::string expected = "|F:" + std::to_string(allsize) + "|";

	for (int i = 0 ; i < MAX_ALLOC ; ++i) {
		sfree(array[i]);
	}

	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testInit(void *array[MAX_ALLOC]) {
	std::string expected = "|F:1||U:2|";
	printMemory<Metadata3>(memory_start_addr, true);
	DO_MALLOC(array[0] = smalloc(2));
	printMemory<Metadata3>(memory_start_addr, true);
	return expected;
}

std::string testBadArgs(void *array[MAX_ALLOC]) {
	std::string expected = "";
	size_t options[3] = {static_cast<size_t>(-1), 0, static_cast<size_t>(10e8 + 1)};
	DO_MALLOC(array[9] = smalloc(1));
	for (size_t option : options) {
		array[0] = smalloc(option);  //need to fail
		array[1] = scalloc(option, 1); //need to fail
		array[2] = scalloc(1, option); //need to fail
		array[3] = srealloc(array[9], option); //need to fail
		if (array[0] || array[1] || array[2] || array[3]) {
			std::cout << "missed edge case: " << std::to_string(option) << std::endl;
		}
	}

	return expected;
}


/////////////////////////////////////////////////////

#ifdef USE_COLORS
#define PRED(x) FRED(x)
#define PGRN(x) FGRN(x)
#endif
#ifndef USE_COLORS
#define PRED(x) x
#define PGRN(x) x
#endif

void *getMemoryStart() {
	void *first = smalloc(1);
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
		std::cout << ": " << PRED("FAIL") << std::endl;
		std::cout << "expected: '" << expected << "\'" << std::endl;
		std::cout << "recived:  '" << text << "\'" << std::endl;
		std::cout.flush();
		return false;
	} else {
		printTestName(test_name);
		std::cout << ": " << PGRN("PASS") << std::endl;
	}
	std::cout.flush();
	return true;
}
/////////////////////////////////////////////////////

TestFunc functions[NUM_FUNC] = {testInit, allocNoFree, allocandFree, testFreeAllAndMerge, allocandFreeMerge, testRealloc, testRealloc2, testWild,
								testSplitAndMerge, testCalloc, testBadArgs};
std::string function_names[NUM_FUNC] = {"testInit", "allocNoFree", "allocandFree", "testFreeAllAndMerge", "allocandFreeMerge", "testRealloc",
										"testRealloc2",
										"testWild",
										"testSplitAndMerge", "testCalloc", "testBadArgs"};


void initTests() {
	DO_MALLOC(memory_start_addr = getMemoryStart());
	size_of_metadata = sizeof(Metadata3);
	default_block_size = 4 * (size_of_metadata + (4 * 128)); // big enough to split a lot
	if (default_block_size * 3 + size_of_metadata * 2 >= 128 * 1024) {
		default_block_size /= 2;
		std::cerr << "Metadata may be to big for some of the tests" << std::endl;
	}

	default_block = std::to_string(default_block_size);
	block_of_2 = std::to_string(default_block_size * 2 + size_of_metadata);
	block_of_3 = std::to_string(default_block_size * 3 + size_of_metadata * 2);

	max_test_name_len = function_names[0].length();
	for (int i = 0 ; i < NUM_FUNC ; ++i) {
		if (max_test_name_len < (int) function_names[i].length()) {
			max_test_name_len = function_names[i].length();
		}
	}
	max_test_name_len++;
}

void printInitFail() {
	std::cerr << "Init Failed , ignore all other tests" << std::endl;
	std::cerr << "The test get the start of the memory list using an allocation of size 1 and free it right after" << std::endl;
	std::cerr << "If this failed you didnt increase it to allocate the next one (Wilderness)" << std::endl;
	std::cerr.flush();
}

void printDebugInfo(){
	std::cout << "Info For Debuging:" << std::endl << "Your Metadata size is: " << size_of_metadata << std::endl;
	std::cout << "Default block size for tests is: " << default_block_size << std::endl;
	std::cout << "Default 2 block after merge size is: " << default_block_size *2 +size_of_metadata << std::endl;
	std::cout << "Default 3 block after merge size is: " << default_block_size *3 +size_of_metadata*2 << std::endl << std::endl;
}

void printStartRunningTests(){
	std::cout << "RUNNING TESTS: (MALLOC PART 3)" << std::endl;
	std::string header = "TEST NAME";
	std::string line = "";
	int offset = (max_test_name_len - (int) header.length()) / 2;
	header.insert(0, offset, ' ');
	line.insert(0, max_test_name_len + 9, '-');
	std::cout << line << std::endl;
	printTestName(header);
	std::cout << " STATUS" << std::endl;
	std::cout << line << std::endl;
}

int main() {
	void *allocations[MAX_ALLOC];
	initTests();

	printDebugInfo();
	std::cout.flush();
	printStartRunningTests();
	int wait_status;
	bool ans;
	for (int i = 0 ; i < NUM_FUNC ; ++i) {
		pid_t pid = fork();
		if (pid == 0) {
			ans = checkFunc(functions[i], allocations, function_names[i]);
			if (i == 0 && !ans) {
				printInitFail();
			}
			exit(0);
		} else {
			wait(&wait_status);
			if (!WIFEXITED(wait_status) || (WEXITSTATUS(wait_status)) != 0) {
				printTestName(function_names[i]);
				std::cout << ": " << PRED("FAIL + CRASHED");
				if (WCOREDUMP (wait_status)) {
					std::cout << PRED(" (Core Dumped)");
				} else if (WIFSIGNALED(wait_status)) {
					std::cout << " Exit Signal:" << WTERMSIG(wait_status) << std::endl;
				}
				std::cout << std::endl;
			}
		}
	}

	return 0;
}