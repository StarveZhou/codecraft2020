// thread 5

int answer_mt_5[5][MAX_ANSW * 7];
int answer_num_mt_5[5];
int answer_header_mt_5[5][MAX_NODE], answer_tailer_mt_5[5][MAX_NODE];
int total_answer_num_mt_5 = 0;

int edge_topo_starter_mt_5[MAX_NODE];

int visit_mt_5[MAX_NODE], mask_mt_5;
int dfs_path_mt_5[8], dfs_path_num_mt_5;

int front_visit_mt_5[MAX_NODE], front_mask_mt_5;

int back_visit_mt_5[MAX_NODE], back_mask_mt_5;
int back_search_edges_mt_5[MAX_BACK][3];
int back_search_edges_num_mt_5;
int back_search_contains_mt_5[MAX_NODE], back_search_contains_num_mt_5;
int back_search_header_mt_5[MAX_NODE], back_search_ptr_mt_5[MAX_BACK];
int back_search_tailer_mt_5[MAX_NODE];

int back_search_answer_mt_5[2][MAX_BACK][3];
int back_search_answer_num_mt_5[2];

void back_dfs_mt_5() {
    int u, v, tid;
    u = dfs_path_mt_5[dfs_path_num_mt_5 - 1];
    for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
        v = rev_edge_topo_edges[i];
        if (v <= dfs_path_mt_5[0]) break;
        if (visit_mt_5[v] == mask_mt_5) continue;
        if (dfs_path_num_mt_5 == 2) {
            if (front_visit_mt_5[v] == front_mask_mt_5) {
                tid = back_search_answer_num_mt_5[0];
                back_search_answer_mt_5[0][tid][0] = v;
                back_search_answer_mt_5[0][tid][1] = dfs_path_mt_5[1];
                back_search_answer_num_mt_5[0] ++;
            }
        }
        if (dfs_path_num_mt_5 == 3) {
            if (front_visit_mt_5[v] == front_mask_mt_5) {
                tid = back_search_answer_num_mt_5[1];
                back_search_answer_mt_5[1][tid][0] = v;
                back_search_answer_mt_5[1][tid][1] = dfs_path_mt_5[2];
                back_search_answer_mt_5[1][tid][2] = dfs_path_mt_5[1];
                back_search_answer_num_mt_5[1] ++;
            }
            if (back_visit_mt_5[v] != back_mask_mt_5) {
                back_search_header_mt_5[v] = -1;
                back_search_contains_mt_5[back_search_contains_num_mt_5 ++] = v;
            }
            back_visit_mt_5[v] = back_mask_mt_5;

            tid = back_search_edges_num_mt_5;
            back_search_edges_mt_5[tid][0] = dfs_path_mt_5[2];
            back_search_edges_mt_5[tid][1] = dfs_path_mt_5[1];
            back_search_ptr_mt_5[tid] = back_search_header_mt_5[v];
            back_search_header_mt_5[v] = tid;
            back_search_edges_num_mt_5 ++;
            continue;
        }
        visit_mt_5[v] = mask_mt_5;
        dfs_path_mt_5[dfs_path_num_mt_5 ++] = v;
        back_dfs_mt_5();
        visit_mt_5[v] = -1;
        dfs_path_num_mt_5 --;
    }
}

int back_search_id_mt_5[MAX_BACK];

void sort_out_mt_5(int begin_with) {
    int tnum, id;
    if (back_search_answer_num_mt_5[0] > 0) {
        tnum = back_search_answer_num_mt_5[0];
        total_answer_num_mt_5 += tnum;
        memcpy(back_search_id_mt_5, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_5, back_search_id_mt_5 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_5[0][i][0] != back_search_answer_mt_5[0][j][0]) return back_search_answer_mt_5[0][i][0] < back_search_answer_mt_5[0][j][0];
            return back_search_answer_mt_5[0][i][1] < back_search_answer_mt_5[0][j][1];
        });
        answer_header_mt_5[0][begin_with] = answer_num_mt_5[0];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_5[i];
            answer_mt_5[0][answer_num_mt_5[0] ++] = back_search_answer_mt_5[0][id][0];
            answer_mt_5[0][answer_num_mt_5[0] ++] = back_search_answer_mt_5[0][id][1];
        }
        answer_tailer_mt_5[0][begin_with] = answer_num_mt_5[0];
    }
    // printf("X\n"); fflush(stdout);
    if (back_search_answer_mt_5[1] > 0) {
        tnum = back_search_answer_num_mt_5[1];
        total_answer_num_mt_5 += tnum;
        memcpy(back_search_id_mt_5, std_id_arr, sizeof(int) * tnum);
        std::sort(back_search_id_mt_5, back_search_id_mt_5 + tnum, [](int i, int j) -> bool{
            if (back_search_answer_mt_5[1][i][0] != back_search_answer_mt_5[1][j][0]) return back_search_answer_mt_5[1][i][0] < back_search_answer_mt_5[1][j][0];
            if (back_search_answer_mt_5[1][i][1] != back_search_answer_mt_5[1][j][1]) return back_search_answer_mt_5[1][i][1] < back_search_answer_mt_5[1][j][1];
            return back_search_answer_mt_5[1][i][2] < back_search_answer_mt_5[1][j][2];
        });
        answer_header_mt_5[1][begin_with] = answer_num_mt_5[1];
        for (int i=0; i<tnum; ++i) {
            id = back_search_id_mt_5[i];
            answer_mt_5[1][answer_num_mt_5[1] ++] = back_search_answer_mt_5[1][id][0];
            answer_mt_5[1][answer_num_mt_5[1] ++] = back_search_answer_mt_5[1][id][1];
            answer_mt_5[1][answer_num_mt_5[1] ++] = back_search_answer_mt_5[1][id][2];
        }
        answer_tailer_mt_5[1][begin_with] = answer_num_mt_5[1];
    }
    // printf("Y\n"); fflush(stdout);
    if (back_search_edges_num_mt_5 > 0) {
        int u, last_tnum; tnum = 0;
        for (int i=0; i<back_search_contains_num_mt_5; ++i) {
            u = back_search_contains_mt_5[i];
            last_tnum = tnum;
            for (int j=back_search_header_mt_5[u]; j!=-1; j=back_search_ptr_mt_5[j]) {
                back_search_id_mt_5[tnum ++] = j;
            }
            back_search_header_mt_5[u] = last_tnum;
            back_search_tailer_mt_5[u] = tnum;
            std::sort(back_search_id_mt_5 + last_tnum, back_search_id_mt_5 + tnum, [](int i, int j) -> bool{
                if (back_search_edges_mt_5[i][0] != back_search_edges_mt_5[j][0]) return back_search_edges_mt_5[i][0] < back_search_edges_mt_5[j][0];
                return back_search_edges_mt_5[i][1] < back_search_edges_mt_5[j][1];
            });
        }
    }
    // printf("Z\n"); fflush(stdout);
}

void inline extract_answer_mt_5(int u) {
    int id, x, y;
    for (int i=back_search_header_mt_5[u]; i<back_search_tailer_mt_5[u]; ++i) {
        id = back_search_id_mt_5[i];
        x = back_search_edges_mt_5[id][0]; y = back_search_edges_mt_5[id][1];
        if (visit_mt_5[x] == mask_mt_5 || visit_mt_5[y] == mask_mt_5) continue;
        for (int j=1; j<dfs_path_num_mt_5; ++j) {
            answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = dfs_path_mt_5[j];
        }
        answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = u;
        answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = x;
        answer_mt_5[dfs_path_num_mt_5][answer_num_mt_5[dfs_path_num_mt_5] ++] = y;
        total_answer_num_mt_5 ++;
    }
}

void forw_dfs_mt_5() {
    int u, v;
    u = dfs_path_mt_5[dfs_path_num_mt_5 - 1];
    // printf("search %d =%d\n", u, dfs_path_num_mt_5); fflush(stdout);
    while (edge_topo_starter_mt_5[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_5[u]] <= dfs_path_mt_5[0]) edge_topo_starter_mt_5[u] ++;
    for (int i=edge_topo_starter_mt_5[u]; i<edge_topo_header[u+1]; ++i) {
        // printf("here: %d\n", i); fflush(stdout);
        v = edge_topo_edges[i];
        if (visit_mt_5[v] == mask_mt_5) continue;
        if (dfs_path_num_mt_5 >= 2 && dfs_path_num_mt_5 <= 4) {
            if (back_visit_mt_5[v] == back_mask_mt_5) {
                extract_answer_mt_5(v);
            }
        }
        
        if (dfs_path_num_mt_5 == 4) continue;

        visit_mt_5[v] = mask_mt_5;
        dfs_path_mt_5[dfs_path_num_mt_5 ++] = v;
        forw_dfs_mt_5();
        visit_mt_5[v] = -1;
        dfs_path_num_mt_5 --;
    }
}

void do_search_mt_5(int begin_with) {
    front_mask_mt_5 ++;
    while (edge_topo_starter_mt_5[begin_with] < edge_topo_header[begin_with+1] && edge_topo_edges[edge_topo_starter_mt_5[begin_with]] <= begin_with) edge_topo_starter_mt_5[begin_with] ++;
    for (int i=edge_topo_starter_mt_5[begin_with]; i<edge_topo_header[begin_with+1]; ++i) {
        front_visit_mt_5[edge_topo_edges[i]] = front_mask_mt_5;
    }

    back_search_edges_num_mt_5 = back_search_contains_num_mt_5 = 0;
    back_search_answer_num_mt_5[0] = back_search_answer_num_mt_5[1] = 0;
    back_mask_mt_5 ++; back_visit_mt_5[begin_with] = back_mask_mt_5;
    mask_mt_5 ++; visit_mt_5[begin_with] = mask_mt_5;
    dfs_path_mt_5[0] = begin_with; dfs_path_num_mt_5 = 1;
    back_dfs_mt_5();
    // printf("back search over\n"); fflush(stdout);
    sort_out_mt_5(begin_with);
    if (back_search_edges_num_mt_5 == 0) return;
    // printf("back over\n"); fflush(stdout);
    mask_mt_5 ++; visit_mt_5[begin_with] = mask_mt_5;
    dfs_path_mt_5[0] = begin_with; dfs_path_num_mt_5 = 1;

    answer_header_mt_5[2][begin_with] = answer_num_mt_5[2];
    answer_header_mt_5[3][begin_with] = answer_num_mt_5[3];
    answer_header_mt_5[4][begin_with] = answer_num_mt_5[4];
    forw_dfs_mt_5();
    answer_tailer_mt_5[2][begin_with] = answer_num_mt_5[2];
    answer_tailer_mt_5[3][begin_with] = answer_num_mt_5[3];
    answer_tailer_mt_5[4][begin_with] = answer_num_mt_5[4];
    // printf("all over %d\n", total_answer_num_mt_5); fflush(stdout);
}

void* search_mt_5(void* args) {
    memcpy(edge_topo_starter_mt_5, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_counter, 1);
        if (u >= node_num) break;
        global_assign[u] = -1;
        if (edge_topo_starter_mt_5[u] == edge_topo_header[u+1]) continue;
        if (!searchable_nodes[0][u]) continue;
        global_assign[u] = 5;
        do_search_mt_5(u);
    }
    return NULL;
}

// thread 5