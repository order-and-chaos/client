#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>
#include <assert.h>

#include "../ai.h"

__attribute__((noreturn)) void usage(int argc,char **argv){
	(void)argc;
	fprintf(stderr,
		"Usage: %s [players...]\n"
		"If called with no players, calls `make players` and collects all the\n"
		"dylibs or so's that it finds. If called with arguments, they should\n"
		"be the base names of the libraries (e.g. pass 'mmab' for the player\n"
		"contained in mmab.dylib or mmab.so).\n",
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

typedef struct Llistdata{
	void *handle;
	int score;
} Llistdata;

typedef struct Llist{
	Llistdata *data;
	struct Llist *next;
} Llist;

Llist* llist_prepend(Llist *head,Llistdata* data){
	Llist *newhead=calloc(1,sizeof(Llist));
	assert(newhead);
	newhead->data=data;
	newhead->next=head;
	return newhead;
}

void llist_foreach(Llist *head,void (*f)(Llistdata*)){
	if(!head)return;
	f(head->data);
	if(head->next)llist_foreach(head->next,f);
}

void llist_free(Llist *head){
	if(!head)return;
	if(head->data)free(head->data);
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
		if(dire->d_namlen>suflen&&strcmp(dire->d_name+(dire->d_namlen-suflen),suffix)==0){
			char *str;
			asprintf(&str,"%s/%s",dirpath,dire->d_name);
			assert(str);
			Llistdata *data=malloc(sizeof(Llistdata));
			data->handle=openlibrary(str);
			data->score=0;
			handles=llist_prepend(handles,data);
			free(str);
		}
	}
	return handles;
}

void* getsymbol(void *handle,const char *name){
	printf("Getting symbol %s from handle %p\n",name,handle);
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

void tempfunc(Llistdata *data){
	Board *board=libmakeboard(data->handle);
	libprintboard(data->handle,board);
}

void dlclosemapper(Llistdata *data){
	if(dlclose(data->handle)!=0){
		fprintf(stderr,"dlclose failed: %s\n",dlerror());
	}
}

void runmatch(Llistdata *data1,Llistdata *data2){
	;
}

int main(int argc,char **argv){
	if(argc==2&&strcmp(argv[1],"-h")==0)usage(argc,argv);

	Llist *handles;
	if(argc>1){
		handles=NULL;
		for(int i=argc-1;i>=1;i--){
			handles=llist_prepend(handles,openlibrary(argv[i]));
		}
	} else {
		handles=libsfromdir(".");
	}

	for(Llist *it1=handles;it1!=NULL;it1=it1->next){
		Llistdata *data1=it1->data;
		for(Llist *it2=handles;it2!=NULL;it2=it2->next){
			Llistdata *data2=it2->data;
			runmatch(data1,data2);
		}
	}

	llist_foreach(handles,tempfunc);

	llist_foreach(handles,dlclosemapper);
	llist_free(handles);
}
