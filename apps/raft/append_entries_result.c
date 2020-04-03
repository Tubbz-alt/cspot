#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "woofc.h"
#include "raft.h"

char function_tag[] = "append_entries_result";

int append_entries_result(WOOF *wf, unsigned long seq_no, void *ptr)
{
	RAFT_APPEND_ENTRIES_RESULT *result = (RAFT_APPEND_ENTRIES_RESULT *)ptr;
	
	// log_set_level(LOG_INFO);
	log_set_level(LOG_DEBUG);
	log_set_output(stdout);

	// get the server's current term
	unsigned long last_server_state = WooFGetLatestSeqno(RAFT_SERVER_STATE_WOOF);
	if (WooFInvalid(last_server_state)) {
		sprintf(log_msg, "couldn't get the latest seqno from %s", RAFT_SERVER_STATE_WOOF);
		log_error(function_tag, log_msg);
		exit(1);
	}
	RAFT_SERVER_STATE server_state;
	int err = WooFGet(RAFT_SERVER_STATE_WOOF, &server_state, last_server_state);
	if (err < 0) {
		log_error(function_tag, "couldn't get the server state");
		exit(1);
	}

	if (result->term > server_state.current_term) {
		// fall back to follower
		RAFT_TERM_ENTRY new_term;
		new_term.term = result->term;
		new_term.role = RAFT_FOLLOWER;
		unsigned long seq = WooFPut(RAFT_TERM_ENTRIES_WOOF, NULL, &new_term);
		if (WooFInvalid(seq)) {
			sprintf(log_msg, "couldn't ask term chair to fall back to follower at term %lu", new_term.term);
			log_error(function_tag, log_msg);
			exit(1);
		}
		sprintf(log_msg, "result term %lu is higher than server's term %lu, fall back to follower", result->term, server_state.current_term);
		log_debug(function_tag, log_msg);
	}
	int i;
	for (i = 0; i < server_state.members; i++) {
		if (memcmp(result->server_woof, server_state.member_woofs[i], RAFT_WOOF_NAME_LENGTH) == 0) {
			RAFT_NEXT_INDEX next_indexes;
			next_indexes.term = result->term;
			// TODO: guess we can ignore result->success since we have next_index anyway
			next_indexes.next_index[i] = result->next_index;
			unsigned long seq = WooFPut(RAFT_NEXT_INDEX_WOOF, NULL, &next_indexes);
			if (WooFInvalid(seq)) {
				sprintf(log_msg, "couldn't update next_index for member %d", i);
				log_debug(function_tag, log_msg);
				exit(1);
			}
			sprintf(log_msg, "updated next_index for member %d: %lu", i, result->next_index);
			log_debug(function_tag, log_msg);
			break;
		}
	}
	return 1;
}