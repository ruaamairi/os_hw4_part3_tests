
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <sys/mman.h>
#include "malloc_2.h"
///////////////////////////////////////////////////////////////////////////////
///                                 DEFINES
///////////////////////////////////////////////////////////////////////////////
#define  BIT_SIZE 128
class MallocMetadata;
class SortedList;
const size_t MAX_ALLOCATED_BYTES = 100000000;
const size_t MIN_SPACE_MMAP = 128 * 1024;
///////////////////////////////////////////////////////////////////////////////
///                                 LIST
///////////////////////////////////////////////////////////////////////////////

static MallocMetadata* head = nullptr;
static MallocMetadata* mmap_head = nullptr;
SortedList* hist[BIT_SIZE]={nullptr};
size_t free_blocks = 0;

class MallocMetadata {
public:
    size_t size;
    bool is_free;
    MallocMetadata* previous;
    MallocMetadata* next;
    MallocMetadata* prev_sorted;
    MallocMetadata* next_sorted;
    static void updateMetaData(MallocMetadata* lower, MallocMetadata* current, MallocMetadata* upper, size_t size, bool is_free,MallocMetadata* prev,MallocMetadata* next){
        if (not current){
            return;
        }
        current->previous = lower;
        current->next = upper;
        current->size = size;
        current->is_free = is_free;
        current->prev_sorted=prev;
        current->next_sorted=next;
    }

    static  MallocMetadata* getFirstFreeSector(size_t size){
        MallocMetadata* iter = head;
        while (iter){
            if(iter->isFree() && iter->getSize() >= size){
                return iter;
            }
            iter = iter->getNext();
        }
        return nullptr;
    }

    static MallocMetadata* getLast(){
        MallocMetadata* iter = head;
        if(not iter){
            return nullptr;
        } else {
            while (iter->next){
                iter = iter->getNext();
            }
            return iter;
        }
    }

    static void FreeFromMap(MallocMetadata *curr){
        if(curr->previous){
            curr->previous->next = curr->next;
        }
        if(curr->next){
            curr->next->previous = curr->previous;
        }
        if(curr == mmap_head){
            assert(not mmap_head->previous);
            mmap_head = curr->next;
        }
        munmap(curr,curr->size + sizeof(MallocMetadata));

    }
   /* static void* Challenge0(MallocMetadata* p){
        auto iter = head;
        while(iter){
            if(iter->isFree()){
                freed_bytes += iter->getSize();
            }
            iter = iter->getNext();
        }
        for(int ){
            for(int )
        }

    }*/
    /**
     * turns free block given into two blocks, onw with size given and the other one with
     * the ~rest~ of the size, only if the remaining size is bigger than 128
     * @param freed_sector, current block
     * @param size, size wanted from sector
     * @return pointer to the bottom sector(used)
     */
    static void* Challenge1(MallocMetadata* freed_sector , size_t size){
        assert(freed_sector);
        if((long int)freed_sector->size - (long int)sizeof(MallocMetadata) - (long int)size < 128){
            freed_sector->is_free = false;
            return (static_cast<unsigned char*>(static_cast<void*>(freed_sector)) + sizeof(MallocMetadata));
        }else{
            auto new_sector = reinterpret_cast<MallocMetadata*>(
                    static_cast<unsigned char*>(static_cast<void*>(freed_sector)) + sizeof(MallocMetadata) + size);
            new_sector->size = freed_sector->size - sizeof(MallocMetadata) - size;
            new_sector->is_free = true;
            new_sector->previous = freed_sector;
            new_sector->next =freed_sector->next;
            freed_sector->size = size;
            freed_sector->next = new_sector;
            freed_sector->is_free = false;
            return (static_cast<unsigned char*>(static_cast<void*>(freed_sector)) + sizeof(MallocMetadata));
        }
    }

    static void Challenge2(MallocMetadata* p)
    {
        assert(p);
        if(not p->is_free){
            p->is_free = true;
            if(p->next and p->next->is_free){
                p->size += p->next->size + sizeof(MallocMetadata);
                p->next = p->next->next;
                if(p->next){
                    p->next->previous = p;
                }
            }
            if(p->previous and p->previous->is_free){
                p->previous->size += p->size + sizeof(MallocMetadata);
                p->previous->next = p->next;
                if(p->next){
                    p->next->previous = p->previous;
                }
            }
        }
    }

    /**
     * tries to find a section that is both free and fits size
     * @param size, the size you want
     * @return null if no section exists else the pointer to the meta data
     * of that sector.
     */
    static MallocMetadata* Challenge3(size_t size){
        MallocMetadata* open_sector = MallocMetadata::getFirstFreeSector(size);
        if(open_sector){
            return open_sector;
        } else {
            MallocMetadata *last = MallocMetadata::getLast();
            if(not last or not last->is_free){
                return nullptr;
            }else{ ///if the last is free
                assert(size > last->size);
                void *ptr = sbrk(size - last->size);
                if (ptr == (void *) (-1)) {
                    return nullptr;
                }else{
                    last->size = size;
                    return last;
                }
            }
        }
    }

    static void* Challenge4 (size_t size){
        void *result = mmap(nullptr, size + sizeof(MallocMetadata), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON,-1, 0);
        if(result == (void*)(-1)){
            return nullptr;
        }else{
            if(not mmap_head){///we have to add it in the head (first meta data)
                MallocMetadata* last= MallocMetadata::getLast();
                mmap_head = static_cast<MallocMetadata*>(result);
                MallocMetadata::updateMetaData(nullptr,mmap_head, nullptr,size, false,last, nullptr);
                return (static_cast<unsigned char*>(result) + sizeof(MallocMetadata));
            }else{///adding it in the last
                auto new_first =  static_cast<MallocMetadata*>(result);
                MallocMetadata::updateMetaData(nullptr,new_first, mmap_head,size, false, nullptr, nullptr);
                mmap_head->previous = new_first;
                mmap_head = new_first;
                return (static_cast<unsigned char*>(result) + sizeof(MallocMetadata));
            }
        }
    }

    static void* MergeWithNext(MallocMetadata* curr, size_t size){
        assert(curr);
        if(curr->next)
        {
            curr->size += curr->next->size + sizeof(MallocMetadata);
            curr->next = curr->next->next;
            if(curr->next){
                curr->next->previous = curr;
            }
        }
        else{
            void * ptr = sbrk(size - curr->size); ///This is the last node we allocate above it
             if(ptr == (void*)(-1)){
                return nullptr;
            }
            size_t  tmp = size - curr->size;
            curr->size += tmp;
            return (static_cast<unsigned char*>(static_cast<void*>(curr)) + sizeof(MallocMetadata));;
        }
        return Challenge1(curr,size);
    }

    static void* MergeWithPrev(MallocMetadata* curr,size_t size){
        assert(curr && curr->previous);
        curr->previous->size += curr->size + sizeof(MallocMetadata);
        curr->previous->next = curr->next;
        if(curr->previous->next){
            curr->next->previous = curr->previous;
        }
        return Challenge1(curr->previous,size);
    }

    static void* MergeBoth(MallocMetadata* curr, size_t size){
        assert(curr->previous && curr && curr->next);
        curr->previous->size += curr->size + curr->next->size + 2*sizeof(MallocMetadata);
        curr->previous->next = curr->next->next;
        if(curr->previous->next){
           curr->next->next->previous = curr->previous;
        }
        return Challenge1(curr->previous,size);
    }
    MallocMetadata* getPrev(){
        return previous;
    }

    MallocMetadata* getNext(){
        return next;
    }

    size_t getSize(){
        return size;
    }
    bool isFree(){
        return is_free;
    }

    void setFree(bool free){
        is_free = free;
    }

};

class SortedList{
public:
    size_t size;
    MallocMetadata* first;
    MallocMetadata* last;
    SortedList():first(nullptr),last(nullptr),size(0){}
    ~SortedList()= default;
    void insertBlock(MallocMetadata* freeBlock){
        if(first==nullptr){
            first=freeBlock;
            last=freeBlock;
            return;
        }
       MallocMetadata* ptr=first;
       while(ptr)
       {
           if(ptr->getSize()<freeBlock->getSize()){
               break;
           }
           ptr=ptr->next_sorted;
       }
        if(ptr==nullptr){/*insert last*/
            last->next_sorted=freeBlock;
            freeBlock->next_sorted=nullptr;
            freeBlock->prev_sorted=last;
            last=freeBlock;
            return;
        }
       if(ptr->getPrev()==nullptr){/*insert first*/
           freeBlock->next_sorted=ptr;
           ptr->prev_sorted=freeBlock;
           first=freeBlock;
           return;
       }
       MallocMetadata* prev=ptr->prev_sorted;
       prev->next_sorted=freeBlock;
       freeBlock->next_sorted=ptr;
       ptr->prev_sorted=freeBlock;
       freeBlock->prev_sorted=prev;
   }
    void remove(MallocMetadata* freeBlock){
        if(first==nullptr){
            return;
        }
        MallocMetadata* ptr=first;
        while(ptr)
        {
            if(ptr->getSize()==freeBlock->getSize()){
                break;
            }
            ptr=ptr->getNext();
        }
        if(ptr==nullptr){
            return;
        }
        if(ptr->getPrev()==nullptr && ptr->getNext()==nullptr){
            first= nullptr;
            last= nullptr;
            return;
        }
        if(ptr->getPrev()==nullptr){
            first=first->next_sorted;
            first->prev_sorted= nullptr;
            return;
        }
        if(ptr->getNext()==nullptr){
            last=last->prev_sorted;
            last->next_sorted= nullptr;
            return;
        }
        MallocMetadata* prev=ptr->prev_sorted;
        prev->next_sorted=ptr->next_sorted;
        freeBlock->next_sorted= nullptr;
        ptr->prev_sorted=prev;
        freeBlock->prev_sorted= nullptr;
    }

};

///////////////////////////////////////////////////////////////////////////////
///                                 LIBRARY
///////////////////////////////////////////////////////////////////////////////
void* smalloc(size_t size)
{
    if(size <= 0 or size > MAX_ALLOCATED_BYTES){
        return nullptr;
    }else if(size > MIN_SPACE_MMAP){
        return MallocMetadata::Challenge4(size);
    }else{
        MallocMetadata* open_sector = MallocMetadata::Challenge3(size);
        if(not open_sector)
        {     /// There is no freed meta data that fits the size
            void * ptr = sbrk(size + sizeof(MallocMetadata));
            if(ptr == (void*)(-1))
            {
                return nullptr;
            }
            else if(not head)
            { ///we have to add it in the head (first meta data)
                head = static_cast<MallocMetadata *>(ptr);
                MallocMetadata::updateMetaData(nullptr,head, nullptr,size, false, nullptr, nullptr);
                return (static_cast<unsigned char*>(ptr) + sizeof(MallocMetadata));
            }
            else{              ///adding it in the last
                auto curr = static_cast<MallocMetadata *>(ptr);
                auto last_in_list =  MallocMetadata::getLast();
                MallocMetadata::updateMetaData(last_in_list->getPrev(),last_in_list,curr,last_in_list->getSize(),last_in_list->isFree(),last_in_list->prev_sorted,curr);
                MallocMetadata::updateMetaData(last_in_list,curr, nullptr,size, false,last_in_list, nullptr);
                return (static_cast<unsigned char*>(ptr) + sizeof(MallocMetadata));
            }
        }
        else
        {  ///we have found a freed meta data that fit the size
            return MallocMetadata::Challenge1(open_sector,size);
        }
    }

}

void* scalloc(size_t num, size_t size){
    auto ptr = smalloc(size*num);
    if(not ptr){
        return nullptr;
    }else{
        std::memset(ptr,0,size*num);
        return ptr;
    }
}

void sfree(void* p){
    if(p)
    {
        auto curr = reinterpret_cast<MallocMetadata *>(static_cast<unsigned char *>(p) - sizeof(MallocMetadata));
        if(curr->getSize() < MIN_SPACE_MMAP){
            MallocMetadata::Challenge2(curr);
            size_t block_size = curr->getSize();
            free_blocks+=block_size;
            if(hist[block_size/1000] == nullptr){
                hist[block_size/1000] = new SortedList();
            }
            hist[block_size/1000]->insertBlock(curr);
        }else{
            MallocMetadata::FreeFromMap(curr);
        }
    }
}

void* srealloc(void* oldp, size_t size) {
    if (not oldp) 
    {
        return smalloc(size);
    }
    auto curr = reinterpret_cast<MallocMetadata *>(static_cast<unsigned char *>(oldp) - sizeof(MallocMetadata));
    if(size < MIN_SPACE_MMAP)
    {
        if(size == 0){
            return nullptr;
        }
        if (curr->getSize() >= size)
        { ///size is perfect trust me no need to check
            return oldp;
        } 
        else if (curr->getPrev() and curr->getPrev()->isFree() and
                  (curr->getSize() + curr->getPrev()->getSize() + sizeof(MallocMetadata)) >= size)
        {
            return MallocMetadata::MergeWithPrev(curr,size);
        } 
        else if ((not curr->getNext()) or ((curr->getNext() and curr->getNext()->isFree()) and
                ((curr->getSize() + curr->getNext()->getSize() + sizeof(MallocMetadata)) >= size))){
            return MallocMetadata::MergeWithNext(curr,size);
        }
        else if((curr->getNext() and curr->getNext()->isFree()) and (curr->getPrev() and curr->getPrev()->isFree())
        and (curr->getSize() + curr->getPrev()->getSize() + curr->getNext()->getSize() + 2* sizeof(MallocMetadata)) >= size){
            return MallocMetadata::MergeBoth(curr,size);
        }else 
        {
            curr->setFree(true);
            auto ptr = smalloc(size);
            if (not ptr){
                curr->setFree(false);
                return nullptr;
            }else{
                curr->setFree(false);
                std::memcpy(ptr,oldp,(curr->getSize() < size ? curr->getSize() : size));
                if(oldp == ptr){
                    return ptr;
                }else{
                    sfree(oldp);
                    return ptr;
                }
            }
        }
    }
    else
    { /// mmap realloc
        auto ptr =  smalloc(size);
        if(not ptr){
            return nullptr;
        }else{
            std::memcpy(ptr,oldp,(curr->getSize() < size ? curr->getSize() : size));
            sfree(oldp);
            return ptr;
        }
    }
}

size_t _num_free_blocks(){
    size_t freed_blocks = 0;
    if(not head){
        return freed_blocks;
    } else {
        auto iter = head;
        while(iter){
            freed_blocks += iter->isFree();
            iter = iter->getNext();
        }
        return freed_blocks;
    }
}

size_t _num_free_bytes(){
    size_t freed_bytes = 0;
    if(not head){
        return freed_bytes;
    }else{
        auto iter = head;
        while(iter){
            if(iter->isFree()){
                freed_bytes += iter->getSize();
            }
            iter = iter->getNext();
        }
        return freed_bytes;
    }
}

size_t _num_allocated_blocks(){
    size_t allocated_blocks = 0;
    if(not head)
    {
        if(not mmap_head)
        {
            return allocated_blocks;
        }
        else
        {
            auto iter = mmap_head;
            while(iter)
            {
                allocated_blocks++;
                iter = iter->getNext();
            }
            return allocated_blocks;
        }
    }

    else
    {
        auto iter = head;
        while(iter)
        {
            allocated_blocks++;
            iter = iter->getNext();
        }
        if(not mmap_head)
        {
            return allocated_blocks;
        }
        else
        {
            iter = mmap_head;
            while(iter)
            {
                allocated_blocks++;
                iter = iter->getNext();
            }
            return allocated_blocks;
        }
    }
}

size_t _num_allocated_bytes(){
    size_t allocated_bytes = 0;
    if(not head)
    {
        if(not mmap_head)
        {
            return allocated_bytes;
        }
        auto iter = mmap_head;
        while(iter)
        {
            allocated_bytes += iter->getSize();
            iter = iter->getNext();
        }
        return allocated_bytes;
    }
    else
    {
        auto iter = head;
        while(iter)
        {
            allocated_bytes += iter->getSize();
            iter = iter->getNext();
        }
        if(not mmap_head)
        {
            return allocated_bytes;
        }
        else
        {
            iter = mmap_head;
            while(iter)
            {
                allocated_bytes += iter->getSize();
                iter = iter->getNext();
            }
            return allocated_bytes;
        }
    }
}

size_t _num_meta_data_bytes(){
    return _num_allocated_blocks()*(sizeof(MallocMetadata));
}

size_t _size_meta_data(){
    return sizeof(MallocMetadata);
}




