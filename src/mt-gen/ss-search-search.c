// search 3

int CACHE_LINE_ALIGN sp_dist_mt_3[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_3 = -1;

int CACHE_LINE_ALIGN depth_mt_3, dfs_path_mt_3[8];
int CACHE_LINE_ALIGN all_answer_mt_3[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_3[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_3[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_3[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_3[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_3 = 1111;

int CACHE_LINE_ALIGN node_queue_mt_3[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_3 = 1, answer_contain_num_mt_3 = 0, answer_mapper_id_mt_3 = 0;

void shortest_path_mt_3(int u) {
    memset(sp_dist_mt_3, -1, sizeof(int) * node_num);
    sp_dist_mt_3[u] = 0; shortest_path_now_mt_3 = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_3[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_3[head ++];
        height = sp_dist_mt_3[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_3[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_3[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_3[tail ++] = v;
        }
    }
}

int CACHE_LINE_ALIGN edge_topo_starter_mt_3[MAX_NODE];
inline int decide_search_start_mt_3(int u) {
    while (edge_topo_starter_mt_3[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_3[u]] < dfs_path_mt_3[0]) {
        edge_topo_starter_mt_3[u] ++;
    }
    return edge_topo_starter_mt_3[u];
}

void extract_answer_mt_3() {
    memcpy(all_answer_mt_3, dfs_path_mt_3 + 1, depth_mt_3-1);
    int x = depth_mt_3 - 3, y = answer_mapper_id_mt_3;
    if (unlikely(answer_header_mt_3[x][y] == 0)) {
        answer_header_mt_3[x][y] = answer_tail_mt_3[x][y] = answer_num_mt_3;
    } else {
        all_answer_ptr_mt_3[answer_tail_mt_3[x][y]] = answer_num_mt_3;
        answer_tail_mt_3[x][y] = answer_num_mt_3;
    }
    answer_num_mt_3 ++;
}

void do_search_mt_3() {
    int u, v, mid; 
    u = dfs_path_mt_3[depth_mt_3-1];
    for (int i=decide_search_start_mt_3(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_3[0])) {
            if (depth_mt_3 >= 3 && depth_mt_3 <= 7) extract_answer_mt_3();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_3[v] == mask_mt_3) continue;
        if (depth_mt_3 == 7) continue;
        if (likely(depth_mt_3 + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_3 != dfs_path_mt_3[0])) shortest_path_mt_3(dfs_path_mt_3[0]);
            if (likely(sp_dist_mt_3[v] == -1 || sp_dist_mt_3[v] + depth_mt_3 > 7)) continue;
        }
        dfs_path_mt_3[depth_mt_3 ++] = v; visit_mt_3[v] = mask_mt_3;
        do_search_mt_3();
        depth_mt_3 --; visit_mt_3[v] = 0;
    }
}

void* search_mt_3(void* args) {
    memcpy(edge_topo_starter_mt_3, edge_topo_header, sizeof(int) * node_num);
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        // answer_contain_mt_3[answer_contain_num_mt_3 ++] = u;
        global_search_assignment[u] = 3;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_3 ++; visit_mt_3[u] = mask_mt_3;
        depth_mt_3 = 1; dfs_path_mt_3[0] = u;
        do_search_mt_3();
        answer_mapper_id_mt_3 ++;
    }
    return NULL;
}

// end thread 3