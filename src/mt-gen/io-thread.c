// io thread $
char CACHE_LINE_ALIGN output_buffer_mt_$[(MAX_EDGE * MAX_FOR_EACH_LINE) << 5];
int CACHE_LINE_ALIGN buffer_index_mt_$ = 0;
inline void deserialize_id_mt_$(int x) {
    int orginal_index = buffer_index_mt_$;
    memcpy(output_buffer_mt_$ + buffer_index_mt_$, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_$ += integer_buffer_size[x];
}
void* deserialize_ans_mt_$(void* args) {
    int x = $;
    int x_2 = x+2, x_3 = x+3;
    for (int y=0; y<node_num; ++y) {
        if (global_search_assignment[y] == 0) {
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_$(all_answer_mt_0[i][j]);
                    output_buffer_mt_$[buffer_index_mt_$ ++] = j == x_2 ? '\n' : ',';
                }
            }
        } else if (global_search_assignment[y] == 1) {
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_$(all_answer_mt_1[i][j]);
                    output_buffer_mt_$[buffer_index_mt_$ ++] = j == x_2 ? '\n' : ',';
                }
            }
        } else if (global_search_assignment[y] == 2) {
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_$(all_answer_mt_2[i][j]);
                    output_buffer_mt_$[buffer_index_mt_$ ++] = j == x_2 ? '\n' : ',';
                }
            }
        } else {
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_$(all_answer_mt_3[i][j]);
                    output_buffer_mt_$[buffer_index_mt_$ ++] = j == x_2 ? '\n' : ',';
                }
            }
        }
    }
    return NULL;
}
// end io thread $