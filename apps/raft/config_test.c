#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raft.h"

int test_encode_decode() {
	RAFT_DATA_TYPE data;
	int members = 3;
	char member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
	sprintf(member_woofs[0], "woof://192.168.0.1/home/centos/cspot1");
	sprintf(member_woofs[1], "woof://192.168.0.2/home/centos/cspot2");
	sprintf(member_woofs[2], "woof://192.168.0.3/home/centos/cspot3");

	int err = encode_config(data.val, members, member_woofs);
	if (err < 0) {
		fprintf(stderr, "can't encode config\n");
		return -1;
	}
	printf("encoded string: %s\n", data.val);

	int decoded_members = 0;
	char decoded_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
	err = decode_config(data.val, &decoded_members, decoded_woofs);
	if (err < 0) {
		fprintf(stderr, "can't decode config\n");
		return -1;
	}
	printf("decoded: %d members\n", decoded_members);
	if (members != decoded_members) {
		fprintf(stderr, "decoded members do not match\n");
		return -1;
	}
	int i;
	for (i = 0; i < decoded_members; ++i) {
		printf("%s\n", decoded_woofs[i]);
		if (strcmp(member_woofs[i], decoded_woofs[i]) != 0) {
			fprintf(stderr, "decoded members do not match\n");
			return -1;
		}
	}
}

int test_compute_joint() {
	int old_members = 3;
	char old_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
	sprintf(old_member_woofs[0], "woof://192.168.0.1/home/centos/cspot1");
	sprintf(old_member_woofs[1], "woof://192.168.0.2/home/centos/cspot2");
	sprintf(old_member_woofs[2], "woof://192.168.0.3/home/centos/cspot3");

	int new_members = 4;
	char new_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
	sprintf(new_member_woofs[0], "woof://192.168.0.1/home/centos/cspot1");
	sprintf(new_member_woofs[1], "woof://192.168.0.3/home/centos/cspot3");
	sprintf(new_member_woofs[2], "woof://192.168.0.4/home/centos/cspot4");
	sprintf(new_member_woofs[3], "woof://192.168.0.5/home/centos/cspot5");

    int joint_members;
    char joint_member_woofs[RAFT_MAX_MEMBERS + RAFT_MAX_OBSERVERS][RAFT_WOOF_NAME_LENGTH];
    int err = compute_joint_config(old_members, old_member_woofs, new_members, new_member_woofs, &joint_members, joint_member_woofs);
    if (err < 0) {
        fprintf(stderr, "compute_joint_config failed\n");
        return err;
    }
    if (joint_members != 5) {
        fprintf(stderr, "joint_members should be 5, have %d\n", joint_members);
        return -1;
    }
    int i;
    for (i = 0; i < joint_members; ++i) {
        char woof[RAFT_WOOF_NAME_LENGTH];
        sprintf(woof, "woof://192.168.0.%d/home/centos/cspot%d", i + 1, i + 1);
        if (strcmp(joint_member_woofs[i], woof) != 0) {
            fprintf(stderr, "joint_member_woofs[%i] should be %s, have %s\n", i, woof, joint_member_woofs[i]);
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv) {
	int err = test_encode_decode();
	if (err < 0) {
		fprintf(stderr, "test_encode_decode() failed\n");
		exit(1);
	}

	err = test_compute_joint();
	if (err < 0) {
		fprintf(stderr, "test_compute_joint() failed\n");
		exit(1);
	} 
	
	return 0;
}

