#define _GNU_SOURCE  // asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>
#include <assert.h>

#include "../ai.h"

#define NMATCHES 2

#define WINSCORE 3
#define LOSSSCORE 1

int max(int a,int b){return b>a?b:a;}

__attribute__((noreturn)) void usage(int argc,char **argv){
	(void)argc;
	fprintf(stderr,
		"Usage: %s [players...]\n"
		"If called with no players, calls `make players` and collects all the\n"
		"dylibs or so's that it finds. If called with arguments, they should\n"
		"be the full names of the libraries (e.g. 'mmab.dylib').\n",
		argv[0]);
	exit(1);
}

void* openlibrary(const char *path){
	char *dotpath;
	asprintf(&dotpath,"./%s",path);
	assert(dotpath);
	void *handle=dlopen(dotpath,RTLD_NOW|RTLD_LOCAL);
	free(dotpath);
	if(handle==NULL){
		const char *err=dlerror();
		if(err==NULL)fprintf(stderr,"%s: dlopen failed without error?\n",path);
		else fprintf(stderr,"%s: dlopen failed: %s\n",path,err);
		exit(1);
	}
	fprintf(stderr,"Library '%s' loaded (%p)\n",path,handle);
	return handle;
}

typedef struct Llist{
	void *handle;
	char *name;
	struct Llist *next;
} Llist;

Llist* llist_prepend(Llist *head,void* handle,const char *name){
	Llist *newhead=calloc(1,sizeof(Llist));
	assert(newhead);
	newhead->handle=handle;
	asprintf(&newhead->name,"%s",name);
	assert(newhead->name);
	newhead->next=head;
	return newhead;
}

int llist_len(Llist *head){
	if(head==NULL)return 0;
	int len=1;
	while((head=head->next)!=NULL)len++;
	return len;
}

void llist_foreach(Llist *head,void (*f)(void*,const char*)){
	if(!head)return;
	f(head->handle,head->name);
	if(head->next)llist_foreach(head->next,f);
}

void llist_free(Llist *head){
	if(!head)return;
	if(head->name)free(head->name);
	if(head->next)llist_free(head->next);
	free(head);
}

Llist* libsfromdir(const char *dirpath){
	DIR *dirp=opendir(dirpath);
	if(dirp==NULL)fprintf(stderr,"%s: opendir failed: %s\n",dirpath,strerror(errno));
#ifdef __APPLE__
	const char *suffix=".dylib";
#else
	const char *suffix=".so";
#endif
	const int suflen=strlen(suffix);
	struct dirent *dire;
	Llist *handles=NULL;
	while((dire=readdir(dirp))!=NULL){
		int namlen=strlen(dire->d_name);
		if(namlen>suflen&&strcmp(dire->d_name+(namlen-suflen),suffix)==0){
			char *str;
			asprintf(&str,"%s/%s",dirpath,dire->d_name);
			assert(str);
			handles=llist_prepend(handles,openlibrary(str),dire->d_name);
			free(str);
		}
	}
	return handles;
}

void* getsymbol(void *handle,const char *name){
	//printf("Getting symbol %s from handle %p\n",name,handle);
	void *addr=dlsym(handle,name);
	if(!addr){
		fprintf(stderr,"Symbol '%s' not found in library!\n",name);
		exit(1);
	}
	return addr;
}

Board* libmakeboard(void *handle){
	return ((Board*(*)(void))getsymbol(handle,"makeboard"))();
}
void libapplymove(void *handle,Board *board,Move mv){
	((void(*)(Board*,Move))getsymbol(handle,"applymove"))(board,mv);
}
bool libisempty(void *handle,const Board *board,int pos){
	return ((bool(*)(const Board*,int))getsymbol(handle,"isempty"))(board,pos);
}
void libprintboard(void *handle,const Board *board){
	((void(*)(const Board*))getsymbol(handle,"printboard"))(board);
}
int libcheckwin(void *handle,const Board *board){
	return ((int(*)(const Board*))getsymbol(handle,"checkwin"))(board);
}
Move libcalcmove(void *handle,Board *board,int player){
	return ((Move(*)(Board*,int))getsymbol(handle,"calcmove"))(board,player);
}

typedef struct Playerdata{
	const char *name;
	void *handle;
	int score;
} Playerdata;

void runmatch(Playerdata *p1,Playerdata *p2){
	Board *bd[2]={libmakeboard(p1->handle),libmakeboard(p2->handle)};
	Playerdata *pl[2]={p1,p2};

	Move mv;
	int win=-1;
	while(win==-1){
		for(int i=0;i<2;i++){
			mv=libcalcmove(pl[i]->handle,bd[i],i);
			if(mv.pos<0||mv.pos>=36||mv.stone<0||mv.stone>1){
				fprintf(stderr,"Player %s: returned invalid move (pos=%d,stone=%d)\n",
					pl[i]->name,mv.pos,mv.stone);
				exit(1);
			}
			if(!libisempty(pl[i]->handle,bd[i],mv.pos)){
				fprintf(stderr,"Player %s: returned move on non-empty space (pos=%d,stone=%d)\n",
					pl[i]->name,mv.pos,mv.stone);
				exit(1);
			}

			libapplymove(pl[0]->handle,bd[0],mv);
			libapplymove(pl[1]->handle,bd[1],mv);

			win=libcheckwin(pl[0]->handle,bd[0]);
			int win2=libcheckwin(pl[1]->handle,bd[1]);
			if(win2!=win){
				fprintf(stderr,"Players %s and %s disagree over win status: %d vs %d\n",
					pl[0]->name,pl[1]->name,win,win2);
				exit(1);
			}
			if(win!=-1)break;
		}
	}
	if(win==ORDER){
		pl[0]->score+=WINSCORE;
		pl[1]->score+=LOSSSCORE;
		printf("%s - %s: win - loss\n",pl[0]->name,pl[1]->name);
	} else if(win==CHAOS){
		pl[0]->score+=LOSSSCORE;
		pl[1]->score+=WINSCORE;
		printf("%s - %s: loss - win\n",pl[0]->name,pl[1]->name);
	} else {
		fprintf(stderr,"Invalid win value %d returned by players %s and %s\n",
			win,pl[0]->name,pl[1]->name);
		exit(1);
	}

	free(bd[0]);
	free(bd[1]);
}
#ifdef __APPLE__
int playerindexorder(void *players,const void *a,const void *b){
#else
int playerindexorder(const void *a,const void *b,void *players){
#endif
	Playerdata *pl=(Playerdata*)players;
	return pl[*(int*)b].score-pl[*(int*)a].score;
}

void printscoretable(int nplayers,Playerdata *players){
	int maxlen=4; // for the header
	for(int i=0;i<nplayers;i++){
		maxlen=max(maxlen,strlen(players[i].name));
	}

	int indices[nplayers];
	for(int i=0;i<nplayers;i++)indices[i]=i;
#ifdef __APPLE__
	qsort_r(indices,nplayers,sizeof(int),players,playerindexorder);
#else
	qsort_r(indices,nplayers,sizeof(int),playerindexorder,players);
#endif

	printf("Name");
	for(int i=4;i<maxlen;i++)putchar(' ');
	printf(" | Score\n");
	for(int i=0;i<maxlen;i++)putchar('-');
	printf("-+-------\n");

	for(int i=0;i<nplayers;i++){
		printf("%s",players[indices[i]].name);
		for(int j=strlen(players[indices[i]].name);j<maxlen;j++)putchar(' ');
		printf(" | %d\n",players[indices[i]].score);
	}
}

int main(int argc,char **argv){
	if(argc==2&&(strcmp(argv[1],"-h")==0||strcmp(argv[1],"--help"))){
		usage(argc,argv);
	}

	Playerdata *players;
	int nplayers;
	if(argc>1){
		nplayers=argc-1;
		players=malloc((argc-1)*sizeof(Playerdata));
		assert(players);
		for(int i=0;i<argc-1;i++){
			players[i].name=argv[i+1];
			players[i].handle=openlibrary(argv[i+1]);
			players[i].score=0;
		}
	} else {
		Llist *llist=libsfromdir(".");
		nplayers=llist_len(llist);
		players=malloc((argc-1)*sizeof(Playerdata));
		assert(players);

		Llist *it=llist;
		for(int i=0;i<nplayers;i++){
			players[i].name=it->name;
			it->name=NULL; // we take ownership
			players[i].handle=it->handle;
			players[i].score=0;
			it=it->next;
		}

		llist_free(llist);
	}

	for(int i=0;i<nplayers;i++)for(int j=0;j<nplayers;j++){
		if(i==j)continue;
		for(int k=0;k<NMATCHES;k++)runmatch(players+i,players+j);
	}

	printscoretable(nplayers,players);

	for(int i=0;i<nplayers;i++){
		if(dlclose(players[i].handle)!=0){
			fprintf(stderr,"handle %p: dlclose failed: %s\n",players[i].handle,dlerror());
		}
	}
}
