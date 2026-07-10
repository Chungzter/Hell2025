#define PI 3.14159265359
#define VIEWPORT_INDEX_SHIFT 20

#define HEIGHTMAP_SCALE_Y 40.0
#define HEIGHTMAP_SCALE_XZ 0.25
#define TILE_SIZE 24

#define BLENDING_MODE_ALPHA_DISCARD    0u
#define BLENDING_MODE_BLENDED          1u
#define BLENDING_MODE_DEFAULT          2u
#define BLENDING_MODE_HAIR_UNDER_LAYER 3u
#define BLENDING_MODE_HAIR             4u
#define BLENDING_MODE_TOILET_WATER     5u
#define BLENDING_MODE_MIRROR           6u
#define BLENDING_MODE_GLASS            7u
#define BLENDING_MODE_PLASTIC          8u
#define BLENDING_MODE_DO_NOT_RENDER    9u
#define BLENDING_MODE_STAINED_GLASS    10u
#define BLENDING_MODE_UNDEFINED        11u

//const vec3 WATER_ALBEDO = mix(vec3(0.4, 0.8, 0.6) * 0.1, vec3(0.01, 0.03, 0.04), 0.25);


//const vec3 WATER_ALBEDO = mix(vec3(0.04, 0.08, 0.06), vec3(0.01, 0.03, 0.04), 0.25);
const vec3 WATER_ALBEDO = vec3(0.0325, 0.0675, 0.0625) * 0.95; // same as above

const vec3 UNDER_WATER_TINT = mix(vec3(0.4, 0.8, 0.6) * 1.75, vec3(0.01, 0.03, 0.04), 0.25);


const vec3 MOON_LIGHT_COLOR = vec3(0.881875, 0.894375, 0.73525);
const float MOON_LIGHT_STRENGTH = 0.05;

vec3 FOG_COLOR = vec3(0.222, 0.233, 0.27);

// GI
//#define PROBE_NORMAL_BIAS 0.02

