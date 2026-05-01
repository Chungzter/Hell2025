#include "GL_frameBuffer.h"

OpenGLFrameBuffer::OpenGLFrameBuffer(const std::string& name, int width, int height, uint32_t sampleCount) {
    Create(name, width, height, sampleCount);
}

OpenGLFrameBuffer::OpenGLFrameBuffer(const std::string& name, const glm::ivec2& size, uint32_t sampleCount) {
    Create(name, size, sampleCount);
}

void OpenGLFrameBuffer::Create(const std::string& name, const glm::ivec2& size, uint32_t sampleCount) {
    Create(name, size.x, size.y, sampleCount);
}

void OpenGLFrameBuffer::Create(const std::string& name, int width, int height, uint32_t sampleCount) {
    glCreateFramebuffers(1, &m_handle);
    m_name = name;
    m_width = width;
    m_height = height;
    m_sampleCount = sampleCount;
}

void OpenGLFrameBuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_handle);
}

void OpenGLFrameBuffer::SetViewport() {
    glViewport(0, 0, m_width, m_height);
}

void OpenGLFrameBuffer::CleanUp() {
    m_colorAttachments.clear();
    glDeleteFramebuffers(1, &m_handle);
    m_handle = 0;
}

void OpenGLFrameBuffer::CreateAttachment(const std::string& name, GLenum internalFormat, GLenum minFilter, GLenum magFilter, GLenum wrap, bool allocateMips) {
    ColorAttachment& colorAttachment = m_colorAttachments.emplace_back();
    colorAttachment.name = name;
    colorAttachment.internalFormat = internalFormat;
    colorAttachment.format = OpenGLUtil::GLInternalFormatToGLFormat(internalFormat);
    colorAttachment.type = OpenGLUtil::GLInternalFormatToGLType(internalFormat);

    GLenum target = IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    glCreateTextures(target, 1, &colorAttachment.handle);

    if (IsMultisampled()) {
        glTextureStorage2DMultisample(colorAttachment.handle, m_sampleCount, internalFormat, m_width, m_height, GL_TRUE);
    }
    else {
        int levels = 1;
        if (allocateMips) {
            int maxDim = std::max(m_width, m_height);
            levels = 1 + (int)floor(log2(maxDim));
        }

        glTextureStorage2D(colorAttachment.handle, levels, internalFormat, m_width, m_height);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_WRAP_S, wrap);
        glTextureParameteri(colorAttachment.handle, GL_TEXTURE_WRAP_T, wrap);

        if (allocateMips) {
            glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }

    glNamedFramebufferTexture(m_handle, GL_COLOR_ATTACHMENT0 + m_colorAttachments.size() - 1, colorAttachment.handle, 0);
    std::string debugLabel = "Texture (FBO: " + m_name + " Tex: " + name + ")";
    glObjectLabel(GL_TEXTURE, colorAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());
}

void OpenGLFrameBuffer::CreateDepthAttachment(GLenum internalFormat, GLenum minFilter, GLenum magFilter, GLint wrap, glm::vec4 borderColor) {
    m_depthAttachment.internalFormat = internalFormat;

    GLenum target = IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    glCreateTextures(target, 1, &m_depthAttachment.handle);

    if (IsMultisampled()) {
        glTextureStorage2DMultisample(m_depthAttachment.handle, m_sampleCount, internalFormat, m_width, m_height, GL_TRUE);
    }
    else {
        glTextureStorage2D(m_depthAttachment.handle, 1, internalFormat, m_width, m_height);

        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MAG_FILTER, magFilter);
        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_S, wrap);
        glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_T, wrap);

        glTextureParameterfv(m_depthAttachment.handle, GL_TEXTURE_BORDER_COLOR, &borderColor[0]);
    }

    glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, m_depthAttachment.handle, 0);

    std::string debugLabel = "Texture (FBO: " + m_name + " Tex: Depth)";
    glObjectLabel(GL_TEXTURE, m_depthAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());
}

void OpenGLFrameBuffer::BindDepthAttachmentFrom(const OpenGLFrameBuffer& srcFrameBuffer) {
    GLenum attach = (srcFrameBuffer.m_depthAttachment.internalFormat == GL_DEPTH24_STENCIL8 ||
        srcFrameBuffer.m_depthAttachment.internalFormat == GL_DEPTH32F_STENCIL8)
        ? GL_DEPTH_STENCIL_ATTACHMENT
        : GL_DEPTH_ATTACHMENT;

    glNamedFramebufferTexture(m_handle, attach, srcFrameBuffer.m_depthAttachment.handle, 0);
}

void OpenGLFrameBuffer::DrawBuffers(const std::vector<std::string>& attachmentNames) {
    std::vector<GLuint> attachments;
    for (const std::string& attachmentName : attachmentNames) {
        attachments.push_back(GetColorAttachmentSlotByName(attachmentName));
    }
    glDrawBuffers(static_cast<GLsizei>(attachments.size()), attachments.data());
}

void OpenGLFrameBuffer::DrawBuffer(GLenum buffer) {
    glDrawBuffer(buffer);
}

void OpenGLFrameBuffer::DrawBuffer(const std::string& attachmentName) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearTexImage(const std::string& attachmentName, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    int index = -1;
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            index = i;
            break;
        }
    }
    if (index >= 0) {
        GLuint tex = m_colorAttachments[index].handle;
        float cc[4] = { r, g, b, a };
        glClearTexImage(tex, 0, GL_RGBA, GL_FLOAT, cc);
    }
    else {
        std::cout << "OpenGLFrameBuffer::ClearTexImage() failed: '" << attachmentName << "' not found!\n";
    }
}

void OpenGLFrameBuffer::ClearAttachment(const std::string& attachmentName, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLfloat clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, 0, 0, 0, GetWidth(), GetHeight(), 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmentI(const std::string& attachmentName, GLint r, GLint g, GLint b, GLint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, 0, 0, 0, GetWidth(), GetHeight(), 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmentUI(const std::string& attachmentName, GLint r, GLint g, GLint b, GLint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLuint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, 0, 0, 0, GetWidth(), GetHeight(), 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmenSubRegion(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLfloat clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, xOffset, yOffset, 0, width, height, 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmenSubRegionInt(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLint r, GLint g, GLint b, GLint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, xOffset, yOffset, 0, width, height, 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearAttachmenSubRegionUInt(const std::string& attachmentName, GLint xOffset, GLint yOffset, GLsizei width, GLsizei height, GLuint r, GLuint g, GLuint b, GLuint a) {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (attachmentName == m_colorAttachments[i].name) {
            GLuint texture = m_colorAttachments[i].handle;
            GLenum format = m_colorAttachments[i].format;
            GLenum type = m_colorAttachments[i].type;
            GLuint clearColor[4] = { r, g, b, a };
            glClearTexSubImage(texture, 0, xOffset, yOffset, 0, width, height, 1, format, type, clearColor);
            return;
        }
    }
}

void OpenGLFrameBuffer::ClearDepthAttachment() {
    glClear(GL_DEPTH_BUFFER_BIT);
}

void OpenGLFrameBuffer::ClearDepthAttachment(float value) {
    // Warning! this does not work for depth stencil
    glClearNamedFramebufferfv(m_handle, GL_DEPTH, 0, &value);
}

void OpenGLFrameBuffer::Resize(int width, int height) {
	m_width = std::max(width, 1);
	m_height = std::max(height, 1);

	for (size_t i = 0; i < m_colorAttachments.size(); ++i) {
		ColorAttachment& colorAttachment = m_colorAttachments[i];
		glDeleteTextures(1, &colorAttachment.handle);

		GLenum target = IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		glCreateTextures(target, 1, &colorAttachment.handle);

		if (IsMultisampled()) {
			glTextureStorage2DMultisample(colorAttachment.handle, m_sampleCount, colorAttachment.internalFormat, m_width, m_height, GL_TRUE);
		}
		else {
			glTextureStorage2D(colorAttachment.handle, 1, colorAttachment.internalFormat, m_width, m_height);
			glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(colorAttachment.handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(colorAttachment.handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(colorAttachment.handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glNamedFramebufferTexture(m_handle, GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i), colorAttachment.handle, 0);
		std::string debugLabel = "Texture (FBO: " + m_name + " Tex: " + colorAttachment.name + ")";
		glObjectLabel(GL_TEXTURE, colorAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());
	}

	if (m_depthAttachment.handle != 0) {
		glDeleteTextures(1, &m_depthAttachment.handle);

		GLenum target = IsMultisampled() ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
		glCreateTextures(target, 1, &m_depthAttachment.handle);

		if (IsMultisampled()) {
			glTextureStorage2DMultisample(m_depthAttachment.handle, m_sampleCount, m_depthAttachment.internalFormat, m_width, m_height, GL_TRUE);
		}
		else {
			glTextureStorage2D(m_depthAttachment.handle, 1, m_depthAttachment.internalFormat, m_width, m_height);
			glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(m_depthAttachment.handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glNamedFramebufferTexture(m_handle, GL_DEPTH_ATTACHMENT, m_depthAttachment.handle, 0);
		std::string debugLabel = "Texture (FBO: " + m_name + " Tex: Depth)";
		glObjectLabel(GL_TEXTURE, m_depthAttachment.handle, static_cast<GLsizei>(debugLabel.length()), debugLabel.c_str());
	}
}

GLuint OpenGLFrameBuffer::GetColorAttachmentHandleByName(const std::string& name) const {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (name == m_colorAttachments[i].name) {
            return m_colorAttachments[i].handle;
        }
    }
    std::cerr << "GetColorAttachmentHandleByName() with name '" << name << "' failed. Name does not exist in FrameBuffer '" << this->m_name << "'\n";
    return GL_NONE;
}

GLenum OpenGLFrameBuffer::GetColorAttachmentSlotByName(const std::string& name) const {
    for (int i = 0; i < m_colorAttachments.size(); i++) {
        if (name == m_colorAttachments[i].name) {
            return GL_COLOR_ATTACHMENT0 + i;
        }
    }
    std::cerr << "GetColorAttachmentSlotByName() with name '" << name << "' failed. Name does not exist in FrameBuffer '" << this->m_name << "'\n";
    return GL_INVALID_VALUE;
}

void OpenGLFrameBuffer::BlitToDefaultFrameBuffer(const std::string& srcName, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, GetHandle());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glReadBuffer(GetColorAttachmentSlotByName(srcName));
    glDrawBuffer(GL_BACK);
    glBlitFramebuffer(0, 0, GetWidth(), GetHeight(), dstX0, dstY0, dstX1, dstY1, mask, filter);
}