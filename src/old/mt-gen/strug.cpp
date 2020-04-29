// thread $

int answer_mt_$[5][MAX_ANSW * 7];
int answer_num_mt_$[5];
int answer_header_mt_$[5][MAX_NODE], answer_tailer_mt_$[5][MAX_NODE];
int total_answer_num_mt_$ = 0;

int edge_topo_starter_mt_$[MAX_NODE];

int visit_mt_$[MAX_NODE], mask_mt_$;
int dfs_path_mt_$[8], dfs_path_num_mt_$;

int front_visit_mt_$[MAX_NODE], front_mask_mt_$;

int back_visit_mt_$[MAX_NODE], back_mask_mt_$;
int back_search_edges_mt_$[MAX_BACK][3];
int back_search_edges_num_mt_$;
int back_search_contains_mt_$[MAX_NODE], back_search_contains_num_mt_$;
int back_search_header_mt_$[MAX_NODE], back_search_ptr_mt_$[MAX_BACK];
int back_search_tailer_mt_$[MAX_NODE];

int back_search_answer_mt_$[2][MAX_BACK][3];
int back_search_answer_num_mt_$[2];

void back_dfs_mt_$() {
    int u, v, tid;
    u = dfs_path_mt_$[dfs_path_num_mt_$ - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_$[0]) break;
        if (visit_mt_$[v] == mask_mt_$) continue;
        if (dfs_path_num_mt_$ == 2) {
            if (front_visit_mt_$[v] == front_mask_mt_$) {
                tid = back_search_answer_num_mt_$[0];
                back_search_answer_mt_$[0][tid][0] = v;
                back_search_answer_mt_$[0][tid][1] = dfs_path_mt_$[1];
                back_search_answer_num_mt_$[0] ++;
            }
        }
        if (dfs_path_num_mt_$ == 3) {
            if (front_visit_mt_$[v] == front_mask_mt_$) {
                tid = back_search_answer_num_mt_$[1];
                back_search_answer_mt_$[1][tid][0] = v;
                back_search_answer_mt_$[1][tid][1] = dfs_path_mt_$[2];
                back_search_answer_mt_$[1][tid][2] = dfs_path_mt_$[1];
                back_search_answer_num_mt_$[1] ++;
            }
            if (back_visit_mt_$[v] != back_mask_mt_$) {
                back_search_header_mt_$[v] = -1;
                back_search_contains_mt_$[back_search_contains_num_mt_$ ++] = v;
            }
            back_visit_mt_$[v] = back_mask_mt_$;

            tid = back_search_edges_num_mt_$;
            back_search_edges_mt_$[tid][0] = dfs_path_mt_$[2];
            back_search_edges_mt_$[tid][1] = dfs_path_mt_$[1];
            back_search_ptr_mt_$[tid] = back_search_header_mt_$[v];
            back_search_header_mt_$[v] = tid;
            back_search_edges_num_mt_$ ++;
            continue;
        }
        visit_mt_$[v] = mask_mt_$;
        dfs_path_mt_$[dfs_path_num_mt_$ ++] = v;
        back_dfs_mt_$();
        visit_mt_$[v] = -1;
        dfs_path_num_mt_$ --;
    }
}

int back_search_id_mt_$[MAX_BACK];

void sort_out_mt_$(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_$[0] > 0) {
        tnum = back_search_answer_num_mt_$[0];
        total_answer_num_mt_$ += tnum;
        memcpy(back_search_id_mt_$, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_$, back_search_id_mt_$ + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_$[0][i][0] != back_search_answer_mt_$[0][j][0]) return back_search_answer_mt_$[0][i][0] < back_search_answer_mt_$[0][j][0];
            return back_search_answer_mt_$[0][i][1] < back_search_answer_mt_$[0][j][1];
        });
        answer_header_mt_$[0][begin_with] = answer_num_mt_$[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_$[i];
            answer_mt_$[0][answer_num_mt_$[0] ++] = back_search_answer_mt_$[0][id][0];
            answer_mt_$[0][answer_num_mt_$[0] ++] = back_search_answer_mt_$[0][id][1];
        }
        answer_tailer_mt_$[0][begin_with] = answer_num_mt_$[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_$[1] > 0) {
        tnum = back_search_answer_num_mt_$[1];
        total_answer_num_mt_$ += tnum;
        memcpy(back_search_id_mt_$, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_$, back_search_id_mt_$ + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_$[1][i][0] != back_search_answer_mt_$[1][j][0]) return back_search_answer_mt_$[1][i][0] < back_search_answer_mt_$[1][j][0];
            if (back_search_answer_mt_$[1][i][1] != back_search_answer_mt_$[1][j][1]) return back_search_answer_mt_$[1][i][1] < back_search_answer_mt_$[1][j][1];
            return back_search_answer_mt_$[1][i][2] < back_search_answer_mt_$[1][j][2];
        });
        answer_header_mt_$[1][begin_with] = answer_num_mt_$[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_$[i];
            answer_mt_$[1][answer_num_mt_$[1] ++] = back_search_answer_mt_$[1][id][0];
            answer_mt_$[1][answer_num_mt_$[1] ++] = back_search_answer_mt_$[1][id][1];
            answer_mt_$[1][answer_num_mt_$[1] ++] = back_search_answer_mt_$[1][id][2];
        }
        answer_tailer_mt_$[1][begin_with] = answer_num_mt_$[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_$ > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_$; ++i) {
            u = back_search_contains_mt_$[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_$[u]; j!=-1; j=back_search_ptr_mt_$[j]) {
                back_search_id_mt_$[tnum ++] = j;
            }
            back_search_header_mt_$[u] = last_tnum;
            back_search_tailer_mt_$[u] = tnum;
            std::sort(back_search_id_mt_$ + last_tnum, back_search_id_mt_$ + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_$[i][0] != back_search_edges_mt_$[j][0]) return back_search_edges_mt_$[i][0] < back_search_edges_mt_$[j][0];
                return back_search_edges_mt_$[i][1] < back_search_edges_mt_$[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_$(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_$[u]; i<back_search_tailer_mt_$[u]; ++i) {
        id = back_search_id_mt_$[i];
        x = back_search_edges_mt_$[id][0]; y = back_search_edges_mt_$[id][1];
        if (visit_mt_$[x] == mask_mt_$ || visit_mt_$[y] == mask_mt_$) continue;
        for (int j=1; j<dfs_path_num_mt_$; ++j) {
            answer_mt_$[dfs_path_num_mt_$][answer_num_mt_$[dfs_path_num_mt_$] ++] = dfs_path_mt_$[j];
        }
        answer_mt_$[dfs_path_num_mt_$][answer_num_mt_$[dfs_path_num_mt_$] ++] = u;
        answer_mt_$[dfs_path_num_mt_$][answer_num_mt_$[dfs_path_num_mt_$] ++] = x;
        answer_mt_$[dfs_path_num_mt_$][answer_num_mt_$[dfs_path_num_mt_$] ++] = y;
        total_answer_num_mt_$ ++;
    }
}

void forw_dfs_mt_$() {
    int u, v;
    u = dfs_path_mt_$[dfs_path_num_mt_$ - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_$); fflush(stdout);
    while (edge_topo_starter_mt_$[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_$[u]] <= dfs_path_mt_$[0]) edge_topo_starter_mt_$[u] ++;
    for (int i=edge_topo_starter_mt_$[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_$[v] == mask_mt_$) continue;
        if (dfs_path_num_mt_$ >= 2 && dfs_path_num_mt_$ <= 4) {
            if (back_visit_mt_$[v] == back_mask_mt_$) {
                extract_answer_mt_$(v);
            }
        }
        
        if (dfs_path_num_mt_$ == 4) continue;

        visit_mt_$[v] = mask_mt_$;
        dfs_path_mt_$[dfs_path_num_mt_$ ++] = v;
        forw_dfs_mt_$();
        visit_mt_$[v] = -1;
        dfs_path_num_mt_$ --;
    }
}

void do_search_mt_$(int begin_with) {
    front_mask_mt_$ ++;
    while (edge_topo_starter_mt_$[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_$[begin_with]] <= begin_with) edge_topo_starter_mt_$[begin_with] ++;
    for (int i=edge_topo_starter_mt_$[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_$[edge_topo_edges[i]] = front_mask_mt_$;
    }

    back_search_edges_num_mt_$ = back_search_contains_num_mt_$ = 0;
    back_search_answer_num_mt_$[0] = back_search_answer_num_mt_$[1] = 0;
    back_mask_mt_$ ++; back_visit_mt_$[begin_with] = back_mask_mt_$;
    mask_mt_$ ++; visit_mt_$[begin_with] = mask_mt_$;
    dfs_path_mt_$[0] = begin_with; dfs_path_num_mt_$ = 1;
    back_dfs_mt_$();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_$(begin_with);
    if (back_search_edges_num_mt_$ == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_$ ++; visit_mt_$[begin_with] = mask_mt_$;
    dfs_path_mt_$[0] = begin_with; dfs_path_num_mt_$ = 1;

    answer_header_mt_$[2][begin_with] = answer_num_mt_$[2];
    answer_header_mt_$[3][begin_with] = answer_num_mt_$[3];
    answer_header_mt_$[4][begin_with] = answer_num_mt_$[4];
    forw_dfs_mt_$();
    answer_tailer_mt_$[2][begin_with] = answer_num_mt_$[2];
    answer_tailer_mt_$[3][begin_with] = answer_num_mt_$[3];
    answer_tailer_mt_$[4][begin_with] = answer_num_mt_$[4];
    // printf("all over %d\n", total_answer_num_mt_$); fflush(stdout);
}

void* search_mt_$(void* args) {
    memcpy(edge_topo_starter_mt_$, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_$[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = $;
        do_search_mt_$(u);
    }
    return NULL;
}

// thread $