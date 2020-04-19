// io thread 4
char CACHE_LINE_ALIGN output_buffer_mt_4[ONE_INT_CHAR_SIZE * MAX_ANSW * 3 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_4 = 0;
inline void deserialize_int_mt_4(int x) {
    if (x == 0) {
        output_buffer_mt_4[buffer_index_mt_4 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_4;
    while (x) {
        output_buffer_mt_4[buffer_index_mt_4 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_4-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_4[i];
        output_buffer_mt_4[i] = output_buffer_mt_4[j];
        output_buffer_mt_4[j] = orignial_index;
    }
}
inline void deserialize_id_mt_4(int x) {
    int orginal_index = buffer_index_mt_4;
    memcpy(output_buffer_mt_4 + buffer_index_mt_4, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_4 += integer_buffer_size[x];
}
void* deserialize_ans_mt_4(void* args) {
    deserialize_int_mt_4(answer_num);
    output_buffer_mt_4[buffer_index_mt_4 ++] = '\n';
    int x = 0;
    int x_2 = x+2, x_3 = x+3;
    int y_0 = 0, y_1 = 0, y_2 = 0, y_3 = 0;
    for (int y=0; y<node_num; ++y) {
        if (global_search_assignment[y] == 0) {
            if (likely(answer_header_mt_0[x][y_0] != 0)) {
                for (int i=answer_header_mt_0[x][y_0]; i!=0; i=all_answer_ptr_mt_0[i]) {
                    deserialize_id_mt_4(y);
                    output_buffer_mt_4[buffer_index_mt_4] += ',';
                    for (int j=0; j<x_3; ++j) {
                        deserialize_id_mt_4(all_answer_mt_0[i][j]);
                        output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                    }
                }
            }
            ++ y_0;
        } else if (global_search_assignment[y] == 1) {
            if (likely(answer_header_mt_1[x][y_1] != 0)) {
                for (int i=answer_header_mt_1[x][y_1]; i!=0; i=all_answer_ptr_mt_1[i]) {
                    deserialize_id_mt_4(y);
                    output_buffer_mt_4[buffer_index_mt_4] += ',';
                    for (int j=0; j<x_3; ++j) {
                        deserialize_id_mt_4(all_answer_mt_1[i][j]);
                        output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                    }
                }
            }
            ++ y_1;
        } else if (global_search_assignment[y] == 2) {
            if (likely(answer_header_mt_2[x][y_2] != 0)) {
                for (int i=answer_header_mt_2[x][y_2]; i!=0; i=all_answer_ptr_mt_2[i]) {
                    deserialize_id_mt_4(y);
                    output_buffer_mt_4[buffer_index_mt_4] += ',';
                    for (int j=0; j<x_3; ++j) {
                        deserialize_id_mt_4(all_answer_mt_2[i][j]);
                        output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                    }
                }
            }
            ++ y_2;
        } else {
            if (likely(answer_header_mt_3[x][y_3] != 0)) {
                for (int i=answer_header_mt_3[x][y_3]; i!=0; i=all_answer_ptr_mt_3[i]) {
                    deserialize_id_mt_4(y);
                    output_buffer_mt_4[buffer_index_mt_4] += ',';
                    for (int j=0; j<x_3; ++j) {
                        deserialize_id_mt_4(all_answer_mt_3[i][j]);
                        output_buffer_mt_4[buffer_index_mt_4 ++] = j == x_2 ? '\n' : ',';
                    }
                }
            }
            ++ y_3;
        }
    }
    return NULL;
}
// end io thread 4