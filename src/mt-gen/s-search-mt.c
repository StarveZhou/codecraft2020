// search $

int CACHE_LINE_ALIGN sp_dist_mt_$[MAX_NODE];
int CACHE_LINE_ALIGN shortest_path_now_mt_$ = -1;

int CACHE_LINE_ALIGN depth_mt_$, dfs_path_mt_$[8];
int CACHE_LINE_ALIGN all_answer_mt_$[MAX_ANSW + 1][7];
int CACHE_LINE_ALIGN all_answer_ptr_mt_$[MAX_ANSW];
int CACHE_LINE_ALIGN answer_header_mt_$[5][MAX_NODE];
int CACHE_LINE_ALIGN answer_tail_mt_$[5][MAX_NODE];

int CACHE_LINE_ALIGN visit_mt_$[MAX_NODE];
int CACHE_LINE_ALIGN mask_mt_$ = 1111;

int CACHE_LINE_ALIGN node_queue_mt_$[MAX_NODE];

int CACHE_LINE_ALIGN answer_num_mt_$ = 1, answer_contain_num_mt_$ = 0;
int CACHE_LINE_ALIGN answer_contain_mt_$[MAX_NODE];

void shortest_path_mt_$(int u) {
    memset(sp_dist_mt_$, -1, sizeof(int) * node_num);
    sp_dist_mt_$[u] = 0; shortest_path_now_mt_$ = u;

    int v, head = 0, tail = 0, height, orig_u = u;
    bool reach_max;
    node_queue_mt_$[tail ++] = u;
    while (unlikely(head < tail)) {
        u = node_queue_mt_$[head ++];
        height = sp_dist_mt_$[u];
        reach_max = height == MAX_SP_DIST-1;
        for (int i=rev_edge_topo_header[u]; i<rev_edge_topo_header[u+1]; ++i) {
            v = rev_edge_topo_edges[i];
            if (sp_dist_mt_$[v] != -1) continue;
            if (v <= orig_u) break;
            sp_dist_mt_$[v] = height + 1;
            if (reach_max) continue;
            node_queue_mt_$[tail ++] = v;
        }
    }
}

inline int decide_search_start_mt_$(int u) {
    int start = edge_topo_header[u], end = edge_topo_header[u+1];
    if (start == end) return start;
    else if (edge_topo_edges[start] >= dfs_path_mt_$[0]) return start;
    else if (edge_topo_edges[end-1] < dfs_path_mt_$[0]) return end;
    else {
        // 该步是否需要调参，在小于阈值的时候遍历，大于阈值才二分？
        int mid; end --;
        while (start+1 < end) {
            mid = (start + end) >> 1;
            if (edge_topo_edges[mid] < dfs_path_mt_$[0]) start = mid;
            else end = mid;
        }
        return end;
    }
}

void extract_answer_mt_$() {
    for (int i=0; i<depth_mt_$; ++i) {
        all_answer_mt_$[answer_num_mt_$][i] = dfs_path_mt_$[i];
    }
    int x = depth_mt_$ - 3, y = dfs_path_mt_$[0];
    if (unlikely(answer_header_mt_$[x][y] == 0)) {
        answer_header_mt_$[x][y] = answer_tail_mt_$[x][y] = answer_num_mt_$;
    } else {
        all_answer_ptr_mt_$[answer_tail_mt_$[x][y]] = answer_num_mt_$;
        answer_tail_mt_$[x][y] = answer_num_mt_$;
    }
    answer_num_mt_$ ++;
}

void do_search_mt_$() {
    int u, v, mid; 
    u = dfs_path_mt_$[depth_mt_$-1];
    for (int i=decide_search_start_mt_$(u); i<edge_topo_header[u+1]; ++i) {
        v = edge_topo_edges[i];
        if (unlikely(v == dfs_path_mt_$[0])) {
            if (depth_mt_$ >= 3 && depth_mt_$ <= 7) extract_answer_mt_$();
            continue;
        }
        // if (v < dfs_path[0]) continue;
        if (visit_mt_$[v] == mask_mt_$) continue;
        if (depth_mt_$ == 7) continue;
        if (likely(depth_mt_$ + MAX_SP_DIST >= 7)) {
            if (unlikely(shortest_path_now_mt_$ != dfs_path_mt_$[0])) shortest_path_mt_$(dfs_path_mt_$[0]);
            if (likely(sp_dist_mt_$[v] == -1 || sp_dist_mt_$[v] + depth_mt_$ > 7)) continue;
        }
        dfs_path_mt_$[depth_mt_$ ++] = v; visit_mt_$[v] = mask_mt_$;
        do_search_mt_$();
        depth_mt_$ --; visit_mt_$[v] = 0;
    }
}

void* search_mt_$(void* args) {
    int u;
    while (true) {
        u = __sync_fetch_and_add(&global_search_flag, 1);
        if (u >= node_num) break;
        answer_contain_mt_$[answer_contain_num_mt_$ ++] = u;
        if (unlikely(edge_header[u] == edge_header[u+1])) continue;
        if (unlikely(!searchable_nodes[0][u])) continue;
        mask_mt_$ ++; visit_mt_$[u] = mask_mt_$;
        depth_mt_$ = 1; dfs_path_mt_$[0] = u;
        do_search_mt_$();
    }
    return NULL;
}

// end thread $