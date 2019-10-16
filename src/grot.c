	if(str[0] != '/') { /* not an absolute path name */
		getcwd(Host_dir,sizeof(Host_dir));
		if(str[0] == '.') {
			strncat(Host_dir,&(str[1]),
				sizeof(Host_dir)-strlen(Host_dir));
		} else {
			strncat(Host_dir,"/",sizeof(Host_dir)-strlen(Host_dir));
			strncat(Host_dir,str,
				sizeof(Host_dir)-strlen(Host_dir));
		}
	} else {
		strncpy(Host_dir,str,sizeof(Host_dir));
	}

	if(strcmp(Host_dir,"/") == 0) {
		fprintf(stderr,"WooFInit: CSPOT_HOST_DIR can't be %s\n",
				Host_dir);
		exit(1);
	}

	if(strlen(str) >= (sizeof(Host_dir)-1)) {
		fprintf(stderr,"WooFInit: %s too long for directory name\n",
				str);
		exit(1);
	}

	if(Host_dir[strlen(Host_dir)-1] == '/') {
		Host_dir[strlen(Host_dir)-1] = 0;
	}

	memset(putbuf,0,sizeof(putbuf));
	sprintf(putbuf,"CSPOT_HOST_DIR=%s",Host_dir);
	putenv(putbuf);
#ifdef DEBUG
	fprintf(stdout,"WooFInit: %s\n",putbuf);
	fflush(stdout);
#endif
