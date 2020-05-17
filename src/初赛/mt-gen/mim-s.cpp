// search thread $

int CACHE_LINE_ALIGN visit_mt_$[MAX_NODE], mask_mt_$ = 1111;
int CACHE_LINE_ALIGN edge_topo_starter_mt_$[MAX_NODE];
int dfs_path_mt_$[5], dfs_path_num_mt_$;

char CACHE_LINE_ALIGN answer_3_mt_$[MAX_3_ANSW * ONE_INT_CHAR_SIZE * 3];
char CACHE_LINE_ALIGN answer_4_mt_$[MAX_4_ANSW * ONE_INT_CHAR_SIZE * 4];
int answer_3_buffer_num_mt_$, answer_4_buffer_num_mt_$;
int answer_3_num_mt_$ = 0, answer_4_num_mt_$ = 0;

inline void extract_3_answer_mt_$() {
    answer_3_num_mt_$ ++;
    deserialize_id(answer_3_mt_$, answer_3_buffer_num_mt_$, dfs_path_mt_$[0]);
    answer_3_mt_$[answer_3_buffer_num_mt_$ ++] = ',';
    deserialize_id(answer_3_mt_$, answer_3_buffer_num_mt_$, dfs_path_mt_$[1]);
    answer_3_mt_$[answer_3_buffer_num_mt_$ ++] = ',';
    deserialize_id(answer_3_mt_$, answer_3_buffer_num_mt_$, dfs_path_mt_$[2]);
    answer_3_mt_$[answer_3_buffer_num_mt_$ ++] = '\n';
}

inline void extract_4_answer_mt_$() {
    answer_4_num_mt_$ ++;
    deserialize_id(answer_4_mt_$, answer_4_buffer_num_mt_$, dfs_path_mt_$[0]);
    answer_4_mt_$[answer_4_buffer_num_mt_$ ++] = ',';
    deserialize_id(answer_4_mt_$, answer_4_buffer_num_mt_$, dfs_path_mt_$[1]);
    answer_4_mt_$[answer_4_buffer_num_mt_$ ++] = ',';
    deserialize_id(answer_4_mt_$, answer_4_buffer_num_mt_$, dfs_path_mt_$[2]);
    answer_4_mt_$[answer_4_buffer_num_mt_$ ++] = ',';
    deserialize_id(answer_4_mt_$, answer_4_buffer_num_mt_$, dfs_path_mt_$[3]);
    answer_4_mt_$[answer_4_buffer_num_mt_$ ++] = '\n';
}


int backward_3_path_header_mt_$[MAX_NODE];
int backward_3_path_ptr_mt_$[MAX_NODE];
int backward_3_path_value_mt_$[PATH_PAR_3][2];
int backward_3_num_mt_$ = 0;
int backward_contains_mt_$[MAX_NODE], backward_contains_num_mt_$ = 0;

inline void extract_backward_3_path_mt_$() {
    if (backward_3_path_header_mt_$[dfs_path_mt_$[3]] == -1) backward_contains_mt_$[backward_contains_num_mt_$ ++] = dfs_path_mt_$[3];
    backward_3_path_value_mt_$[backward_3_num_mt_$][0] = dfs_path_mt_$[1];
    backward_3_path_value_mt_$[backward_3_num_mt_$][1] = dfs_path_mt_$[2];
    backward_3_path_ptr_mt_$[backward_3_num_mt_$] = backward_3_path_header_mt_$[dfs_path_mt_$[3]];
    backward_3_path_header_mt_$[dfs_path_mt_$[3]] = backward_3_num_mt_$ ++;
}



void backward_dfs_mt_$() {
    int u, v, original_path_num = dfs_path_num_mt_$;
    u = dfs_path_mt_$[dfs_path_num_mt_$ - 1]; dfs_path_num_mt_$ ++;
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_$[0]) break;
        if (visit_mt_$[v] == mask_mt_$) continue;
        dfs_path_mt_$[original_path_num] = v; 
        if (dfs_path_num_mt_$ == 4) {
            extract_backward_3_path_mt_$();
            continue;
        }
        visit_mt_$[v] = mask_mt_$;
        backward_dfs_mt_$();
        visit_mt_$[v] = 0;
    }
    dfs_path_num_mt_$ --;
}

int sorted_backward_3_path_mt_$[PATH_PAR_3];
int sorted_backward_3_path_fin_mt_$[PATH_PAR_3][2];
int sorted_backward_3_path_from_mt_$[MAX_NODE];
int sorted_backward_3_path_to_mt_$[MAX_NODE];
int sorted_backward_id_mt_$[PATH_PAR_3];

void sort_out_mt_$(int begin_with) {
    int i, u, v, num = 0, from, to;
    for (i=0; i<backward_contains_num_mt_$; ++i) {
        u = backward_contains_mt_$[i];
        from = sorted_backward_3_path_from_mt_$[u] = num;
        for (int j=backward_3_path_header_mt_$[u]; j!=-1; j=backward_3_path_ptr_mt_$[j]) {
            sorted_backward_3_path_mt_$[num ++] = j;
        }
        to = sorted_backward_3_path_to_mt_$[u] = num;
        std::sort(sorted_backward_3_path_mt_$ + from, sorted_backward_3_path_mt_$ + to, [](int x, int y) -> bool {
            if (backward_3_path_value_mt_$[x][1] != backward_3_path_value_mt_$[y][1]) return backward_3_path_value_mt_$[x][1] < backward_3_path_value_mt_$[y][1];
            else return backward_3_path_value_mt_$[x][0] < backward_3_path_value_mt_$[y][0];
        });
        for (int j=from; j<to; ++j) {
            sorted_backward_3_path_fin_mt_$[j][0] = backward_3_path_value_mt_$[sorted_backward_3_path_mt_$[j]][0];
            sorted_backward_3_path_fin_mt_$[j][1] = backward_3_path_value_mt_$[sorted_backward_3_path_mt_$[j]][1];
        }
    }
} 


int answer_5_mt_$[MAX_ANSW][5];
int answer_6_mt_$[MAX_ANSW][6];
int answer_7_mt_$[MAX_ANSW][7];
int answer_5_num_mt_$ = 0, answer_6_num_mt_$ = 0, answer_7_num_mt_$ = 0;

int answer_3_header_mt_$[MAX_NODE], answer_3_tailer_mt_$[MAX_NODE];
int answer_4_header_mt_$[MAX_NODE], answer_4_tailer_mt_$[MAX_NODE];
int answer_5_header_mt_$[MAX_NODE], answer_5_tailer_mt_$[MAX_NODE];
int answer_6_header_mt_$[MAX_NODE], answer_6_tailer_mt_$[MAX_NODE];
int answer_7_header_mt_$[MAX_NODE], answer_7_tailer_mt_$[MAX_NODE];

inline void extract_forward_2_path_mt_$() {
    int v = dfs_path_mt_$[2], x, y;
    for (int i=sorted_backward_3_path_from_mt_$[v]; i<sorted_backward_3_path_to_mt_$[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_$[i][0];
        y = sorted_backward_3_path_fin_mt_$[i][1];
        if (visit_mt_$[x] == mask_mt_$ || visit_mt_$[y] == mask_mt_$) continue;
        memcpy(answer_5_mt_$[answer_5_num_mt_$], dfs_path_mt_$, sizeof(int) * 3);
        answer_5_mt_$[answer_5_num_mt_$][3] = y;
        answer_5_mt_$[answer_5_num_mt_$][4] = x;
        answer_5_num_mt_$ ++;
    }
}


inline void extract_forward_3_path_mt_$() {
    int v = dfs_path_mt_$[3], x, y;
    for (int i=sorted_backward_3_path_from_mt_$[v]; i<sorted_backward_3_path_to_mt_$[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_$[i][0];
        y = sorted_backward_3_path_fin_mt_$[i][1];
        if (visit_mt_$[x] == mask_mt_$ || visit_mt_$[y] == mask_mt_$) continue;
        memcpy(answer_6_mt_$[answer_6_num_mt_$], dfs_path_mt_$, sizeof(int) * 4);
        answer_6_mt_$[answer_6_num_mt_$][4] = y;
        answer_6_mt_$[answer_6_num_mt_$][5] = x;
        answer_6_num_mt_$ ++;
    }
}

inline void extract_forward_4_path_mt_$() {
    int v = dfs_path_mt_$[4], x, y;
    for (int i=sorted_backward_3_path_from_mt_$[v]; i<sorted_backward_3_path_to_mt_$[v]; ++i) {
        x = sorted_backward_3_path_fin_mt_$[i][0];
        y = sorted_backward_3_path_fin_mt_$[i][1];
        if (visit_mt_$[x] == mask_mt_$ || visit_mt_$[y] == mask_mt_$) continue;
        memcpy(answer_7_mt_$[answer_7_num_mt_$], dfs_path_mt_$, sizeof(int) * 5);
        answer_7_mt_$[answer_7_num_mt_$][5] = y;
        answer_7_mt_$[answer_7_num_mt_$][6] = x;
        answer_7_num_mt_$ ++;
    }
}

void forward_dfs_mt_$() {
    int u, v, original_path_num = dfs_path_num_mt_$;
    u = dfs_path_mt_$[dfs_path_num_mt_$ - 1];  dfs_path_num_mt_$ ++;
    while (edge_topo_starter_mt_$[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_$[u]] < dfs_path_mt_$[0]) edge_topo_starter_mt_$[u] ++;
    for (int i=edge_topo_starter_mt_$[u]; i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (v == dfs_path_mt_$[0]) {
            if (original_path_num == 3) extract_3_answer_mt_$();
            if (original_path_num == 4) extract_4_answer_mt_$();
            continue;
        }
        if (visit_mt_$[v] == mask_mt_$) continue;
        dfs_path_mt_$[original_path_num] = v; 
        if (backward_3_path_header_mt_$[v] != -1) {
            switch (original_path_num)
            {
            case 2:
                extract_forward_2_path_mt_$();
                break;
            case 3:
                extract_forward_3_path_mt_$();
                break;
            case 4:
                extract_forward_4_path_mt_$();
                break;
            default:
                break;
            }
        }
        if (original_path_num == 4) continue;
        visit_mt_$[v] = mask_mt_$;
        forward_dfs_mt_$();
        visit_mt_$[v] = 0;
    }
    dfs_path_num_mt_$ --;
}

void do_search_mim_mt_$(int begin_with) {
    backward_3_num_mt_$ = 0;
    backward_contains_num_mt_$ = 0;
    // 可以不用memset，用一个visit就好了
    memset(backward_3_path_header_mt_$, -1, sizeof_int_mul_node_num);
    // printf("started with: %d %d\n", begin_with, data_rev_mapping[begin_with]); fflush(stdout);
    dfs_path_mt_$[0] = begin_with; dfs_path_num_mt_$ = 1;
    mask_mt_$ ++; visit_mt_$[begin_with] = mask_mt_$;
    backward_dfs_mt_$();
    sort_out_mt_$(begin_with);
    dfs_path_mt_$[0] = begin_with; dfs_path_num_mt_$ = 1;
    mask_mt_$ ++; visit_mt_$[begin_with] = mask_mt_$;

    answer_3_header_mt_$[begin_with] = answer_3_buffer_num_mt_$;
    answer_4_header_mt_$[begin_with] = answer_4_buffer_num_mt_$;
    answer_5_header_mt_$[begin_with] = answer_5_num_mt_$;
    answer_6_header_mt_$[begin_with] = answer_6_num_mt_$;
    answer_7_header_mt_$[begin_with] = answer_7_num_mt_$;

    forward_dfs_mt_$();
    
    answer_3_tailer_mt_$[begin_with] = answer_3_buffer_num_mt_$;
    answer_4_tailer_mt_$[begin_with] = answer_4_buffer_num_mt_$;
    answer_5_tailer_mt_$[begin_with] = answer_5_num_mt_$;
    answer_6_tailer_mt_$[begin_with] = answer_6_num_mt_$;
    answer_7_tailer_mt_$[begin_with] = answer_7_num_mt_$;    
}

void* search_mt_$(void* args) {
    memcpy(edge_topo_starter_mt_$, edge_topo_header, sizeof_int_mul_node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // NOTE: this should be replaced
        global_search_assignment[u] = $;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        do_search_mim_mt_$(u);
    }
    return NULL;
}

// search thread $