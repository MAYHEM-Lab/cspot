/*
 * add events to GLOG from log tail extracted from some local log
 */
int ImportLogTail(GLOG *gl, LOG *ll, unsigned long remote_host)
{
	LOG *lt;
	unsigned long earliest;
	HOST *host;
	unsigned long curr;
	EVENT *ev;
	int err;

	/*
	 * find the max seq_no from the remote gost that the GLOG has seen
	 */
	host = HostListFind(gl->host_list,remote_host);
	if(host == NULL) {
		earliest = 0;
	} else {
		earliest = host->max_seen;
	}


	lt = LogTail(ll,earliest,100);
	if(lt == NULL) {
		fprintf(stderr,"couldn't get log tail from local log\n");
		fflush(stderr);
		return(0);
	}

	/*
	 * do the import
	 */
	curr = lt->head;
	ev = (EVENT *)(MIOAddr(lt->m_buf) + sizeof(LOG));
	while(ev[curr].seq_no > earliest) {
		err = GLogEvent(gl,&ev[curr]);
		if(err < 0) {
			fprintf(stderr,"couldn't add seq_no %llu\n",
				ev[curr].seq_no);
			fflush(stderr);
			LogFree(lt);
			return(0);
		}
		if(curr == lt->tail) {
			break;
		}
		curr = curr - 1;
		if(curr >= lt->size) {
			curr = (lt->size-1);
		}
	}

	LogFree(lt);

	return(1);
}
	
