//
// Created by ariel on 25/06/2021.
//

#ifndef OS_WET4_PRINTMEMORYLIST_H
#define OS_WET4_PRINTMEMORYLIST_H

//void printMemory1(void* start);
//void printMemory2(void* start);
//void printMemory3(void* start);
//void printMemory4(void* start);

#include <iostream>

template<class T>
void printMemory(void *start,bool onlyList) {
	T *current = (T *) start;
	size_t size = 0;
	int blocks = 0;
	if(!onlyList)
		std::cout << "Printing Memory List\n";
	while (current) {
		if (current->is_free) {
			std::cout << "|F:" << current->size;
		} else {
			std::cout << "|U:" << current->size;
		}
		size += current->size;
		blocks++;
		current = current->next;
	}
	std::cout << "|"<< std::endl;
	if(!onlyList){
		std::cout << "Memory Info:\nNumber Of Blocks: " << blocks << "\nTotal Size (without Metadata): " << size << std::endl;
		std::cout << "Size of Metadata: " << sizeof(T) << std::endl;
	}
}


#endif //OS_WET4_PRINTMEMORYLIST_H
