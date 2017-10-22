/*
 * cthread.h: arquivo de inclusão com os protótipos das funções a serem
 *            implementadas na realização do trabalho.
 *
 * NÃO MODIFIQUE ESTE ARQUIVO.
 *
 * VERSÃO: 11/09/2017
 *
 */
#define	CRIACAO	0
#define	APTO	1
#define	EXEC	2
#define	BLOQ	3
#define	TERM	4

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/support.h"
#include "../include/cthread.h"
#include "../include/cdata.h"


//???
PFILA2 aptos;
PFILA2 bloqueados;
PFILA2 terminados;
PFILA2 aguardando_join;
int inicializado = 0;
int novotid = 0;
ucontext_t dispatcher_ctxt; 
ucontext_t kill_ctxt;
TCB_t* main;
TCB_t* thread_atual;

//
// FUNÇÕES AUXILIARES
/*--------------------------------------------------------------------
Função: Insere um nodo na lista indicada, segundo o campo "prio" do TCB_t
	A fila deve estar ordenada (ou ter sido construída usado apenas essa funcao)
	O primeiro elemento da lista (first) é aquele com menor vamor de "prio"
Entra:	pfila -> objeto FILA2
	pnodo -> objeto a ser colocado na FILA2
Ret:	==0, se conseguiu
	!=0, caso contrário (erro)
--------------------------------------------------------------------*/
int	InsertByPrio(PFILA2 pfila, TCB_t *tcb) {
	TCB_t *tcb_it;
	
	if (FirstFila2(pfila)==0) {	// pfile vazia?
		do {
			tcb_it = (TCB_t *) GetAtIteratorFila2(pfila);
			if (tcb->prio < tcb_it->prio) {
				return InsertBeforeIteratorFila2(pfila, tcb);
			}
		} while (NextFila2(pfila)==0);
	}
	return AppendFila2(pfila, (void *)tcb);
}

// todas threads criadas devem retornar pra cá para serem mortas
void kill_thread(){
	TCB_t* novotcb;
	novotcb = thread_atual;
	int tid = novotcb->tid; //para a busca a seguir
	AppendFila2(terminados, (void *) novotcb->tid);
	//talvez tenha que desalocar essa pilha? já que ela foi alocada antes?
	free(novotcb->context.uc_stack.ss_sp);
	//parece ser o suficiente para acabar com a thread, let's see
	free(novotcb);

	//verificamos se há alguma thread trancada em um join por esta thread
	JOIN_t* join_it;
	if (FirstFila2(aguardando_join)==0) {	// pfile vazia?
		do {
			join_it = (JOIN_t*) GetAtIteratorFila2(aguardando_join);
			if (tid == join_it->tid) {
				join_it->tid = -1;
				DeleteAtIteratorFila2(aguardando_join);
				setcontext(&(join_it->context)); //retornamos o contexto para a função de join, que chamará um dispatcher 
			}
		} while (NextFila2(aguardando_join)==0);
	}
	
	
	return; //dispatcher();
}

void dispatcher(){
	FirstFila2(aptos);
	thread_atual = (TCB_t*)GetAtIteratorFila2(aptos);
	DeleteAtIteratorFila2(aptos);
	thread_atual->state = EXEC;
	startTimer();
	setcontext(&(thread_atual->context));
}

void inicializar(){
	CreateFila2(aptos);
	CreateFila2(bloqueados);
	CreateFila2(terminados);
	CreateFila2(aguardando_join);
	//
	main->tid = novotid;
	novotid++;
    	main->state = EXEC;
    	main->prio = 0;
    	thread_atual = main;
	//
	getcontext(&dispatcher_ctxt);
	dispatcher_ctxt.uc_stack.ss_sp = malloc(16384);
	dispatcher_ctxt.uc_stack.ss_size = 16384;
	dispatcher_ctxt.uc_link = &(main->context);
	makecontext(&dispatcher_ctxt, (void (*)(void))dispatcher, 0);

	getcontext(&kill_ctxt);
	kill_ctxt.uc_stack.ss_sp = malloc(16384);
	kill_ctxt.uc_stack.ss_size = 16384;
	kill_ctxt.uc_link = &dispatcher_ctxt;  //depois de matar a thread vai para o dispatcher
	makecontext(&kill_ctxt, (void (*)(void))kill_thread, 0); //as threads precisam voltar para a função de matar threads!	
	
	//
	getcontext(&(main->context));
	// finaliza inicialização, retorna ao primeiro comando da lib chamado e volta ao "fluxo" da main
}

//
// FUNÇÕES PRINCIPAIS

int cidentify (char *name, int size){
	char id_grupo[65] = "Fernando Bock 242255\nJoao Henz 242251\nLeonardo Wellausen 261571\n";
	if (!inicializado){
		inicializar();
	}
	if (size >= 65){
		strncpy(name, id_grupo, size);
		return 0;
	}

	return -1;
}

int ccreate (void* (*start)(void*), void *arg, int prio){
	// garantia de inicialização da main e das filas
	if (!inicializado){
		inicializar();
	}
	// criação do Thread Control Block
	TCB_t* novotcb = (TCB_t*) malloc(sizeof(TCB_t));
    	novotcb->state = CRIACAO;
    	novotcb->prio = 0;
    	novotcb->tid = novotid;
	novotid++;
	// criação de contexto da thread
	getcontext(&(novotcb->context));
    	novotcb->context.uc_stack.ss_sp = malloc(16384);
    	novotcb->context.uc_stack.ss_size = 16384;
    	novotcb->context.uc_link = &kill_ctxt;
   	makecontext(&(novotcb->context), (void (*)(void)) start, 1, arg);
   	novotcb->state = APTO;
	InsertByPrio(aptos, novotcb);
	
	return novotcb->tid;
}

int cyield(void){
	// garantia de inicialização da main e das filas
	if (!inicializado){
		inicializar();
	}
	TCB_t* novotcb;
	novotcb = thread_atual;
	novotcb->prio += stopTimer();
	novotcb->state = APTO;
	InsertByPrio(aptos, novotcb);
	dispatcher();
	
	return 0;
}

int cjoin(int tid){
	int tid_it;
	if (FirstFila2(terminados)==0) {	// pfile vazia?
		do {
			tid_it = (int) GetAtIteratorFila2(terminados);
			if (tid_it == tid) {
				return 0;      //se a thread esperada já terminou retornamos 0
			}
		} while (NextFila2(terminados)==0);
	}
	
	if (tid >= novotid)
		return -1;    //se o tid recebido é maior que o atual a thread não existe

	//Crio uma estrutura join que tem o tid aguardado e esse conxteto de cjoin para poder retornar à função chamadora
	// quando a thread aguardada terminar
	TCB_t* tcb_chamador = thread_atual;	
	JOIN_t* join = (JOIN_t*) malloc(sizeof(JOIN_t));
	AppendFila2(aguardando_join, join);
	join->tid = tid;	
	ucontext_t context;
	getcontext(&context);    //contexto voltará aqui
	join->context = context;

	if (join->tid !=-1){  // GAMBIARRA quando o contexto vier após o térmido da thread aguardada o tid será -1
		tcb_chamador->state = BLOQ;
		AppendFila2(bloqueados, tcb_chamador);
		dispatcher();
	}

	//Acha e remove o tcb chamador da fila de bloqueados
	if (FirstFila2(bloqueados)==0) {	// pfile vazia?
		TCB_t *tcb_it;
		do {
			tcb_it = (TCB_t *) GetAtIteratorFila2(bloqueados);
			if (tcb_chamador->tid == tcb_it->tid) {   //retorna o chamador para a fila de aptos
				DeleteAtIteratorFila2(bloqueados);
				tcb_chamador->state = APTO;
				InsertByPrio(aptos, tcb_chamador);  //como retornar? dispatcher ou return 0? ): 
				// mudar o contexto da chamadora para cá?? sounds like madness....why not then
				getcontext(&(tcb_chamador->context));
				if(tcb_chamador->state == APTO){ //o dispatcher irá alterar o estado para EXEC
					dispatcher(); //quando a função chamadora estiver sendo executada, irá para o retorno
				} 
				return 0;
			}
		} while (NextFila2(bloqueados)==0);
	}
	else{
		return -1;  //se a fila de bloqueados estiver vázia, tem algo errado. (provável)	
	}
		

	return 0;
}

int csem_init(csem_t *sem, int count){
	if (CreateFila2(sem->fila) == 0 && count == 1){
		sem->count = count;
		return 0;
	}
	
	return -1;
}

int cwait(csem_t *sem){
	if (sem->count>0){

		sem->count--;

		return 0;
	}
	else{
		LastFila2(sem->fila);
		InsertAfterIteratorFila2(sem->fila, thread_atual);

		return 0;

	}
}

int csignal(csem_t *sem){



	sem->count++;
	FirstFila2(sem->fila);
	InsertByPrio(aptos, GetAtIteratorFila2(sem->fila));
	DeleteAtIteratorFila2(sem->fila);

	return 0;
}
