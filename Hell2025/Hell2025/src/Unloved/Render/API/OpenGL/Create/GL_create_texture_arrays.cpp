#include "Hell/Common/Constants.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Ocean/Ocean.h"

namespace OpenGL::Renderer {

    void CreateTextureArrays() {
        // TODO: probably move this out of OpenGL init and into the API agnostic init
        // TODO: probably move this out of OpenGL init and into the API agnostic init
        // TODO: probably move this out of OpenGL init and into the API agnostic init

        Hell::TextureArray& woundMasks = Hell::ResourceManager::CreateTextureArray("WoundMasks");
        woundMasks.CleanUp();
        woundMasks.AllocateMemory(WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE, GL_R8, 1, WOUND_MASK_TEXTURE_ARRAY_SIZE); // consider adding mipmaps

    }
}