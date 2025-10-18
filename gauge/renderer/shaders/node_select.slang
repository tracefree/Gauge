void write_hovered_node(float3 fragment_position) {
    if (globals.mouse_position.x == uint16_t(fragment_position.x) && globals.mouse_position.y == uint16_t(fragment_position.y)) {
        uint index = readback[0].node_handle.load();
        index++;
        readback[index].node_handle = pcs.node_handle;
        readback[index].depth = fragment_position.z;
        readback[0].node_handle++;
    }
}