#include "GL_renderer.h"
#include "Hell/Logging.h"

namespace OpenGLRenderer {

    OpenGLRasterizerState g_globalState;
    bool g_stateInitialized = false;

    void ForceRasterizerState(const std::string& name) {
        OpenGLRasterizerState* rasterizerState = GetRasterizerState(name);
        if (!rasterizerState) {
            std::cout << "OpenGLRenderer::ForceRasterizerState(const std::string& name) failed! " << name << " does not exist!\n";
            return;
        }
        ForceRasterizerState(*rasterizerState);
    }

    void ForceRasterizerState(const OpenGLRasterizerState& newState) {
        newState.blendEnable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        newState.cullfaceEnable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        newState.depthMask ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
        newState.depthTestEnabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        newState.colorMask ? glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE) : glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glCullFace(newState.cullfaceMode);

        if (newState.blendEnable)      glBlendFunc(newState.blendFuncSrcfactor, newState.blendFuncDstfactor);
        if (newState.depthTestEnabled) glDepthFunc(newState.depthFunc);
        if (newState.pointSize > 1.0f) glPointSize(newState.pointSize);

        newState.stencilTestEnabled ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
        if (newState.stencilTestEnabled) {
            glStencilFunc(newState.stencilFunc, newState.stencilRef, newState.stencilReadMask);
            glStencilOp(newState.stencilFailOp, newState.stencilDepthFailOp, newState.stencilPassOp);
            glStencilMask(newState.stencilWriteMask);
        }

        g_globalState = newState;
        g_stateInitialized = true;
    }

    void SetRasterizerState(const OpenGLRasterizerState& newState) {
        if (!g_stateInitialized) {
            ForceRasterizerState(newState);
            return;
        }

        bool blendWasDisabled = !g_globalState.blendEnable;
        if (g_globalState.blendEnable != newState.blendEnable) {
            newState.blendEnable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        }

        if (newState.blendEnable) {
            if (blendWasDisabled ||
                g_globalState.blendFuncSrcfactor != newState.blendFuncSrcfactor ||
                g_globalState.blendFuncDstfactor != newState.blendFuncDstfactor) {
                glBlendFunc(newState.blendFuncSrcfactor, newState.blendFuncDstfactor);
            }
        }

        if (g_globalState.cullfaceEnable != newState.cullfaceEnable) {
            newState.cullfaceEnable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        }

        if (newState.cullfaceEnable) {
            if (g_globalState.cullfaceMode != newState.cullfaceMode) {
                glCullFace(newState.cullfaceMode);
            }
        }

        if (g_globalState.depthMask != newState.depthMask) {
            glDepthMask(newState.depthMask ? GL_TRUE : GL_FALSE);
        }

        bool depthWasDisabled = !g_globalState.depthTestEnabled;
        if (g_globalState.depthTestEnabled != newState.depthTestEnabled) {
            newState.depthTestEnabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
        }

        if (newState.depthTestEnabled) {
            if (depthWasDisabled || g_globalState.depthFunc != newState.depthFunc) {
                glDepthFunc(newState.depthFunc);
            }
        }

        if (g_globalState.colorMask != newState.colorMask) {
            if (newState.colorMask) glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            else glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        }

        if (g_globalState.pointSize != newState.pointSize) {
            if (newState.pointSize > 1.0f) {
                glPointSize(newState.pointSize);
            }
        }

        // Stencil
        bool stencilWasDisabled = !g_globalState.stencilTestEnabled;
        if (g_globalState.stencilTestEnabled != newState.stencilTestEnabled) {
            newState.stencilTestEnabled ? glEnable(GL_STENCIL_TEST) : glDisable(GL_STENCIL_TEST);
        }

        if (newState.stencilTestEnabled) {
            if (stencilWasDisabled ||
                g_globalState.stencilFunc != newState.stencilFunc ||
                g_globalState.stencilRef != newState.stencilRef ||
                g_globalState.stencilReadMask != newState.stencilReadMask) {
                glStencilFunc(newState.stencilFunc, newState.stencilRef, newState.stencilReadMask);
            }

            if (stencilWasDisabled ||
                g_globalState.stencilFailOp != newState.stencilFailOp ||
                g_globalState.stencilDepthFailOp != newState.stencilDepthFailOp ||
                g_globalState.stencilPassOp != newState.stencilPassOp) {
                glStencilOp(newState.stencilFailOp, newState.stencilDepthFailOp, newState.stencilPassOp);
            }

            if (stencilWasDisabled || g_globalState.stencilWriteMask != newState.stencilWriteMask) {
                glStencilMask(newState.stencilWriteMask);
            }
        }

        g_globalState = newState;
    }

    void VerifyStateCache() {
        if (!g_stateInitialized) {
            return;
        }

        GLboolean glBool;
        GLint glInt;
        GLfloat glFloat;

        // Blend Enable
        glBool = glIsEnabled(GL_BLEND);
        if (g_globalState.blendEnable != (glBool == GL_TRUE)) {
            Logging::Error() << "State Leak: GL_BLEND out of sync\n";
        }

        // Cull Face
        glBool = glIsEnabled(GL_CULL_FACE);
        if (g_globalState.cullfaceEnable != (glBool == GL_TRUE)) {
            Logging::Error() << "State Leak: GL_CULL_FACE out of sync\n";
        }

        // Cull Face Mode
        if (g_globalState.cullfaceEnable) {
            glGetIntegerv(GL_CULL_FACE_MODE, &glInt);
            if (g_globalState.cullfaceMode != (GLenum)glInt) {
                Logging::Error() << "State Leak: GL_CULL_FACE_MODE out of sync\n";
            }
        }

        // Depth Mask
        glGetBooleanv(GL_DEPTH_WRITEMASK, &glBool);
        if (g_globalState.depthMask != (glBool == GL_TRUE)) {
            Logging::Error() << "State Leak: GL_DEPTH_WRITEMASK out of sync\n";
        }

        // Depth Test
        glBool = glIsEnabled(GL_DEPTH_TEST);
        if (g_globalState.depthTestEnabled != (glBool == GL_TRUE)) {
            Logging::Error() << "State Leak: GL_DEPTH_TEST out of sync\n";
        }

        // Color Mask
        GLboolean colorMasks[4];
        glGetBooleanv(GL_COLOR_WRITEMASK, colorMasks);
        if (g_globalState.colorMask != (colorMasks[0] == GL_TRUE)) {
            Logging::Error() << "State Leak: GL_COLOR_WRITEMASK out of sync\n";
        }

        // Blend Functions
        if (g_globalState.blendEnable) {
            glGetIntegerv(GL_BLEND_SRC_RGB, &glInt);
            if (g_globalState.blendFuncSrcfactor != glInt) {
                Logging::Error() << "State Leak: GL_BLEND_SRC_RGB out of sync\n";
            }

            glGetIntegerv(GL_BLEND_DST_RGB, &glInt);
            if (g_globalState.blendFuncDstfactor != glInt) {
                Logging::Error() << "State Leak: GL_BLEND_DST_RGB out of sync\n";
            }
        }

        // Depth Function
        if (g_globalState.depthTestEnabled) {
            glGetIntegerv(GL_DEPTH_FUNC, &glInt);
            if (g_globalState.depthFunc != glInt) {
                Logging::Error() << "State Leak: GL_DEPTH_FUNC out of sync\n";
            }
        }

        // Point Size
        if (g_globalState.pointSize > 1.0f) {
            glGetFloatv(GL_POINT_SIZE, &glFloat);
            if (g_globalState.pointSize != glFloat) {
                Logging::Error() << "State Leak: GL_POINT_SIZE out of sync\n";
            }
        }

        // Stencil
        glBool = glIsEnabled(GL_STENCIL_TEST);
        if (g_globalState.stencilTestEnabled != (glBool == GL_TRUE)) {
            Logging::Error() << "State Leak: GL_STENCIL_TEST out of sync\n";
        }

        if (g_globalState.stencilTestEnabled) {
            glGetIntegerv(GL_STENCIL_FUNC, &glInt);
            if (g_globalState.stencilFunc != (GLenum)glInt) Logging::Error() << "State Leak: GL_STENCIL_FUNC out of sync\n";

            glGetIntegerv(GL_STENCIL_REF, &glInt);
            if (g_globalState.stencilRef != glInt) Logging::Error() << "State Leak: GL_STENCIL_REF out of sync\n";

            glGetIntegerv(GL_STENCIL_VALUE_MASK, &glInt);
            if (g_globalState.stencilReadMask != (GLuint)glInt) Logging::Error() << "State Leak: GL_STENCIL_VALUE_MASK out of sync\n";

            glGetIntegerv(GL_STENCIL_FAIL, &glInt);
            if (g_globalState.stencilFailOp != (GLenum)glInt) Logging::Error() << "State Leak: GL_STENCIL_FAIL out of sync\n";

            glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &glInt);
            if (g_globalState.stencilDepthFailOp != (GLenum)glInt) Logging::Error() << "State Leak: GL_STENCIL_PASS_DEPTH_FAIL out of sync\n";

            glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &glInt);
            if (g_globalState.stencilPassOp != (GLenum)glInt) Logging::Error() << "State Leak: GL_STENCIL_PASS_DEPTH_PASS out of sync\n";

            glGetIntegerv(GL_STENCIL_WRITEMASK, &glInt);
            if (g_globalState.stencilWriteMask != (GLuint)glInt) Logging::Error() << "State Leak: GL_STENCIL_WRITEMASK out of sync\n";
        }
    }

    void InitRasterizerStates() {
        OpenGLRasterizerState* decalPass = CreateRasterizerState("DecalPass");
        decalPass->depthTestEnabled = true;
        decalPass->blendEnable = true;
        decalPass->cullfaceEnable = true;
        decalPass->depthMask = false;
        decalPass->depthFunc = GL_GREATER;
        decalPass->blendFuncSrcfactor = GL_SRC_ALPHA;
        decalPass->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGLRasterizerState* emissivePass = CreateRasterizerState("EmissivePass");
        emissivePass->depthTestEnabled = true;
        emissivePass->blendEnable = false;
        emissivePass->cullfaceEnable = true;
        emissivePass->depthMask = false;
        emissivePass->depthFunc = GL_GREATER;

        OpenGLRasterizerState* geometryPassDefault = CreateRasterizerState("GeometryPass_Default");
        geometryPassDefault->depthTestEnabled = true;
        geometryPassDefault->blendEnable = false;
        geometryPassDefault->cullfaceEnable = true;
        geometryPassDefault->depthMask = true;
        geometryPassDefault->depthFunc = GL_GREATER;

        OpenGLRasterizerState* geometryPassAlphaDiscard = CreateRasterizerState("GeometryPass_AlphaDiscard");
        geometryPassAlphaDiscard->depthTestEnabled = true;
        geometryPassAlphaDiscard->blendEnable = false;
        geometryPassAlphaDiscard->cullfaceEnable = true;
        geometryPassAlphaDiscard->depthMask = true;
        geometryPassAlphaDiscard->depthFunc = GL_GEQUAL;

        OpenGLRasterizerState* geometryPassBlended = CreateRasterizerState("GeometryPass_Blended");
        geometryPassBlended->depthTestEnabled = true;
        geometryPassBlended->blendEnable = true;
        geometryPassBlended->cullfaceEnable = false;
        geometryPassBlended->depthMask = false;
        geometryPassBlended->depthFunc = GL_GEQUAL;
        geometryPassBlended->blendFuncSrcfactor = GL_SRC_ALPHA;
        geometryPassBlended->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGLRasterizerState* glassPass = CreateRasterizerState("GlassPass");
        glassPass->depthTestEnabled = true;
        glassPass->blendEnable = false;
        glassPass->cullfaceEnable = true;
        glassPass->depthMask = false;
        glassPass->depthFunc = GL_GREATER;

        OpenGLRasterizerState* hairPassViewspaceDepth = CreateRasterizerState("HairViewspaceDepth");
        hairPassViewspaceDepth->depthTestEnabled = true;
        hairPassViewspaceDepth->blendEnable = false;
        hairPassViewspaceDepth->cullfaceEnable = true;
        hairPassViewspaceDepth->depthMask = true;
        hairPassViewspaceDepth->depthFunc = GL_GREATER;
        hairPassViewspaceDepth->blendFuncSrcfactor = GL_SRC_ALPHA;
        hairPassViewspaceDepth->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        hairPassViewspaceDepth->pointSize = 8;

        OpenGLRasterizerState* hairPassLighting = CreateRasterizerState("HairLighting");
        hairPassLighting->depthTestEnabled = true;
        hairPassLighting->blendEnable = false;
        hairPassLighting->cullfaceEnable = true;
        hairPassLighting->depthMask = true;
        hairPassLighting->depthFunc = GL_EQUAL;
        hairPassLighting->blendFuncSrcfactor = GL_SRC_ALPHA;
        hairPassLighting->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        hairPassLighting->pointSize = 8;

        OpenGLRasterizerState* spriteSheet = CreateRasterizerState("SpriteSheetPass");
        spriteSheet->depthTestEnabled = true;
        spriteSheet->blendEnable = true;
        spriteSheet->cullfaceEnable = false;
        spriteSheet->depthMask = false;
        spriteSheet->depthFunc = GL_GREATER;
        spriteSheet->blendFuncSrcfactor = GL_SRC_ALPHA;
        spriteSheet->blendFuncDstfactor = GL_ONE; // was GL_ONE_MINUS_SRC_ALPHA

        OpenGLRasterizerState* skybox = CreateRasterizerState("SkyBox");
        skybox->depthTestEnabled = false;
        skybox->blendEnable = false;
        skybox->cullfaceEnable = false;
        skybox->depthMask = false;
        skybox->depthFunc = GL_GREATER;
    }
}