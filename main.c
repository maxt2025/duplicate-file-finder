#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <stdint.h>
#include <sys/stat.h>
typedef struct{
    char *path;
    size_t size;
} FileInfo;
typedef struct{
    FileInfo *allFiles;
    int capacity;
    int count;
} FileList;
typedef struct Node{
    char *path;
    size_t size;
    uint64_t hash;
    struct Node *next;
} Node;
bool addFile(FileList *list, char *path, size_t size){
    if(list->count==list->capacity){
        int newCapacity=(list->capacity)*2;
        FileInfo *tmp=realloc(list->allFiles, newCapacity*sizeof(FileInfo));
        if(tmp==NULL) return false;
        list->capacity=newCapacity;
        list->allFiles=tmp;
    }
    list->allFiles[list->count].size=size;
    size_t pathLen=strlen(path);
    list->allFiles[list->count].path=malloc(pathLen+1);
    strcpy(list->allFiles[list->count].path, path);
    (list->count)++;
    return true;
}
bool scanDirectory(const char *path, FileList *list){
    DIR *dir=opendir(path);
    if(dir==NULL){
        perror("open");
        return false;
    }
    struct dirent *entry;
    while((entry=readdir(dir))!=NULL){
        if(strcmp(entry->d_name, ".")==0 || strcmp(entry->d_name, "..")==0) continue;
        struct stat info;
        size_t pathLen=strlen(path);
        size_t nameLen=strlen(entry->d_name);
        char fullPath[pathLen+nameLen+2];
        strcpy(fullPath, path);
        strcpy(fullPath+pathLen, "/");
        strcpy(fullPath+pathLen+1, entry->d_name);
        if(lstat(fullPath, &info)==-1) continue;

        if(S_ISDIR(info.st_mode)){
            if(!scanDirectory(fullPath, list)){
                return false;
            }
        } else if(S_ISREG(info.st_mode)){
            if(!addFile(list, fullPath, info.st_size)){
                closedir(dir);
                return false;
            }
            
        }
    }
    closedir(dir);
    return true;
}
int compare(const void *a, const void *b){
    FileInfo *first=(FileInfo *)a;
    FileInfo *second=(FileInfo *)b;
    if(first->size<second->size){
        return -1;
    } else if(first->size>second->size){
        return 1;
    }
    return 0;
}
uint64_t hashFile(const char *path){
    FILE *file=fopen(path, "rb");
    if(file==NULL){
        perror(path);
        return 0;
    }

    uint64_t hash=14695981039346656037ULL;
    unsigned char buffer[8192];
    size_t bytesRead;
    while((bytesRead=fread(buffer, 1, sizeof(buffer), file))>0){
        for(size_t i=0; i<bytesRead; i++){
            hash^=buffer[i];
            hash*=1099511628211ULL;
        }
    }
    fclose(file);
    return hash;
    
}
void addFilesToHashTable(Node **hashTable, int tableSize, FileList list){
    int i=0;
    int k=1;
    while(k<list.count){
        if(list.allFiles[k].size!=list.allFiles[i].size){
            i=k;
            k++;
        } else{
            uint64_t hash=hashFile(list.allFiles[k].path);
            int hashIndex=hash%tableSize;
            Node *newNode=malloc(sizeof(Node));
            newNode->hash=hash;
            newNode->size=list.allFiles[k].size;
            size_t lenOfPath=strlen(list.allFiles[k].path);
            newNode->path=malloc(lenOfPath+1);
            strcpy(newNode->path, list.allFiles[k].path);
            newNode->next=hashTable[hashIndex];
            hashTable[hashIndex]=newNode;
            if(i+1==k){
                hash=hashFile(list.allFiles[k-1].path);
                hashIndex=hash%tableSize;
                Node *newNode2=malloc(sizeof(Node));
                newNode2->hash=hash;
                newNode2->size=list.allFiles[k-1].size;
                lenOfPath=strlen(list.allFiles[k-1].path);
                newNode2->path=malloc(lenOfPath+1);
                strcpy(newNode2->path, list.allFiles[k-1].path);
                newNode2->next=hashTable[hashIndex];
                hashTable[hashIndex]=newNode2;
            }
            k++;
        }
    }
}
bool areSame(Node *curFile, Node *fileFromGroup){
    if(curFile->hash!=fileFromGroup->hash || curFile->size!=fileFromGroup->size) return false;
    FILE *file1=fopen(curFile->path, "rb");
    FILE *file2=fopen(fileFromGroup->path, "rb");

    if(file1==NULL || file2==NULL){
        if(file1!=NULL){
            fclose(file1);
        }
        if(file2!=NULL){
            fclose(file2);
        }
        return false;
    }

    unsigned char buffer1[8192];
    unsigned char buffer2[8192];

    while(1){
        size_t read1=fread(buffer1, 1, sizeof(buffer1), file1);
        size_t read2=fread(buffer2, 1, sizeof(buffer2), file2);
        if(read1!=read2){
            fclose(file1);
            fclose(file2);
            return false;
        }
        if(memcmp(buffer1, buffer2, read1)!=0){
            fclose(file1);
            fclose(file2);
            return false;
        }
        if(read1<sizeof(buffer1)) break;
    }

    fclose(file1);
    fclose(file2);
    return true;
}
void freeLinkedList(Node *head){
    while(head!=NULL){
        Node *prev=head;
        head=head->next;
        free(prev->path);
        free(prev);
    }
}
Node **findSameFiles(Node *first, int *count){
    Node *cur=first;
    int tmpGroupsSize=10;
    Node **tmpGroups=malloc(tmpGroupsSize*sizeof(Node *));
    int tmpGroupsCount=0;
    while(cur!=NULL){
        bool found=false;
        for(int i=0; i<tmpGroupsCount; i++){
            if(areSame(cur, tmpGroups[i])){
                Node *newNode=malloc(sizeof(Node));
                newNode->size=cur->size;
                newNode->hash=cur->hash;
                size_t lenOfPath=strlen(cur->path);
                newNode->path=malloc(lenOfPath+1);
                strcpy(newNode->path, cur->path);
                newNode->next=tmpGroups[i];
                tmpGroups[i]=newNode;
                found=true;
                break;
            }
        }

        if(!found){
            if(tmpGroupsSize==tmpGroupsCount){
                tmpGroupsSize*=2;
                Node **reSize=realloc(tmpGroups, tmpGroupsSize*sizeof(Node *));
                if(reSize==NULL){
                    for(int i=0; i<tmpGroupsCount; i++){
                        freeLinkedList(tmpGroups[i]);
                    }
                    free(tmpGroups);
                    *count=0;
                    return NULL;
                }
                tmpGroups=reSize;
            }
            Node *newNode=malloc(sizeof(Node));
            newNode->size=cur->size;
            newNode->hash=cur->hash;
            size_t lenOfPath=strlen(cur->path);
            newNode->path=malloc(lenOfPath+1);
            strcpy(newNode->path, cur->path);
            newNode->next=NULL;
            tmpGroups[tmpGroupsCount]=newNode;
            tmpGroupsCount++;
        }
        cur=cur->next;
    }
    *count=tmpGroupsCount;
    return tmpGroups;
}
void freeTmpGroups(Node **groups, int count){
    for(int i=0; i<count; i++){
        if(groups[i][0].next==NULL){
            free(groups[i]->path);
            free(groups[i]);
        }   
    }
    free(groups);
}
void printSizeOfFile(size_t size){
    if(size<1024){
        printf("%zu B\n", size);
        printf("\n");
        return;
    }   
    size_t limit=1024*1024;
    double result=size/1024.0;
    if(size<limit){
        printf("%.2lf KB\n", result);
        printf("\n");
        return;
    }
    limit*=1024;
    result/=1024.0;
    if(size<limit){
        printf("%.2lf MB\n", result);
        printf("\n");
        return;
    }
    limit*=1024;
    result/=1024.0;
    if(size<limit){
        printf("%.2lf GB\n", result);
        printf("\n");
        return;
    }
    result/=1024.0;
    printf("%.2lf TB\n", result);
    printf("\n");
}
void printResult(Node **allGroups, int count){
    size_t totalSize=0;
    if(count==0){
        printf("No duplicates.\n");
        return;
    }
    for(int i=0; i<count; i++){
        size_t sizeInOneGroup=0;
        printf("Group #%d:\n", i+1);
        printf("Size: ");
        printSizeOfFile(allGroups[i]->size);
        Node *cur=allGroups[i];
        while(cur!=NULL){
            printf("%s\n", cur->path);
            if(cur!=allGroups[i]){
                sizeInOneGroup+=cur->size;
            }
            cur=cur->next;
        }
        printf("Potential saving: ");
        printSizeOfFile(sizeInOneGroup);
        totalSize+=sizeInOneGroup;
    }
    printf("Total duplicate data: ");
    printSizeOfFile(totalSize);
}

void freeAll(FileList *list, Node **hashTable, Node **allGroups, int countOfGroups){
    for(int i=0; i<list->count; i++){
        free(list->allFiles[i].path);
    }
    free(list->allFiles);

    for(int i=0; i<2000; i++){
        freeLinkedList(hashTable[i]);
    }
    free(hashTable);

    for(int i=0; i<countOfGroups; i++){
        freeLinkedList(allGroups[i]);
    }
    free(allGroups);
}

int main(int argc, char *argv[]){
    if(argc<2){
        printf("You did not enter any directory.\n");
        return EXIT_FAILURE;
    }
    FileList list;
    list.capacity=50;
    list.allFiles=malloc(list.capacity*sizeof(FileInfo));
    list.count=0;
    char *home=getenv("HOME");

    for(int i=1; i<argc; i++){
        if(home==NULL){
            printf("Home is not set.\n");
            free(list.allFiles);
            return EXIT_FAILURE;
        }
        char documents[1000];
        char downloads[1000];
        char pictures[1000];
        snprintf(documents, sizeof(documents), "%s/Documents", home);
        snprintf(downloads, sizeof(downloads), "%s/Downloads", home);
        snprintf(pictures, sizeof(pictures), "%s/Pictures", home);
        if((strcmp(argv[i], documents)!=0 && strcmp(argv[i], downloads)!=0  && strcmp(argv[i], pictures)!=0)){
            printf("You have entered wrong directory.\n");
            for(int k=0; k<list.count; k++){
                free(list.allFiles[k].path);
            }
            free(list.allFiles);
            return EXIT_FAILURE;
        }
        for(int k=1; k<i; k++){
            if(strcmp(argv[k], argv[i])==0){
                printf("Directory was entered more than once.\n");
                for(int k=0; k<list.count; k++){
                    free(list.allFiles[k].path);
                }
                free(list.allFiles);
                return EXIT_FAILURE;
            }
        }
        if(!scanDirectory(argv[i], &list)){
            printf("Not enough memory or directory scan failed.\n");
            for(int k=0; k<list.count; k++){
                free(list.allFiles[k].path);
            }
            free(list.allFiles);
            return EXIT_FAILURE;
        }
    }

    qsort(list.allFiles, list.count, sizeof(FileInfo), compare);
    
    Node **hashTable=calloc(2000, sizeof(Node *));
    addFilesToHashTable(hashTable, 2000, list);
    int sizeOfGroupsArr=200;
    Node **allGroups=malloc(sizeOfGroupsArr*sizeof(Node *));
    int countOfGroups=0;
    int tmpGroupsCount=0;
    for(int i=0; i<2000; i++){
        if(hashTable[i]!=NULL && hashTable[i][0].next!=NULL){
            Node **tmpGroups=findSameFiles(hashTable[i],  &tmpGroupsCount);
            for(int i=0; i<tmpGroupsCount; i++){
                if(tmpGroups[i][0].next!=NULL){
                    if(sizeOfGroupsArr==countOfGroups){
                        sizeOfGroupsArr*=2;
                        Node **reSize=realloc(allGroups, sizeOfGroupsArr*sizeof(Node *));
                        if(reSize==NULL){
                            printf("Not enough memory or directory scan failed.\n");
                            freeTmpGroups(tmpGroups, tmpGroupsCount);
                            freeAll(&list, hashTable, allGroups, countOfGroups);
                            return EXIT_FAILURE;
                        }
                        allGroups=reSize;
                    }
                    allGroups[countOfGroups]=tmpGroups[i];
                    countOfGroups++;
                }
            }
            freeTmpGroups(tmpGroups, tmpGroupsCount);
        }
    }
    printResult(allGroups, countOfGroups);
    freeAll(&list, hashTable, allGroups, countOfGroups);
    return EXIT_SUCCESS;
}