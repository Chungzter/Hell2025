float SignNotZero(float value) {
    return value >= 0.0 ? 1.0 : -1.0;
}

vec3 DecodeNormal(vec2 encodedNormal) {
    vec2 normalXZ = encodedNormal * 2.0 - 1.0;

    vec3 normal;
    normal.x = normalXZ.x;
    normal.z = normalXZ.y;
    normal.y = 1.0 - abs(normal.x) - abs(normal.z);

    if (normal.y < 0.0) {
        vec2 foldedNormalXZ = normal.xz;

        normal.x = (1.0 - abs(foldedNormalXZ.y)) * SignNotZero(foldedNormalXZ.x);
        normal.z = (1.0 - abs(foldedNormalXZ.x)) * SignNotZero(foldedNormalXZ.y);
    }

    return normalize(normal);
}

vec2 EncodeNormal(vec3 normal) {
    normal = normalize(normal);

    float denominator = abs(normal.x) + abs(normal.y) + abs(normal.z);
    vec2 encodedNormal = normal.xz / denominator;

    if (normal.y <= 0.0) {
        encodedNormal = vec2(
            (1.0 - abs(encodedNormal.y)) * (encodedNormal.x >= 0.0 ? 1.0 : -1.0),
            (1.0 - abs(encodedNormal.x)) * (encodedNormal.y >= 0.0 ? 1.0 : -1.0)
        );
    }

    return encodedNormal * 0.5 + 0.5;
}