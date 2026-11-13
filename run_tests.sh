!/bin/bash

# # Raster base
# ./build/voxel-rendering tests/raster/raster_128_sphere.json
# ./build/voxel-rendering tests/raster/raster_128_cubes.json
# ./build/voxel-rendering tests/raster/raster_128_random.json
# ./build/voxel-rendering tests/raster/raster_256_sphere.json
# ./build/voxel-rendering tests/raster/raster_256_cubes.json
# ./build/voxel-rendering tests/raster/raster_256_random.json
#
# # Raster culling
# # Face culling
# ./build/voxel-rendering tests/extractor/fculled_128_sphere.json
# ./build/voxel-rendering tests/extractor/fculled_128_cubes.json
# ./build/voxel-rendering tests/extractor/fculled_128_random.json
# ./build/voxel-rendering tests/extractor/fculled_256_sphere.json
# ./build/voxel-rendering tests/extractor/fculled_256_cubes.json
# ./build/voxel-rendering tests/extractor/fculled_256_random.json
# ./build/voxel-rendering tests/extractor/fculled_512_sphere.json
# ./build/voxel-rendering tests/extractor/fculled_512_cubes.json
# ./build/voxel-rendering tests/extractor/fculled_512_random.json
# ./build/voxel-rendering tests/extractor/fculled_512_bistro.json
# ./build/voxel-rendering tests/extractor/fculled_512_sponza.json
# ./build/voxel-rendering tests/extractor/fculled_mrbrain.json
#
# # Greedy meshing
# ./build/voxel-rendering tests/extractor/greedy_128_sphere.json
# ./build/voxel-rendering tests/extractor/greedy_128_cubes.json
# ./build/voxel-rendering tests/extractor/greedy_128_random.json
# ./build/voxel-rendering tests/extractor/greedy_256_sphere.json
# ./build/voxel-rendering tests/extractor/greedy_256_cubes.json
# ./build/voxel-rendering tests/extractor/greedy_256_random.json
# ./build/voxel-rendering tests/extractor/greedy_512_sphere.json
# ./build/voxel-rendering tests/extractor/greedy_512_cubes.json
# ./build/voxel-rendering tests/extractor/greedy_512_random.json
# ./build/voxel-rendering tests/extractor/greedy_512_bistro.json
# ./build/voxel-rendering tests/extractor/greedy_512_sponza.json
# ./build/voxel-rendering tests/extractor/greedy_mrbrain.json
#
# # Raster culling with chunking
# ./build/voxel-rendering tests/extractor_chunked/fculled_512_bistro.json
# ./build/voxel-rendering tests/extractor_chunked/fculled_512_cubes.json
# ./build/voxel-rendering tests/extractor_chunked/fculled_512_random.json
# ./build/voxel-rendering tests/extractor_chunked/fculled_512_sphere.json
# ./build/voxel-rendering tests/extractor_chunked/fculled_512_sponza.json
# ./build/voxel-rendering tests/extractor_chunked/fculled_mrbrain.json
#
# # Raster culling with chunking (indexed)
# ./build/voxel-rendering tests/extractor_indexed/fculled_512_bistro.json
# ./build/voxel-rendering tests/extractor_indexed/fculled_512_cubes.json
# ./build/voxel-rendering tests/extractor_indexed/fculled_512_random.json
# ./build/voxel-rendering tests/extractor_indexed/fculled_512_sphere.json
# ./build/voxel-rendering tests/extractor_indexed/fculled_512_sponza.json
# ./build/voxel-rendering tests/extractor_indexed/fculled_mrbrain.json
#
# # Raster max with all optimizations
# ./build/voxel-rendering tests/extractor_max/fculled_512_bistro.json
# ./build/voxel-rendering tests/extractor_max/fculled_512_cubes.json
# ./build/voxel-rendering tests/extractor_max/fculled_512_random.json
# ./build/voxel-rendering tests/extractor_max/fculled_512_sphere.json
# ./build/voxel-rendering tests/extractor_max/fculled_512_sponza.json
# ./build/voxel-rendering tests/extractor_max/fculled_mrbrain.json
#
# # Instanced
# ./build/voxel-rendering tests/instanced/base_512_bistro.json
# ./build/voxel-rendering tests/instanced/base_512_sponza.json
# ./build/voxel-rendering tests/instanced/base_mrbrain.json
#
# ./build/voxel-rendering tests/instanced/fculled_512_bistro.json
# ./build/voxel-rendering tests/instanced/fculled_512_sponza.json
# ./build/voxel-rendering tests/instanced/fculled_mrbrain.json
#
# ./build/voxel-rendering tests/instanced/full_512_bistro.json
# ./build/voxel-rendering tests/instanced/full_512_sponza.json
# ./build/voxel-rendering tests/instanced/full_mrbrain.json
#
# ./build/voxel-rendering tests/instanced/indexed_512_bistro.json
# ./build/voxel-rendering tests/instanced/indexed_512_sponza.json
# ./build/voxel-rendering tests/instanced/indexed_mrbrain.json
#
# # Ray marching
# ./build/voxel-rendering tests/raymarch/base_512_bistro.json
# ./build/voxel-rendering tests/raymarch/base_512_cubes.json
# ./build/voxel-rendering tests/raymarch/base_512_random.json
# ./build/voxel-rendering tests/raymarch/base_512_sponza.json
# ./build/voxel-rendering tests/raymarch/base_mrbrain.json
#
# ./build/voxel-rendering tests/raymarch/chunked_512_bistro.json
# ./build/voxel-rendering tests/raymarch/chunked_512_cubes.json
# ./build/voxel-rendering tests/raymarch/chunked_512_random.json
# ./build/voxel-rendering tests/raymarch/chunked_512_sponza.json
# ./build/voxel-rendering tests/raymarch/chunked_mrbrain.json
#
# ./build/voxel-rendering tests/raymarch/nocull_512_bistro.json
# ./build/voxel-rendering tests/raymarch/nocull_512_cubes.json
# ./build/voxel-rendering tests/raymarch/nocull_512_random.json
# ./build/voxel-rendering tests/raymarch/nocull_512_sponza.json
# ./build/voxel-rendering tests/raymarch/nocull_mrbrain.json
#
# # Grid hierarchy
# ./build/voxel-rendering tests/grid/base_512_bistro.json
# ./build/voxel-rendering tests/grid/base_512_cubes.json
# ./build/voxel-rendering tests/grid/base_512_random.json
# ./build/voxel-rendering tests/grid/base_512_sponza.json
# ./build/voxel-rendering tests/grid/base_mrbrain.json
#
# ./build/voxel-rendering tests/grid/chunked_512_bistro.json
# ./build/voxel-rendering tests/grid/chunked_512_cubes.json
# ./build/voxel-rendering tests/grid/chunked_512_random.json
# ./build/voxel-rendering tests/grid/chunked_512_sponza.json
# ./build/voxel-rendering tests/grid/chunked_mrbrain.json
#
# # Brickmap
# ./build/voxel-rendering tests/brickmap/base_512_bistro.json
# ./build/voxel-rendering tests/brickmap/base_512_cubes.json
# ./build/voxel-rendering tests/brickmap/base_512_random.json
# ./build/voxel-rendering tests/brickmap/base_512_sponza.json
# ./build/voxel-rendering tests/brickmap/base_mrbrain.json
#
# # SVO
# ./build/voxel-rendering tests/svo/base_512_bistro.json
# ./build/voxel-rendering tests/svo/base_512_cubes.json
# ./build/voxel-rendering tests/svo/base_512_random.json
# ./build/voxel-rendering tests/svo/base_512_sponza.json
# ./build/voxel-rendering tests/svo/base_mrbrain.json
#
# # Final comp
# ./build/voxel-rendering tests/final_comp/brickmap_1024_cubes.json
# ./build/voxel-rendering tests/final_comp/brickmap_1024_sphere.json
# ./build/voxel-rendering tests/final_comp/brickmap_2048_bistro.json
# ./build/voxel-rendering tests/final_comp/brickmap_2048_sponza.json
# ./build/voxel-rendering tests/final_comp/brickmap_512_bistro.json
# ./build/voxel-rendering tests/final_comp/brickmap_512_sponza.json
# ./build/voxel-rendering tests/final_comp/brickmap_512_random.json
# ./build/voxel-rendering tests/final_comp/brickmap_729_sponge.json
# ./build/voxel-rendering tests/final_comp/brickmap_mrbrain.json
#
# ./build/voxel-rendering tests/final_comp/fculled_1024_cubes.json
# ./build/voxel-rendering tests/final_comp/fculled_1024_sphere.json
# ./build/voxel-rendering tests/final_comp/fculled_2048_bistro.json
# ./build/voxel-rendering tests/final_comp/fculled_2048_sponza.json
./build/voxel-rendering tests/final_comp/fculled_512_bistro.json
./build/voxel-rendering tests/final_comp/fculled_512_sponza.json
# ./build/voxel-rendering tests/final_comp/fculled_512_random.json
# ./build/voxel-rendering tests/final_comp/fculled_729_sponge.json
# ./build/voxel-rendering tests/final_comp/fculled_mrbrain.json
