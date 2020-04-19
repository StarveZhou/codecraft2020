// io thread 11
char CACHE_LINE_ALIGN output_buffer_mt_11[ONE_INT_CHAR_SIZE * MAX_ANSW * 4 + 100];
int CACHE_LINE_ALIGN buffer_index_mt_11 = 0;
inline void deserialize_int_mt_11(int x) {
    if (x == 0) {
        output_buffer_mt_11[buffer_index_mt_11 ++] = '0';
        return;
    }
    int orignial_index = buffer_index_mt_11;
    while (x) {
        output_buffer_mt_11[buffer_index_mt_11 ++] = (x % 10) + '0';
        x /= 10;
    }
    for (int i=orignial_index, j=buffer_index_mt_11-1; i<j; ++i, --j) {
        orignial_index = output_buffer_mt_11[i];
        output_buffer_mt_11[i] = output_buffer_mt_11[j];
        output_buffer_mt_11[j] = orignial_index;
    }
}
inline void deserialize_id_mt_11(int x) {
    int orginal_index = buffer_index_mt_11;
    memcpy(output_buffer_mt_11 + buffer_index_mt_11, integer_buffer[x], integer_buffer_size[x] * sizeof(char));
    buffer_index_mt_11 += integer_buffer_size[x];
}
void* deserialize_ans_mt_11(void* args) {
    int x, x_2, x_3;
    int begin, end;

    x = 4;
    x_2 = x+2, x_3 = x+3;
    begin = node_num/2; end = node_num;
    for (int y=begin; y<end; ++y) {
        switch (global_search_assignment[y])
        {
        case 0:
            if (unlikely(answer_header_mt_0[x][y] == 0)) continue;
            for (int i=answer_header_mt_0[x][y]; i!=0; i=all_answer_ptr_mt_0[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_0[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 1:
            if (unlikely(answer_header_mt_1[x][y] == 0)) continue;
            for (int i=answer_header_mt_1[x][y]; i!=0; i=all_answer_ptr_mt_1[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_1[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 2:
            if (unlikely(answer_header_mt_2[x][y] == 0)) continue;
            for (int i=answer_header_mt_2[x][y]; i!=0; i=all_answer_ptr_mt_2[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_2[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 3:
            if (unlikely(answer_header_mt_3[x][y] == 0)) continue;
            for (int i=answer_header_mt_3[x][y]; i!=0; i=all_answer_ptr_mt_3[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_3[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 4:
            if (unlikely(answer_header_mt_4[x][y] == 0)) continue;
            for (int i=answer_header_mt_4[x][y]; i!=0; i=all_answer_ptr_mt_4[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_4[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 5:
            if (unlikely(answer_header_mt_5[x][y] == 0)) continue;
            for (int i=answer_header_mt_5[x][y]; i!=0; i=all_answer_ptr_mt_5[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_5[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 6:
            if (unlikely(answer_header_mt_6[x][y] == 0)) continue;
            for (int i=answer_header_mt_6[x][y]; i!=0; i=all_answer_ptr_mt_6[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_6[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;
        case 7:
            if (unlikely(answer_header_mt_7[x][y] == 0)) continue;
            for (int i=answer_header_mt_7[x][y]; i!=0; i=all_answer_ptr_mt_7[i]) {
                for (int j=0; j<x_3; ++j) {
                    deserialize_id_mt_11(all_answer_mt_7[i][j]);
                    output_buffer_mt_11[buffer_index_mt_11 ++] = j == x_2 ? '\n' : ',';
                }
            }
            break;        
        default:
            break;
        }
    }

    return NULL;
}
// end io thread 11

