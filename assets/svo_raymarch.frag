#version 460 core

out vec4 frag_color;

uniform ivec3 u_data_size;
uniform vec3 u_upper_left;
uniform vec3 u_delta_width;
uniform vec3 u_delta_height;
uniform vec3 u_camera_origin;

const uint EMPTY_VOXEL = 0;

struct SVONode {
    uint mask;
    uint children_pointer;
};

layout (std430, binding = 0) readonly buffer SVONodes {
    SVONode data[];
} nodes;

layout (std430, binding = 1) readonly buffer SVOColours {
    uint data[];
} node_colours;

layout (std430, binding = 2) readonly buffer ColoursBuffer {
    vec4 data[];
} colours;

bool should_cull(vec3 direction, vec3 origin) {
    vec3 inv_direction = 1.0f / direction;

    vec3 t0 = (-origin) * inv_direction;
    vec3 t1 = (u_data_size - origin) * inv_direction;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    float max_tmin = max(max(tmin.x, tmin.y), tmin.z);
    float min_tmax = min(min(tmax.x, tmax.y), tmax.z);

    return max_tmin > min_tmax || (max_tmin < 0 && min_tmax < 0);
}

uint position_to_index(ivec3 position) {
    return position.x + (position.y * u_data_size.x) + (position.z * u_data_size.x * u_data_size.y);
}

ivec3 init_step(vec3 direction) {
    return ivec3(sign(direction));
}

vec3 start_position(vec3 direction, vec3 origin) {
    vec3 inv_direction = 1.0f / direction;

    vec3 t0 = (-origin) * inv_direction;
    vec3 t1 = (u_data_size - origin) * inv_direction;

    vec3 tmin = min(t0, t1);
    float max_tmin = max(max(tmin.x, tmin.y), tmin.z);

    return (max_tmin + 1e-2) * direction + origin;
}

uint child_mask_index(ivec3 position) {
    return (position.x & 0x1) << 2 | (position.y & 0x1) << 1 | (position.z & 0x1);
}

bool is_child_set(uint node, ivec3 position) {
    uint mask = 1 << child_mask_index(position);

    return ((nodes.data[node].mask >> 8) & mask) != 0;
}

bool is_leaf_set(uint node, ivec3 position) {
    uint mask = 1 << child_mask_index(position);

    return (nodes.data[node].mask & mask) != 0;
}

uint child_index(uint node, ivec3 position) {
    uint index = 0;
    uint mask_index = child_mask_index(position);

    uint mask = 0x1;
    uint children = nodes.data[node].mask >> 8;

    for(int i = 0; i < mask_index; i++) {
        if((children & mask) != 0) index++;
        mask = mask << 1;
    }

    return index;
}

uint leaf_index(uint node, ivec3 position) {
    uint index = 0;
    uint mask_index = child_mask_index(position);

    uint mask = 0x1;
    uint leaves = nodes.data[node].mask;

    for(int i = 0; i < mask_index; i++) {
        if((leaves & mask) != 0) index++;
        mask = mask << 1;
    }

    return index;
}

ivec3 round_multiple(ivec3 val, int multiple) {
    return val + ivec3(multiple) - (val % ivec3(multiple));
}

vec4 get_ray_color(vec3 direction, vec3 origin) {
    if(should_cull(direction, origin)) {
        return vec4(0.2f, 0.2f, 0.2f, 1.0f);
    }

    uint current_node = 0;
    int current_block_size = u_data_size.x / 2;

    direction = vec3(
        abs(direction.x) < 1e-8 ? (direction.x < 0.0f ? -1e-8 : 1e-8) : direction.x,
        abs(direction.y) < 1e-8 ? (direction.y < 0.0f ? -1e-8 : 1e-8) : direction.y,
        abs(direction.z) < 1e-8 ? (direction.z < 0.0f ? -1e-8 : 1e-8) : direction.z
    );

    const ivec3 step = init_step(direction);

    vec3 direction_original = direction;

    ivec3 current_position = ivec3(floor(origin));
    if(any(lessThan(current_position, ivec3(0, 0, 0))) || any(greaterThanEqual(current_position, u_data_size))) {
        origin = start_position(direction, origin);
    }

    vec3 current_origin = origin / current_block_size;
    vec3 current_direction = direction / current_block_size;

    vec3 tdelta = abs(1.0f / current_direction);

    ivec3 child_pos;
    current_position = ivec3(floor(current_origin));

    vec3 tmax = (current_position - current_origin + max(step, 0)) / current_direction;

    bvec3 mask = lessThanEqual(tmax.xyz, min(tmax.yzx, tmax.zxy));

    vec3 intersect = current_origin * current_block_size;
    uint layer = 0;
    
    if(all(greaterThanEqual(current_position, ivec3(0, 0, 0))) && all(lessThan(current_position, u_data_size))) {
        while(is_child_set(current_node, current_position) && current_block_size != 1) {
            layer += 1;

            uint index = child_index(current_node, current_position);
            current_node = nodes.data[current_node].children_pointer + index;

            current_block_size /= 2;
            current_origin = intersect / current_block_size;
            current_direction = direction_original / current_block_size;

            tdelta = abs(1.0f / current_direction);

            current_position = ivec3(floor(current_origin));
            tmax = (current_position - current_origin + max(step, 0)) / current_direction;
            current_position = ivec3(mod(current_position, 2));
        }
    }

    bool exit = false;

    for(int i = 0; i < 512; ++i) {
        bool in_bounds = all(greaterThanEqual(current_position, ivec3(0))) && all(lessThan(current_position, ivec3(2)));

        if(!in_bounds && layer == 0) {
             return vec4(0.2f, 0.2f, 0.2f, 1.0f);
        }

        if(exit) {
            current_node = 0;
            layer = 0;

            current_block_size = u_data_size.x / 2;
            current_direction = direction_original / current_block_size;
            current_origin = intersect / current_block_size;

            tdelta = abs(1.0f / current_direction);

            current_position = ivec3(floor(current_origin));
            tmax = (current_position - current_origin + max(step, 0)) / current_direction;
            in_bounds = all(greaterThanEqual(current_position, ivec3(0))) && all(lessThan(current_position, ivec3(2)));

            exit = false;
        }

        while(is_child_set(current_node, current_position) && current_block_size > 1 && in_bounds) {
            layer += 1;

            uint index = child_index(current_node, current_position);
            current_node = nodes.data[current_node].children_pointer + index;

            current_block_size /= 2;
            current_direction = direction_original / current_block_size;
            current_origin = intersect / current_block_size;

            tdelta = abs(1.0f / current_direction);

            current_position = ivec3(floor(current_origin));
            tmax = (current_position - current_origin + max(step, 0)) / current_direction;
            current_position = ivec3(mod(current_position, 2));
        }

        if(is_leaf_set(current_node, current_position)) {
            uint index = leaf_index(current_node, current_position);
            return colours.data[node_colours.data[nodes.data[current_node].children_pointer + index]];
        }

        float previous_t = min(min(tmax.x, tmax.y), tmax.z);
        intersect = (current_origin + current_direction * previous_t + step * 1e-4) * current_block_size;

        mask = lessThanEqual(tmax.xyz, min(tmax.yzx, tmax.zxy));

        current_position += step * ivec3(mask);
        tmax += tdelta * vec3(mask);

        exit = !(all(greaterThanEqual(current_position, ivec3(0))) && all(lessThan(current_position, ivec3(2))));
    }


    return vec4(0.2f, 0.2f, 0.2f, 1.0f);
}

void main() {
    vec3 world_position = u_upper_left + gl_FragCoord.x * u_delta_width + gl_FragCoord.y * u_delta_height;
    vec3 direction = normalize(world_position - u_camera_origin);

    frag_color = get_ray_color(direction, u_camera_origin);
}
