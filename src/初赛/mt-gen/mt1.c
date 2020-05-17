int CACHE_LINE_ALIGN edge_topo_starter_mt_3[MAX_NODE];
inline int decide_search_start_mt_3(int u) {
    while (edge_topo_starter_mt_3[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_3[u]] < dfs_path_mt_3[0]) {
        edge_topo_starter_mt_3[u] ++;
    }
    return edge_topo_starter_mt_3[u];
}