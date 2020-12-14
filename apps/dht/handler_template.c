// use this instead of WooFGet() if getting element from RAFT
int get_element(char* raft_addr, void* element, unsigned long seq_no);

int handler_template(char* raft_addr, char* topic_name, unsigned long seq_no, void* ptr) {
    return 1;
}