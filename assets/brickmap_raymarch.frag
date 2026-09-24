#version 460 core

out vec4 frag_color;

uniform ivec3 u_data_size;
uniform vec3 u_upper_left;
uniform vec3 u_delta_width;
uniform vec3 u_delta_height;
uniform vec3 u_camera_origin;

const int BRICK_SIZE = 8;

struct Brick {
    uint voxels[BRICK_SIZE * BRICK_SIZE * BRICK_SIZE / 32];
    uint colour_pointer;
};

layout(std430, binding = 0) readonly buffer Brickgrid {
    uint grid[];
} brickgrid;

layout(std430, binding = 1) readonly buffer Brickmap {
    Brick bricks[];
} brickmap;

layout(std430, binding = 2) readonly buffer Colour {
    uint map[];
} colours;

layout(std430, binding = 3) readonly buffer ColourMapping {
    vec4 data[];
} colour_map;

bool should_cull(vec3 direction, vec3 origin) {
    vec3 inv_direction = 1.0f / direction;

    vec3 t0 = (-origin) * inv_direction;
    vec3 t1 = (u_data_size * BRICK_SIZE - origin) * inv_direction;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    float max_tmin = max(max(tmin.x, tmin.y), tmin.z);
    float min_tmax = min(min(tmax.x, tmax.y), tmax.z);

    return max_tmin > min_tmax || (max_tmin < 0 && min_tmax < 0);
}

int position_to_index(ivec3 position) {
    return position.x + (position.y * u_data_size.x) + (position.z * u_data_size.x * u_data_size.y);
}

ivec3 init_step(vec3 direction) {
    return ivec3(sign(direction));
}

bool brick_voxel(uint brick_pointer, ivec3 voxel_position) {
    int index = voxel_position.x + voxel_position.y * BRICK_SIZE + voxel_position.z * BRICK_SIZE * BRICK_SIZE;
    int index_internal = (index / 32);
    int position = index % 32;

    return (brickmap.bricks[brick_pointer].voxels[index_internal] & (0x1 << position)) != 0;
}

vec4 voxel_colour(uint brick_pointer, ivec3 voxel_position) {
    int index = voxel_position.x + voxel_position.y * BRICK_SIZE + voxel_position.z * BRICK_SIZE * BRICK_SIZE;
    int index_internal = (index / 32);
    int position = index % 32;

    int bits = 0;

    for(int i = 0; i < index_internal; i++) {
        bits += bitCount(brickmap.bricks[brick_pointer].voxels[i]);
    }

    for(int i = 0; i < position; i++) {
        bits += int((brickmap.bricks[brick_pointer].voxels[index_internal] & (0x1 << i)) != 0);
    }

    uint colour_pointer = brickmap.bricks[brick_pointer].colour_pointer + bits;

    return colour_map.data[colours.map[colour_pointer]];
}

vec3 start_position(vec3 direction, vec3 origin) {
    vec3 inv_direction = 1.0f / direction;

    vec3 t0 = (-origin) * inv_direction;
    vec3 t1 = (u_data_size - origin) * inv_direction;

    vec3 tmin = min(t0, t1);
    float max_tmin = max(max(tmin.x, tmin.y), tmin.z);

    return (max_tmin + 1e-2) * direction + origin;
}

vec4 trace_brick(uint brick_pointer, vec3 direction, vec3 origin) {
    vec3 inv_direction = 1.0f / direction;
    vec3 step = vec3(init_step(direction));

    vec3 tdelta = abs(inv_direction);

    vec3 current_origin = clamp(origin, 0.00001f, BRICK_SIZE - 0.00001f);
    ivec3 current_position = ivec3(floor(current_origin));

    vec3 tmax = (current_position - current_origin + max(step, 0)) * inv_direction;

    bvec3 mask;
    bool inside = false;

    for(int i = 0; i < BRICK_SIZE * 3; i++) {
        if(any(lessThan(current_position, ivec3(0, 0, 0))) || any(greaterThanEqual(current_position, ivec3(BRICK_SIZE, BRICK_SIZE, BRICK_SIZE)))) {
            return vec4(0.0f);
        }

        if(brick_voxel(brick_pointer, current_position)) {
            return voxel_colour(brick_pointer, current_position);
        }

        mask = lessThanEqual(tmax.xyz, min(tmax.yzx, tmax.zxy));

        current_position += ivec3(step * vec3(mask));
        tmax += tdelta * vec3(mask);
    }

    return vec4(0.3f, 0.3f, 1.0f, 1.0f);
}

vec4 trace_world(vec3 direction, vec3 origin) {
    if(should_cull(direction, origin)) {
        return vec4(0.2f, 0.2f, 0.2f, 1.0f);
    }

    direction = direction / BRICK_SIZE;
    origin = origin / BRICK_SIZE;

    ivec3 current_position = ivec3(floor(origin));
    if(any(lessThan(current_position, ivec3(0, 0, 0))) || any(greaterThanEqual(current_position, u_data_size))) {
        origin = start_position(direction, origin);
    }

    vec3 inv_direction = 1.0f / direction;
    vec3 step = vec3(init_step(direction));

    vec3 tdelta = abs(inv_direction);

    current_position = ivec3(floor(origin));

    vec3 tmax = (current_position - origin + max(step, 0)) * inv_direction;

    bvec3 mask = bvec3(false, false, false);

    for(int i = 0; i < 512; i++) {
        bool in_bounds = all(greaterThanEqual(current_position, ivec3(0, 0, 0))) && all(lessThan(current_position, u_data_size));
        if(!in_bounds) {
            return vec4(0.2f, 0.2f, 0.2f, 1.0f);
        }

        uint brick = brickgrid.grid[position_to_index(current_position)];

        if(brick != 0) {
            vec3 temp = tmax - vec3(mask) * tdelta;
            float previous_t = min(min(temp.x, temp.y), temp.z);
            vec3 intersect = origin + direction * previous_t;
            vec3 uv3d = intersect - current_position;

            if(current_position == floor(origin)) {
                uv3d = origin - current_position;
            }

            vec4 val = trace_brick(brick - 1, direction, uv3d * BRICK_SIZE);

            if(val.w > 0.95) {
                return val;
            }
        }

        mask = lessThanEqual(tmax.xyz, min(tmax.yzx, tmax.zxy));

        current_position += ivec3(step * vec3(mask));
        tmax += tdelta * vec3(mask);
    }

    return vec4(0.2f, 0.2f, 0.2f, 1.0f);
}

void main() {
    vec3 world_position = u_upper_left + gl_FragCoord.x * u_delta_width + gl_FragCoord.y * u_delta_height;
    vec3 direction = normalize(world_position - u_camera_origin);

    frag_color = trace_world(direction, u_camera_origin);
}
