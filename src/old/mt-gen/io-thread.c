char buffer_io_$[ONE_INT_CHAR_SIZE * MAX_ANSW * 7];
int buffer_num_io_$;

void* write_to_disk_io_$(void* args) {
    deserialize_int(buffer_io_$, buffer_num_io_$, answer_num);
    buffer_io_$[buffer_num_io_$ ++] = '\n';
    write_to_disk(buffer_io_$, buffer_num_io_$, 0, 0, node_num);
    return NULL;
}