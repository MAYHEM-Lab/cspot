#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <yaml.h>
#include "woofc-caplets.h"

#ifndef TEST
#include "debug.h"
#else
void WooFCapPrint(char *woof_name, WCAP *cap)
{
        printf("woof:\n");
        printf("\tname: %s\n",woof_name);
        printf("\tpermissions: %8.8x\n",cap->permissions);
        printf("\tcheck: %lu\n",cap->check);
        return;
}
#endif

int SearchKeychain(const char *filename, char *woof_name, WCAP *cap) 
{
	FILE *file = fopen(filename, "r");
	yaml_parser_t parser;
	yaml_event_t event;
	int done = 0;
	char k_woof_name[4096];
	char k_check[21];
	char k_perms[20];
	uint32_t k_perm_value;
	uint64_t k_check_value;
	int state; // 0 => looking, 1 => woof: 2 => check: 
	int found;


	if(file == NULL) {
#ifndef TEST
		DEBUG_WARN("SearchKeychain: could not open %s\n",filename);
#endif
		return(-1);
    	}


	if(!yaml_parser_initialize(&parser)) {
#ifndef TEST
		DEBUG_WARN("SearchKeychain: could not initialize parser\n");
#endif
        	fclose(file);
		return(-1);
	}

#ifndef TEST
	if(woof_name == NULL) {
#ifndef TEST
		DEBUG_WARN("SearchKeychain: NULL woof name\n");
#endif
                fclose(file);
		return(-1);
	}
#endif

	yaml_parser_set_input_file(&parser, file);

	state = 0;
	found = 0;
	while(done == 0) {
		if(!yaml_parser_parse(&parser, &event)) {
#ifdef TEST
			fprintf(stderr, "SearchKeychain: YAML parsing error in %s\n",
					filename);
#endif
			break;
		}

		switch(event.type) {
			case YAML_SCALAR_EVENT:
//printf("Key/Value: %s\n", event.data.scalar.value);
				// NULL implies just print
				if(woof_name == NULL) {
					break;
				}
				if(state == 0) {
					if(strncmp(event.data.scalar.value,
						"woof",strlen("woof")) == 0) {
						state = 1; //next state
					}
				} else if(state == 1) {
					if(strncmp(event.data.scalar.value,
						"name",strlen("name")) == 0) {
					state = 2;
					}
					if(strncmp(event.data.scalar.value,
                                                "woof",strlen("woof")) == 0) {
                                        state = 1;
                                        }

				} else if(state == 2) {
					memset(k_woof_name,0,sizeof(k_woof_name));
					strncpy(k_woof_name,event.data.scalar.value,
                                                        sizeof(k_woof_name)-1);
//printf("k_woof_name: %s -- %s\n",event.data.scalar.value,k_woof_name);
					if((strncmp(k_woof_name,
						"woof",sizeof(k_woof_name)) == 0) ||
					   (strncmp(k_woof_name,
                                                woof_name,sizeof(k_woof_name)) != 0))	{
						state = 1; // start over
					} else {
//printf("woof found: %s\n",k_woof_name);
						state = 3;
					}
				} else if(state == 3) {
					memset(k_perms,0,sizeof(k_perms));
					strncpy(k_perms,event.data.scalar.value,
							sizeof(k_perms)-1);
//printf("k_perms: %s -- %s\n",event.data.scalar.value,k_perms);
					if(strncmp(k_perms,"permissions",
							strlen("permissions")) == 0) {
						state = 4;
					}
					if(strncmp(k_perms,"woof",
							strlen("woof")) == 0) {
						state = 1;
					}
				} else if(state == 4) {
					k_perm_value = strtol(event.data.scalar.value,
								NULL,16);
//printf("perm found: %x\n",k_perm_value);
					state = 5;
				} else if(state == 5) {
					memset(k_check,0,sizeof(k_check));
					strncpy(k_check,event.data.scalar.value,
							sizeof(k_check)-1);
					if(strncmp(k_check,"check",
							strlen("check")) == 0) {
						state = 6;
					}
					if(strncmp(k_check,"woof",
							strlen("woof")) == 0) {
						state = 1;
					}
				} else if(state == 6) {
					k_check_value = strtoull(event.data.scalar.value,
								NULL,10);
//printf("k_check_value: %s -- %llu\n",event.data.scalar.value,k_check_value);
					found = 1;
					done = 1;
					state = 0;
				}
				break;
			case YAML_STREAM_END_EVENT:
				done = 1;
				break;
			default:
				break;
		}
		yaml_event_delete(&event);
	}

    yaml_parser_delete(&parser);
    fclose(file);
    if(found == 1) {
	    cap->permissions = k_perm_value;
	    cap->check = k_check_value;
	    cap->flags = 0;
	    cap->frame_size = 0;
//printf("search: ");
//WooFCapPrint(woof_name,cap);
	    return(1);
    } else {
//printf("search: no cap %s %s\n",filename, woof_name);
	    return(-1);
    }
}

#ifdef TEST
#define ARGS "f:W:"
char *Usage = "woofc-keychain-search -f config.yaml\n\
\t-W woof-name\n";
char Fname[4096];
char Wname[4096];
int main(int argc, char **argv) 
{
	int c;
	uint64_t check;
	int err;
	WCAP cap;

	while((c = getopt(argc,argv,ARGS)) != EOF) {
		switch(c) {
			case 'f':
				strncpy(Fname,optarg,sizeof(Fname));
				break;
			case 'W':
				strncpy(Wname,optarg,sizeof(Fname));
				break;
			default:
				fprintf(stderr,"unrecognized command %c\n",
						(char)c);
				fprintf(stderr,"usage: %s",Usage);
				exit(1);
		}
	}
	if(Fname[0] == 0) {
		fprintf(stderr,"must specify config file name\n");
		fprintf(stderr,"usage: %s",Usage);
		exit(1);
	}
	if(Wname[0] == 0) {
		(void)SearchKeychain(Fname,NULL,&cap);
	} else {
		err = SearchKeychain(Fname,Wname,&cap);
		if(err < 0) {
			fprintf(stderr,"could not find check for %s in %s\n",
					Wname,Fname);
		} else {
			WooFCapPrint(Wname,&cap);
		}
	}
	return 0;
}
#endif

