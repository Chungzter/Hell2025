#include "GL_backend.h"
#include <Hell/Logging.h>
#include <string>
#include <vector>

namespace OpenGLBackEnd {

	struct SupportQuery {
		int value = 0;
		std::string name;
	};

	struct DeviceCapabilities {
        int maxAttachments = 0;
        int maxDrawBuffers = 0;
	} g_deviceCapabilities;

    bool CheckSupport() {
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &g_deviceCapabilities.maxAttachments);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &g_deviceCapabilities.maxDrawBuffers);

        Logging::Support() << "Max attachments: " << g_deviceCapabilities.maxAttachments << "\n";
        Logging::Support() << "Max draw buffers: " << g_deviceCapabilities.maxDrawBuffers << "\n";

		// Define requirements locally to avoid global state issues
		std::vector<SupportQuery> requirements = {
			{ GLAD_GL_NV_mesh_shader, "GLAD_GL_NV_mesh_shader" },
			// Add more here...
		};

		bool allFound = true;

		for (const auto& query : requirements) {
			if (query.value) {
				Logging::Support() << query.name << " supported\n";
			}
			else {
				// Use Error instead of Fatal to see all missing extensions
				Logging::Error() << query.name << " not supported\n";
				allFound = false;
			}
		}

		if (allFound) {
			Logging::Support() << "All requirements met\n";
			return true;
		}
		else {
			Logging::Fatal() << "Hardware does not meet minimum requirements\n";
			return false;
		}
	}
}