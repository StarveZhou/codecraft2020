int CACHE_LINE_ALIGN edge_topo_starter_mt_$[MAX_NODE];
inline int decide_search_start_mt_$(int u) {
    while (edge_topo_starter_mt_$[u] < edge_topo_header[u+1] && edge_topo_edges[edge_topo_starter_mt_$[u]] < dfs_path_mt_$[0]) {
        edge_topo_starter_mt_$[u] ++;
    }
    return edge_topo_starter_mt_$[u];
}