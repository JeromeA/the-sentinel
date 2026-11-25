/* Shared polygon workspace aliases for the primary span walker.
 * Geometry setup lives in render_geometry.c. */

#define polygon_edge_end_x (((struct UnalignedWord *)&edge_end_x)->value)
#define polygon_edge_end_y (((struct UnalignedWord *)&edge_end_y)->value)

extern uint8_t terrain_triangle_vertex_delta[];
