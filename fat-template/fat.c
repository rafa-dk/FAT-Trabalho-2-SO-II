#include "fat.h"
#include "ds.h"
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define SUPER 0
#define TABLE 2
#define DIR 1

#define SIZE 1024

// the superblock
#define MAGIC_N           0xAC0010DE
typedef struct{
	int magic;
	int number_blocks;
	int n_fat_blocks;
	char empty[BLOCK_SIZE-3*sizeof(int)];
} super;

super sb;

//item
#define MAX_LETTERS 6
#define OK 1
#define NON_OK 0
typedef struct{
	unsigned char used;
	char name[MAX_LETTERS+1];
	unsigned int length;
	unsigned int first;
} dir_item;

#define N_ITEMS (BLOCK_SIZE / sizeof(dir_item))
dir_item dir[N_ITEMS];

// table
#define FREE 0
#define EOFF 1
#define BUSY 2
unsigned int *fat;

int mountState = 0;

int fat_format(){ 
  	return 0;
}

void fat_debug(){

	printf("depurando\n\n");
	
	//Superblock
	ds_read(SUPER, (char*)&sb); //Read do superblock
	printf("superblock:\n");
	printf("magic is ");
	if (sb.magic != MAGIC_N) { //Verifica se o magic number esta correto
		printf("wrong: 0x%x\n", sb.magic);
	}
	else {
		printf("ok\n");
	}
	printf("%d blocks\n", sb.number_blocks);
	printf("%d block fat\n", sb.n_fat_blocks);

	//Acessar Diretorio
	ds_read(DIR, (char*)dir);
	for (int i = 0; i < N_ITEMS; i++) {
		if (dir[i].used) { //Se o item estiver em uso (caso 0 significa deletado/livre)
			printf("File \"%s\":\n	size %d bytes\n	Blocks %d\n", dir[i].name, dir[i].length, dir[i].first);
		}
	}
}

int fat_mount(){
  	return 0;
}

int fat_create(char *name){
  	return 0;
}

int fat_delete( char *name){
  	return 0;
}

int fat_getsize( char *name){ 
	return 0;
}

//Retorna a quantidade de caracteres lidos
int fat_read( char *name, char *buff, int length, int offset){
	return 0;
}

//Retorna a quantidade de caracteres escritos
int fat_write( char *name, const char *buff, int length, int offset){
	return 0;
}
