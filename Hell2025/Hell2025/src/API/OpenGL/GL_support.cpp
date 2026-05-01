#include "GL_backend.h"
#include <Hell/Logging.h>
#include <string>
#include <vector>

namespace OpenGLBackEnd {

	struct SupportQuery {
		int value = 0;
		std::string name;
	};

	bool CheckSupport() {
		// Define requirements locally to avoid global state issues
		std::vector<SupportQuery> requirements = {
			{ GLAD_GL_NV_mesh_shader, "GLAD_GL_NV_mesh_shader" },
			// Add more here...
		};

		bool allFound = true;

		for (const auto& query : requirements) {
			if (query.value) {
				Logging::Support() << query.name << " found\n";
			}
			else {
				// Use Error instead of Fatal to see all missing extensions
				Logging::Error() << query.name << " not found\n";
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