#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <spawn.h>

#include "debug.h"

#define SPLAY (0)


#if defined(__linux__) && defined(__ELF__) && defined(__aarch64__)
#define CSPOT_ELF64_ARM64 1
#endif

#if defined(__linux__) && defined(__ELF__) && defined(__x86_64__)
#define CSPOT_ELF64_X86_64 1
#endif

#if defined(__APPLE__) && defined(__MACH__) && defined(__aarch64__)
#define CSPOT_MACHO_ARM64 1
#endif


#if defined(CSPOT_ELF64_X86_64) || defined(CSPOT_ELF64_ARM64)
#include <elf.h>

static int CheckHandlerWatermarkELF64(char *handler_name)
{
	/*
	* check_watermark.c
	*
	* Usage:
	*      check_watermark <elf-binary>
	*
	* Looks for a section named ".note.my_codebase"
	* and prints its contents.
	*
	* Supports:
	*      ELF64
	*      Little-endian
	*/

	if(handler_name == NULL) {
		return(1);
	}

	FILE *fp;
	Elf64_Ehdr ehdr;
	Elf64_Shdr *shdrs = NULL;
	Elf64_Shdr shstr;
	char *shstrtab = NULL;
	int i;

	fp = fopen(handler_name, "rb");
	if (!fp) {
		//perror(handler_name);
		return -1;
	}

	if (fread(&ehdr, sizeof(ehdr), 1, fp) != 1) {
#ifdef DEBUG
		fprintf(stdout, "Couldn't read ELF header for %s\n",handler_name);
#endif
		fclose(fp);
		return -1;
	}

	if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
#ifdef DEBUG
		fprintf(stdout, "Not an ELF file: %s\n",handler_name);
#endif
		fclose(fp);
		return -1;
	}

	if (ehdr.e_ident[EI_CLASS] != ELFCLASS64) {
#ifdef DEBUG
		fprintf(stdout, "Only ELF64 supported: %s\n",handler_name);
#endif
		fclose(fp);
		return -1;
	}

	if (ehdr.e_ident[EI_DATA] != ELFDATA2LSB) {
#ifdef DEBUG
		fprintf(stdout, "Only little-endian ELF supported: %s\n",handler_name);
#endif
		fclose(fp);
		return -1;
	}

	/* Read all section headers */

	shdrs = (Elf64_Shdr *)malloc(ehdr.e_shentsize * ehdr.e_shnum);
	if (!shdrs) {
		//perror("malloc");
		fclose(fp);
		return -1;
	}

	if (fseek(fp, ehdr.e_shoff, SEEK_SET) != 0) {
		//perror("fseek");
		goto fail;
	}

	if (fread(shdrs,
	      ehdr.e_shentsize,
	      ehdr.e_shnum,
	      fp) != ehdr.e_shnum) {
#ifdef DEBUG
			fprintf(stdout, "Couldn't read section headers: %s\n",handler_name);
#endif
			goto fail;
	}

	/* Read section-name string table */

	shstr = shdrs[ehdr.e_shstrndx];

	shstrtab = (char *)malloc(shstr.sh_size);
	if (!shstrtab) {
		//perror("malloc");
		goto fail;
	}

	if (fseek(fp, shstr.sh_offset, SEEK_SET) != 0) {
		//perror("fseek");
		goto fail;
	}

	if (fread(shstrtab, 1, shstr.sh_size, fp) != shstr.sh_size) {
#ifdef DEBUG
		fprintf(stdout, "Couldn't read string table: %s\n",handler_name);
#endif
		goto fail;
	}

	/* Search for our watermark section */

	for (i = 0; i < ehdr.e_shnum; i++) {
		const char *name = shstrtab + shdrs[i].sh_name;
		if (strcmp(name, ".note.my_codebase") == 0) {
		    char *buf = (char *)malloc(shdrs[i].sh_size + 1);
		    if (!buf) {
			//perror("malloc");
			goto fail;
		    }

		    if (fseek(fp, shdrs[i].sh_offset, SEEK_SET) != 0) {
			//perror("fseek");
			free(buf);
			goto fail;
		    }

		    if (fread(buf, 1, shdrs[i].sh_size, fp) != shdrs[i].sh_size) {
#ifdef DEBUG
			fprintf(stdout, "Couldn't read cspot handler watermark: %s\n",
					handler_name);
#endif
			free(buf);
			goto fail;
		    }

		    buf[shdrs[i].sh_size] = '\0';
		    //printf("%s\n", buf);

		    free(buf);
		    free(shstrtab);
		    free(shdrs);
		    fclose(fp);
		    return 1;
		}
	}


	fail:
	free(shstrtab);
	free(shdrs);
	fclose(fp);
	return -1;
}
#endif

#if defined(CSPOT_MACHO_ARM64)

#include <mach-o/loader.h>

static int CheckHandlerWatermarkMachO64(char *handler_name)
{
    FILE *fp;
    struct mach_header_64 mh;
    uint32_t i;

    if (handler_name == NULL) {
        return 1;
    }

    fp = fopen(handler_name, "rb");
    if (!fp) {
        //perror(handler_name);
        return -1;
    }

    if (fread(&mh, sizeof(mh), 1, fp) != 1) {
#ifdef DEBUG
        fprintf(stdout, "Couldn't read Mach-O header for %s\n", handler_name);
#endif
        fclose(fp);
        return -1;
    }

    if (mh.magic != MH_MAGIC_64) {
#ifdef DEBUG
        fprintf(stdout, "Not a 64-bit Mach-O file: %s\n", handler_name);
#endif
        fclose(fp);
        return -1;
    }

    for (i = 0; i < mh.ncmds; i++) {
        long cmd_pos = ftell(fp);
        struct load_command lc;

        if (fread(&lc, sizeof(lc), 1, fp) != 1) {
#ifdef DEBUG
            fprintf(stdout, "Couldn't read Mach-O load command: %s\n", handler_name);
#endif
            fclose(fp);
            return -1;
        }

        if (fseek(fp, cmd_pos, SEEK_SET) != 0) {
            //perror("fseek");
            fclose(fp);
            return -1;
        }

        if (lc.cmd == LC_SEGMENT_64) {
            struct segment_command_64 seg;

            if (fread(&seg, sizeof(seg), 1, fp) != 1) {
#ifdef DEBUG
                fprintf(stdout, "Couldn't read Mach-O segment: %s\n", handler_name);
#endif
                fclose(fp);
                return -1;
            }

            for (uint32_t j = 0; j < seg.nsects; j++) {
                struct section_64 sec;

                if (fread(&sec, sizeof(sec), 1, fp) != 1) {
#ifdef DEBUG
                    fprintf(stdout, "Couldn't read Mach-O section: %s\n", handler_name);
#endif
                    fclose(fp);
                    return -1;
                }

                /*
                 * Mach-O section names are at most 16 bytes.
                 * Use "__cspotwm" rather than ".note.my_codebase".
                 */
                if (strncmp(sec.sectname, "__cspotwm", 16) == 0) {
                    fclose(fp);
                    return 1;
                }
            }
        }

        if (fseek(fp, cmd_pos + lc.cmdsize, SEEK_SET) != 0) {
            //perror("fseek");
            fclose(fp);
            return -1;
        }
    }

    fclose(fp);
    return -1;
}

#endif


int CheckHandlerWatermark(char *handler_name)
{
#if defined(CSPOT_ELF64_X86_64) || defined(CSPOT_ELF64_ARM64)
    return CheckHandlerWatermarkELF64(handler_name);
#elif defined(CSPOT_MACHO_ARM64)
    return CheckHandlerWatermarkMachO64(handler_name);
#else
    fprintf(stdout, "Watermark check unsupported on this platform\n");
    return -1;
#endif
}

int main(int argc,char **argv, char **env)
{
	int err;
	char hbuff[255];
	int i;
	int j;
	int k;
	char *fargv[3]; // in case we are timing
	pid_t pid;
	pid_t npid;
	char *menv[12];
	char args[12*255];
	char *str;
	char c;
	int status;
	int splay_count = 0;
#ifdef TIMING
	double start;
	double end;
	double end1;
#endif
#ifdef TRACK
	int hid;
	char tbuff[255];
#endif

	signal(SIGPIPE, SIG_IGN);

#ifdef DEBUG
	fprintf(stdout,"woofc-forker-helper: running\n");
	fflush(stdout);
#endif

	while(1) {
		/*
		 * we need 11 env variables and a handler
		 */
		memset(args,0,sizeof(args));
		err = read(0,args,sizeof(args));
		if(err <= 0) {
			fprintf(stdout,"woofc-forker-helper read error %d\n",err);
			fflush(stdout);
			exit(0);
		}
		j = 0;
		k = 0;
		for(i=0; i < 11; i++) {
			while((args[j] != 0) && (j < sizeof(args))) { // look for NULL terminator
				j++;
			}
			if(j >= sizeof(args)) {
				fprintf(stdout,"woofc-forker-helper corrupt launch %s\n",args);
				fflush(stdout);
				exit(0);
			}
			menv[i] = &args[k];
#ifdef DEBUG
			fprintf(stdout,"woofc-forker-helper: received %s\n",menv[i]);
			fflush(stdout);
#endif
			j++;
			k = j;
		}
		menv[11] = NULL;

		fargv[0] = &args[k]; // handler is last
#ifdef TIMING
		/*
		 * if we are timing, there is one more
		 */
		j++;
		while((args[j] != 0) && (j < sizeof(args))) { // look for NULL terminator
				j++;
		}
		j++;
		fargv[1] = &args[j];
		fargv[2] = NULL;
#else
		fargv[1] = NULL;
#endif
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: received handler: %s\n",fargv[0]);
		fflush(stdout);
#endif

		if(CheckHandlerWatermark(fargv[0]) <= 0) {
#ifdef DEBUG
			fprintf(stdout,"woofc-forker-helper: handler %s failed watermark check\n",
					fargv[0]);
#endif
			// release thread in the container waiting for a reply
			err = write(2,&c,1);
			if(err < 1) {
				printf("woof-forker-helper: ERROR sending response signal for unwatermarked handler %s\n",
						fargv[0]);
				fflush(stdout);
			}
			continue;
		}

#ifdef TRACK
		/*
		 * read the hid
		 */
		err = read(0,tbuff,sizeof(tbuff));
		if(err <= 0) {
			fprintf(stdout,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}
		hid = atoi(tbuff);
		/*
		 * read the woof name
		 */
		err = read(0,tbuff,sizeof(tbuff));
		if(err <= 0) {
			fprintf(stdout,"woofc-forker-helper read %d for handler\n",err);
			exit(0);
		}

		printf("%s %d RECVD\n",tbuff,hid);
		fflush(stdout);
#endif
		STOPCLOCK(&end1);
		err = posix_spawn(&pid,fargv[0],NULL,NULL,fargv,menv);
		if(err < 0) {
			printf("woof-forker-helper: spawn of %s failed\n",fargv[0]);
		}
#ifdef DEBUG
		printf("woof-forker-helper: SPAWNED %lu\n",pid);
		fflush(stdout);
#endif
		STOPCLOCK(&end);
#ifdef TIMING
		start = strtod(fargv[1],NULL);
#endif
		TIMING_PRINT("PRESPWN %lf [%d]\n",
			DURATION(start,end1)*1000,pid);
		/*
		 * must be printf since stdout is in use
		 */
		TIMING_PRINT("SPAWNED %lf [%d]\n",
			DURATION(start,end)*1000,pid);
		/*
		 * send WooFForker completion signal
		 */
		err = write(2,&c,1);
		if(err < 1) {
			printf("woof-forker-helper: ERROR sending response signal for %lu\n",pid);
			fflush(stdout);
		} else {
#ifdef DEBUG
		printf("woof-forker-helper: sent response signal for %lu\n",pid);
		fflush(stdout);
#endif
		}

		/*
		 * if SPLAY > 0, clean up as best we can when we have over
		 * threshold zombies
		 */
		if(SPLAY != 0) {
			splay_count++;
			if(splay_count >= SPLAY) {
	#ifdef DEBUG
				fprintf(stdout,"woofc-forker-helper: about to wait for %s\n",fargv[0]);
				fflush(stdout);
	#endif
				/*
				 * reap the zombie handlers
				 */
				while((pid = waitpid(-1,&status,WNOHANG)) > 0) {
					splay_count--;
	#ifdef DEBUG
					fprintf(stdout,"woofc-forker-helper: completed wait for %s as proc %d\n",fargv[0],pid);
					fflush(stdout);
	#endif
				}
			}
		}
		/*
		 * if SPLAY is zero, wait
		 */
		if(SPLAY == 0) {
//printf("Helper[%d]: calling wait for pid: %d\n",getpid(),pid);
//fflush(stdout);
			npid = waitpid(pid,&status,0);
			if(npid < 0) {
//printf("Helper[%d]: pid: %d exited with error %d\n",getpid(),npid,errno);
//fflush(stdout);
			}
			if(WIFEXITED(status)) {
//printf("Helper[%d]: pid: %d exited with status %d\n",getpid(),npid,WEXITSTATUS(status));
//fflush(stdout);
			} else if(WIFSIGNALED(status)) {
				if(WTERMSIG(status)) {
//printf("Helper[%d]: pid: %d exited with signal\n",getpid(),npid);
//fflush(stdout);
				} else {
//printf("Helper[%d]: pid: %d exited with signal\n",getpid(),npid);
//fflush(stdout);
				}
			}
		}
			
#ifdef DEBUG
		fprintf(stdout,"woofc-forker-helper: signaled parent for %s after proc %d reaped\n",fargv[0],pid);
		fflush(stdout);
#endif
		fflush(stdout);
	}

	fprintf(stdout,"woofc-forker-helper exiting\n");	
	fflush(stdout);
	exit(0);
}

		
