#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define TOTAL_COLOR 6

typedef struct trem {
    char id[50];
    char estacao[50];
    char hora[50];
} Trem;

typedef struct linha {
    Trem trem; 
} Linha;

/*  n representa o número de arquivos
    e t representa o número de threads
*/
int n, t, l;
int arq_cobertos;

//threads do sistema
pthread_t *threads;

pthread_t print_thread;

pthread_mutex_t gen_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t *l_mutex = NULL;

// Representa o painel de linhas
Linha *painel = NULL;

//indica o fim do sistema pelo termino das outras threads
bool p_status;


void color(int i) {
    switch(i) {
        case 0:
            printf("\033[0;31m"); // vermelho
            break;
        case 1:
            printf("\033[0;32m"); // verde
            break;
        case 2:
            printf("\033[0;33m"); // amarelo
            break;
        case 3:
            printf("\033[0;34m"); // azul
            break;
        case 4:
            printf("\033[0;35m"); // magenta
            break;
        case 5:
            printf("\033[0;36m"); // ciano
            break;
    }
}


Trem constroi_trem(char *id, char *estacao, char *hora){
    Trem r;
    strcpy(r.id, id);
    strcpy(r.estacao, estacao);
    strcpy(r.hora, hora);
    return r;
}

Linha constroi_linha(Trem e) {
    Linha r;

    r.trem = e;

    return r;
}


void *t_change(void *threadid) {
    int id = *((int *)threadid);
    int a;

    printf("comecou thread %d\n", id);
    //vai acontecer enquanto existam arquivos a serem lidos
    while(arq_cobertos < n) {
        
        // Travo o acesso quando uma thread está descobrindo qual arquivo vai ler e destravo no fim
        // com o próximo arquivo atualizado
        pthread_mutex_lock(&gen_mutex);
        a = arq_cobertos;

        //printf("THREAD_ID=%d, pegou o %d.txt\n", id, a);
        
        arq_cobertos++;
        
        pthread_mutex_unlock(&gen_mutex);
        
        char b[50];
        sprintf(b, "%d.txt", a);
        
        FILE * f = fopen(b, "r");
        
        // Percorre o arquivo enquanto nao eh eof
        while(!feof(f)) {

            // separa a linha na qual vai se trabalhar
            int i;
            fscanf(f, "%d", &i);
            
            // so para debug
            //printf("thread=%d linha=%d\n", id, i);
            
            // tranca-se a linha
            pthread_mutex_lock(&l_mutex[i]);

            // Constrói toda  a linha das informaçoes tiradas do arquivo
            Trem e;
            
            fscanf(f, "%s %s %s", e.id, e.estacao, e.hora);
            painel[i] = constroi_linha(e);
            
            // Adormece a linha por 2 segundos
            sleep(2);            

            // libera a linha para outras threads
            pthread_mutex_unlock(&l_mutex[i]);
        }
    }

    pthread_exit(NULL);
}

void *print() {
    while(p_status) {
        
        // adormece para retomar o próximo print
        sleep(2);
        
        // limpa a tela para para o próximo quadro
        system("clear");

        for(int i = 0; i < l; i++) {
            
            // Escolhe cor de acordo com o index
            color(i%TOTAL_COLOR);
            
            printf("%s %s %s - %i\n", painel[i].trem.id, painel[i].trem.estacao, painel[i].trem.hora, i);
            
            // reseta para branco
            printf("\033[0m"); 
            
        }
    }

    pthread_exit(NULL);
}

int main() {
    

    // Inicializando e alocando variáveis necessárias
    printf("Digite o valor de arquivos, threads e linhas para o programa:\n");
    scanf("%d %d %d", &n, &t, &l);
    
    p_status = true;

    //variável que se refere aos arquivos ja cobertos
    arq_cobertos = 0;
    
    //Vetor de linhas do painel
    painel = (Linha *)malloc(l*sizeof(Linha));
    for(int i = 0; i < l; i++) 
        painel[i] = constroi_linha(constroi_trem("", "", ""));
    
    //Vetor de threads
    threads = (pthread_t *)malloc(t*sizeof(pthread_t));
    int *threads_id[t];

    //vetor de mutex para cada linha
    l_mutex = (pthread_mutex_t *)malloc(l*sizeof(pthread_mutex_t));
    
    for(int i = 0; i < l; i++)
        pthread_mutex_init(&l_mutex[i], 0);
    
    // vai armazenar o retorno para o thread_create
    int rc;

    //inicialização das threads
    for(int i = 0; i < t; i++) {
        threads_id[i] = (int *)malloc(sizeof(int));
        *(threads_id[i]) = i;
        rc = pthread_create(&threads[i], NULL, t_change, (void *)threads_id[i]);

        if(rc) {
            printf("Deu problema na criacao da thread %d\n", rc);
            exit(-1);
        }

    }

    rc = pthread_create(&print_thread, NULL, print, NULL);
    if(rc) {
        printf("Thread_Print ERROR");
        exit(-1);
    }

    for(int i = 0; i < t; i++) 
        pthread_join(threads[i], NULL);
    
    p_status = false;
    pthread_join(print_thread, NULL);

    /*FILE * f = fopen("0.txt", "r");
    while (!feof(f))
    {
        int i;
        Trem e;
        fscanf(f, "%d", &i);
        fscanf(f, "%s %s %s\n", e.id, e.estacao, e.hora);
        printf("%d\n%s %s %s\n", i, e.id, e.estacao, e.hora);
    }*/
    //fclose(f);
    
    pthread_mutex_destroy(&gen_mutex);
    for(int i = 0; i < l; i++)
        pthread_mutex_destroy(&l_mutex[i]);
    for(int i =0; i < t; i++) 
        free(threads_id[i]);    
    
    pthread_exit(NULL);

    free(threads);

    return 0;
}