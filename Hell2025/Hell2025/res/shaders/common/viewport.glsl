vec2 ViewportUVFromPixel_GL(ivec2 px, ivec2 outputImageSize, ViewportData viewportData) {
    vec2 viewportPosition = vec2(px) + 0.5 - vec2(viewportData.xOffset, viewportData.yOffset);
    return viewportPosition / vec2(viewportData.width, viewportData.height);
}

vec2 ViewportUVFromPixel_VK(ivec2 px, ivec2 outputImageSize, ViewportData viewportData) {
    vec2 viewportOrigin = vec2(viewportData.xOffset, outputImageSize.y - viewportData.yOffset - viewportData.height);
    vec2 viewportPosition = vec2(px) + 0.5 - viewportOrigin;
    return viewportPosition / vec2(viewportData.width, viewportData.height);
}

vec2 ViewportNDCFromPixel_GL(ivec2 px, ivec2 outputImageSize, ViewportData viewportData) {
    vec2 viewportUV = ViewportUVFromPixel_GL(px, outputImageSize, viewportData);
    vec2 viewportNDC = viewportUV * 2.0 - 1.0;
    viewportNDC.y = -viewportNDC.y;
    return viewportNDC;
}

vec2 ViewportNDCFromPixel_VK(ivec2 px, ivec2 outputImageSize, ViewportData viewportData) {
    vec2 viewportUV = ViewportUVFromPixel_VK(px, outputImageSize, viewportData);
    return viewportUV * 2.0 - 1.0;
}

uint ViewportIndexFromSplitScreenMode_VK(ivec2 px, ivec2 size, int mode) {
    if (px.x < 0 || px.y < 0 || px.x >= size.x || px.y >= size.y) {
        return 0u;
    }

    if (mode == 0) {
        return 0u;
    }

    uint x = uint(px.x >= (size.x / 2));
    uint y = uint(px.y <  (size.y / 2));

    if (mode == 1) {
        return y;
    }

    if (mode == 2) {
        return x + (y << 1);
    }

    return 0u;
}
