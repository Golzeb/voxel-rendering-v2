#version 460 core

out vec4 frag_color;

uniform ivec3 u_data_size;
uniform vec3 u_upper_left;
uniform vec3 u_delta_width;
uniform vec3 u_delta_height;
uniform vec3 u_camera_origin;

const uint EMPTY_VOXEL = 0;

layout(std430, binding = 0) readonly buffer VoxelGrid {
    uint voxels[];
} voxel_grid;

layout (std430, binding = 1) readonly buffer ColoursBuffer {
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

int position_to_index(ivec3 position) {
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

vec4 get_ray_color(vec3 direction, vec3 origin) {
    if(should_cull(direction, origin)) {
        return vec4(0.2f, 0.2f, 0.2f, 1.0f);
    }

    ivec3 step = init_step(direction);

    vec3 tdelta = abs(1.0f / direction);

    ivec3 current_position = ivec3(floor(origin));

    if(any(bvec2(any(lessThan(current_position, ivec3(0, 0, 0))),
            any(greaterThanEqual(current_position, u_data_size))))) {
        origin = start_position(direction, origin);
        current_position = ivec3(floor(origin));
    }

    vec3 tmax = (current_position - origin + max(step, 0)) / direction;

    bvec3 mask;

    for(int i = 0; i < 512; i++) {
        bool in_bounds = all(greaterThanEqual(current_position, ivec3(0, 0, 0))) && all(lessThan(current_position, u_data_size));
        if(!in_bounds) {
            return vec4(0.2f, 0.2f, 0.2f, 1.0f);
        }

        uint index = position_to_index(current_position);
        uint shift = (index % 2) * 16;

        uint voxel = (voxel_grid.voxels[index / 2] >> shift) & 0xFFFF;

        if(voxel != EMPTY_VOXEL) {
            return colours.data[voxel];
        }

        mask = lessThanEqual(tmax.xyz, min(tmax.yzx, tmax.zxy));

        current_position += step * ivec3(mask);
        tmax += tdelta * vec3(mask);
    }

    return vec4(0.2f, 0.2f, 0.2f, 1.0f);
}

void main() {
    vec3 world_position = u_upper_left + gl_FragCoord.x * u_delta_width + gl_FragCoord.y * u_delta_height;
    vec3 direction = normalize(world_position - u_camera_origin);

    frag_color = get_ray_color(direction, u_camera_origin);
}
