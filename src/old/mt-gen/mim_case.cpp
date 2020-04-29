// io $

char output_buffer_$[MAX_ANSW * ONE_INT_CHAR_SIZE * 7];
int output_buffer_num_$ = 0;
void* write_to_disk_$(void* args) {
    int from, to, u, i;
    from = 0; to = node_num;
    for (u=from; u<to; ++u) {
        switch (global_search_assignment[u])
        {
        case 0:
            for (i=answer_7_header_mt_0[u]; i<answer_7_tailer_mt_0[u]; ++i) {
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_0[i][0]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_0[i][1]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_0[i][2]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_0[i][3]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_0[i][4]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_0[i][5]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_0[i][6]);
                output_buffer_$[output_buffer_num_$ ++] = '\n';
            }
            break;
        case 1:
            for (i=answer_7_header_mt_1[u]; i<answer_7_tailer_mt_1[u]; ++i) {
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_1[i][0]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_1[i][1]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_1[i][2]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_1[i][3]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_1[i][4]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_1[i][5]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_1[i][6]);
                output_buffer_$[output_buffer_num_$ ++] = '\n';
            }
            break;
        case 2:
            for (i=answer_7_header_mt_2[u]; i<answer_7_tailer_mt_2[u]; ++i) {
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_2[i][0]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_2[i][1]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_2[i][2]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_2[i][3]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_2[i][4]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_2[i][5]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_2[i][6]);
                output_buffer_$[output_buffer_num_$ ++] = '\n';
            }
            break;
        case 3:
            for (i=answer_7_header_mt_3[u]; i<answer_7_tailer_mt_3[u]; ++i) {
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_3[i][0]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_3[i][1]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_3[i][2]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_3[i][3]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_3[i][4]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_3[i][5]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_3[i][6]);
                output_buffer_$[output_buffer_num_$ ++] = '\n';
            }
            break;
        case 4:
            for (i=answer_7_header_mt_4[u]; i<answer_7_tailer_mt_4[u]; ++i) {
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_4[i][0]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_4[i][1]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_4[i][2]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_4[i][3]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_4[i][4]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_4[i][5]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_4[i][6]);
                output_buffer_$[output_buffer_num_$ ++] = '\n';
            }
            break;
        case 5:
            for (i=answer_7_header_mt_5[u]; i<answer_7_tailer_mt_5[u]; ++i) {
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_5[i][0]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_5[i][1]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_5[i][2]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_5[i][3]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_5[i][4]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_5[i][5]);
                output_buffer_$[output_buffer_num_$ ++] = ',';
                deserialize_id(output_buffer_$, output_buffer_num_$, answer_7_mt_5[i][6]);
                output_buffer_$[output_buffer_num_$ ++] = '\n';
            }
            break;
        default:
            break;
        }
    }

    return NULL;
}

// end io $