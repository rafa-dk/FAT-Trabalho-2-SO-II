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
    //Verifica se já foi montado
    if (mountState == 1) {
        printf("ERRO: Nao e possivel formatar um sistema de arquivos montado.\n");
        return -1;
    }

    printf("formatando disco\n");

	int i;

    sb.magic = MAGIC_N;
    sb.number_blocks = ds_size(); //Numero total de blocos

    //Calcula quantos blocos sao necessarios para a FAT
    int fat_total = sb.number_blocks * sizeof(unsigned int);
    sb.n_fat_blocks = (fat_total + BLOCK_SIZE - 1) / BLOCK_SIZE; //Divisao de teto com inteiros

    //Write superblock no disco
    memset(sb.empty, 0, sizeof(sb.empty));
    ds_write(SUPER, (char*)&sb);

    //Limpa todas as registros do diretorio
    for (i = 0; i < N_ITEMS; i++) {
        dir[i].used = NON_OK; //NON_OK = 0.
    }
    ds_write(DIR, (char*)&dir);

    //FAT
    fat = malloc(sb.n_fat_blocks * BLOCK_SIZE);
    if (fat == NULL) {
        printf("ERRO: Falha ao alocar memoria para a FAT.\n");
        return -1;
    }

    //Marca todos os blocos como livres
    for (i = 0; i < sb.number_blocks; i++) {
        fat[i] = FREE;
    }

    //Marca os blocos de metadados como ocupados
    fat[SUPER] = EOFF; //Bloco 0: Superbloco.
    fat[DIR]   = EOFF; //Bloco 1: Diretório.
    //FAT.
    for (i = 0; i < sb.n_fat_blocks; i++) {
        fat[TABLE + i] = EOFF;
    }

    //Write a FAT no disco
    for (i = 0; i < sb.n_fat_blocks; i++) {
        ds_write(TABLE + i, ((char*)fat) + (i * BLOCK_SIZE));
    }

    free(fat);

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

	//Diretorio
	ds_read(DIR, (char*)&dir); //Read do diretorio
	if (mountState == 0) {
		fat = malloc(sb.n_fat_blocks * BLOCK_SIZE); //Aloca memoria para a tabela FAT
	}
	int fat_atual;
	for (int i = 0; i < sb.n_fat_blocks; i++) { //Loop para acessar cada bloco da tabela FAT
		ds_read(TABLE + i, ((char *)fat) + (i * BLOCK_SIZE));
	}
	for (int i = 0; i < N_ITEMS; i++) { //Loop para acessar cada item
		if (dir[i].used) { //Se o item estiver em uso (1) printar (caso 0 significa deletado/livre)
			printf("File \"%s\":\n	size %d bytes\n	Blocks %d ", dir[i].name, dir[i].length, dir[i].first);
			fat_atual = dir[i].first;
			while (fat[fat_atual] != EOFF) { //Enquanto o bloco atual for diferente de EOFF (1)
				printf("%d ", fat[fat_atual]);
				fat_atual = fat[fat_atual]; //Pega o proximo bloco do item de FAT
			}
			printf("\n");
		}
	}

	if (mountState == 0) {
		free(fat); 
	}
}

int fat_mount(){
	if (mountState == 1) //Já montado
	{
		printf("Esse sistema de arquivos já foi montado!\n");
		return -1;
	}
	ds_read(SUPER, (char*)&sb); //Read do superblock
	if (sb.magic != MAGIC_N) // Checa se o magic number é valido
	{
		printf("Erro no magic number!\n");
		return -1;
	} else{ // Sistema de arquivos é valido:
		// Alocação
		fat = malloc(sb.n_fat_blocks * BLOCK_SIZE);
		if (fat == NULL){
			printf("Erro na alocação da FAT na memória!\n");
			return -1;
		}
		// Leitura (será guardada num buffer)
		for (int i = 0; i < sb.n_fat_blocks; i++) {
			ds_read(TABLE + i, ((char*)fat) + (i * BLOCK_SIZE));
		}

		// Leitura do Diretorio (guardado em dir)
		ds_read(DIR, (char*)&dir);
		mountState = 1;

		return 0;
	}
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

//Encontra a registro de um arquivo no dir, retorna o indice do bloco
static int encontrar_registro(const char *name) {
    for (int i = 0; i < N_ITEMS; i++) {
        if (dir[i].used && strncmp(dir[i].name, name, MAX_LETTERS) == 0) {
            return i;
        }
    }
    return -1;
}

//Retorna a quantidade de caracteres lidos
int fat_read(char *name, char *buff, int length, int offset) {
    if (mountState == 0) {
        printf("ERRO: Sistema de arquivos nao montado.\n");
        return -1;
    }

    int index_arq = encontrar_registro(name);
    if (index_arq == -1) {
        printf("ERRO: Arquivo '%s' nao encontrado.\n", name);
        return -1;
    }

    dir_item *registro = &dir[index_arq];

    if (offset < 0 || offset > registro->length) {
        printf("ERRO: Offset de leitura invalido.\n");
        return -1;
    }

    //Nao ultrapassa do fim do arquivo
    if (offset + length > registro->length) {
        length = registro->length - offset;
    }

    if (length <= 0) {
        return 0;
    }

    char block_buffer[BLOCK_SIZE];
    int bytes_lido = 0;
    int block_atual = registro->first;

    //Pula blocos p/ chegar ao bloco do offset inicial
    int blocks_pular = offset / BLOCK_SIZE;
    for (int i = 0; i < blocks_pular; i++) {
        if (block_atual == EOFF) {
             return 0; //Offset está além do fim do arquivo
        }
        block_atual = fat[block_atual];
    }
    
    int offset_block = offset % BLOCK_SIZE;

    //Ler os dados bloco a bloco
    while (bytes_lido < length && block_atual != EOFF) {

        ds_read(block_atual, block_buffer);

        int bytes_to_copy_from_block = BLOCK_SIZE - offset_block;
        if (bytes_to_copy_from_block > (length - bytes_lido)) {
            bytes_to_copy_from_block = length - bytes_lido;
        }

        memcpy(buff + bytes_lido, block_buffer + offset_block, bytes_to_copy_from_block);
        
        bytes_lido += bytes_to_copy_from_block;
        offset_block = 0; //O offset dentro do bloco so se aplica na primeira iteracao

        block_atual = fat[block_atual];
    }

    return bytes_lido; //Retorna o total de bytes lidos 
}

static int find_free_block() {
    // Começa a busca após os blocos de metadados
    for (int i = sb.n_fat_blocks + 2; i < sb.number_blocks; i++) {
        if (fat[i] == FREE) {
            return i;
        }
    }
    return -1; // Disco cheio
}

// Retorna a quantidade de caracteres escritos
int fat_write(char *name, const char *buff, int length, int offset) {
    if (mountState == 0) {
        printf("ERRO: Sistema de arquivos nao montado.\n");
        return -1;
    }

    int file_idx = encontrar_registro(name);
    if (file_idx == -1) {
        printf("ERRO: Arquivo '%s' nao encontrado.\n", name);
        return -1;
    }
    
    if (offset < 0) {
        printf("ERRO: Offset de escrita invalido.\n");
        return -1;
    }

    dir_item *entry = &dir[file_idx];
    char block_buffer[BLOCK_SIZE];
    int bytes_written = 0;
    
    int current_block = entry->first;
    int prev_block = -1;

    // Se o arquivo é vazio, tenta alocar o primeiro bloco
    if (current_block == EOFF && length > 0) {
        int new_block = find_free_block();
        if (new_block == -1) return 0; // Disco cheio
        entry->first = new_block;
        current_block = new_block;
        fat[current_block] = EOFF;
    }

    // 1. Pular blocos até o offset
    int blocks_to_skip = offset / BLOCK_SIZE;
    for (int i = 0; i < blocks_to_skip; i++) {
        prev_block = current_block;
        current_block = fat[current_block];
        
        // Se precisar estender o arquivo para chegar ao offset
        if (current_block == EOFF) {
            int new_block = find_free_block();
            if (new_block == -1) { // Disco cheio
                // Atualiza metadados antes de sair
                entry->length = offset + bytes_written;
                ds_write(DIR, (char*)dir);
                for(int j=0; j<sb.n_fat_blocks; j++) ds_write(TABLE+j, ((char*)fat)+(j*BLOCK_SIZE));
                return bytes_written;
            }
            fat[prev_block] = new_block;
            fat[new_block] = EOFF;
            current_block = new_block;
        }
    }
    
    int offset_in_block = offset % BLOCK_SIZE;

    // 2. Escrever os dados bloco a bloco
    while (bytes_written < length) {
        // Se o arquivo acabou, aloca um novo bloco 
        if (current_block == EOFF) {
            int new_block = find_free_block();
            if (new_block == -1) { // Disco cheio 
                break; // Sai do loop, vai retornar o que escreveu até agora
            }
            fat[new_block] = EOFF;
            if (prev_block != -1) {
                fat[prev_block] = new_block;
            }
            current_block = new_block;
        }

        ds_read(current_block, block_buffer); // Lê bloco atual para não sobrescrever dados desnecessariamente

        int bytes_to_write_in_block = BLOCK_SIZE - offset_in_block;
        if (bytes_to_write_in_block > (length - bytes_written)) {
            bytes_to_write_in_block = length - bytes_written;
        }

        memcpy(block_buffer + offset_in_block, buff + bytes_written, bytes_to_write_in_block);
        ds_write(current_block, block_buffer);

        bytes_written += bytes_to_write_in_block;
        offset_in_block = 0;

        prev_block = current_block;
        current_block = fat[current_block];
    }

    // 3. Atualizar metadados
    if (offset + bytes_written > entry->length) {
        entry->length = offset + bytes_written;
    }
    ds_write(DIR, (char*)dir); // Salva o diretório atualizado no disco

    // Salva a FAT atualizada no disco
    for(int i = 0; i < sb.n_fat_blocks; i++) {
        ds_write(TABLE + i, ((char*)fat) + (i * BLOCK_SIZE));
    }

    return bytes_written; // Retorna o total de bytes escritos 
}

int fat_unmount() {
    if (mountState == 0) {
        // printf("Sistema de arquivos nao esta montado.\n");
        return -1;
    }

    if (fat != NULL) {
        free(fat);
        fat = NULL;
    }

    mountState = 0;
    //printf("Sistema de arquivos FAT desmontado\n");
    return 0;
}
