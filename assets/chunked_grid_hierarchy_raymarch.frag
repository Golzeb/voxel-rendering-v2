#version 460 core

out vec4 frag_color;

uniform ivec3 u_data_size;
uniform vec3 u_upper_left;
uniform vec3 u_delta_width;
uniform vec3 u_delta_height;
uniform vec3 u_camera_origin;
uniform uint u_layers;
uniform uint u_scale;

const uint EMPTY_VOXEL = 0;

struct Layer {
    ivec3 size;
    uint pointer;
};

layout(std430, binding = 0) readonly buffer HierarchyLayers {
    Layer data[];
} layers;

layout(std430, binding = 1) readonly buffer HierarchyGrid {
    uint data[];
} grid;

layout(std430, binding = 2) readonly buffer VoxelGrid {
    uint data[];
} voxels;

layout (std430, binding = 3) readonly buffer ColoursBuffer {
    vec4 data[];
} colours;

layout (std430, binding = 4) buffer DepthBuffer {
    float data[];
} depth_buffer;


bool layer_voxel(uint layer, ivec3 voxel_position) {
    ivec3 size = layers.data[layer].size;
    uint index = voxel_position.x + voxel_position.y * size.x + voxel_position.z * size.x * size.y;
    uint index_internal = (index / 32);
    uint position = index % 32;

    uint mask = 0x1 << position;
    uint voxel = grid.data[layers.data[layer].pointer + index_internal] & mask;

    return voxel != 0;
}

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

uint grid_voxel(ivec3 position) {
    if(any(greaterThanEqual(position, u_data_size))) {
        return EMPTY_VOXEL;
    }

    uint index = position_to_index(position);
    uint shift = (index % 2) * 16;

    return (voxels.data[index / 2] >> shift) & 0xFFFF;
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

vec4 get_ray_color(vec3 direction, vec3 origin) {
    if(should_cull(direction, origin)) {
        discard;
    }

    direction = vec3(
        abs(direction.x) < 1e-8 ? (direction.x < 0.0f ? -1e-8 : 1e-8) : direction.x,
        abs(direction.y) < 1e-8 ? (direction.y < 0.0f ? -1e-8 : 1e-8) : direction.y,
        abs(direction.z) < 1e-8 ? (direction.z < 0.0f ? -1e-8 : 1e-8) : direction.z
    );

    const ivec3 step = init_step(direction);

    uint layer = 0;
    uint current_block_size = int(pow(u_scale, u_layers - layer - 1));

    vec3 direction_original = direction;

    ivec3 current_position = ivec3(floor(origin));
    if(any(lessThan(current_position, ivec3(0, 0, 0))) || any(greaterThanEqual(current_position, u_data_size))) {
        origin = start_position(direction, origin);
    }

    vec3 current_origin = origin / current_block_size;
    vec3 current_direction = direction / current_block_size;

    vec3 tdelta = abs(1.0f / current_direction);

    current_position = ivec3(floor(current_origin));
    vec3 tmax = (current_position - current_origin + max(step, 0)) / current_direction;

    bvec3 mask;
    bool inside = false;

    ivec3 entry_position;

    const uint MAX_DEPTH = u_layers - 1;

    if(all(greaterThanEqual(current_position, ivec3(0, 0, 0))) && all(lessThan(current_position, layers.data[layer].size))) {
        while(layer < MAX_DEPTH && layer_voxel(layer, current_position)) {
            entry_position = current_position;
            layer += 1;

            current_block_size /= u_scale;
            current_origin *= u_scale;
            current_direction *= u_scale;

            tdelta = abs(1.0f / current_direction);
            current_position = ivec3(floor(current_origin));
            tmax = (current_position - current_origin + max(step, 0)) / current_direction;
        }
    }

    for(int i = 0; i < 512; ++i) {
        bool in_bounds = all(greaterThanEqual(current_position, ivec3(0, 0, 0))) && all(lessThan(current_position, layers.data[layer].size));
        bool temp_oob = all(greaterThanEqual(current_position / u_scale, ivec3(0, 0, 0))) && all(lessThan(current_position / u_scale, layers.data[layer - 1].size));

        in_bounds = in_bounds || temp_oob;

        if(!in_bounds) {
            discard;
        }

        vec3 temp = tmax - vec3(mask) * tdelta;
        float previous_t = min(min(temp.x, temp.y), temp.z);
        vec3 intersect = (current_origin + current_direction * previous_t) * current_block_size;

        while(layer > 0 && current_position / u_scale != entry_position) {
            layer -= 1;

            current_block_size *= u_scale;
            current_origin = intersect / current_block_size;
            current_direction = direction_original / current_block_size;

            tdelta = abs(1.0f / (current_direction + 1e-4));

            current_position = current_position / int(u_scale);
            entry_position = current_position / int(u_scale);
            tmax = (current_position - current_origin + max(step, 0)) / current_direction;
        }

        while(layer != MAX_DEPTH && layer_voxel(layer, current_position)) {
            entry_position = current_position;

            layer += 1;

            current_block_size /= u_scale;
            current_origin = intersect / current_block_size;
            current_direction = direction_original / current_block_size;

            tdelta = abs(1.0f / current_direction);

            current_position = ivec3(floor(current_origin));
            tmax = (current_position - current_origin + max(step, 0)) / current_direction;
        }

        if(layer == MAX_DEPTH) {
            uint voxel = grid_voxel(current_position);
            float depth = length(u_camera_origin - vec3(intersect));

            if(depth >= depth_buffer.data[int(gl_FragCoord.x) + int(gl_FragCoord.y) * 1920]) {
                discard;
            }

            if(voxel != EMPTY_VOXEL) {
                depth_buffer.data[int(gl_FragCoord.x) + int(gl_FragCoord.y) * 1920] = depth;
                return colours.data[voxel];
            }
        }
 
        mask = lessThanEqual(tmax.xyz, min(tmax.yzx, tmax.zxy));

        current_position += step * ivec3(mask);
        tmax += tdelta * vec3(mask);
    }

    discard;
}

void main() {
    vec3 world_position = u_upper_left + gl_FragCoord.x * u_delta_width + gl_FragCoord.y * u_delta_height;
    vec3 direction = normalize(world_position - u_camera_origin);

    frag_color = get_ray_color(direction, u_camera_origin);
}
